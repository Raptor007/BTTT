/*
 *  BattleTechGame.cpp
 */

#include "BattleTechGame.h"

#include <cstddef>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <algorithm>
#include "BattleTechDefs.h"
#include "BattleTechServer.h"
#include "FirstLoadScreen.h"
#include "JoinMenu.h"
#include "HexBoard.h"
#include "SpawnMenu.h"
#include "GameMenu.h"
#include "Mech.h"
#include "Event.h"
#include "Num.h"
#include "Math2D.h"
#include "Screensaver.h"
#ifndef DT_DIR
#include <sys/stat.h>
#endif


BattleTechGame::BattleTechGame( std::string version ) : RaptorGame( "BTTT", version )
{
	DefaultPort = 3050;
	X = 0.;
	Y = 0.;
	Zoom = 20.;
	TeamTurn = 0;
	SelectedID = 0;
	TargetID = 0;
}


BattleTechGame::~BattleTechGame()
{
}


void BattleTechGame::SetDefaults( void )
{
	RaptorGame::SetDefaults();
	
	Gfx.AspectRatio = 1.5f;
	Cfg.Settings[ "g_fullscreen" ] = "false";
	Cfg.Settings[ "g_res_windowed_x" ] = "0";
	Cfg.Settings[ "g_res_windowed_y" ] = "0";
	Cfg.Settings[ "g_fsaa" ] = "4";
	Cfg.Settings[ "g_shader_enable" ] = "false";
	Cfg.Settings[ "s_volume" ] = "0.3";
	Cfg.Settings[ "sv_port" ] = "3050";
	Cfg.Settings[ "sv_maxfps" ] = "10";
	Cfg.Settings[ "host_address" ] = "www.raptor007.com:3050";
	
	if( Cfg.Settings[ "name" ] == "Name" )
		Cfg.Settings[ "name" ] = "Cadet";
	
	Cfg.Settings[ "mech" ] = " 60T [C] Mad Dog (Vulture)  Prime";
	Cfg.Settings[ "piloting" ] = "4";
	Cfg.Settings[ "gunnery"  ] = "3";
	
	Cfg.Settings[ "event_speed" ] = "0.5";
	Cfg.Settings[ "show_ecm" ] = "true";
	Cfg.Settings[ "record_sheet_popup" ] = "true";
	Cfg.Settings[ "map_drag" ] = "false";
}


void BattleTechGame::Setup( int argc, char **argv )
{
	bool safemode = false;
	for( int i = 1; i < argc; i ++ )
	{
		if( strcasecmp( argv[ i ], "-safe" ) == 0 )
			safemode = true;
	}
	
	Mouse.SetCursor( Res.GetAnimation("cursor.png"), 16, 1, 1 );
#ifdef USE_SYSTEM_CURSOR
	SDL_ShowCursor( SDL_ENABLE );
	Mouse.ShowCursor = false;
#endif
	
	Cam.SetPos( 0, 0, 1 );
	Cam.SetUpVec( 0, 1, 0 );
	Cam.SetFwdVec( 0, 0, -1 );
	
	// Show a loading screen while precaching resources.
	Gfx.Clear( 0.f, 0.f, 0.f );
	FirstLoadScreen *load_screen = new FirstLoadScreen("menu.png");
	Layers.Add( load_screen );
	Layers.Draw();
	Gfx.SwapBuffers();
	
	// Load HeavyMetal databases and parse ini files.
	LoadDatabases();
	LoadMechTex();
	LoadBiomes();
	
	// Precache any other resources.
	if( ! safemode )
		Precache();
	
	// Load all mech variants.
	load_screen->Text = "Loading Mechs...";
	Layers.Draw();
	Gfx.SwapBuffers();
	LoadVariants( Res.Find("Mechs") );
	
	// Add the main menu to the now-empty Layers stack.
	Layers.Remove( load_screen );
	load_screen = NULL;
	Layers.Add( new JoinMenu() );
}


void BattleTechGame::LoadDatabases( void )
{
	Struct   = HeavyMetal::Structures( Res.Find("INTRNAL.DAT").c_str() );
	ClanWeap = HeavyMetal::Weapons(    Res.Find("CL-WEAP.DAT").c_str() );
	ISWeap   = HeavyMetal::Weapons(    Res.Find("IS-WEAP.DAT").c_str() );
}


void BattleTechGame::LoadMechTex( void )
{
	std::ifstream input( Res.Find("MechTex.ini").c_str() );
	if( input.is_open() )
	{
		MechTex.clear();
		
		std::string name;
		char buffer[ 1024 ] = "";
		
		while( ! input.eof() )
		{
			buffer[ 0 ] = '\0';
			input.getline( buffer, sizeof(buffer) );
			
			// Remove comments.
			char *comment = strchr( buffer, ';' );
			if( comment )
				comment[ 0 ] = '\0';
			
			// Remove unnecessary characters from the buffer and skip empty lines.
			snprintf( buffer, sizeof(buffer), "%s", Str::Join( CStr::SplitToList( buffer, "\r\n]" ), "" ).c_str() );
			while( buffer[ 0 ] && (buffer[ strlen(buffer) - 1 ] == ' ') )
				buffer[ strlen(buffer) - 1 ] = '\0';
			if( ! strlen(buffer) )
				continue;
			
			// [Mech Name]
			if( buffer[ 0 ] == '[' )
			{
				name = buffer + 1;
				MechTex[ name ].clear();
				continue;
			}
			
			// XX=texture.png
			std::vector<std::string> pair = CStr::SplitToVector( buffer, "=" );
			if( pair.size() != 2 )
				continue;
			if( ! pair[ 1 ].length() )
				continue;
			const Animation *animation = Res.GetAnimation( pair[ 1 ] );
			if( pair[ 0 ] == "HD" )
				MechTex[ name ][ BattleTech::Loc::HEAD         ] = animation;
			else if( pair[ 0 ] == "CT" )
				MechTex[ name ][ BattleTech::Loc::CENTER_TORSO ] = animation;
			else if( pair[ 0 ] == "RT" )
				MechTex[ name ][ BattleTech::Loc::RIGHT_TORSO  ] = animation;
			else if( pair[ 0 ] == "LT" )
				MechTex[ name ][ BattleTech::Loc::LEFT_TORSO   ] = animation;
			else if( pair[ 0 ] == "*T" )
			{
				MechTex[ name ][ BattleTech::Loc::RIGHT_TORSO  ] = animation;
				MechTex[ name ][ BattleTech::Loc::LEFT_TORSO   ] = animation;
			}
			else if( pair[ 0 ] == "RA" )
				MechTex[ name ][ BattleTech::Loc::RIGHT_ARM    ] = animation;
			else if( pair[ 0 ] == "LA" )
				MechTex[ name ][ BattleTech::Loc::LEFT_ARM     ] = animation;
			else if( pair[ 0 ] == "*A" )
			{
				MechTex[ name ][ BattleTech::Loc::RIGHT_ARM    ] = animation;
				MechTex[ name ][ BattleTech::Loc::LEFT_ARM     ] = animation;
			}
			else if( pair[ 0 ] == "RL" )
				MechTex[ name ][ BattleTech::Loc::RIGHT_LEG    ] = animation;
			else if( pair[ 0 ] == "LL" )
				MechTex[ name ][ BattleTech::Loc::LEFT_LEG     ] = animation;
			else if( pair[ 0 ] == "*L" )
			{
				MechTex[ name ][ BattleTech::Loc::RIGHT_LEG    ] = animation;
				MechTex[ name ][ BattleTech::Loc::LEFT_LEG     ] = animation;
			}
		}
		
		input.close();
	}
}


void BattleTechGame::LoadBiomes( void )
{
	std::ifstream input( Res.Find("MapBiome.ini").c_str() );
	if( input.is_open() )
	{
		Biomes.clear();
		
		std::string name;
		char buffer[ 1024 ] = "";
		
		while( ! input.eof() )
		{
			buffer[ 0 ] = '\0';
			input.getline( buffer, sizeof(buffer) );
			
			// Remove comments.
			char *comment = strchr( buffer, ';' );
			if( comment )
				comment[ 0 ] = '\0';
			
			// Remove unnecessary characters from the buffer and skip empty lines.
			snprintf( buffer, sizeof(buffer), "%s", Str::Join( CStr::SplitToList( buffer, "\r\n]" ), "" ).c_str() );
			while( buffer[ 0 ] && (buffer[ strlen(buffer) - 1 ] == ' ') )
				buffer[ strlen(buffer) - 1 ] = '\0';
			if( ! strlen(buffer) )
				continue;
			
			// [Biome Name]
			if( buffer[ 0 ] == '[' )
			{
				name = buffer + 1;
				continue;
			}
			
			// colors=R,G,B,Rs,Gs,Bs
			std::vector<std::string> pair = CStr::SplitToVector( buffer, "=" );
			if( pair.size() != 2 )
				continue;
			if( pair[ 0 ] == "colors" )
				Biomes.push_back(std::pair<std::string,std::string>( name, pair[ 1 ] ));
		}
		
		input.close();
	}
}


size_t BattleTechGame::LoadVariants( std::string dir )
{
	size_t count = 0;
	
	if( DIR *dir_p = opendir( dir.c_str() ) )
	{
		bool debug = Cfg.SettingAsBool("debug");
		std::set<std::string> untextured;
		
		while( struct dirent *dir_entry_p = readdir(dir_p) )
		{
			if( ! dir_entry_p->d_name )
				continue;
			if( dir_entry_p->d_name[ 0 ] == '.' )
				continue;
#ifndef DT_DIR
			struct stat st;
			if( (fstat( dir_entry_p->d_ino, &st ) == 0) && S_ISDIR( st.st_mode ) )
#else
			if( dir_entry_p->d_type == DT_DIR )
#endif
			{
				count += LoadVariants( dir + std::string("/") + std::string(dir_entry_p->d_name) );
				continue;
			}
			
			Variant v;
			if( v.Load( (dir + std::string("/") + std::string(dir_entry_p->d_name)).c_str() ) )
			{
				char prefix[ 10 ] = "100T [C] ";
				snprintf( prefix, sizeof(prefix), "%3iT %s ", v.Tons, v.Clan?"[C]":"   " );
				Variants[ std::string(prefix) + v.Name + std::string(" ") + v.Var ] = v;
				count ++;
				
				if( v.Equipment.begin() == v.Equipment.end() )
					Console.Print( std::string("Bad variant: ") + dir + std::string("/") + std::string(dir_entry_p->d_name), TextConsole::MSG_ERROR );
				else if( v.Equipment.begin()->ID < 0x33 )
					Console.Print( std::string("Unarmed variant: ") + dir + std::string("/") + std::string(dir_entry_p->d_name), TextConsole::MSG_ERROR );
				
				if( debug && ! untextured.count(v.Name) && ! HasMechTex(v.Name) && v.Stock )
				{
					untextured.insert( v.Name );
					Console.Print( std::string("No MechTex match: ") + v.Name, TextConsole::MSG_ERROR );
				}
			}
		}
		closedir( dir_p );
	}
	
	return count;
}


bool BattleTechGame::HasMechTex( std::string name ) const
{
	for( std::map< std::string, std::map<uint8_t,const Animation*> >::const_iterator tex = MechTex.begin(); tex != MechTex.end(); tex ++ )
	{
		if( ! tex->first.length() )
			continue;
		int at = Str::FindInsensitive( name, tex->first );
		if( at >= 0 )
			return true;
	}
	return false;
}


void BattleTechGame::Precache( void )
{
	if( DIR *dir_p = opendir( Res.Find("Sounds").c_str() ) )
	{
		while( struct dirent *dir_entry_p = readdir(dir_p) )
		{
			if( ! dir_entry_p->d_name )
				continue;
			if( dir_entry_p->d_name[ 0 ] == '.' )
				continue;
			
			Res.GetSound( dir_entry_p->d_name );
		}
		closedir( dir_p );
	}
	
	if( DIR *dir_p = opendir( Res.Find("Textures").c_str() ) )
	{
		while( struct dirent *dir_entry_p = readdir(dir_p) )
		{
			if( ! dir_entry_p->d_name )
				continue;
			if( dir_entry_p->d_name[ 0 ] == '.' )
				continue;
			
			Res.GetAnimation( dir_entry_p->d_name );
		}
		closedir( dir_p );
	}
}


std::map< std::string, const Variant* > BattleTechGame::VariantSearch( std::string text, bool search_equipment ) const
{
	std::map<std::string,const Variant*> matches;
	std::list<std::string> words = Str::SplitToList( text, " " );
	
	for( std::list<std::string>::iterator word = words.begin(); word != words.end(); word ++ )
	{
		for( size_t i = 0; i < word->length(); i ++ )
		{
			// Replace underscores with spaces to allow searching for LRM_20 etc.
			if( (*word)[ i ] == '_' )
				(*word)[ i ] = ' ';
		}
	}
	
	for( std::map<std::string,Variant>::const_iterator var = Variants.begin(); var != Variants.end(); var ++ )
	{
		bool match = true;
		for( std::list<std::string>::const_iterator word = words.begin(); word != words.end(); word ++ )
		{
			size_t word_len = word->length();
			if( ! word_len )
				continue;
			
			int found = Str::FindInsensitive( var->first, *word );
			
			// Single letter search terms are probably the variant, so ignore mid-word matches.
			if( (found > 0) && (word_len == 1) )
				found = Str::FindInsensitive( var->first, std::string(" ") + *word );
			
			if( found < 0 )
			{
				match = false;
				
				if( (! var->second.Clan) && ((*word == "[IS]") || (*word == "[I]")) )
					match = true;
				else if( var->second.Stock && (! var->second.Quad) && (*word == "stock") )
					match = true;
				else if( (! var->second.Stock) && (*word == "custom") )
					match = true;
				else if( var->second.Quad && (*word == "quad") )
					match = true;
				else if( (*word == "textured") && HasMechTex(var->second.Name) )
					match = true;
				else if( (*word == "untextured") && ! HasMechTex(var->second.Name) )
					match = true;
				
				// Tonnage Range (xxT-xxT or xxT-100T)
				else if( (word_len >= 5) && (word->at(0) >= '0') && (word->at(0) <= '9')
				&&       (word->at(word_len-2) >= '0') && (word->at(word_len-2) <= '9')
				&&       (toupper(word->at(word_len-1)) == 'T') && (Str::FindInsensitive( *word, "T-" ) >= 0) )
				{
					const char *word_cstr = word->c_str();
					uint8_t min_tons = atoi( word_cstr );
					uint8_t max_tons = 100;
					int index = CStr::FindInsensitive( word_cstr, "T-" );
					if( index >= 0 )
						max_tons = atoi( word_cstr + index + 2 );
					match = (var->second.Tons >= min_tons) && (var->second.Tons <= max_tons);
				}
				
				// Era (xxxx or xxxx-xxxx)
				else if( atoi(word->c_str()) == var->second.Era )
					match = true;
				else if( (word_len == 9) && (word->at(4) == '-')
				&&       (word->at(0) >= '0') && (word->at(0) <= '9')
				&&       (word->at(1) >= '0') && (word->at(1) <= '9')
				&&       (word->at(2) >= '0') && (word->at(2) <= '9')
				&&       (word->at(3) >= '0') && (word->at(3) <= '9')
				&&       (word->at(5) >= '0') && (word->at(5) <= '9')
				&&       (word->at(6) >= '0') && (word->at(6) <= '9')
				&&       (word->at(7) >= '0') && (word->at(7) <= '9')
				&&       (word->at(8) >= '0') && (word->at(8) <= '9') )
				{
					int min_era = atoi(word->c_str());
					int max_era = atoi(word->c_str() + 5);
					match = (var->second.Era >= min_era) && (var->second.Era <= max_era);
				}
				
				else if( search_equipment && (word_len > 1) )
				{
					const std::map< short, HMWeapon > *weapons = var->second.Clan ? &ClanWeap : &ISWeap;
					for( std::vector<VariantEquipment>::const_iterator eq = var->second.Equipment.begin(); eq != var->second.Equipment.end(); eq ++ )
					{
						std::string eq_name = HeavyMetal::CritName( eq->ID, weapons );
						int eq_found = Str::FindInsensitive( eq_name, *word );
						
						// Equipment search should always match start of words, unless the search term is something like "/20" or "-X".
						if( (eq_found > 0) && (word->at(0) != '/') && (word->at(0) != '-') )
							eq_found = Str::FindInsensitive( eq_name, std::string(" ") + *word );
						
						if( eq_found >= 0 )
						{
							match = true;
							break;
						}
					}
				}
				
				if( ! match )
					break;
			}
		}
		
		if( match )
			matches[ var->first ] = &(var->second);
	}
	
	return matches;
}


void BattleTechGame::Update( double dt )
{
	RaptorGame::Update( dt );
	
	bool screensaver = Cfg.SettingAsBool("screensaver");
	
	if( Events.size() && (EventClock.RemainingSeconds() <= 0.) )
	{
		Event e = Events.front();
		Events.pop();
		
		double duration = e.Duration;
		double event_speed = std::max<double>( 0.1, Cfg.SettingAsDouble("event_speed",1.) );
		if( e.Text.length() || (event_speed > 1.) )
			duration /= event_speed;
		
		EventClock.Reset( duration );
		PlayEvent( &e, duration );
		
		// Keep screensaver zoomed to the action.
		if( screensaver )
		{
			HexBoard *hex_board = (HexBoard*) Layers.Find("HexBoard");
			if( hex_board )
				hex_board->KeyDown( SDLK_e );
		}
	}
	
	if( screensaver && (State == BattleTech::State::GAME_OVER) && Raptor::Server->IsRunning() && ! PlayingEvents() && (EventClock.ElapsedSeconds() >= 3.) )
	{
		BattleTechServer *server = (BattleTechServer*) Raptor::Server;
		std::set<uint32_t> remove;
		for( std::map<uint32_t,GameObject*>::iterator obj_iter = server->Data.GameObjects.begin(); obj_iter != server->Data.GameObjects.end(); obj_iter ++ )
		{
			if( obj_iter->second->Type() == BattleTech::Object::MAP )
			{
				HexMap *map = (HexMap*) obj_iter->second;
				map->Clear();
				map->Randomize();
				server->Console->Print( "================================================================================" );
				server->Console->Print( std::string("Map seed: ") + Num::ToString((int) map->Seed) );
				
				Packet update = Packet( Raptor::Packet::UPDATE );
				update.AddChar( 127 ); // Precision
				update.AddUInt( 1 );   // Number of Objects
				update.AddUInt( map->ID );
				map->AddToUpdatePacketFromServer( &update, 127 );
				server->Net.SendAll( &update );
			}
			else
				remove.insert( obj_iter->second->ID );
		}
		if( remove.size() )
		{
			Packet objects_remove = Packet( Raptor::Packet::OBJECTS_REMOVE );
			objects_remove.AddUInt( remove.size() );
			for( std::set<uint32_t>::iterator id_iter = remove.begin(); id_iter != remove.end(); id_iter ++ )
			{
				server->Data.RemoveObject( *id_iter );
				objects_remove.AddUInt( *id_iter );
			}
			server->Net.SendAll( &objects_remove );
		}
		
		Cfg.Command( "ready" );
	}
}


void BattleTechGame::PlayEvent( const Event *e )
{
	PlayEvent( e, e->Duration );
}


void BattleTechGame::PlayEvent( const Event *e, double duration )
{
	if( e->Text.length() )
		Console.Print( e->Text );
	if( e->Sound.length() )
		Snd.Play( Res.GetSound(e->Sound) );
	
	Mech *mech = e->MechID ? (Mech*) Data.GetObject( e->MechID ) : NULL;
	if( mech && (mech->Type() == BattleTech::Object::MECH) )
	{
		// Events may change Mech readiness.
		ReadyAndAbleCache.clear();
		
		e->ReadStat( mech );
		
		if( e->HealthUpdate || e->CriticalHit )
		{
			mech->HitClock.Reset();
			mech->Locations[ e->Loc ].HitClock.Sync( &(mech->HitClock) );
			mech->Locations[ e->Loc ].HitWhere = e->Arc;
			
			if( Cfg.SettingAsBool("record_sheet_popup") )
				ShowRecordSheet( mech, std::max<double>( 5., duration ) );
		}
		
		if( e->HealthUpdate )
		{
			MechLocation *loc = &(mech->Locations[ e->Loc ]);
			bool had_structure = loc->Structure;
			*(loc->ArmorPtr( e->Arc )) = e->HP;
			if( (e->Arc == BattleTech::Arc::STRUCTURE) && ! e->HP )
			{
				while( loc->CriticalSlots.size() )
				{
					std::vector<MechEquipment*>::iterator crit = loc->CriticalSlots.begin();
					(*crit)->Damaged ++;
					loc->DamagedCriticals.insert( *crit );
					loc->CriticalSlots.erase( crit );
				}
				if( had_structure && (e->Loc != BattleTech::Loc::CENTER_TORSO) )
				{
					loc->Destroyed = true;
					loc->DestroyedClock.Reset();
				}
			}
		}
		
		if( e->CriticalHit && (e->Eq < mech->Equipment.size()) )
		{
			MechEquipment *eq = &(mech->Equipment[ e->Eq ]);
			MechLocation *loc = &(mech->Locations[ e->Loc ]);
			for( std::vector<MechEquipment*>::iterator crit = loc->CriticalSlots.begin(); crit != loc->CriticalSlots.end(); crit ++ )
			{
				if( (*crit) == eq )
				{
					eq->Damaged ++;
					loc->DamagedCriticals.insert( *crit );
					loc->CriticalSlots.erase( crit );
					break;
				}
			}
		}
		
		if( (e->Effect >= BattleTech::Effect::FIRST_MOVEMENT)
		&&  (e->Effect <= BattleTech::Effect::LAST_MOVEMENT) )
		{
			if( (e->Effect == BattleTech::Effect::STEP) || (e->Effect == BattleTech::Effect::TURN) || (e->Effect == BattleTech::Effect::FALL) )
			{
				double event_speed = std::max<double>( 1., Cfg.SettingAsDouble("event_speed",1.) );
				duration = std::min<double>( 0.3, 0.5 / event_speed );
			}
			else if( (State == BattleTech::State::SETUP) && ((e->Effect == BattleTech::Effect::JUMP) || (e->Effect == BattleTech::Effect::STAND) || (e->Effect == BattleTech::Effect::FALL)) )
				duration = 0.2;
			else if( e->Effect == BattleTech::Effect::TORSO_TWIST )
				duration = 0.2;
			mech->Animate( duration, e->Effect );
			mech->X = e->X;
			mech->Y = e->Y;
			e->ReadFacing( mech );
			if( (mech->ID == SelectedID) || (mech->ID == TargetID) )
			{
				HexBoard *hex_board = (HexBoard*) Layers.Find("HexBoard");
				if( hex_board )
					hex_board->UpdateAim();
			}
			if( State == BattleTech::State::SETUP )
			{
				mech->ArmsFlipped = false;
				mech->TorsoTwist = 0;
				mech->ProneFire = BattleTech::Loc::UNKNOWN;
			}
			else if( (e->Effect == BattleTech::Effect::FALL)
			||       (e->Effect == BattleTech::Effect::JUMP) )
			{
				HexMap *map = Map();
				Pos3D pos = map->Center( e->X, e->Y );
				Data.Effects.push_back( Effect( Res.GetAnimation("dust.ani"), (e->Effect == BattleTech::Effect::FALL) ? 1.5 : 1.0, NULL, 0, &pos, NULL, 0., 1. ) );
				if( e->Effect == BattleTech::Effect::JUMP )
				{
					Data.Effects.back().Lifetime.CountUpToSecs = duration;
					Pos3D prev = map->Center( mech->FromX, mech->FromY );
					Data.Effects.push_back( Effect( Res.GetAnimation("dust.ani"), 1.0, NULL, 0, &prev, NULL, 0., 1. ) );
					if( State != BattleTech::State::SETUP )
					{
						int hexes = map->HexDist( mech->FromX, mech->FromY, mech->X, mech->Y );
						for( int i = 1; i < hexes; i ++ )
						{
							double progress = i / (double) hexes;
							Pos3D inter( &pos );
							inter.X = pos.X * progress + prev.X * (1. - progress);
							inter.Y = pos.Y * progress + prev.Y * (1. - progress);
							Data.Effects.push_back( Effect( Res.GetAnimation("dust.ani"), 0.7, NULL, 0, &inter, NULL, 0., 1.5 ) );
							Data.Effects.back().Rotation = Rand::Double(0.,360.);
							Data.Effects.back().Lifetime.CountUpToSecs = duration * progress;
						}
					}
				}
			}
			else if( e->Effect == BattleTech::Effect::STAND )
				mech->StandAttempts ++;
		}
		else if( e->Effect == BattleTech::Effect::SMOKE )
		{
			HexMap *map = Map();
			Pos3D pos = map->Center( mech->X, mech->Y );
			for( uint8_t i = 0; i < e->Misc; i ++ )
			{
				Vec3D vec( Rand::Double(0.,0.5), Rand::Double(-1.5,-1.), 0. );
				double dist = vec.Length();
				double speed = Rand::Double(0.5,1.);
				double size = Rand::Double(0.4,0.7);
				double delay = i * 0.1;
				Data.Effects.push_back( Effect( Res.GetAnimation("smoke.ani"), size, NULL, 0, &pos, &vec, 30., Rand::Double(0.9,1.1), std::max<double>( 0.1, dist - size*0.2 ) / speed ) );
				Data.Effects.back().Rotation = Rand::Double(0.,360.);
				Data.Effects.back().Lifetime.CountUpToSecs = delay;
				Data.Effects.back().MoveAlong( &vec, delay * speed * -1. );
			}
		}
		else if( e->Effect == BattleTech::Effect::EXPLOSION )
		{
			HexMap *map = Map();
			Pos3D pos = map->Center( e->X, e->Y );
			if( e->Loc < BattleTech::Loc::COUNT )
			{
				int dir = mech->Facing;
				if( ! mech->Locations[ e->Loc ].IsLeg )
					dir += mech->TorsoTwist;
				Vec3D fwd = map->Direction( dir );
				Vec3D axis(0,0,1);
				Vec3D right = fwd;
				right.RotateAround( &axis, 90. );
				double radius = 0.5 * pow( mech->Tons / (double) 100., 0.333 );
				if( e->Loc == BattleTech::Loc::LEFT_ARM )
				{
					pos += right * -0.8 * radius;
					if( mech->ArmsFlipped )
						pos += fwd * -0.7 * radius;
				}
				else if( e->Loc == BattleTech::Loc::RIGHT_ARM )
				{
					pos += right * 0.8 * radius;
					if( mech->ArmsFlipped )
						pos += fwd * -0.7 * radius;
				}
				else if( e->Loc == BattleTech::Loc::LEFT_TORSO )
				{
					pos += right * -0.4 * radius;
					if( e->Arc == BattleTech::Arc::REAR )
						pos += fwd * -0.5 * radius;
				}
				else if( e->Loc == BattleTech::Loc::RIGHT_TORSO )
				{
					pos += right * 0.4 * radius;
					if( e->Arc == BattleTech::Arc::REAR )
						pos += fwd * -0.5 * radius;
				}
				else if( e->Loc == BattleTech::Loc::LEFT_LEG )
				{
					pos += right * -0.5 * radius;
					pos += fwd * 0.3 * radius;
				}
				else if( e->Loc == BattleTech::Loc::RIGHT_LEG )
				{
					pos += right * 0.5 * radius;
					pos += fwd * 0.3 * radius;
				}
				else if( e->Loc == BattleTech::Loc::CENTER_TORSO )
					pos += fwd * ((e->Arc == BattleTech::Arc::REAR) ? -0.5 : 0.2) * radius;
			}
			const char *texture = "explosion.ani";
			double size = 1.3;
			if( e->Misc )
			{
				size = 0.7;
				if( (e->Misc == BattleTech::Effect::LARGE_LASER)
				||  (e->Misc == BattleTech::Effect::LARGE_PULSE) )
				{
					texture = "llas_hit.ani";
					size = 0.8;
				}
				else if( (e->Misc == BattleTech::Effect::MEDIUM_LASER)
				||       (e->Misc == BattleTech::Effect::MEDIUM_PULSE) )
					texture = "mlas_hit.ani";
				else if( (e->Misc == BattleTech::Effect::SMALL_LASER)
				||       (e->Misc == BattleTech::Effect::SMALL_PULSE) )
				{
					texture = "slas_hit.ani";
					size = 0.65;
				}
				else if( e->Misc == BattleTech::Effect::FLAMER )
					size = 0.8;
				else if( e->Misc == BattleTech::Effect::PLASMA )
					size = 0.9;
				else if( e->Misc == BattleTech::Effect::PPC )
				{
					texture = "ppc_hit.ani";
					size = 1.2;
				}
				else if( e->Misc == BattleTech::Effect::GAUSS )
				{
					texture = "boom.ani";
					size = 0.8;
				}
				else if( e->Misc == BattleTech::Effect::AUTOCANNON )
				{
					texture = "boom.ani";
					size = 0.65;
				}
				else if( e->Misc == BattleTech::Effect::MACHINEGUN )
				{
					texture = "boom.ani";
					size = 0.4;
				}
				else if( e->Misc == BattleTech::Effect::NARC )
				{
					texture = "dust.ani";
					size = 0.5;
				}
				else if( e->Misc == BattleTech::Effect::TAG )
					texture = NULL;
				else if( (e->Misc >= BattleTech::Effect::FIRST_MELEE)
				&&       (e->Misc <= BattleTech::Effect::LAST_MELEE) )
				{
					texture = "boom.ani";
					size = 0.6;
				}
				else if( e->Misc == BattleTech::Effect::EXPLOSION )
					size = 10.;
			}
			else  // If e->Misc is NONE, this is a Mech death event.
			{
				mech->Locations[ BattleTech::Loc::CENTER_TORSO ].Destroyed = true;
				mech->Locations[ BattleTech::Loc::CENTER_TORSO ].DestroyedClock.Reset();
			}
			if( texture )
				Data.Effects.push_back( Effect( Res.GetAnimation(texture), size, NULL, 0, &pos, NULL, 0., 1. ) );
		}
		else if( (e->Effect >= BattleTech::Effect::FIRST_WEAPON)
		&&       (e->Effect <= BattleTech::Effect::LAST_WEAPON) )
		{
			uint8_t count = e->Misc & 0x7F;
			bool    miss  = e->Misc & 0x80;
			HexMap *map = Map();
			Pos3D pos1 = map->Center( mech->X, mech->Y );
			if( e->Loc < BattleTech::Loc::COUNT )
			{
				int dir = mech->Facing;
				if( ! mech->Locations[ e->Loc ].IsLeg )
					dir += mech->TorsoTwist;
				Vec3D fwd = map->Direction( dir );
				Vec3D axis(0,0,1);
				Vec3D right = fwd;
				right.RotateAround( &axis, 90. );
				double radius = 0.5 * pow( mech->Tons / (double) 100., 0.333 );
				if( e->Loc == BattleTech::Loc::LEFT_ARM )
					pos1 += right * -0.8 * radius;
				else if( e->Loc == BattleTech::Loc::RIGHT_ARM )
					pos1 += right * 0.8 * radius;
				else if( e->Loc == BattleTech::Loc::LEFT_TORSO )
					pos1 += right * -0.4 * radius;
				else if( e->Loc == BattleTech::Loc::RIGHT_TORSO )
					pos1 += right * 0.4 * radius;
				else if( e->Loc == BattleTech::Loc::LEFT_LEG )
					pos1 += right * -0.5 * radius;
				else if( e->Loc == BattleTech::Loc::RIGHT_LEG )
					pos1 += right * 0.5 * radius;
			}
			Pos3D pos2 = map->Center( e->X, e->Y );
			Vec3D vec = pos2 - pos1;
			if( miss )
			{
				bool left = (e->Loc == BattleTech::Loc::LEFT_ARM) || (e->Loc == BattleTech::Loc::LEFT_TORSO) || (e->Loc == BattleTech::Loc::LEFT_LEG);
				if( e->Loc == BattleTech::Loc::CENTER_TORSO )
					left = Rand::Bool();
				if( mech->ArmsFlipped || (e->Loc == mech->TurnedArm) )
					left = ! left;
				Vec3D offset( &vec );
				offset.ScaleTo( Rand::Double( 0.45, 0.5 ) );
				offset.RotateAround( &(Cam.Fwd), left ? Rand::Double( 50., 130. ) : Rand::Double( -130., -50. ) );
				vec += offset;
			}
			double dist = vec.Length();
			const char *texture = "missile.ani";
			double size = 0.8;
			double speed = 15.;
			double spacing = 0.1;
			if( e->Effect == BattleTech::Effect::MISSILE )
			{
				speed = 9.;
				size = 0.5;
			}
			else if( (e->Effect == BattleTech::Effect::LARGE_LASER)
			||       (e->Effect == BattleTech::Effect::LARGE_PULSE) )
			{
				texture = "llas.png";
				size = 0.85;
			}
			else if( (e->Effect == BattleTech::Effect::MEDIUM_LASER)
			||       (e->Effect == BattleTech::Effect::MEDIUM_PULSE) )
			{
				texture = "mlas.png";
				size = 0.8;
			}
			else if( (e->Effect == BattleTech::Effect::SMALL_LASER)
			||       (e->Effect == BattleTech::Effect::SMALL_PULSE) )
			{
				texture = "slas.png";
				size = 0.6;
			}
			else if( e->Effect == BattleTech::Effect::PPC )
			{
				texture = "ppc.ani";
				speed = 13.;
				size = 1.1;
			}
			else if( e->Effect == BattleTech::Effect::FLAMER )
			{
				texture = "flamer.png";
				speed = 10.;
				size = 0.6;
			}
			else if( e->Effect == BattleTech::Effect::PLASMA )
			{
				texture = "flamer.png";
				speed = 14.;
				size = 0.8;
			}
			else if( e->Effect == BattleTech::Effect::GAUSS )
			{
				texture = "gauss.png";
				speed = 13.;
				size = 0.3;
			}
			else if( e->Effect == BattleTech::Effect::AUTOCANNON )
			{
				texture = "ballistic.png";
				speed = 13.;
				size = 0.9;
			}
			else if( e->Effect == BattleTech::Effect::MACHINEGUN )
			{
				texture = "ballistic.png";
				speed = 11.;
				size = 0.5;
			}
			else if( e->Effect == BattleTech::Effect::NARC )
			{
				texture = "narc.ani";
				speed = 6.;
				size = 0.6;
			}
			else if( e->Effect == BattleTech::Effect::TAG )
			{
				texture = "slas.png";
				count *= miss ? 25 : 40;
				spacing = 0.01;
				speed = 20.;
				size = 0.3;
			}
			if( (e->Effect == BattleTech::Effect::LARGE_PULSE)
			||  (e->Effect == BattleTech::Effect::MEDIUM_PULSE)
			||  (e->Effect == BattleTech::Effect::SMALL_PULSE) )
			{
				count *= 2;
				spacing = 0.03;
				size *= 0.75;
			}
			speed *= std::max<double>( 1., Cfg.SettingAsDouble("event_speed",1.) );
			vec.ScaleTo( speed );
			for( uint8_t i = 0; i < count; i ++ )
			{
				double delay = i * spacing;
				Data.Effects.push_back( Effect( Res.GetAnimation(texture), size, NULL, 0, &pos1, &vec, 0., Rand::Double(0.8,1.2), std::max<double>( 0.1, dist - size*0.2 ) / speed ) );
				Data.Effects.back().Rotation = Num::RadToDeg( Math2D::Angle( vec.X, vec.Y * -1. ) );
				Data.Effects.back().Lifetime.CountUpToSecs = delay;
				Data.Effects.back().MoveAlong( &vec, delay * speed * -1. );
			}
		}
		else if( (e->Effect >= BattleTech::Effect::FIRST_MELEE)
		&&       (e->Effect <= BattleTech::Effect::LAST_MELEE) )
		{
			mech->Animate( 0.5, e->Effect );
		}
		else if( e->Effect == BattleTech::Effect::DECLARE_ATTACK )
		{
			HexMap *map = Map();
			const Mech *target = ((e->X != mech->X) || (e->Y != mech->Y)) ? map->MechAt_const( e->X, e->Y ) : NULL;
			mech->DeclaredTarget = target ? target->ID : 0;
			if( mech->DeclaredTarget )
			{
				if( State == BattleTech::State::WEAPON_ATTACK )
					mech->DeclaredWeapons[ e->Misc ] = 1;  // FIXME: Fire count!
				else if( State == BattleTech::State::PHYSICAL_ATTACK )
					mech->DeclaredMelee.insert( e->Misc );
				Pos3D pos1 = map->Center( mech->X, mech->Y );
				Pos3D pos2 = map->Center( e->X, e->Y );
				Vec3D vec = pos2 - pos1;
				Pos3D center = pos1 + vec/2.;
				double dist = vec.Length();
				vec.ScaleTo( 1. );
				center.MoveAlong( &vec, -0.25 );
				double event_speed = std::max<double>( 1., Cfg.SettingAsDouble("event_speed",1.) );
				duration = 0.6 / event_speed;
				Vec3D motion = vec * 0.5 / duration;
				double angle = Math2D::Angle( vec.X, vec.Y * -1. );
				Clock clock;
				Data.Effects.push_back( Effect( Res.GetAnimation("line.png"), std::max<double>( 0.8, dist - 0.5 ), 0.03, NULL, 0, &center, &motion, 0., 1., duration ) );
				Data.Effects.back().Rotation = Num::RadToDeg(angle);
				Data.Effects.back().Lifetime.Sync( &clock );
				Data.Effects.back().Red = Data.Effects.back().Green = Data.Effects.back().Blue = 0.4f;
				Data.Effects.back().Alpha = 0.9f;
				for( int i = 0; i < 5; i ++ )
				{
					angle += M_PI/3.;
					vec.Rotate( -60. );
					motion.Rotate( -60. );
					for( uint8_t i = 0; i < 3; i ++ )
					{
						Data.Effects.push_back( Effect( Res.GetAnimation("line.png"), 0.25, 0.03, NULL, 0, &pos2, &motion, 0., 1., duration ) );
						Data.Effects.back().Rotation = Num::RadToDeg(angle);
						Data.Effects.back().MoveAlong( &vec, -0.625 );
						Data.Effects.back().Lifetime.Sync( &clock );
						Data.Effects.back().Red = Data.Effects.back().Green = Data.Effects.back().Blue = 0.4f;
						Data.Effects.back().Alpha = 0.9f;
					}
				}
			}
		}
		else if( e->Effect == BattleTech::Effect::BLINK )
		{
			HexMap *map = Map();
			Pos3D pos = map->Center( mech->X, mech->Y );
			Clock clock;
			float r = ((e->Misc & 0xE0) >> 5) / 7.f;
			float g = ((e->Misc & 0x1C) >> 2) / 7.f;
			float b =  (e->Misc & 0x03)       / 3.f;
			for( double angle = 0; angle < M_PI * 1.9; angle += M_PI/3. )
			{
				Vec3D vec( sin(angle) * 0.5, cos(angle) * -0.5, 0. );
				for( uint8_t i = 0; i < 3; i ++ )
				{
					double delay = i * 0.2;
					Data.Effects.push_back( Effect( Res.GetAnimation("line.png"), 0.6, 0.1, NULL, 0, &pos, NULL, 0., 1., 0.125 ) );
					Data.Effects.back().Rotation = Num::RadToDeg(-angle) - 90.;
					Data.Effects.back().MoveAlong( &vec, 0.5 );
					Data.Effects.back().Lifetime.Sync( &clock );
					Data.Effects.back().Lifetime.CountUpToSecs = delay;
					Data.Effects.back().Red   = r;
					Data.Effects.back().Green = g;
					Data.Effects.back().Blue  = b;
				}
			}
		}
	}
	else if( e->Effect == BattleTech::Effect::ATTENTION )
	{
		HexMap *map = Map();
		Pos3D pos = map->Center( e->X, e->Y );
		Clock clock;
		float r = ((e->Misc & 0xE0) >> 5) / 7.f;
		float g = ((e->Misc & 0x1C) >> 2) / 7.f;
		float b =  (e->Misc & 0x03)       / 3.f;
		for( double angle = 0; angle < M_PI * 1.9; angle += M_PI/3. )
		{
			Vec3D vec( sin(angle) * 0.5, cos(angle) * -0.5, 0. );
			Vec3D motion = vec * -42.;
			for( uint8_t i = 0; i < 3; i ++ )
			{
				double delay = i ? (0.4 + i * 0.3) : 0.;
				Data.Effects.push_back( Effect( Res.GetAnimation("line.png"), i?0.6:1., i?0.1:0.2, NULL, 0, &pos, i?NULL:&motion, 0., 1., i?0.2:0.48 ) );
				Data.Effects.back().Rotation = Num::RadToDeg(-angle) - 90.;
				Data.Effects.back().MoveAlong( &vec, i?0.5:10.5 );
				Data.Effects.back().Lifetime.Sync( &clock );
				Data.Effects.back().Lifetime.CountUpToSecs = delay;
				Data.Effects.back().Red   = r;
				Data.Effects.back().Green = g;
				Data.Effects.back().Blue  = b;
			}
		}
	}
}


void BattleTechGame::ShowRecordSheet( const Mech *mech, double duration )
{
	// Remove pop-up record sheets for any other Mechs.
	uint32_t mech_id = mech ? mech->ID : 0;
	for( std::list<Layer*>::reverse_iterator layer_iter = Layers.Layers.rbegin(); layer_iter != Layers.Layers.rend(); layer_iter ++ )
	{
		if( (! (*layer_iter)->Removed) && ((*layer_iter)->Name == "RecordSheet") )
		{
			RecordSheet *rs = (RecordSheet*) *layer_iter;
			if( rs->AutoHide && (rs->MechID != mech_id) )
				rs->Remove();
		}
	}
	
	RecordSheet *rs = GetRecordSheet( mech );
	
	if( ! rs )
	{
		if( ! mech )
			return;
		rs = new RecordSheet( mech );
		Layers.Add( rs );
		rs->AutoHide = duration;
	}
	else if( rs->AutoHide && ! duration )
		rs->AutoHide = false;
	
	rs->Lifetime.CountUpToSecs = duration;
	rs->Lifetime.Reset();
	rs->MoveToTop();
	
	if( rs->AutoHide )
		rs->AutoPosition();
}


RecordSheet *BattleTechGame::GetRecordSheet( const Mech *mech )
{
	for( std::list<Layer*>::reverse_iterator layer_iter = Layers.Layers.rbegin(); layer_iter != Layers.Layers.rend(); layer_iter ++ )
	{
		if( (! (*layer_iter)->Removed) && ((*layer_iter)->Name == "RecordSheet") )
		{
			RecordSheet *rs = (RecordSheet*) *layer_iter;
			if( (! mech) || (rs->MechID == mech->ID) )
				return rs;
		}
	}
	
	return NULL;
}


bool BattleTechGame::HandleEvent( SDL_Event *event )
{
	return RaptorGame::HandleEvent( event );
}


bool BattleTechGame::HandleCommand( std::string cmd, std::vector<std::string> *params )
{
	if( cmd == "map" )
	{
		if( State >= Raptor::State::CONNECTED )
		{
			uint16_t w = 0, h = 0;
			uint32_t seed = time(NULL) % 9999 + 1;
			if( params && (params->size() >= 2) )
			{
				w = atoi(params->at(0).c_str());
				h = atoi(params->at(1).c_str());
			}
			if( params && params->size() && (params->size() != 2) )
				seed = atoi(params->at( std::min<size_t>(3,params->size()-1) ).c_str());
			Packet change_map( BattleTech::Packet::CHANGE_MAP );
			change_map.AddUShort( w );
			change_map.AddUShort( h );
			change_map.AddUInt( seed );
			Net.Send( &change_map );
		}
		else
			Console.Print( "Must be in a game to set map seed.", TextConsole::MSG_ERROR );
	}
	else if( cmd == "team" )
	{
		if( State >= Raptor::State::CONNECTED )
		{
			if( params && params->size() )
			{
				Packet player_properties = Packet( Raptor::Packet::PLAYER_PROPERTIES );
				player_properties.AddUShort( PlayerID );
				player_properties.AddUInt( 1 );
				player_properties.AddString( "team" );
				player_properties.AddString( params->at(0) );
				Net.Send( &player_properties );
				
				GameMenu *gm = (GameMenu*) Layers.Find("GameMenu");
				if( gm )
					gm->Refresh( 0.3 );
			}
			else
			{
				int team_num = MyTeam();
				if( team_num )
				{
					std::string team_name = TeamName( team_num );
					Console.Print( std::string("You are on team ") + Num::ToString(team_num) + (team_name.length() ? (std::string(": ") + team_name) : std::string(".")) );
				}
				else
					Console.Print( "You are not on a team." );
			}
		}
		else
			Console.Print( "Must be in a game to choose team.", TextConsole::MSG_ERROR );
	}
	else if( cmd == "spawn" )
	{
		if( State < Raptor::State::CONNECTED )
		{
			Console.Print( "Must be in a game to spawn Mechs.", TextConsole::MSG_ERROR );
			return true;
		}
		
		const Variant *variant = NULL;
		if( params && params->size() )
		{
			std::map<std::string,const Variant*> matches = VariantSearch( params->at(0) );
			if( matches.size() )
			{
				std::map<std::string,const Variant*>::const_iterator match = matches.begin();
				int index = Rand::Int( 0, matches.size() - 1 );
				for( int i = 0; i < index; i ++ )
					match ++;
				variant = match->second;
			}
			else
			{
				Console.Print( "Could not find Mech: " + params->at(0), TextConsole::MSG_ERROR );
				return true;
			}
		}
		else if( Variants.size() )
		{
			std::map<std::string,Variant>::const_iterator mech = Variants.begin();
			int index = Rand::Int( 0, Variants.size() - 1 );
			for( int i = 0; i < index; i ++ )
				mech ++;
			variant = &(mech->second);
		}
		else
		{
			Console.Print( "Cannot spawn from empty Mech list.", TextConsole::MSG_ERROR );
			return true;
		}
		
		HexMap *map = Map();
		size_t w, h;
		map->GetSize( &w, &h );
		uint8_t x = Rand::Int( 0, w - 1 );
		uint8_t y = Rand::Int( 0, h - 1 - x%2 );
		uint8_t facing = ((size_t)y+1 > h/2) ? 0 : 3;
		int8_t piloting = 4, gunnery = 3;
		double chance = 1.;
		
		if( params && (params->size() > 1) )
		{
			for( size_t i = 1; i < params->size(); i ++ )
			{
				const char *param = params->at(i).c_str();
				if( param[ 0 ] == 'X' )
					x = atoi( param + 1 );
				else if( param[ 0 ] == 'Y' )
					y = atoi( param + 1 );
				else if( param[ 0 ] == 'F' )
					facing = (atoi( param + 1 ) + 6) % 6;
				else if( param[ 0 ] == 'P' )
					piloting = atoi( param + 1 );
				else if( param[ 0 ] == 'G' )
					gunnery = atoi( param + 1 );
				else if( param[ 0 ] == 'C' )
					chance = atof( param + 1 ) / 100.;
			}
		}
		
		if( (chance < 1.) && ! Rand::Bool( chance ) )
			return true;
		
		Packet spawn_mech( BattleTech::Packet::SPAWN_MECH );
		spawn_mech.AddUChar( x );
		spawn_mech.AddUChar( y );
		spawn_mech.AddUChar( facing );
		variant->AddToPacket( &spawn_mech );
		spawn_mech.AddChar( piloting );
		spawn_mech.AddChar( gunnery );
		Net.Send( &spawn_mech );
		
		GameMenu *gm = (GameMenu*) Layers.Find("GameMenu");
		if( gm )
			gm->Refresh( 0.3 );
		
		SpawnMenu *sm = (SpawnMenu*) Layers.Find("SpawnMenu");
		if( sm && sm->SearchBox && sm->SearchBox->Container )
			sm->SearchBox->Container->Selected = NULL;
	}
	else if( cmd == "clear" )
	{
		if( State != BattleTech::State::SETUP )
		{
			Console.Print( "Must be at Setup phase to clear all Mechs.", TextConsole::MSG_ERROR );
			return true;
		}
		
		std::set<uint8_t> x, y, t;
		if( params && params->size() )
		{
			for( size_t i = 0; i < params->size(); i ++ )
			{
				const char *param = params->at(i).c_str();
				if( param[ 0 ] == 'X' )
					x.insert( atoi( param + 1 ) );
				else if( param[ 0 ] == 'Y' )
					y.insert( atoi( param + 1 ) );
				else if( param[ 0 ] == 'T' )
					t.insert( atoi( param + 1 ) );
			}
		}
		
		for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
		{
			if( obj_iter->second->Type() == BattleTech::Object::MECH )
			{
				const Mech *mech = (const Mech*) obj_iter->second;
				if( x.size() && (x.find( mech->X ) == x.end()) )
					continue;
				if( y.size() && (y.find( mech->Y ) == y.end()) )
					continue;
				if( t.size() && (t.find( mech->Team ) == t.end()) )
					continue;
				
				Packet remove_mech( BattleTech::Packet::REMOVE_MECH );
				remove_mech.AddUInt( mech->ID );
				Net.Send( &remove_mech );
			}
		}
		
		GameMenu *gm = (GameMenu*) Layers.Find("GameMenu");
		if( gm )
			gm->Refresh( 0.3 );
	}
	else if( cmd == "ready" )
	{
		uint8_t team_num = MyTeam();
		if( (team_num && (State >= Raptor::State::CONNECTED)) || (State == BattleTech::State::GAME_OVER) )
		{
			Packet ready = Packet( BattleTech::Packet::READY );
			ready.AddUChar( team_num );
			Net.Send( &ready );
		}
		else
			Console.Print( "Must be in a game and on a team to ready up.", TextConsole::MSG_ERROR );
	}
	else if( cmd == "go" )
		Cfg.Command( "sv state ++" );
	else if( cmd == "reload" )
	{
		bool reload_mechtex = true;
		bool reload_mechs = true;
		bool reload_biomes = true;
		if( params && params->size() )
		{
			reload_mechtex = (std::find( params->begin(), params->end(), "mechtex" ) != params->end());
			reload_mechs   = (std::find( params->begin(), params->end(), "mechs"   ) != params->end());
			reload_biomes  = (std::find( params->begin(), params->end(), "biomes"  ) != params->end());
		}
		if( reload_mechtex )
		{
			LoadMechTex();
			for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
			{
				if( obj_iter->second->Type() == BattleTech::Object::MECH )
					((Mech*)( obj_iter->second ))->InitTex();
			}
		}
		if( reload_mechs )
		{
			Variants.clear();
			LoadVariants( Res.Find("Mechs") );
		}
		if( reload_biomes )
		{
			Biomes.clear();
			LoadBiomes();
		}
	}
	else
		return RaptorGame::HandleCommand( cmd, params );
	return true;
}


bool BattleTechGame::ProcessPacket( Packet *packet )
{
	packet->Rewind();
	PacketType type = packet->Type();
	
	if( type == Raptor::Packet::MESSAGE )
	{
		packet->NextString();
		if( packet->Remaining() )
		{
			uint32_t msg_type = packet->NextUInt();
			if( msg_type == TextConsole::MSG_CHAT )
				Snd.Play( Res.GetSound("i_chat.wav") );
		}
		return RaptorGame::ProcessPacket( packet );
	}
	else if( type == BattleTech::Packet::EVENTS )
	{
		uint16_t count = packet->NextUShort();
		for( uint16_t i = 0; i < count; i ++ )
			Events.push( Event( packet ) );
	}
	else if( type == BattleTech::Packet::ATTENTION )
	{
		Event e( packet );
		PlayEvent( &e );
	}
	else if( type == BattleTech::Packet::SPOT )
	{
		Mech *mech = GetMech( packet->NextUInt() );
		int8_t spotted = packet->NextChar();
		uint8_t tag_eq = packet->NextUChar();
		Mech *from = GetMech( packet->NextUInt() );
		if( mech )
		{
			mech->Spotted = spotted;
			mech->Tagged = tag_eq;
		}
		if( from )
		{
			tag_eq &= 0x7F;
			if( tag_eq && (tag_eq < from->Equipment.size()) )
				from->Equipment[ tag_eq ].Fired ++;
			else
				from->WeaponsToFire[ 0xFF ] = 1;  // Spotted for indirect fire without TAG.
		}
	}
	else if( type == BattleTech::Packet::TEAM_TURN )
	{
		TeamTurn = packet->NextUChar();
		ReadyAndAbleCache.clear();
		
		Mech *mech = GetMech( packet->NextUInt() );
		if( mech )
			mech->TookTurn = true;
		
		uint32_t prev_selected = SelectedID;
		
		bool hotseat = Hotseat();
		
		std::vector<int> ai_teams = Data.PropertyAsInts("ai_team");
		bool ai_turn = (std::find( ai_teams.begin(), ai_teams.end(), TeamTurn ) != ai_teams.end());
		
		if( hotseat && (! ai_turn) && MyTeam() )
		{
			std::string team_str = Num::ToString((int)TeamTurn);
			Data.Players[ PlayerID ]->Properties[ "team" ] = team_str;
			
			Packet player_properties = Packet( Raptor::Packet::PLAYER_PROPERTIES );
			player_properties.AddUShort( PlayerID );
			player_properties.AddUInt( 1 );
			player_properties.AddString( "team" );
			player_properties.AddString( team_str );
			Net.Send( &player_properties );
			
			SelectedID = 0;  // In hotseat mode, always select a Mech for the active team.
		}
		
		uint8_t my_team = MyTeam();
		if( (TeamTurn == my_team) && ! BotControlsTeam(TeamTurn) )
		{
			Event e;
			e.Sound = "i_ready.wav";
			if( ! hotseat )
				e.Text = "It is now your team's turn.";
			else
				e.Duration = 0.f;
			Events.push( e );
			
			const Mech *selected = SelectedMech();
			
			// If our turn begins with an enemy Mech selected, we'll try to select one of ours if available.
			if( selected && (selected->Team != my_team) )
			{
				SelectedID = 0;
				selected = NULL;
			}
			
			// If your team's turn begins without a ready-and-able Mech selected, select the first that you own.
			if( !( SelectedID && selected && selected->ReadyAndAble() ) )
			{
				for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
				{
					if( obj_iter->second->Type() == BattleTech::Object::MECH )
					{
						const Mech *mech = (const Mech*) obj_iter->second;
						if( (mech->Team == TeamTurn) && mech->ReadyAndAble() && ((mech->PlayerID == PlayerID) || (Data.Players.size() == 1)) )
						{
							SelectedID = mech->ID;
							selected = mech;
							break;
						}
					}
				}
			}
			
			// If no ready-and-able available, just look for ready.
			if( !( SelectedID && selected && selected->Ready() ) )
			{
				for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
				{
					if( obj_iter->second->Type() == BattleTech::Object::MECH )
					{
						const Mech *mech = (const Mech*) obj_iter->second;
						if( (mech->Team == TeamTurn) && mech->Ready() && ((mech->PlayerID == PlayerID) || (Data.Players.size() == 1)) )
						{
							SelectedID = mech->ID;
							selected = mech;
							break;
						}
					}
				}
			}
			
			// If we didn't change the selection, restore the previous.
			if( ! SelectedID )
				SelectedID = prev_selected;
			
			// Clear aim if the selected Mech was changed.
			if( SelectedID != prev_selected )
				TargetID = 0;
			HexBoard *hex_board = (HexBoard*) Layers.Find("HexBoard");
			if( hex_board )
				hex_board->UpdateAim();
		}
	}
	else
		return RaptorGame::ProcessPacket( packet );
	return true;
}


void BattleTechGame::ChangeState( int state )
{
	TeamTurn = 0;
	ReadyAndAbleCache.clear();
	
	if( (State < BattleTech::State::SETUP) && (state >= BattleTech::State::SETUP) )
	{
		// Just connected.
		Layers.RemoveAll();
		Layers.Add( new HexBoard() );
		if( state == BattleTech::State::SETUP )
		{
			if( Cfg.SettingAsBool("screensaver") )
			{
				if( Biomes.size() && Raptor::Server->IsRunning() )
				{
					Packet info = Packet( Raptor::Packet::INFO );
					info.AddUShort( 1 );
					info.AddString( "biome" );
					info.AddString( Biomes.at( Rand::Int( 0, Biomes.size() - 1 ) ).second );
					Net.Send( &info );
				}
				Layers.Add( new Screensaver() );
			}
			else if( Raptor::Server->IsRunning() )
			{
				if( Biomes.size() )
				{
					// When starting a server, set biome to the first defined.
					Data.Properties[ "biome" ] = Biomes.begin()->second;
					Packet info = Packet( Raptor::Packet::INFO );
					info.AddUShort( 1 );
					info.AddString( "biome" );
					info.AddString( Biomes.begin()->second );
					Net.Send( &info );
				}
				Layers.Add( new GameMenu() );
			}
			else
				Layers.Add( new SpawnMenu() );
		}
		Cfg.Settings[ "gunnery"  ] = "3";
		Cfg.Settings[ "piloting" ] = "4";
	}
	else if( (State == BattleTech::State::SETUP) && (state > BattleTech::State::SETUP) )
	{
		// Initiate combat.
		HexBoard *hex_board = (HexBoard*) Layers.Find("HexBoard");
		if( hex_board )
			hex_board->RemoveSetupMenus();
		else
		{
			Layers.RemoveAll();
			Layers.Add( new HexBoard() );
		}
	}
	else if( (state >= BattleTech::State::MOVEMENT) && (state <= BattleTech::State::PHYSICAL_ATTACK) )
	{
		HexBoard *hex_board = (HexBoard*) Layers.Find("HexBoard");
		if( hex_board )
			hex_board->UpdateAim();
	}
	else if( (State > BattleTech::State::SETUP) && (state == BattleTech::State::SETUP) )
	{
		// Back to Setup.
		Layers.RemoveAll();
		TargetID = 0;
		while( Events.size() )
			Events.pop();
		EventClock.Reset( 0. );
		Layers.Add( new HexBoard() );
		if( Cfg.SettingAsBool("screensaver") )
		{
			if( Biomes.size() && Raptor::Server->IsRunning() )
			{
				Packet info = Packet( Raptor::Packet::INFO );
				info.AddUShort( 1 );
				info.AddString( "biome" );
				info.AddString( Biomes.at( Rand::Int( 0, Biomes.size() - 1 ) ).second );
				Net.Send( &info );
			}
			Layers.Add( new Screensaver() );
		}
		else
			Layers.Add( new SpawnMenu() );
	}
	
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			Mech *mech = (Mech*) obj_iter->second;
			mech->BeginPhase( state );
		}
	}
	
	RaptorGame::ChangeState( state );
	
	if( (state == BattleTech::State::SETUP) && Cfg.SettingAsBool("screensaver") && Raptor::Server->IsRunning() && Variants.size() )
	{
		Mouse.ShowCursor = false;
		Data.Properties[ "ai_team" ] = Raptor::Server->Data.Properties[ "ai_team" ] = "1,2,3,4,5";
		Cfg.Settings[ "event_speed" ] = "2";
		int mechs_per_team = Rand::Int( 1, 31 );
		
		std::map< std::string, Variant > variants;
		for( std::map< std::string, Variant >::const_iterator var = Variants.begin(); var != Variants.end(); var ++ )
		{
			if( HasMechTex(var->second.Name) && ! var->second.Quad )
				variants[ var->first ] = var->second;
		}
		if( ! variants.size() )
			variants = Variants;
		
		for( int team = 2; team >= 1; team -- )
		{
			std::string team_str = Num::ToString( team );
			Data.Players[ PlayerID ]->Properties[ "team" ] = team_str;
			Packet player_properties = Packet( Raptor::Packet::PLAYER_PROPERTIES );
			player_properties.AddUShort( PlayerID );
			player_properties.AddUInt( 1 );
			player_properties.AddString( "team" );
			player_properties.AddString( team_str );
			Net.Send( &player_properties );
			
			for( int i = 0; i < mechs_per_team; i ++ )
			{
				Packet spawn_mech( BattleTech::Packet::SPAWN_MECH );
				spawn_mech.AddUChar( (team == 1) ? (30 - i)   : i ); // X
				spawn_mech.AddUChar( (team == 1) ? (16 - i%2) : 0 ); // Y
				spawn_mech.AddUChar( (team == 1) ? 0          : 3 ); // Facing
				std::map<std::string,Variant>::const_iterator mech = variants.begin();
				int index = Rand::Int( 0, variants.size() - 1 );
				for( int j = 0; j < index; j ++ )
					mech ++;
				mech->second.AddToPacket( &spawn_mech );
				spawn_mech.AddChar( 4 ); // Piloting
				spawn_mech.AddChar( 3 ); // Gunnery
				Net.Send( &spawn_mech );
			}
		}
		
		Cfg.Command( "ready" );
	}
}


void BattleTechGame::Disconnected( void )
{
	if( State >= Raptor::State::CONNECTED )
	{
		Layers.RemoveAll();
		Layers.Add( new JoinMenu() );
	}
	
	while( Events.size() )
		Events.pop();
	EventClock.Reset( 0. );
	
	RaptorGame::Disconnected();
}


GameObject *BattleTechGame::NewObject( uint32_t id, uint32_t type )
{
	if( type == BattleTech::Object::MAP )
		return new HexMap( id );
	else if( type == BattleTech::Object::MECH )
		return new Mech( id );
	
	return RaptorGame::NewObject( id, type );
}


HexMap *BattleTechGame::Map( void )
{
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MAP )
			return (HexMap*) obj_iter->second;
	}
	
	return NULL;
}


bool BattleTechGame::PlayingEvents( void ) const
{
	return Events.size() || (EventClock.RemainingSeconds() > -0.1);
}


bool BattleTechGame::Hotseat( void ) const
{
	return Data.PropertyAsBool("hotseat");
}


bool BattleTechGame::FF( void ) const
{
	if( Data.PropertyAsBool("ff") )
		return true;
	
	// Allow friendly fire after all enemies are eliminated.
	return TeamsAlive() < 2;
}


bool BattleTechGame::Admin( void )
{
	if( Raptor::Server->IsRunning() )
		return true;
	
	if( Data.PropertyAsString("permissions") == "all" )
		return true;
	
	Player *p = Data.GetPlayer( PlayerID );
	if( p && p->PropertyAsBool("admin") )
		return true;
	
	return false;
}


bool BattleTechGame::ReadyToBegin( void )
{
	return (TeamsAlive() >= 2) && MyMech();
}


Mech *BattleTechGame::MyMech( void )
{
	uint8_t my_team = MyTeam();
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			Mech *mech = (Mech*) obj_iter->second;
			if( (mech->Team == my_team) && ! mech->Destroyed() && ((mech->PlayerID == PlayerID) || (Data.Players.size() == 1)) )
				return mech;
		}
	}
	return NULL;
}


uint8_t BattleTechGame::TeamsAlive( void ) const
{
	std::set<uint8_t> teams_alive;
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			if( mech->Team && ! mech->Destroyed() )
				teams_alive.insert( mech->Team );
		}
	}
	return teams_alive.size();
}


uint8_t BattleTechGame::MyTeam( void )
{
	Player *p = Data.GetPlayer( PlayerID );
	return p ? p->PropertyAsInt("team") : 0;
}


std::string BattleTechGame::TeamName( uint8_t team_num ) const
{
	return Data.PropertyAsString( std::string("team") + Num::ToString((int)team_num), (std::string("Team ") + Num::ToString((int)team_num)).c_str() );
}


bool BattleTechGame::BotControlsTeam( uint8_t team_num ) const
{
	if( ! team_num )
		return false;
	
	std::vector<int> ai_teams = Data.PropertyAsInts("ai_team");
	if( std::find( ai_teams.begin(), ai_teams.end(), team_num ) != ai_teams.end() )
		return true;
	
	if( Data.PropertyAsString("ai_team") == "auto" )
	{
		for( std::map<uint16_t,Player*>::const_iterator player = Data.Players.begin(); player != Data.Players.end(); player ++ )
		{
			uint8_t player_team = player->second->PropertyAsInt("team");
			if( player_team == team_num )
				return false;
		}
		return true;
	}
	
	return false;
}


Mech *BattleTechGame::GetMech( uint32_t mech_id )
{
	GameObject *obj = Data.GetObject( mech_id );
	if( obj && (obj->Type() == BattleTech::Object::MECH) )
		return (Mech*) obj;
	return NULL;
}


Mech *BattleTechGame::SelectedMech( void )
{
	return GetMech( SelectedID );
}


Mech *BattleTechGame::TargetMech( void )
{
	return GetMech( TargetID );
}


void BattleTechGame::GetPixel( const Pos3D *pos, int *x, int *y )
{
	*x = (int)((pos->X - X) * Zoom) + Gfx.W/2;
	*y = (int)((pos->Y - Y) * Zoom) + Gfx.H/2;
}


std::string BattleTechGame::PhaseName( int state ) const
{
	if( state < 0 )
		state = State;
	
	if( state == BattleTech::State::SETUP )
		return "Setup";
	if( state == BattleTech::State::INITIATIVE )
		return "Initiative";
	if( state == BattleTech::State::MOVEMENT )
		return "Movement";
	if( state == BattleTech::State::TAG )
		return "TAG";
	if( state == BattleTech::State::WEAPON_ATTACK )
		return "Weapon Attack";
	if( state == BattleTech::State::PHYSICAL_ATTACK )
		return "Physical Attack";
	if( state == BattleTech::State::HEAT )
		return "Heat";
	if( state == BattleTech::State::END )
		return "End";
	if( state == BattleTech::State::GAME_OVER )
		return "Game Over";
	if( state == Raptor::State::DISCONNECTED )
		return "Disconnected";
	if( state == Raptor::State::CONNECTING )
		return "Connecting";
	if( state == Raptor::State::CONNECTED )
		return "Connected";
	return "";
}
