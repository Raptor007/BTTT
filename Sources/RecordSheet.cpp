/*
 *  RecordSheet.cpp
 */

#include "RecordSheet.h"

#include "BattleTechDefs.h"
#include "BattleTechGame.h"
#include "HexBoard.h"
#include "SpawnMenu.h"
#include "Num.h"
#include <cmath>


RecordSheet::RecordSheet( const Mech *mech )
{
	Name = "RecordSheet";
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	MechID = mech ? mech->ID : game->SelectedID;
	AutoHide = false;
	
	Rect.w = 520;
	Rect.h = game->Gfx.H;
	
	Red = 0.4f;
	Green = 0.4f;
	Blue = 0.4f;
	Alpha = 0.75f;
	
	TitleFont = game->Res.GetFont( "ProFont.ttf", 18 );
	Font *head_font = game->Res.GetFont( "ProFont.ttf", 14 );
	Font *item_font = game->Res.GetFont( "ProFont.ttf", 12 );
	
	SDL_Rect rect;
	
	rect.x = 10;
	rect.y = TitleFont->GetHeight() + 7;
	rect.w = (Rect.w - 30) / 2;
	rect.h = item_font->GetHeight() * 2 + 20;
	AddElement( new RecordSheetStatus( &rect, head_font, item_font ) );
	
	rect.x += rect.w + 10;
	AddElement( new RecordSheetPilot( &rect, head_font, item_font ) );
	
	rect.x = 10;
	rect.w = (Rect.w - 40) / 3;
	int y = rect.y + rect.h + 10;
	
	for( size_t loc = 0; loc < BattleTech::Loc::COUNT; loc ++ )
	{
		if( (loc == BattleTech::Loc::LEFT_ARM) || (loc == BattleTech::Loc::RIGHT_ARM) || (loc == BattleTech::Loc::HEAD) )
			rect.y = y;
		
		rect.h = item_font->GetLineSkip() * 12 + 39;
		if( (loc == BattleTech::Loc::HEAD) || (loc == BattleTech::Loc::LEFT_LEG) || (loc == BattleTech::Loc::RIGHT_LEG) )
			rect.h = item_font->GetLineSkip() * 6 + 39;
		
		rect.x = 10;
		if( loc >= BattleTech::Loc::HEAD )
			rect.x = (Rect.w - rect.w) / 2;
		else if( loc >= BattleTech::Loc::RIGHT_ARM )
			rect.x = Rect.w - rect.w - 10;
		
		AddElement( new RecordSheetLoc( loc, &rect, head_font, item_font ) );
		
		rect.y += rect.h + 10;
		
		if( loc == BattleTech::Loc::CENTER_TORSO )
		{
			rect.h = rect.w;
			AddElement( new RecordSheetHealth( &rect ) );
		}
	}
	
	SizeToElements();
	AutoPosition();
	
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new RecordSheetCloseButton( &rect, head_font ) );
}


RecordSheet::~RecordSheet()
{
}


void RecordSheet::AutoPosition( void )
{
	Rect.x = Raptor::Game->Gfx.W - Rect.w - 5;
	Rect.y = Raptor::Game->Gfx.H - Rect.h - 30;
	
	Mech *mech = GetMech();
	if( mech )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		HexMap *map = game->Map();
		uint8_t x, y;
		mech->GetPosition( &x, &y );
		Pos3D pos = map->Center( x, y );
		int px, py;
		game->GetPixel( &pos, &px, &py );
		int hex = game->Zoom * 0.6;
		int left = (px - hex) - Rect.w;
		int up   = (py - hex) - Rect.h;
		int right = game->Gfx.W - (px + hex) - Rect.w;
		int down  = game->Gfx.H - (py + hex) - Rect.h;
		
		// FIXME: Prefer not to cover attacker!
		
		if( (up >= 0) || (down >= 0) )
		{
			Rect.x = px - Rect.w/2;
			if( Rect.x < 0 )
				Rect.x = 0;
			else if( Rect.x + Rect.w > game->Gfx.W )
				Rect.x = game->Gfx.W - Rect.w;
			
			if( (up >= 0) && (down >= 0) )
				Rect.y = (up > down) ? (py + hex) : (py - hex - Rect.h);
			else if( up >= 0 )
				Rect.y = py - hex - Rect.h;
			else //if( down >= 0 )
				Rect.y = py + hex;
		}
		else if( (left >= 0) || (right >= 0) )
		{
			Rect.y = py - Rect.h/2;
			if( Rect.y < 0 )
				Rect.y = 0;
			else if( Rect.y + Rect.h > game->Gfx.H )
				Rect.y = game->Gfx.H - Rect.h;
			
			if( (left >= 0) && (right >= 0) )
				Rect.x = (left > right) ? (px + hex) : (px - hex - Rect.w);
			else if( left >= 0 )
				Rect.x = px - hex - Rect.w;
			else //if( right >= 0 )
				Rect.x = px + hex;
		}
		else if( pos.X > game->X )
			Rect.x = 5;
	}
}


Mech *RecordSheet::GetMech( void )
{
	if( AutoHide && (Lifetime.RemainingSeconds() <= 0.) )
		return NULL;
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	return game->GetMech( MechID );
}


bool RecordSheet::KeyDown( SDLKey key )
{
	HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
	if( hex_board && (hex_board->Selected == hex_board->MessageInput) )
		return false;
	else if( key == SDLK_ESCAPE )
		Close();
	else
		return false;
	return true;
}


bool RecordSheet::MouseDown( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
	{
		AutoHide = false;
		MoveToTop();
	}
	
	Draggable = (button == SDL_BUTTON_LEFT);
	return Window::MouseDown( button );
}


void RecordSheet::Draw( void )
{
	Mech *mech = GetMech();
	if( ! mech )
	{
		Remove();
		return;
	}
	
	Window::Draw();
	
	std::string title = mech->FullName();
	float r = 1.f, g = 1.f, b = 1.f;
	if( mech->Team )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		title += std::string(" - ") + game->TeamName( mech->Team );
		uint8_t my_team = game->MyTeam();
		if( my_team && (my_team == mech->Team) && (game->SelectedID == mech->ID) )
			r = b = 0.6f;
		else if( my_team && (my_team != mech->Team) )
			g = b = (game->SelectedID == mech->ID) ? 0.5f : 0.6f;
		else if( game->SelectedID == mech->ID )
			b = 0.7f;
	}
	TitleFont->DrawText( title, Rect.w/2 + 2, 7, Font::ALIGN_TOP_CENTER, 0,0,0,0.8f );
	TitleFont->DrawText( title, Rect.w/2,     5, Font::ALIGN_TOP_CENTER, r,g,b,1.f );
}


void RecordSheet::Close( void )
{
	// Closing a pop-up record sheet disables them.
	if( AutoHide )
		Raptor::Game->Cfg.Settings[ "record_sheet_popup" ] = "false";
	
	Remove();
}


// ---------------------------------------------------------------------------


RecordSheetCloseButton::RecordSheetCloseButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, "x", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Name = "RecordSheetCloseButton";
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


RecordSheetCloseButton::~RecordSheetCloseButton()
{
}


void RecordSheetCloseButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
	{
		RecordSheet *rs = (RecordSheet*) Container;
		rs->Close();
	}
}


// -----------------------------------------------------------------------------


RecordSheetLoc::RecordSheetLoc( int8_t loc, SDL_Rect *rect, Font *title_font, Font *item_font )
: GroupBox( rect, "", title_font )
{
	Name = "RecordSheetLoc";
	Loc = loc;
	ItemFont = item_font ? item_font : title_font;
}


RecordSheetLoc::~RecordSheetLoc()
{
}


void RecordSheetLoc::Draw( void )
{
	RecordSheet *rs = (RecordSheet*) Container;
	Mech *mech = rs ? rs->GetMech() : NULL;
	if( ! mech )
		return;
	
	MechLocation *location = &(mech->Locations[ Loc ]);
	
	TitleText = location->Name;
	GroupBox::Draw();
	
	float armor = location->Armor / (float) location->MaxArmor;
	ItemFont->DrawText( Num::ToString(location->Armor) + std::string("/") + Num::ToString(location->MaxArmor), 10, 17, Font::ALIGN_TOP_LEFT, 1.f,armor,floorf(armor),1.f );
	
	float structure = location->Structure / (float) location->MaxStructure;
	ItemFont->DrawText( Num::ToString(location->Structure) + std::string("/") + Num::ToString(location->MaxStructure), Rect.w - 10, 17, Font::ALIGN_TOP_RIGHT, 1.f,structure,floorf(structure),1.f );
	
	if( location->IsTorso )
	{
		float rear = location->RearArmor / (float) location->MaxRearArmor;
		ItemFont->DrawText( Num::ToString(location->RearArmor) + std::string("/") + Num::ToString(location->MaxRearArmor), Rect.w / 2, 17, Font::ALIGN_TOP_CENTER, 1.f,rear,floorf(rear),1.f );
	}
	
	if( location->Narced && location->Structure )
		ItemFont->DrawText( "Narc", Rect.w - 1 - (int) BorderWidth, Rect.h - 2 - (int) BorderWidth, Font::ALIGN_BASELINE_RIGHT, 1.f,1.f,0.f,1.f );
	
	std::set<MechEquipment*> eq_ids = location->Equipment();
	int y = ItemFont->GetLineSkip() + 27;
	
	for( std::set<MechEquipment*>::const_iterator eq_iter = eq_ids.begin(); eq_iter != eq_ids.end(); eq_iter ++ )
	{
		MechEquipment *eq = *eq_iter;
		size_t damaged = location->DamagedCriticals.count( eq );
		bool destroyed = ((eq->Damaged >= eq->HitsToDestroy()) && (eq->Weapon || damaged || (eq->HitsToDestroy() > 1))) || ! location->Structure;
		size_t slots = damaged;
		
		for( std::vector<MechEquipment*>::const_iterator slot = location->CriticalSlots.begin(); slot != location->CriticalSlots.end(); slot ++ )
		{
			if( *slot == eq )
				slots ++;
		}
		
		if( slots > 1 )
		{
			float gb = destroyed ? 0.f : 1.f;
			Raptor::Game->Gfx.DrawLine2D( 6, y - 4, 6, y + 4 + (slots - 1) * ItemFont->GetLineSkip(), 1.5f, 1.f,gb,gb,1.f );
		}
		
		bool one_shot = eq->Weapon && (eq->Weapon->Type == HMWeapon::ONE_SHOT);
		std::string name = eq->Name;
		if( eq->WeaponArcs.count(BattleTech::Arc::REAR) && (eq->WeaponArcs.size() == 1) )
			name += " (Rear)";
		if( eq->ID > 400 )
			name += std::string(" ") + Num::ToString(eq->Ammo);
		if( eq->Jammed && ! destroyed && ! one_shot )
			name += std::string(" [Jam]");
		if( (eq->ID == BattleTech::Equipment::MASC) && mech->MASCTurns )
			name += std::string(" [") + Num::ToString(mech->MASCTurns) + std::string("]");
		if( (eq->ID == BattleTech::Equipment::SUPERCHARGER) && mech->SuperchargerTurns )
			name += std::string(" [") + Num::ToString(mech->SuperchargerTurns) + std::string("]");
		
		for( size_t i = 0; i < slots; i ++ )
		{
			if( i == 1 )
				name = eq->Name;
			
			if( i < damaged )
				ItemFont->DrawText( name, 10, y, Font::ALIGN_MIDDLE_LEFT, 1.f,0.f,0.f,1.f );
			else if( destroyed )
				ItemFont->DrawText( name, 10, y, Font::ALIGN_MIDDLE_LEFT, 1.f,0.7f,0.7f,1.f );
			else if( eq->Jammed || ((eq->ID > 400) && ! eq->Ammo) )
				ItemFont->DrawText( name, 10, y, Font::ALIGN_MIDDLE_LEFT, 0.8f,0.8f,0.8f,1.f );
			else
				ItemFont->DrawText( name, 10, y, Font::ALIGN_MIDDLE_LEFT );
			
			y += ItemFont->GetLineSkip();
		}
	}
}


// -----------------------------------------------------------------------------


RecordSheetPilot::RecordSheetPilot( SDL_Rect *rect, Font *title_font, Font *item_font )
: GroupBox( rect, "Pilot", title_font )
{
	Name = "RecordSheetPilot";
	ItemFont = item_font ? item_font : title_font;
}


RecordSheetPilot::~RecordSheetPilot()
{
}


void RecordSheetPilot::Draw( void )
{
	RecordSheet *rs = (RecordSheet*) Container;
	Mech *mech = rs ? rs->GetMech() : NULL;
	if( ! mech )
		return;
	
	GroupBox::Draw();
	
	int y = 17;
	std::string skills = std::string("Gunnery: ") + Num::ToString(mech->GunnerySkill) + std::string("    Piloting: ") + Num::ToString(mech->PilotSkill);
	ItemFont->DrawText( skills, 10, y, Font::ALIGN_TOP_LEFT );
	if( mech->HeatFire )
		ItemFont->DrawText( std::string(" +") + Num::ToString(mech->HeatFire), 67, y, Font::ALIGN_TOP_LEFT, 1.f,1.f,0.f,1.f );
	if( mech->PSRModifiers() )
		ItemFont->DrawText( std::string(" +") + Num::ToString(mech->PSRModifiers()), 157, y, Font::ALIGN_TOP_LEFT, 1.f,1.f,0.f,1.f );
	
	y += ItemFont->GetHeight();
	std::string hits = std::string("Hits Taken: ") + Num::ToString(mech->PilotDamage);
	if( mech->PilotDamage >= 6 )
		hits += std::string("  Killed");
	else if( mech->Unconscious )
		hits += std::string("  Unconscious");
	float health = std::min<float>( 1.f, std::max<float>( 0.f, 1.19f - mech->PilotDamage / 5.f ) );
	ItemFont->DrawText( hits, 10, y, Font::ALIGN_TOP_LEFT, 1.f,health,floorf(health),1.f );
}


// -----------------------------------------------------------------------------


RecordSheetStatus::RecordSheetStatus( SDL_Rect *rect, Font *title_font, Font *item_font )
: GroupBox( rect, "Status", title_font )
{
	Name = "RecordSheetStatus";
	ItemFont = item_font ? item_font : title_font;
}


RecordSheetStatus::~RecordSheetStatus()
{
}


void RecordSheetStatus::Draw( void )
{
	RecordSheet *rs = (RecordSheet*) Container;
	Mech *mech = rs ? rs->GetMech() : NULL;
	if( ! mech )
		return;
	
	GroupBox::Draw();
	
	int y = 17;
	float r = 1.f, g = 1.f, b = 1.f;
	if( mech->WalkDist() < mech->Walk )
		b = 0.f;
	ItemFont->DrawText( std::string("MP: ") + mech->MPString(), 10, y, Font::ALIGN_TOP_LEFT, r,g,b,1.f );
	
	r = g = b = 1.f;
	std::vector<std::string> status;
	if( mech->Destroyed() )
	{
		status.push_back("Destroyed");
		g = b = 0.f;
	}
	else
	{
		if( mech->Prone )
		{
			status.push_back("Prone");
			b = 0.f;
		}
		if( mech->Immobile() )
		{
			status.push_back("Immobile");
			b = 0.f;
		}
	}
	ItemFont->DrawText( Str::Join( status, ", " ), 105, y, Font::ALIGN_TOP_LEFT, r,g,b,1.f );
	
	y += ItemFont->GetHeight();
	float cool = std::min<float>( 1.f, std::max<int16_t>( 0, 34 - mech->Heat ) / 30.f );
	ItemFont->DrawText( std::string("Heat: ") + Num::ToString(mech->Heat), 10, y, Font::ALIGN_TOP_LEFT, 1.f,cool,floorf(cool),1.f );
	
	r = g = b = 1.f;
	for( std::vector<MechEquipment>::const_iterator eq = mech->Equipment.begin(); eq != mech->Equipment.end(); eq ++ )
	{
		if( eq->Damaged
		&& ((eq->ID == BattleTech::Equipment::ENGINE)
		 || (eq->ID == BattleTech::Equipment::SINGLE_HEAT_SINK)
		 || (eq->ID == BattleTech::Equipment::DOUBLE_HEAT_SINK)
		 || (eq->ID == BattleTech::Equipment::LASER_HEAT_SINK)) )
		{
			b = 0.f;
			break;
		}
	}
	ItemFont->DrawText( std::string("(") + Num::ToString(mech->HeatDissipation) + std::string(")"), 65, y, Font::ALIGN_TOP_LEFT, r,g,b,1.f );
	
	if( mech->Shutdown )
		ItemFont->DrawText( "Shutdown", 105, y, Font::ALIGN_TOP_LEFT, 1.f,1.f,0.f,1.f );
}


// -----------------------------------------------------------------------------


RecordSheetHealth::RecordSheetHealth( SDL_Rect *rect )
: Layer( rect )
{
	Name = "RecordSheetHealth";
}


RecordSheetHealth::~RecordSheetHealth()
{
}


void RecordSheetHealth::Draw( void )
{
	RecordSheet *rs = (RecordSheet*) Container;
	Mech *mech = rs ? rs->GetMech() : NULL;
	if( ! mech )
		return;
	
	double size = std::min<double>( Rect.w, Rect.h );
	double x = (Rect.w - size) / 2.;
	double y = (Rect.h - size) / 2.;
	mech->DrawHealth( x, y, size, size );
}
