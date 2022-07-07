/*
 *  HexMap.cpp
 */

#include "HexMap.h"

#include "BattleTechDefs.h"
#include "BattleTechGame.h"
#include "Randomizer.h"
#include "Math2D.h"
#include "Math3D.h"
#include "HexBoard.h"
#include <algorithm>
#include <cmath>


// ---------------------------------------------------------------------------


Pos3D Hex::Center( void ) const
{
	Pos3D c(0,0,0);
	c.X = X * sqrt(0.75);
	c.Y = (X % 2) ? (Y + 0.5) : Y;
	c.Fwd.Set(0,0,1);
	c.Up.Set(0,-1,0);
	c.Right.Set(1,0,0);
	return c;
}


Hex::~Hex()
{
}


int Hex::CostToEnter( const Hex *from, bool reverse ) const
{
	int cost = 1;
	
	int height_diff = from ? abs( Height - from->Height ) : 0;
	if( height_diff >= 3 )
		return 0;  // Invalid movement.
	if( reverse && height_diff )
		return 0;  // Cannot change level when walking backward.
	if( height_diff )
		cost += height_diff;
	
	cost += Forest;
	
	return cost;
}


// ---------------------------------------------------------------------------


HexTouch::HexTouch( const Hex *hex, uint8_t forest, uint8_t cover )
{
	HexPtr = hex;
	Forest = forest;
	Cover = cover;
}


HexTouch::~HexTouch()
{
}


// ---------------------------------------------------------------------------


bool HexTouch::operator < ( const HexTouch &other ) const
{
	return (HexPtr < other.HexPtr);
}


ShotPath::ShotPath( void ) : std::vector<const Hex*>()
{
	Clear();
}


ShotPath::~ShotPath()
{
}


void ShotPath::Clear( void )
{
	clear();
	ECM.clear();
	Modifier = 0;
	Distance = 0;
	LineOfSight = true;
	PartialCover = false;
	LegWeaponsBlocked = false;
}


bool ShotPath::ECMvsTeam( uint8_t team ) const
{
	for( std::map<uint32_t,uint8_t>::const_iterator ecm = ECM.begin(); ecm != ECM.end(); ecm ++ )
	{
		if( ecm->second != team )
			return true;
	}
	return false;
}


// ---------------------------------------------------------------------------


HexMap::HexMap( uint32_t id ) : GameObject( id, BattleTech::Object::MAP )
{
	// Directional unit vectors.
	DirVec.push_back( Vec3D( 0,           -1,   0 ) );
	DirVec.push_back( Vec3D( sqrt(0.75),  -0.5, 0 ) );
	DirVec.push_back( Vec3D( sqrt(0.75),   0.5, 0 ) );
	DirVec.push_back( Vec3D( 0,            1,   0 ) );
	DirVec.push_back( Vec3D( -sqrt(0.75),  0.5, 0 ) );
	DirVec.push_back( Vec3D( -sqrt(0.75), -0.5, 0 ) );
	
	SetSize( 31, 17 );
}


HexMap::~HexMap()
{
}


void HexMap::SetSize( size_t w, size_t h )
{
	Hexes.clear();
	for( size_t x = 0; x < w; x ++ )
	{
		Hexes.push_back( std::vector<Hex>() );
		size_t col_h = (x % 2) ? (h - 1) : h;
		for( size_t y = 0; y < col_h; y ++ )
		{
			Hex h;
			h.X = x;
			h.Y = y;
			h.Height = 0;
			h.Forest = 0;
			Hexes[ x ].push_back( h );
		}
	}
}


void HexMap::Randomize( uint32_t seed )
{
	Seed = seed;
	Randomizer r( seed );
	
	// Generate a "biome" of random factors.
	double tree1 = r.Double( 0.02, 0.03 );
	double tree2 = r.Double( 0.15, 0.30 );
	double tree3 = r.Double( 0.10, 0.15 ) - tree1;
	double hill1 = r.Double( 0.02, 0.10 );
	double hill2 = r.Double( 0.55, 0.60 ) - hill1;
	double hill3 = r.Double( 0.40, 0.80 );
	
	// Randomize the hexes on the map.
	for( std::vector< std::vector<Hex> >::iterator col = Hexes.begin(); col != Hexes.end(); col ++ )
	{
		for( std::vector<Hex>::iterator hex = col->begin(); hex != col->end(); hex ++ )
		{
			if( r.Bool(tree1) )
			{
				hex->Forest += (r.Bool(tree2) ? 2 : 1);
				if( hex->Forest > 2 )
					hex->Forest = 2;
				
				std::map<uint8_t,Hex*> adjacent = Adjacent( hex->X, hex->Y );
				for( std::map<uint8_t,Hex*>::iterator adj = adjacent.begin(); adj != adjacent.end(); adj ++ )
				{
					if( adj->second && r.Bool( tree3 / (1. + abs( adj->second->Height - hex->Height )) ) )
					{
						adj->second->Forest += (r.Bool( tree2 + 0.2 ) ? 2 : 1);
						if( adj->second->Forest > 2 )
							adj->second->Forest = 2;
					}
				}
			}
			
			if( r.Bool(hill1) )
			{
				hex->Height += (r.Bool(hill2) ? 2 : 1);
				
				std::map<uint8_t,Hex*> adjacent = Adjacent( hex->X, hex->Y );
				for( std::map<uint8_t,Hex*>::iterator adj = adjacent.begin(); adj != adjacent.end(); adj ++ )
				{
					if( adj->second && r.Bool(hill3) )
						adj->second->Height += (r.Bool(hill2-0.1) ? 2 : 1);
				}
			}
		}
	}
}


void HexMap::ClientInit( void )
{
	// When client receives the map, center the view and zoom to fit.
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	game->X = (Hexes.size() - 1) * sqrt(0.75)/2.;
	game->Y = (Hexes[ 0 ].size() - 1) / 2.;
	double zoom_w = game->Gfx.W / ((Hexes.size() + 0.5) * sqrt(0.75));
	double zoom_h = game->Gfx.H / (Hexes[ 0 ].size() + 0.25);
	game->Zoom = (zoom_w < zoom_h) ? zoom_w : zoom_h;
}


bool HexMap::PlayerShouldUpdateServer( void ) const
{
	return false;
}

bool HexMap::ServerShouldUpdatePlayer( void ) const
{
	return false;
}

bool HexMap::ServerShouldUpdateOthers( void ) const
{
	return false;
}

bool HexMap::CanCollideWithOwnType( void ) const
{
	return false;
}

bool HexMap::CanCollideWithOtherTypes( void ) const
{
	return false;
}

bool HexMap::IsMoving( void ) const
{
	return false;
}


void HexMap::AddToInitPacket( Packet *packet, int8_t precision )
{
	packet->AddUShort( Hexes.size() );      // Number of columns.
	packet->AddUShort( Hexes[ 0 ].size() ); // Taller column height.
	
	for( std::vector< std::vector<Hex> >::const_iterator col = Hexes.begin(); col != Hexes.end(); col ++ )
	{
		for( std::vector<Hex>::const_iterator hex = col->begin(); hex != col->end(); hex ++ )
		{
			// Condense each hex to one byte.
			uint8_t byte = hex->Forest << 6; // 2-bit forest.
			byte |= hex->Height & 0x3F;      // 6-bit signed height.
			packet->AddUChar( byte );
		}
	}
}


void HexMap::ReadFromInitPacket( Packet *packet, int8_t precision )
{
	size_t prev_w = Hexes.size();
	size_t prev_h = prev_w ? Hexes[ 0 ].size() : 0;
	
	uint16_t w = packet->NextUShort();
	uint16_t h = packet->NextUShort();
	
	if( (w != prev_w) || (h != prev_h) )
		SetSize( w, h );
	
	for( std::vector< std::vector<Hex> >::iterator col = Hexes.begin(); col != Hexes.end(); col ++ )
	{
		for( std::vector<Hex>::iterator hex = col->begin(); hex != col->end(); hex ++ )
		{
			uint8_t byte = packet->NextUChar();
			
			hex->Forest = (byte & 0xC0) >> 6;
			
			if( byte & 0x20 ) // If leading bit of 6-bit height is set, it's negative.
				hex->Height = byte | 0xE0; // Set leading 3 bits of negative signed char.
			else
				hex->Height = byte & 0x1F; // Mask the 5 bits of positive signed value.
		}
	}
	
	if( ClientSide() && ((w != prev_w) || (h != prev_h)) )
	{
		if( (w < prev_w) || (h < prev_h) )
		{
			Layer *hex_board = Raptor::Game->Layers.Find("HexBoard");
			if( hex_board )
				((HexBoard*)( hex_board ))->ClearPath();
		}
		
		ClientInit();
	}
}


void HexMap::AddToUpdatePacketFromServer( Packet *packet, int8_t precision )
{
	AddToInitPacket( packet, precision );
}


void HexMap::ReadFromUpdatePacketFromServer( Packet *packet, int8_t precision )
{
	ReadFromInitPacket( packet, precision );
}


bool HexMap::Valid( int x, int y ) const
{
	if( x < 0 )
		return false;
	if( y < 0 )
		return false;
	if( x >= (int) Hexes.size() )
		return false;
	if( y >= (int) Hexes[ x ].size() )
		return false;
	return true;
}


Pos3D HexMap::Center( int x, int y ) const
{
	Pos3D c(0,0,0);
	c.X = x * sqrt(0.75);
	c.Y = (x % 2) ? (y + 0.5) : y;
	c.Fwd.Set(0,0,1);
	c.Up.Set(0,-1,0);
	c.Right.Set(1,0,0);
	return c;
}


Vec3D HexMap::Direction( int dir ) const
{
	return DirVec[ ((dir % 6) + 6) % 6 ];
}


Vec3D HexMap::Direction( double dir ) const
{
	int dir_a = floor( dir );
	Vec3D a = Direction( dir_a );
	Vec3D b = Direction( dir_a + 1 );
	double progress = dir - dir_a;
	Vec3D result = b * progress + a * (1. - progress);
	result.ScaleTo( a.Length() );
	return result;
}


Hex *HexMap::Nearest( float xf, float yf )
{
	if( (xf < -sqrtf(0.4f)) || (yf < -0.5f) )
		return NULL;
	
	int x = (xf + 0.5f) / sqrtf(0.75f);
	if( (x < 0) || (x >= (int) Hexes.size()) )
		return NULL;
	
	if( (yf < 0.f) && (x % 2) )
		return NULL;
	
	int y = (x % 2) ? yf : (yf + 0.5f);
	if( (y < 0) || (y >= (int) Hexes[ x ].size()) )
		return NULL;
	
	return &(Hexes[ x ][ y ]);
}


std::map<uint8_t,Hex*> HexMap::Adjacent( int x, int y )
{
	std::map<uint8_t,Hex*> adjacent;
	int sideways_up_y = (x % 2) ? y : y - 1;
	adjacent[ BattleTech::Dir::UP         ] = Valid( x,     y - 1             ) ? &(Hexes[ x     ][ y - 1             ]) : NULL;
	adjacent[ BattleTech::Dir::UP_RIGHT   ] = Valid( x + 1, sideways_up_y     ) ? &(Hexes[ x + 1 ][ sideways_up_y     ]) : NULL;
	adjacent[ BattleTech::Dir::DOWN_RIGHT ] = Valid( x + 1, sideways_up_y + 1 ) ? &(Hexes[ x + 1 ][ sideways_up_y + 1 ]) : NULL;
	adjacent[ BattleTech::Dir::DOWN       ] = Valid( x,     y + 1             ) ? &(Hexes[ x     ][ y + 1             ]) : NULL;
	adjacent[ BattleTech::Dir::DOWN_LEFT  ] = Valid( x - 1, sideways_up_y + 1 ) ? &(Hexes[ x - 1 ][ sideways_up_y + 1 ]) : NULL;
	adjacent[ BattleTech::Dir::UP_LEFT    ] = Valid( x - 1, sideways_up_y     ) ? &(Hexes[ x - 1 ][ sideways_up_y     ]) : NULL;
	return adjacent;
}


std::map<uint8_t,const Hex*> HexMap::Adjacent_const( int x, int y ) const
{
	std::map<uint8_t,const Hex*> adjacent;
	int sideways_up_y = (x % 2) ? y : y - 1;
	adjacent[ BattleTech::Dir::UP         ] = Valid( x,     y - 1             ) ? &(Hexes[ x     ][ y - 1             ]) : NULL;
	adjacent[ BattleTech::Dir::UP_RIGHT   ] = Valid( x + 1, sideways_up_y     ) ? &(Hexes[ x + 1 ][ sideways_up_y     ]) : NULL;
	adjacent[ BattleTech::Dir::DOWN_RIGHT ] = Valid( x + 1, sideways_up_y + 1 ) ? &(Hexes[ x + 1 ][ sideways_up_y + 1 ]) : NULL;
	adjacent[ BattleTech::Dir::DOWN       ] = Valid( x,     y + 1             ) ? &(Hexes[ x     ][ y + 1             ]) : NULL;
	adjacent[ BattleTech::Dir::DOWN_LEFT  ] = Valid( x - 1, sideways_up_y + 1 ) ? &(Hexes[ x - 1 ][ sideways_up_y + 1 ]) : NULL;
	adjacent[ BattleTech::Dir::UP_LEFT    ] = Valid( x - 1, sideways_up_y     ) ? &(Hexes[ x - 1 ][ sideways_up_y     ]) : NULL;
	return adjacent;
}


int HexMap::CostToEnter( int x, int y, const Hex *from, bool reverse ) const
{
	if( ! Valid( x, y ) )
		return 0;  // Invalid movement.
	
	return Hexes[ x ][ y ].CostToEnter( from, reverse );
}


int HexMap::HexDist( int x1, int y1, int x2, int y2 ) const
{
	int dist = 0;
	Pos3D b = Center( x2, y2 );
	
	while( (x1 != x2) || (y1 != y2) )
	{
		if( x1 == x2 )
			return dist + abs(y2 - y1);
		if( y1 == y2 )
			return dist + abs(x2 - x1);
		
		dist ++;
		if( dist >= 255 )
			return 255;
		
		Pos3D a = Center( x1, y1 );
		Vec3D v = (b - a).Unit();
		
		const Hex *best_hex = NULL;
		double best_dot = -2;
		
		std::map<uint8_t,const Hex*> adjacent = Adjacent_const( x1, y1 );
		
		for( std::map<uint8_t,const Hex*>::const_iterator adj = adjacent.begin(); adj != adjacent.end(); adj ++ )
		{
			if( ! adj->second )
				continue;
			
			double dot = DirVec[ adj->first ].Dot( &v );
			if( dot > best_dot )
			{
				best_dot = dot;
				best_hex = adj->second;
			}
		}
		
		if( ! best_hex )
			return 255;
		
		x1 = best_hex->X;
		y1 = best_hex->Y;
	}
	
	return dist;
}


double HexMap::FloatDist( int x1, int y1, int x2, int y2 ) const
{
	Pos3D a = Center( x1, y1 );
	Pos3D b = Center( x2, y2 );
	return b.Dist( &a );
}


bool HexMap::LineCrosses( int ax, int ay, int bx, int by, int hx, int hy ) const
{
	Pos3D a = Center( ax, ay );
	Pos3D b = Center( bx, by );
	Pos3D h = Center( hx, hy );
	
	// Don't bother checking any hex that's too far away for the line to touch.
	// Rounded down to avoid "touching" a hex by only the gridline outside a corner.
	// FIXME: This still matches some unnecessary corners at odd angles!
	// The corner coordinates should contract if the line does not pass near 2 corners.
	if( Math3D::PointToLineSegDist( &h, &a, &b ) > 0.5625 )
		return false;
	
	// Edge coordinates are slightly padded to detect both hexes touched along an edge.
	// The Path function will discard the redundant hex with the lower defensive bonus.
	double x1 = h.X - 1.0/sqrt(3.) - 0.0001;
	double x2 = h.X - 0.5/sqrt(3.) - 0.0001;
	double x3 = h.X + 0.5/sqrt(3.) + 0.0001;
	double x4 = h.X + 1.0/sqrt(3.) + 0.0001;
	double y1 = h.Y - 0.5001;
	double y2 = h.Y;
	double y3 = h.Y + 0.5001;
	
	if( Math2D::LineIntersection( x2, y1, x3, y1, a.X, a.Y, b.X, b.Y ) )
		return true;
	if( Math2D::LineIntersection( x3, y1, x4, y2, a.X, a.Y, b.X, b.Y ) )
		return true;
	if( Math2D::LineIntersection( x4, y2, x3, y3, a.X, a.Y, b.X, b.Y ) )
		return true;
	if( Math2D::LineIntersection( x3, y3, x2, y3, a.X, a.Y, b.X, b.Y ) )
		return true;
	if( Math2D::LineIntersection( x2, y3, x1, y2, a.X, a.Y, b.X, b.Y ) )
		return true;
	if( Math2D::LineIntersection( x1, y2, x2, y1, a.X, a.Y, b.X, b.Y ) )
		return true;
	
	return false;
}


ShotPath HexMap::Path( int x1, int y1, int x2, int y2, int8_t h1, int8_t h2 ) const
{
	ShotPath path;
	std::set<HexTouch> touched;
	
	// Prone Mechs cannot fire leg weapons.
	if( h1 == 1 )
		path.LegWeaponsBlocked = true;
	
	// If either point is invalid, return an empty path.
	if( !(Valid( x1, y1 ) && Valid( x2, y2 )) )
	{
		path.LineOfSight = false;
		return path;
	}
	
	path.Distance = HexDist( x1, y1, x2, y2 );
	
	// Always include the start hex in the path.
	const Hex *here = &(Hexes[ x1 ][ y1 ]);
	path.push_back( here );
	
	// If start and end point are the same, return a single hex path.
	if( (x1 == x2) && (y1 == y2) )
		return path;
	
	// If we didn't specify line-of-sight height, determine automatically from Mechs on map.
	if( ! h1 )
	{
		h1 = 2;
		const Mech *m = MechAt_const( x1, y1 );
		if( m && m->Prone )
			h1 = 1;
	}
	if( ! h2 )
	{
		h2 = 2;
		const Mech *m = MechAt_const( x2, y2 );
		if( m && m->Prone )
			h2 = 1;
	}
	
	const Hex *dest = &(Hexes[ x2 ][ y2 ]);
	int8_t here_height = here->Height + h1;
	int8_t dest_height = dest->Height + h2;
	int8_t mid_height = std::max<int8_t>( here_height, dest_height );
	
	// Find all intervening hexes touched by the line.
	for( std::vector< std::vector<Hex> >::const_iterator col = Hexes.begin(); col != Hexes.end(); col ++ )
	{
		for( std::vector<Hex>::const_iterator hex = col->begin(); hex != col->end(); hex ++ )
		{
			// Start and end hexes are NOT intervening terrain.
			if( (&*hex == here) || (&*hex == dest) )
				continue;
			
			if( LineCrosses( x1, y1, x2, y2, hex->X, hex->Y ) )
			{
				// Check height against higher of start/end unless adjacent to one of them.
				size_t here_dist = HexDist( hex->X, hex->Y, x1, y1 );
				size_t dest_dist = HexDist( hex->X, hex->Y, x2, y2 );
				uint8_t los_height = mid_height;
				if( (here_dist == 1) && (dest_dist > 1) )
					los_height = here_height;
				else if( (dest_dist == 1) && (here_dist > 1) )
					los_height = dest_height;
				
				// Cover 2 means fully blocked, cover 1 is partial cover adjacent to dest.
				uint8_t cover = 0;
				if( hex->Height >= los_height )
					cover = 2;
				else if( (here_dist == 1) && (dest_dist == 1) && ((hex->Height >= here_height) || (hex->Height >= dest_height)) )
					cover = 2;
				else if( hex->Height == los_height - 1 )
				{
					if( (dest_dist == 1) && (dest_height >= here_height) )
						cover = 1;
					
					// Leg-mounted weapons cannot fire if the attacker has partial cover.
					if( (here_dist == 1) && (here_height >= dest_height) )
						path.LegWeaponsBlocked = true;
				}
				
				// Forests and standing mechs are 2 units tall.
				uint8_t forest = (hex->Height + 2 >= los_height) ? hex->Forest : 0;
				
				// Keep track of any hexes with ECM coverage.
				// FIXME: If this match is along the hex edge and we keep the other hex for better defense, this hex's ECM should not apply.
				std::set<const Mech*> mech_ecms = MechECMsAt( hex->X, hex->Y );
				for( std::set<const Mech*>::const_iterator ecm = mech_ecms.begin(); ecm != mech_ecms.end(); ecm ++ )
					path.ECM[ (*ecm)->ID ] = (*ecm)->Team;
				
				// Only mark hexes along the path that interfere with line-of-sight.
				if( forest || cover )
					touched.insert( HexTouch( &*hex, forest, cover ) );
			}
		}
	}
	
	// Check start and end hexes for ECM coverage.
	std::set<const Mech*> here_ecms = MechECMsAt( here->X, here->Y );
	std::set<const Mech*> dest_ecms = MechECMsAt( dest->X, dest->Y );
	for( std::set<const Mech*>::const_iterator ecm = here_ecms.begin(); ecm != here_ecms.end(); ecm ++ )
		path.ECM[ (*ecm)->ID ] = (*ecm)->Team;
	for( std::set<const Mech*>::const_iterator ecm = dest_ecms.begin(); ecm != dest_ecms.end(); ecm ++ )
		path.ECM[ (*ecm)->ID ] = (*ecm)->Team;
	
	// Iterate over touched hexes until we've considered them all.
	while( touched.size() )
	{
		std::set<HexTouch>::const_iterator h = touched.begin();
		double hd = FloatDist( h->HexPtr->X, h->HexPtr->Y, x2, y2 );
		
		// Working start to end, so look for the farthest hex from destination.
		for( std::set<HexTouch>::const_iterator t = touched.begin(); t != touched.end(); t ++ )
		{
			if( h == t )
				continue;
			double td = FloatDist( t->HexPtr->X, t->HexPtr->Y, x2, y2 );
			
			if( fabs( td - hd ) < 0.01 )
			{
				// Two possible intervening hexes along an edge; keep the better defensive bonus.
				std::set<HexTouch>::const_iterator rem = (h->Cover + h->Forest > t->Cover + t->Forest) ? t : h;
				if( (h->Cover >= 2) && (t->Cover < 2) )
					rem = t;
				else if( (t->Cover >= 2) && (h->Cover < 2) )
					rem = h;
				
				// When we remove a tie hex, start over looking for the farthest from dest.
				touched.erase( rem );
				h = touched.end();
				break;
			}
			
			if( td > hd )
			{
				h = t;
				hd = td;
			}
		}
		
		if( h != touched.end() )
		{
			// Line of sight is broken by 3+ intervening forests.
			// Destination is checked last, so modifier only has forest and full-cover until the end.
			path.Modifier += h->Forest;
			if( path.Modifier >= 3 )
				path.LineOfSight = false;
			
			// Cover 1 means partial cover adjacent to dest, cover 2 means blocked by mountain.
			path.Modifier += h->Cover;
			if( h->Cover >= 2 )
				path.LineOfSight = false;
			else if( h->Cover )
				path.PartialCover = true;
			
			path.push_back( h->HexPtr );
			touched.erase( h );
		}
	}
	
	// Always include the end hex in the path, and add its forest difficulty modifier.
	path.push_back( dest );
	path.Modifier += dest->Forest;
	
	return path;
}


int HexMap::JumpCost( int x1, int y1, int x2, int y2 ) const
{
	int cost = HexDist( x1, y1, x2, y2 );
	
	if( ! cost )
		return 1;  // Can't jump in place to rotate without any jumpjets.
	
	std::set<const Hex*> touched;
	const Hex *here = &(Hexes[ x1 ][ y1 ]);
	
	// Find all hexes touched by the line (including destination, but not start).
	for( std::vector< std::vector<Hex> >::const_iterator col = Hexes.begin(); col != Hexes.end(); col ++ )
	{
		for( std::vector<Hex>::const_iterator hex = col->begin(); hex != col->end(); hex ++ )
		{
			if( (&*hex != here) && LineCrosses( x1, y1, x2, y2, hex->X, hex->Y ) )
				touched.insert( &*hex );
		}
	}
	
	// Iterate over touched hexes until we've considered them all.
	while( touched.size() )
	{
		std::set<const Hex*>::const_iterator h = touched.begin();
		double hd = FloatDist( (*h)->X, (*h)->Y, x2, y2 );
		
		// Working start to end, so look for the farthest hex from destination.
		for( std::set<const Hex*>::const_iterator t = touched.begin(); t != touched.end(); t ++ )
		{
			if( h == t )
				continue;
			double td = FloatDist( (*t)->X, (*t)->Y, x2, y2 );
			
			if( fabs( td - hd ) < 0.01 )
			{
				// Two possible intervening hexes along an edge; keep the lower altitude.
				std::set<const Hex*>::const_iterator rem = ((*h)->Height < (*t)->Height) ? t : h;
				
				// When we remove a tie hex, start over looking for the farthest from dest.
				touched.erase( rem );
				h = touched.end();
				break;
			}
			
			if( td > hd )
			{
				h = t;
				hd = td;
			}
		}
		
		if( h != touched.end() )
		{
			// See if any hexes in the flight path are taller than the jump distance.
			int rel_height = (*h)->Height - here->Height;
			if( rel_height > cost )
				cost = rel_height;
			touched.erase( h );
		}
	}
	
	return cost;
}


Mech *HexMap::MechAt( int x, int y )
{
	Mech *found = NULL;
	
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			Mech *mech = (Mech*) obj_iter->second;
			uint8_t mx, my;
			mech->GetPosition( &mx, &my );
			if( (mx == x) && (my == y) )
			{
				if( ! mech->Destroyed() )
					return mech;
				if( ! found )
					found = mech;
				else if( mech->HitClock.ElapsedSeconds() < found->HitClock.ElapsedSeconds() )
					found = mech;
			}
		}
	}
	
	return found;
}


const Mech *HexMap::MechAt_const( int x, int y ) const
{
	const Mech *found = NULL;
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (Mech*) obj_iter->second;
			uint8_t mx, my;
			mech->GetPosition( &mx, &my );
			if( (mx == x) && (my == y) )
			{
				if( ! mech->Destroyed() )
					return mech;
				if( ! found )
					found = mech;
				else if( mech->HitClock.ElapsedSeconds() < found->HitClock.ElapsedSeconds() )
					found = mech;
			}
		}
	}
	
	return found;
}


size_t HexMap::MechsAt( int x, int y ) const
{
	size_t count = 0;
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (Mech*) obj_iter->second;
			if( mech->Destroyed() )
				continue;
			uint8_t mx, my;
			mech->GetPosition( &mx, &my );
			if( (mx == x) && (my == y) )
				count ++;
		}
	}
	
	return count;
}


size_t HexMap::TeamsAt( int x, int y ) const
{
	std::set<uint8_t> teams;
	size_t teamless = 0;
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (Mech*) obj_iter->second;
			if( mech->Destroyed() )
				continue;
			uint8_t mx, my;
			mech->GetPosition( &mx, &my );
			if( (mx == x) && (my == y) )
			{
				if( mech->Team )
					teams.insert( mech->Team );
				else
					teamless ++;
			}
		}
	}
	
	return teams.size() + teamless;
}


std::set<const Mech*> HexMap::MechECMsAt( int x, int y ) const
{
	std::set<const Mech*> ecm_mechs;
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (Mech*) obj_iter->second;
			
			if( mech->ECM && ! mech->ECM->Damaged
			&&  ! mech->Shutdown && ! mech->Destroyed()
			&&  (HexDist( x, y, mech->X, mech->Y ) <= 6) )
				ecm_mechs.insert( mech );
		}
	}
	
	return ecm_mechs;
}


void HexMap::Draw( void )
{
	double x1 = -1.0/sqrt(3.);
	double x2 = -0.5/sqrt(3.);
	double x3 =  0.5/sqrt(3.);
	double x4 =  1.0/sqrt(3.);
	double y1 = -0.5;
	double y2 =  0;
	double y3 =  0.5;
	double corners[ 6 ][ 4 ] = {
		{ x2,y1,x3,y1 },
		{ x3,y1,x4,y2 },
		{ x4,y2,x3,y3 },
		{ x3,y3,x2,y3 },
		{ x2,y3,x1,y2 },
		{ x1,y2,x2,y1 }
	};
	float colors[ 6 ][ 4 ] = {
		{ 0.9f,0.9f,0.9f,0.9f },
		{ 0.6f,0.6f,0.6f,1.0f },
		{ 0.0f,0.0f,0.0f,0.9f },
		{ 0.1f,0.1f,0.1f,0.9f },
		{ 0.4f,0.4f,0.4f,0.9f },
		{ 1.0f,1.0f,1.0f,0.9f }
	};
	GLuint texture = Raptor::Game->Res.GetTexture("hex.png");
	
	std::map<int8_t,Color> biome_colors;
	std::map<int8_t,Color> biome_scales;
	std::vector<double> biome_vals = Data->PropertyAsDoubles("biome");
	while( biome_vals.size() > 7 )
	{
		biome_colors[ biome_vals[ 6 ] ].Red   = biome_vals[ 0 ];
		biome_colors[ biome_vals[ 6 ] ].Green = biome_vals[ 1 ];
		biome_colors[ biome_vals[ 6 ] ].Blue  = biome_vals[ 2 ];
		biome_scales[ biome_vals[ 6 ] ].Red   = biome_vals[ 3 ];
		biome_scales[ biome_vals[ 6 ] ].Green = biome_vals[ 4 ];
		biome_scales[ biome_vals[ 6 ] ].Blue  = biome_vals[ 5 ];
		for( size_t i = 0; i < 7; i ++ )
			biome_vals.erase( biome_vals.begin() );
	}
	if( biome_vals.size() >= 3 )
	{
		biome_colors[ 127 ].Red   = biome_vals[ 0 ];
		biome_colors[ 127 ].Green = biome_vals[ 1 ];
		biome_colors[ 127 ].Blue  = biome_vals[ 2 ];
	}
	if( biome_vals.size() >= 6 )
	{
		biome_scales[ 127 ].Red   = biome_vals[ 3 ];
		biome_scales[ 127 ].Green = biome_vals[ 4 ];
		biome_scales[ 127 ].Blue  = biome_vals[ 5 ];
	}
	
	// Default trees are green.
	Color tree1( 0.2f, 0.7f, 0.1f, 1.f ), tree2( 0.f, 0.5f, 0.1f, 1.f );
	std::map<int8_t,Color>::const_iterator biome_color = biome_colors.lower_bound( 0 );
	if( biome_color != biome_colors.end() )
	{
		// Use biome colors if defined.
		Color c( biome_color->second );
		std::map<int8_t,Color>::const_iterator biome_scale = biome_scales.find( biome_color->first );
		if( biome_scale != biome_scales.end() )
		{
			c.Red   *= biome_scale->second.Red;
			c.Green *= biome_scale->second.Green;
			c.Blue  *= biome_scale->second.Blue;
		}
		if( (c.Blue >= c.Red) && (c.Blue >= c.Green) )
		{
			if( c.Red >= c.Green )
			{
				// Lunar
				tree1.Red   = 0.4f;
				tree1.Green = 0.4f;
				tree1.Blue  = 0.5f;
				tree2.Red   = 0.35f;
				tree2.Green = 0.3f;
				tree2.Blue  = 0.4f;
			}
			else
			{
				// Arctic
				tree1.Red   = 0.f;
				tree1.Green = 0.5f;
				tree1.Blue  = 0.4f;
				tree2.Red   = 0.f;
				tree2.Green = 0.3f;
				tree2.Blue  = 0.3f;
			}
		}
		else if( (c.Green >= c.Red) && (c.Green >= c.Blue) )
		{
			if( c.Red >= c.Blue )
			{
				// Grasslands/Alpine
				tree1.Red   = 0.2f;
				tree1.Green = 0.7f;
				tree1.Blue  = 0.1f;
				tree2.Red   = 0.f;
				tree2.Green = 0.5f;
				tree2.Blue  = 0.1f;
			}
			else
			{
				tree1.Red   = 0.f;
				tree1.Green = 0.5f;
				tree1.Blue  = 0.1f;
				tree2.Red   = 0.f;
				tree2.Green = 0.5f;
				tree2.Blue  = 0.4f;
			}
		}
		else // Red
		{
			if( c.Green > c.Blue * 1.25f )
			{
				// Desert
				tree1.Red   = 0.3f;
				tree1.Green = 0.5f;
				tree1.Blue  = 0.1f;
				tree2.Red   = 0.05f;
				tree2.Green = 0.35f;
				tree2.Blue  = 0.05f;
			}
			else if( c.Green >= c.Blue )
			{
				// Volcanic/Martian
				tree1.Red   = 0.7f;
				tree1.Green = 0.25f;
				tree1.Blue  = 0.1f;
				tree2.Red   = 0.5f;
				tree2.Green = 0.f;
				tree2.Blue  = 0.1f;
			}
			else
			{
				tree1.Red   = 0.7f;
				tree1.Blue  = 0.2f;
				tree1.Blue  = 0.1f;
				tree2.Red   = 0.6f;
				tree2.Blue  = 0.f;
				tree2.Green = 0.f;
			}
		}
	}
	
	for( std::vector< std::vector<Hex> >::const_iterator col = Hexes.begin(); col != Hexes.end(); col ++ )
	{
		for( std::vector<Hex>::const_iterator hex = col->begin(); hex != col->end(); hex ++ )
		{
			Pos3D pos = Center( hex->X, hex->Y );
			std::map<uint8_t,const Hex*> adjacent = Adjacent_const( hex->X, hex->Y );
			
			// Default ground color is light green.
			Color c( 0.65f, 0.85f, 0.45f, 1.f );
			std::map<int8_t,Color>::const_iterator biome_color = biome_colors.lower_bound( hex->Height );
			if( biome_color != biome_colors.end() )
			{
				// Use biome colors if defined.
				c = biome_color->second;
				std::map<int8_t,Color>::const_iterator biome_scale = biome_scales.find( biome_color->first );
				if( biome_scale != biome_scales.end() )
				{
					c.Red   *= powf( biome_scale->second.Red,   hex->Height );
					c.Green *= powf( biome_scale->second.Green, hex->Height );
					c.Blue  *= powf( biome_scale->second.Blue,  hex->Height );
				}
			}
			else if( hex->Height < 0 )
			{
				// Pits are brown.
				c.Red   = 0.7f * powf( 1.05f, hex->Height );
				c.Green = 0.7f * powf( 1.15f, hex->Height );
				c.Blue  = 0.4f * powf( 1.15f, hex->Height );
			}
			else if( hex->Height <= 2 )
			{
				// Low hills are dark green.
				c.Red   = 0.65f * powf( 0.8f, hex->Height );
				c.Green = 0.85f * powf( 0.9f, hex->Height );
				c.Blue  = 0.45f * powf( 0.8f, hex->Height );
			}
			else if( hex->Height == 3 )
			{
				// High hills are tan.
				c.Red   = 0.8f;
				c.Green = 0.75f;
				c.Blue  = 0.6f;
			}
			else
			{
				// Mountains are grey/white.
				c.Red   = 0.5465f * powf( 1.1f,  hex->Height );
				c.Green = 0.4288f * powf( 1.15f, hex->Height );
				c.Blue  = 0.3072f * powf( 1.25f, hex->Height );
			}
			
			// Draw the hex color.
			Raptor::Game->Gfx.DrawCircle2D( pos.X, pos.Y, 0.575, 6, texture, c.Red,c.Green,c.Blue,1.0f );
			
			// Draw 3D effect along edges that are taller than neighboring hexes.
			for( std::map<uint8_t,const Hex*>::const_iterator adj = adjacent.begin(); adj != adjacent.end(); adj ++ )
			{
				int adj_height = adj->second ? adj->second->Height : 0;
				int height_diff = hex->Height - adj_height;
				if( height_diff > 0 )
				{
					float *color = colors[ adj->first ];
					double *corner = corners[ adj->first ];
					float fraction = height_diff / (height_diff + 1.f);
					double inner = 1. - 0.4 * fraction;
					glColor4f( color[ 0 ], color[ 1 ], color[ 2 ], color[ 3 ] * fraction );
					glBegin( GL_TRIANGLE_FAN );
						glVertex2d( pos.X + corner[ 0 ] * inner, pos.Y + corner[ 1 ] * inner );
						glVertex2d( pos.X + corner[ 2 ] * inner, pos.Y + corner[ 3 ] * inner );
						glVertex2d( pos.X + corner[ 2 ] * 0.995, pos.Y + corner[ 3 ] * 0.995 );
						glVertex2d( pos.X + corner[ 0 ] * 0.995, pos.Y + corner[ 1 ] * 0.995 );
					glEnd();
				}
			}
			
			// Draw black hex outline.
			Raptor::Game->Gfx.DrawCircleOutline2D( pos.X, pos.Y, 0.57735, 6, 1.f, 0.0f,0.0f,0.0f,0.5f );
			
			// Draw forest.
			if( hex->Forest >= 2 )
			{
				float r = tree2.Red, g = tree2.Green, b = tree2.Blue;
				GLuint tree = Raptor::Game->Res.GetTexture("tree.png");
				if( tree )
				{
					Randomizer rand( hex->X + 100 * hex->Y );
					double x = pos.X + rand.Double( -0.3, -0.2 );
					double y = pos.Y + rand.Double( -0.1,  0.1 );
					Raptor::Game->Gfx.DrawRect2D( x - 0.22, y - 0.44, x + 0.22, y + 0.44, tree, r,g,b,1.f );
					x = pos.X + rand.Double(  0.2, 0.3 );
					y = pos.Y + rand.Double( -0.1, 0.1 );
					Raptor::Game->Gfx.DrawRect2D( x - 0.22, y - 0.44, x + 0.22, y + 0.44, tree, r,g,b,1.f );
					x = pos.X + rand.Double( -0.1, 0.1 );
					y = pos.Y + rand.Double( -0.2, 0.2 );
					Raptor::Game->Gfx.DrawRect2D( x - 0.22, y - 0.44, x + 0.22, y + 0.44, tree, r,g,b,1.f );
				}
				else
					Raptor::Game->Gfx.DrawCircle2D( pos.X, pos.Y, 0.4, 6, 0, r,g,b,1.f );
			}
			else if( hex->Forest )
			{
				float r = tree1.Red, g = tree1.Green, b = tree1.Blue;
				GLuint tree = Raptor::Game->Res.GetTexture("tree.png");
				if( tree )
				{
					Randomizer rand( hex->X + 100 * hex->Y );
					double x = pos.X + rand.Double( -0.2, -0.1 );
					double y = pos.Y + rand.Double( -0.1,  0.1 );
					Raptor::Game->Gfx.DrawRect2D( x - 0.22, y - 0.44, x + 0.22, y + 0.44, tree, r,g,b,1.f );
					x = pos.X + rand.Double(  0.1, 0.2 );
					y = pos.Y + rand.Double( -0.1, 0.1 );
					Raptor::Game->Gfx.DrawRect2D( x - 0.22, y - 0.44, x + 0.22, y + 0.44, tree, r,g,b,1.f );
				}
				else
					Raptor::Game->Gfx.DrawCircle2D( pos.X, pos.Y, 0.4, 6, 0, r,g,b,1.f );
			}
		}
	}
}


void HexMap::DrawHexOutline( int x, int y, float thickness, float r, float g, float b, float a ) const
{
	Pos3D pos = Center( x, y );
	Raptor::Game->Gfx.DrawCircleOutline2D( pos.X, pos.Y, 0.52, 6, thickness, r,g,b,a );
}
