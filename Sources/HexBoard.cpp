/*
 *  HexBoard.cpp
 */

#include "HexBoard.h"

#include "BattleTechDefs.h"
#include "BattleTechGame.h"
#include "SpawnMenu.h"
#include "WeaponMenu.h"
#include "GameMenu.h"
#include "Num.h"
#include <cmath>


HexBoard::HexBoard( void )
{
	Name = "HexBoard";
	
	TargetDist = 0;
	TurnFont = Raptor::Game->Res.GetFont( "ProFont.ttf", 22 );
	
	UpdateRects();
}


HexBoard::~HexBoard()
{
}


void HexBoard::UpdateRects( void )
{
	Rect.x = 0;
	Rect.y = 0;
	Rect.w = Raptor::Game->Gfx.W;
	Rect.h = Raptor::Game->Gfx.H;
	
	UpdateCalcRects();
}


void HexBoard::Draw( void )
{
	UpdateRects();
	glBindTexture( GL_TEXTURE_2D, 0 );
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	double half_w = game->Gfx.W * 0.5 / game->Zoom;
	double half_h = game->Gfx.H * 0.5 / game->Zoom;
	game->Gfx.Setup2D( game->X - half_w, game->Y - half_h, game->X + half_w, game->Y + half_h );
	
	HexMap *map = game->Map();
	if( map )
		map->Draw();
	else
		return;
	
	Mech *selected = game->SelectedMech();
	Mech *target = game->TargetMech();
	
	bool playing_events = (game->EventClock.RemainingSeconds() > -0.1) || game->Events.size();
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			if( game->TeamTurn && (game->TeamTurn == game->MyTeam()) && ! playing_events
			&& (mech->Team == game->MyTeam()) && mech->ReadyAndAble() && (mech != selected) )
			{
				uint8_t x, y;
				mech->GetPosition( &x, &y );
				map->DrawHexOutline( x, y, selected?1.f:2.f, 0.f,0.2f,1.f );
			}
		}
	}
	
	if( selected && ! playing_events )
	{
		uint8_t x, y;
		selected->GetPosition( &x, &y );
		
		float a = ((game->TeamTurn == game->MyTeam()) || ! game->TeamTurn) ? 0.9f : 0.6f;
		
		if( selected->Destroyed() )
			map->DrawHexOutline( x, y, 3.f, 0.5f,0.5f,0.5f,a );
		else if( selected->Team == game->MyTeam() )
		{
			if( selected->Ready() )
				map->DrawHexOutline( x, y, 3.f, 0.0f,1.0f,0.0f,a );
			else
				map->DrawHexOutline( x, y, 3.f, 1.0f,1.0f,1.0f,a );
		}
		else if( selected->Team )
			map->DrawHexOutline( x, y, 3.f, 1.0f,0.0f,0.0f,a );
		else
			map->DrawHexOutline( x, y, 3.f, 1.0f,1.0f,0.0f,a );
		
		if( (selected->X != x) || (selected->Y != y) )
			map->DrawHexOutline( selected->X, selected->Y, 2.f, 0.0f,0.9f,0.0f,a );
		
		if( Path.size() )
		{
			float r = 0.0f, g = 0.0f, b = 0.7f, a = 0.4f;
			if( selected->ReadyAndAble() && (selected->Team == game->TeamTurn) && (selected->Team == game->MyTeam())
			&& ((game->State == BattleTech::State::TAG)
			 || (game->State == BattleTech::State::WEAPON_ATTACK)
			 || (game->State == BattleTech::State::PHYSICAL_ATTACK) ) )
				a = 0.9f;
			if( target )
			{
				g = 0.3f;
				b = 1.0f;
			}
			if( ! Path.LineOfSight )
			{
				r = 1.0f;
				b = 0.0f;
				if( target && ((target->Spotted < 99) || ( target->Narced() && ! (Path.TeamECMs.size() > Path.TeamECMs.count(selected->Team)) )) )
					g = 0.9f;
			}
			for( ShotPath::const_iterator hex = Path.begin(); hex != Path.end(); hex ++ )
			{
				if( hex == Path.begin() )
					continue;
				map->DrawHexOutline( (*hex)->X, (*hex)->Y, 2.f, r,g,b,a );
			}
			if( Path.size() >= 2 )
			{
				Pos3D here = (*(Path.begin()))->Center();
				Pos3D dest = (*(Path.rbegin()))->Center();
				game->Gfx.DrawLine2D( here.X, here.Y, dest.X, dest.Y, 2.f, r,g,b,a*0.9f );
			}
		}
	}
	
	
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second != map )
			obj_iter->second->Draw();
	}
	
	for( std::list<Effect>::iterator effect = game->Data.Effects.begin(); effect != game->Data.Effects.end(); effect ++ )
		effect->Draw();
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			
			uint8_t x, y;
			mech->GetPosition( &x, &y );
			Pos3D pos = map->Center( x, y );
			
			if( (mech->TookTurn || (game->State > BattleTech::State::MOVEMENT)) && ! playing_events && (game->State < BattleTech::State::END) )
			{
				std::string att_str = std::string("+") + Num::ToString(mech->Attack);
				std::string def_str = std::string(/*mech->Tagged?" +":*/"+") + Num::ToString(mech->Defense) + std::string(/*mech->Tagged?"*":*/" ");
				Pos3D att = pos + Vec2D( 0.5, 0.1 );
				Pos3D def = pos + Vec2D( 0.3, 0.4 );
				TurnFont->DrawText3D( att_str, &att, Font::ALIGN_MIDDLE_CENTER, 1.0f,0.4f,0.1f,1.f, 0.016 );
				TurnFont->DrawText3D( def_str, &def, Font::ALIGN_MIDDLE_CENTER, 0.1f,0.6f,1.0f,1.f, 0.016 );
				if( mech->Spotted < 99 )
				{
					std::string spot_str = std::string((mech->Spotted >= 0)?"+":"") + Num::ToString(mech->Spotted);
					Pos3D spot = pos + Vec2D( -0.3, 0.4 );
					TurnFont->DrawText3D( spot_str, &spot, Font::ALIGN_MIDDLE_CENTER, 1.0f,0.8f,0.1f,1.f, 0.016 );
				}
			}
			
			if( (mech->HitClock.ElapsedSeconds() <= 3.5) && (mech->Lifetime.ElapsedSeconds() > 3.5) )
			{
				double zoomed_h = game->Gfx.H / (game->Zoom * 2.);
				double scale = std::max<double>( 0.1 * zoomed_h, std::min<double>( 1., 0.2 * zoomed_h ) );
				bool above = (pos.Y + 0.1 > game->Y);
				if( (fabs(pos.Y - game->Y) + scale * 2. + 0.6) > zoomed_h )
					above = (pos.Y < game->Y);
				pos.Y += above ? (scale + 0.5) : (-1. * scale - 0.5);
				mech->DrawHealth( pos.X - scale, pos.Y - scale, pos.X + scale, pos.Y + scale );
			}
		}
	}
	
	
	game->Gfx.Setup2D();
	
	if( ! playing_events )
	{
		double mx = (game->Mouse.X - game->Gfx.W/2) / game->Zoom + game->X;
		double my = (game->Mouse.Y - game->Gfx.H/2) / game->Zoom + game->Y;
		Hex *hex = map->Nearest( mx, my );
		const Mech *mech = hex ? map->MechAt_const( hex->X, hex->Y ) : NULL;
		
		if( mech )
		{
			// FIXME: Draw a box with detailed Mech status.
			
			if( mech->HitClock.ElapsedSeconds() > 3.5 )
				mech->DrawHealth( game->Mouse.X - 64, game->Mouse.Y + 35, game->Mouse.X + 64, game->Mouse.Y + 163 );
			
			std::string name = mech->ShortFullName();
			if( game->State == BattleTech::State::SETUP )
				name += std::string(" [") + Num::ToString((int)mech->Tons) + std::string("T]");
			if( mech->Destroyed() )
				name += " [DESTROYED]";
			else
			{
				if( mech->Shutdown )
					name += " [SHUTDOWN]";
				if( mech->Unconscious )
					name += " [UNCONSCIOUS]";
				if( mech->Prone )
					name += " [PRONE]";
				if( mech->Narced() )
					name += " [NARC]";
			}
			if( game->Cfg.SettingAsBool("debug") )
			{
				for( std::vector<MechEquipment>::const_iterator eq = selected->Equipment.begin(); eq != selected->Equipment.end(); eq ++ )
				{
					std::string eq_line = eq->Name;
					if( eq->Ammo )
						eq_line += std::string(" ") + Num::ToString(eq->Ammo);
					if( eq->WeaponFCS )
						eq_line += " + Artemis IV FCS";
					if( eq->WeaponTC )
						eq_line += " + Targeting Computer";
					if( eq->WeaponArcs.count(BattleTech::Arc::REAR) && (eq->WeaponArcs.size() == 1) )
						eq_line += " (Rear)";
					if( eq->Damaged )
						eq_line += " [DAMAGED]";
					name += std::string("\n") + eq_line;
				}
			}
			TurnFont->DrawText( name, game->Mouse.X + 1, game->Mouse.Y + 15, Font::ALIGN_TOP_CENTER, 0,0,0,0.8f );
			TurnFont->DrawText( name, game->Mouse.X    , game->Mouse.Y + 14, Font::ALIGN_TOP_CENTER );
		}
		if( hex && (hex->Height || hex->Forest) )
		{
			std::string terrain;
			if( hex->Height )
				terrain += std::string("L") + Num::ToString((int)hex->Height);
			if( hex->Forest )
				terrain += std::string("F") + Num::ToString((int)hex->Forest);
			TurnFont->DrawText( terrain, game->Mouse.X + 1, game->Mouse.Y - 1, Font::ALIGN_BOTTOM_CENTER, 0,0,0,0.8f );
			TurnFont->DrawText( terrain, game->Mouse.X    , game->Mouse.Y - 2, Font::ALIGN_BOTTOM_CENTER );
		}
		if( hex && selected )
		{
			uint8_t x, y;
			selected->GetPosition( &x, &y );
			if( (x != hex->X) || (y != hex->Y) )
			{
				std::string dist = std::string("Dist ") + Num::ToString((int)map->HexDist(x,y,hex->X,hex->Y));
				TurnFont->DrawText( dist, game->Mouse.X + 16, game->Mouse.Y - 5, Font::ALIGN_TOP_LEFT, 0,0,0,0.8f );
				TurnFont->DrawText( dist, game->Mouse.X + 15, game->Mouse.Y - 6, Font::ALIGN_TOP_LEFT );
			}
		}
	}
	
	std::string status;
	if( (game->State != BattleTech::State::SETUP) && playing_events )
		// FIXME: Use event text, not just whatever is in the console.
		status = (*(game->Console.Messages.rbegin()))->Text;
	else if( (game->State == BattleTech::State::MOVEMENT) && selected && selected->Ready() && (selected->Team == game->TeamTurn) && (game->TeamTurn == game->MyTeam()) )
	{
		bool moved = selected->Steps.size() || selected->StandAttempts;
		status = selected->ShortName();
		if( moved )
			status += std::string(" moved ") + Num::ToString(selected->StepCost()) + std::string(" of ");
		else
			status += " can move ";
		status += Num::ToString((int)selected->WalkDist());
		if( selected->RunDist() || selected->JumpDist() )
		{
			status += std::string("/") + Num::ToString((int)selected->RunDist());
			if( selected->MASCDist() )
				status += std::string("[") + Num::ToString((int)selected->MASCDist()) + std::string("]");
			status += std::string("/") + Num::ToString((int)selected->JumpDist());
		}
		uint8_t move_mode = selected->SpeedNeeded(true);
		if( ! moved )
			status += ".";
		else if( move_mode == BattleTech::Move::STOP )
			status += " to stand still.";
		else if( move_mode == BattleTech::Move::WALK )
			status += " which can be walked.";
		else if( move_mode == BattleTech::Move::RUN )
			status += " which requires running.";
		else if( move_mode == BattleTech::Move::MASC )
			status += " which requires MASC.";
		else if( move_mode == BattleTech::Move::JUMP )
			status += " which requires jumping.";
		else if( move_mode == BattleTech::Move::INVALID )
			status += ", an invalid move.";
		
		if( selected->Prone )
			status += selected->CanStand() ? "  Up Arrow to stand." : "  It cannot stand.";
		
		if( selected->Shutdown || selected->Unconscious )
			status = selected->ShortName() + std::string(" cannot move this turn.");
	}
	else if( (game->State == BattleTech::State::SETUP) || ! playing_events )
	{
		status = game->PhaseName();
		if( game->TeamTurn && game->MyTeam() )
		{
			if( ! game->Hotseat() )
				status += (game->TeamTurn == game->MyTeam()) ? " (your team)" : " (enemy team)";
			else
			{
				std::map<std::string,std::string>::const_iterator team_iter = game->Data.Properties.find( std::string("team") + Num::ToString((int)(game->TeamTurn)) );
				if( team_iter != game->Data.Properties.end() )
					status += std::string(" (") + team_iter->second + std::string(")");
			}
		}
		else if( game->State == BattleTech::State::SETUP )
		{
			const Layer *top = game->Layers.TopLayer();
			
			if( selected && selected->Steps.size() )
				status = "Press Enter to submit new spawn position.";
			else if( top == this )
			{
				if( selected && (selected->PlayerID == game->PlayerID) )
				{
					if( game->Hotseat() )
						status = "Press Tab to spawn for the other team.  When done, press Enter.";
					else
						status = "When you are satisfied with your spawn position, press Enter.";
				}
				else if( game->MyTeam() )
					status = "Right-click to drop.  Press Tab to change team/Mech.";
				else
					status = "Press Tab to select a team and BattleMech.";
			}
			else if( top->Name == "SpawnMenu" )
			{
				if( game->MyTeam() )
					status = "Select a BattleMech, then right-click to drop.";
				else
					status = "Select a team and BattleMech, then right-click to drop.";
			}
			else if( top->Name == "GameMenu" )
			{
				if( game->MyTeam() )
					status = "When everyone is ready, click Initiate Combat.";
				else
					status = "Set game options, then click Team/Mech to begin.";
			}
		}
	}
	TurnFont->DrawText( status, Rect.w/2 + 2, Rect.h,     Font::ALIGN_BOTTOM_CENTER, 0,0,0,0.8f );
	TurnFont->DrawText( status, Rect.w/2,     Rect.h - 2, Font::ALIGN_BOTTOM_CENTER );
}


bool HexBoard::MouseDown( Uint8 button )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexMap *map = game->Map();
	if( ! map )
		return false;
	
	double mx = (game->Mouse.X - game->Gfx.W/2) / game->Zoom + game->X;
	double my = (game->Mouse.Y - game->Gfx.H/2) / game->Zoom + game->Y;
	Hex *hex = map->Nearest( mx, my );
	
	Mech *selected = game->SelectedMech();
	Mech *target = game->TargetMech();
	
	bool playing_events = (game->EventClock.RemainingSeconds() > -0.1) || game->Events.size();
	
	bool shift = game->Keys.KeyDown(SDLK_LSHIFT) || game->Keys.KeyDown(SDLK_RSHIFT);
	
	if( (button == SDL_BUTTON_LEFT) && ! shift )
	{
		Mech *clicked = hex ? map->MechAt( hex->X, hex->Y ) : NULL;
		if( clicked != selected )
		{
			if( selected )
				selected->Steps.clear();
			selected = clicked;
			game->SelectedID = selected ? selected->ID : 0;
			target = NULL;
			game->TargetID = 0;
			ClearPath();
			
			if( selected )
				game->Snd.Play( game->Res.GetSound("i_select.wav") );
		}
	}
	else if( (button == SDL_BUTTON_WHEELUP) && IsTop() && ! game->Console.IsActive() )
	{
		game->Zoom *= 1.25;
		double new_mx = (game->Mouse.X - game->Gfx.W/2) / game->Zoom + game->X;
		double new_my = (game->Mouse.Y - game->Gfx.H/2) / game->Zoom + game->Y;
		game->X += (mx - new_mx);
		game->Y += (my - new_my);
	}
	else if( (button == SDL_BUTTON_WHEELDOWN) && IsTop() && ! game->Console.IsActive() )
	{
		game->Zoom /= 1.25;
		double new_mx = (game->Mouse.X - game->Gfx.W/2) / game->Zoom + game->X;
		double new_my = (game->Mouse.Y - game->Gfx.H/2) / game->Zoom + game->Y;
		game->X += (mx - new_mx);
		game->Y += (my - new_my);
	}
	else if( (button == SDL_BUTTON_X1) || ((button == SDL_BUTTON_LEFT) && shift) )
	{
		if( hex )
		{
			Packet attention( BattleTech::Packet::ATTENTION );
			attention.AddUChar( hex->X );
			attention.AddUChar( hex->Y );
			game->Net.Send( &attention );
		}
	}
	else if( (game->State != BattleTech::State::SETUP) && playing_events )
		return true;
	else if( button == SDL_BUTTON_RIGHT )
	{
		if( game->State == BattleTech::State::SETUP )
		{
			if( ! hex )
				return true;
			if( map->MechAt_const( hex->X, hex->Y ) )
				return true;
			if( ! game->MyTeam() )
				return true;
			if( ! game->Variants.size() )
			{
				game->Console.Print( "No Mech variants loaded!", TextConsole::MSG_ERROR );
				return true;
			}
			
			Packet spawn_mech( BattleTech::Packet::SPAWN_MECH );
			spawn_mech.AddUChar( hex->X );
			spawn_mech.AddUChar( hex->Y );
			
			Pos3D a = map->Center( hex->X, hex->Y );
			Pos3D b = map->Center( map->Hexes.size() - 1, map->Hexes[ map->Hexes.size() - 1 ].size() - 1 );
			b.X /= 2.;
			b.Y /= 2.;
			Vec3D v = b - a;
			uint8_t best_dir = 0;
			double best_dot = -2;
			std::map<uint8_t,const Hex*> adjacent = map->Adjacent_const( hex->X, hex->Y );
			for( std::map<uint8_t,const Hex*>::const_iterator adj = adjacent.begin(); adj != adjacent.end(); adj ++ )
			{
				if( ! adj->second )
					continue;
				
				double dot = map->DirVec[ adj->first ].Dot( &v );
				if( dot > best_dot )
				{
					best_dot = dot;
					best_dir = adj->first;
				}
			}
			spawn_mech.AddUChar( best_dir );
			
			std::string mech_str = game->Cfg.SettingAsString( "mech" );
			std::map<std::string,Variant>::const_iterator mech = game->Variants.find( mech_str );
			if( mech == game->Variants.end() )
			{
				// Give them a random Mech if something went wrong.
				game->Console.Print( std::string("Couldn't find variant: ") + mech_str, TextConsole::MSG_ERROR );
				mech = game->Variants.begin();
				int index = Rand::Int( 0, game->Variants.size() - 1 );
				for( int i = 0; i < index; i ++ )
					mech ++;
			}
			game->Console.Print( std::string("Spawning: ") + mech->first );
			mech->second.AddToPacket( &spawn_mech );
			uint8_t piloting = game->Cfg.SettingAsInt( "piloting", mech->second.Clan ? 4 : 5 );
			uint8_t gunnery  = game->Cfg.SettingAsInt( "gunnery",  mech->second.Clan ? 3 : 4 );
			spawn_mech.AddUChar( piloting );
			spawn_mech.AddUChar( gunnery );
			game->Net.Send( &spawn_mech );
			
			// Close the SpawnMenu after we drop a 'Mech so we can use arrow keys to move it.
			//size_t mech_limit = atoi(game->Data.Properties["mech_limit"].c_str());
			if( ! IsTop() /* && ! game->Hotseat() && (mech_limit == 1) */ )
				game->Layers.RemoveTop();
		}
		else if( selected && selected->ReadyAndAble() )
		{
			uint8_t x = 0, y = 0, facing = BattleTech::Dir::UP;
			selected->GetPosition( &x, &y, &facing );
			ClearPath( false );
			Path = map->Path( x, y, hex->X, hex->Y );
			TargetDist = map->HexDist( (*(Path.begin()))->X, (*(Path.begin()))->Y, (*(Path.rbegin()))->X, (*(Path.rbegin()))->Y );
			target = map->MechAt( hex->X, hex->Y );
			if( ! target )
				;
			else if( (target == selected) || target->Destroyed() )
				target = NULL;
			else if( selected->Team && target->Team && (selected->Team == target->Team) && ! game->FF() )
				target = NULL;
			game->TargetID = target ? target->ID : 0;
			
			if( target
			&& ((game->State == BattleTech::State::WEAPON_ATTACK)
			 || (game->State == BattleTech::State::PHYSICAL_ATTACK)
			 || (game->State == BattleTech::State::TAG)) )
			{
				UpdateWeaponsInRange( selected, target );
				game->Snd.Play( game->Res.GetSound("i_select.wav") );
			}
			
			if( ! game->TargetID )
				RemoveAllElements();
			
			if( game->Cfg.SettingAsBool("debug") )
			{
				bool ecm = Path.TeamECMs.size() > Path.TeamECMs.count(selected->Team);
				std::string msg = "Dist from ";
				msg += Num::ToString((*(Path.begin()))->X) + std::string(" ") + Num::ToString((*(Path.begin()))->Y);
				msg += std::string(" to ");
				msg += Num::ToString((*(Path.rbegin()))->X) + std::string(" ") + Num::ToString((*(Path.rbegin()))->Y);
				if( target )
					msg += std::string(" (") + target->ShortName() + std::string(")");
				msg += std::string(" is ") + Num::ToString(TargetDist) + std::string(" hexes");
				if( ! Path.LineOfSight )
				{
					msg += std::string(", NO line of sight.");
					if( target && (target->Spotted < 99) )
						msg += std::string("  Target spotted for indirect fire (+") + Num::ToString((int)( target->Spotted )) + std::string(").");
					else if( target && target->Narced() && ! ecm )
						msg += std::string("  Narc beacon allows indirect fire (+1).");
				}
				else if( Path.Modifier )
					msg += std::string(", modifier +") + Num::ToString(Path.Modifier) + std::string(".");
				else
					msg += std::string(".");
				if( ecm )
					msg += "  Passes through hostile ECM.";
				game->Console.Print( msg );
			}
		}
	}
	else
		return false;
	return true;
}


bool HexBoard::KeyDown( SDLKey key )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexMap *map = game->Map();
	if( ! map )
		return false;
	
	Mech *selected = game->SelectedMech();
	Mech *target = game->TargetMech();
	
	bool enemy_turn = game->TeamTurn && game->MyTeam() && (game->TeamTurn != game->MyTeam());
	bool playing_events = (game->EventClock.RemainingSeconds() > -0.1) || game->Events.size();
	
	bool shift = game->Keys.KeyDown(SDLK_LSHIFT) || game->Keys.KeyDown(SDLK_RSHIFT);
	
	if( (key == SDLK_q) || (key == SDLK_HOME) )
		map->ClientInit();
	else if( (key == SDLK_e) || (key == SDLK_END) )
	{
		Vec3D tl(  9999.,  9999. );
		Vec3D br( -9999., -9999. );
		for( std::map<uint32_t,GameObject*>::iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
		{
			if( obj_iter->second->Type() == BattleTech::Object::MECH )
			{
				Mech *mech = (Mech*) obj_iter->second;
				if( (game->State == BattleTech::State::GAME_OVER) || ! mech->Destroyed() )
				{
					uint8_t x, y;
					mech->GetPosition( &x, &y );
					Pos3D pos = map->Center( x, y );
					if( pos.X - 1.7 < tl.X )
						tl.X = pos.X - 1.7;
					if( pos.Y - 1.7 < tl.Y )
						tl.Y = pos.Y - 1.7;
					if( pos.X + 1.7 > br.X )
						br.X = pos.X + 1.7;
					if( pos.Y + 1.7 > br.Y )
						br.Y = pos.Y + 1.7;
				}
			}
		}
		if( (tl.X > br.X) || (tl.Y > br.Y) )
			map->ClientInit();
		else
		{
			Vec3D center = (tl + br) / 2.;
			game->X = center.X;
			game->Y = center.Y;
			double zoom_w = game->Gfx.W / std::max<double>( 4., br.X - tl.X );
			double zoom_h = game->Gfx.H / std::max<double>( 4., br.Y - tl.Y );
			game->Zoom = (zoom_w < zoom_h) ? zoom_w : zoom_h;
		}
	}
	else if( (key == SDLK_r) || (key == SDLK_EQUALS) || (key == SDLK_PAGEUP) )
		game->Zoom *= 1.25;
	else if( (key == SDLK_f) || (key == SDLK_MINUS) || (key == SDLK_PAGEDOWN) )
		game->Zoom /= 1.25;
	else if( key == SDLK_w )
		game->Y -= 150. / game->Zoom;
	else if( key == SDLK_s )
		game->Y += 150. / game->Zoom;
	else if( key == SDLK_a )
		game->X -= 150. / game->Zoom;
	else if( key == SDLK_d )
		game->X += 150. / game->Zoom;
	
	else if( (key >= SDLK_F1) && (key <= SDLK_F15) && (game->State == BattleTech::State::SETUP) )
	{
		int teams = 2;
		std::map<std::string,std::string>::const_iterator teams_iter = game->Data.Properties.find("teams");
		if( teams_iter != game->Data.Properties.end() )
			teams = atoi( teams_iter->second.c_str() );
		
		int team = key + 1 - SDLK_F1;
		if( team > teams )
			return false;
		
		std::string team_str = Num::ToString(team);
		game->Data.Players[ game->PlayerID ]->Properties[ "team" ] = team_str;
		
		game->Snd.Play( game->Res.GetSound("i_select.wav") );
		
		Packet player_properties = Packet( Raptor::Packet::PLAYER_PROPERTIES );
		player_properties.AddUShort( game->PlayerID );
		player_properties.AddUInt( 1 );
		player_properties.AddString( "team" );
		player_properties.AddString( team_str );
		game->Net.Send( &player_properties );
	}
	else if( (key == SDLK_TAB) && IsTop() )
	{
		RemoveAllElements();
		game->Layers.Add( new SpawnMenu() );
	}
	else if( ((key == SDLK_RETURN) || (key == SDLK_KP_ENTER))
	&& (game->State == BattleTech::State::SETUP) && IsTop()
	&& (! selected || ! selected->Steps.size()) )
	{
		RemoveAllElements();
		game->Layers.Add( new GameMenu() );
	}
	else if( (key == SDLK_ESCAPE) && IsTop() && (playing_events || ! selected) )
	{
		RemoveAllElements();
		game->Layers.Add( new GameMenu() );
	}
	else if( (key == SDLK_ESCAPE) && selected )
	{
		selected->Steps.clear();
		selected = NULL;
		game->SelectedID = 0;
		game->TargetID = 0;
		ClearPath();
	}
	
	// Don't allow doing things when we don't have our team's unit selected.
	else if( ! selected )
		return false;
	else if( (game->State != BattleTech::State::SETUP) && (game->State != BattleTech::State::GAME_OVER) && ! selected->Ready() )
		return false;
	else if( selected->Team && game->MyTeam() && (selected->Team != game->MyTeam()) && (game->State != BattleTech::State::GAME_OVER) )
		return false;
	
	else if( (game->State != BattleTech::State::MOVEMENT)
	&&       (game->State != BattleTech::State::TAG)
	&&       (game->State != BattleTech::State::WEAPON_ATTACK)
	&&       (game->State != BattleTech::State::PHYSICAL_ATTACK)
	&&       (game->State != BattleTech::State::GAME_OVER)
	&&       (game->State != BattleTech::State::SETUP) )
		return false;
	else if( ((key == SDLK_RETURN) || (key == SDLK_KP_ENTER)) && ! enemy_turn && ! playing_events )
	{
		uint8_t speed = (game->State == BattleTech::State::SETUP) ? (uint8_t) BattleTech::Move::JUMP : selected->SpeedNeeded(true);
		if( (game->State == BattleTech::State::MOVEMENT) && selected->Steps.size() && (speed == BattleTech::Move::INVALID) )
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
		else if( (game->State == BattleTech::State::MOVEMENT)
		||      ((game->State == BattleTech::State::SETUP) && selected->Steps.size()) )
		{
			Packet movement( BattleTech::Packet::MOVEMENT );
			movement.AddUInt( selected->ID );
			movement.AddUChar( speed );
			movement.AddUChar( selected->Steps.size() );
			for( std::vector<MechStep>::const_iterator step = selected->Steps.begin(); step != selected->Steps.end(); step ++ )
				movement.AddUChar( step->Move );
			game->Net.Send( &movement );
			
			selected->Steps.clear();
			game->TargetID = 0;
			ClearPath();
			game->EventClock.Reset( 0. );
		}
		else if( (game->State == BattleTech::State::WEAPON_ATTACK)
		||       (game->State == BattleTech::State::TAG) )
		{
			game->Snd.Play( game->Res.GetSound("i_target.wav") );
			
			std::map<uint8_t,int8_t> weapons;
			if( target )
			{
				for( std::map<uint8_t,int8_t>::const_iterator weap = WeaponsInRange.begin(); weap != WeaponsInRange.end(); weap ++ )
				{
					if( selected->WeaponsToFire[ weap->first ] )
						weapons[ weap->first ] = weap->second;
				}
			}
			
			uint8_t firing_arc = target ? selected->FiringArc( target->X, target->Y ) : (uint8_t) BattleTech::Arc::STRUCTURE;
			
			selected->Animate( 0.2, BattleTech::Effect::TORSO_TWIST );
			
			selected->TurnedArm = BattleTech::Loc::UNKNOWN;
			selected->ArmsFlipped = false;
			if( (! selected->TorsoTwist) && (firing_arc == BattleTech::Arc::REAR) )
			{
				for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
				{
					const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
					if( eq->Location && eq->Location->IsArm() )
					{
						selected->ArmsFlipped = true;
						break;
					}
				}
			}
			else if( weapons.size() && (firing_arc == BattleTech::Arc::LEFT_SIDE) )
				selected->TurnedArm = BattleTech::Loc::LEFT_ARM;
			else if( weapons.size() && (firing_arc == BattleTech::Arc::RIGHT_SIDE) )
				selected->TurnedArm = BattleTech::Loc::RIGHT_ARM;
			
			selected->ProneFire = BattleTech::Loc::UNKNOWN;
			if( selected->Prone && weapons.size() )
			{
				selected->ProneFire = BattleTech::Loc::CENTER_TORSO;
				for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
				{
					const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
					if( eq->Location && eq->Location->IsArm() )
					{
						selected->ProneFire = eq->Location->Loc;
						break;
					}
				}
			}
			
			Packet shots( BattleTech::Packet::SHOTS );
			shots.AddUInt( selected->ID );
			shots.AddChar( selected->TorsoTwist );
			shots.AddChar( selected->TurnedArm );
			shots.AddChar( selected->ProneFire );
			
			uint8_t count_and_flags = weapons.size();
			if( selected->ArmsFlipped )
				count_and_flags |= 0x80;
			shots.AddUChar( count_and_flags );
			
			for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
			{
				shots.AddUInt( target->ID );
				
				MechEquipment *eq = &(selected->Equipment[ weap->first ]);
				uint8_t arc_and_flags = target->DamageArc( selected->X, selected->Y );
				if( ! Path.LineOfSight )
					arc_and_flags |= 0x80; // Indirect Fire
				else if( Path.PartialCover )
					arc_and_flags |= 0x40; // Leg Partial Cover
				if( Path.TeamECMs.size() > Path.TeamECMs.count(selected->Team) )
					arc_and_flags |= 0x20; // Within Friendly ECM
				if( target->Narced() && (Path.LineOfSight || (target->Spotted < 99)) )
					arc_and_flags |= 0x10; // NARC Cluster Bonus (with exceptions in MechEquipment::ClusterHits)
				if( eq->WeaponFCS && ! eq->WeaponFCS->Damaged )
					arc_and_flags |= 0x08; // Artemis IV FCS
				
				shots.AddUChar( arc_and_flags );
				shots.AddUChar( weap->first );
				shots.AddUChar( selected->WeaponsToFire[ weap->first ] );
				shots.AddChar( weap->second );
				
				if( !(eq->Weapon && eq->Weapon->TAG) ) // TAG will only count as "fired" if it hits (reported in SPOT packet).
					eq->Fired = weap->second;
			}
			
			if( target && Path.LineOfSight && selected->WeaponsToFire[ 0xFF ] && (game->State == BattleTech::State::WEAPON_ATTACK) && ! selected->FiredTAG() )
			{
				// Spot for Indirect Fire
				
				int8_t spotted = selected->Attack + Path.Modifier + (weapons.size() ? 2 : 1);
				if( selected->Sensors )
					spotted += selected->Sensors->Damaged * 2;
				if( target->Prone )
					spotted += (TargetDist <= 1) ? -2 : 1;
				
				bool ecm = Path.TeamECMs.size() > Path.TeamECMs.count(selected->Team);
				int8_t trees = Path.PartialCover ? (Path.Modifier - 1) : Path.Modifier;
				if( trees && (! ecm) && (TargetDist <= selected->ActiveProbeRange()) )
					spotted --;
				
				shots.AddUInt( target->ID );
				shots.AddChar( spotted );
			}
			else
			{
				shots.AddUInt( 0 );
				shots.AddChar( 99 );
			}
			
			game->Net.Send( &shots );
			
			game->TargetID = 0;
			ClearPath();
			game->EventClock.Reset( 0. );
		}
		else if( game->State == BattleTech::State::PHYSICAL_ATTACK )
		{
			game->Snd.Play( game->Res.GetSound("i_target.wav") );
			
			Packet melee( BattleTech::Packet::MELEE );
			melee.AddUInt( selected->ID );
			if( target )
			{
				melee.AddUChar( selected->SelectedMelee.size() );
				for( std::set<MechMelee>::const_iterator chosen = selected->SelectedMelee.begin(); chosen != selected->SelectedMelee.end(); chosen ++ )
				{
					melee.AddUInt( target->ID );
					melee.AddUChar( target->DamageArc( selected->X, selected->Y ) | (chosen->HitTable << 2) );
					melee.AddUChar( chosen->Attack );
					melee.AddChar( chosen->Difficulty );
					melee.AddUChar( chosen->Damage );
					melee.AddUChar( chosen->Limb );
				}
			}
			else
				melee.AddUChar( 0 );
			game->Net.Send( &melee );
			
			game->TargetID = 0;
			ClearPath();
			game->EventClock.Reset( 0. );
		}
	}
	else if( (key == SDLK_LEFT) && (game->State == BattleTech::State::WEAPON_ATTACK) )
	{
		if( (selected->TorsoTwist > -1) && ! selected->Prone && selected->ReadyAndAble() )
		{
			game->Snd.Play( game->Res.GetSound("m_twist.wav") );
			selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
			selected->TorsoTwist --;
			if( target && Path.size() )
				UpdateWeaponsInRange( selected, target );
		}
	}
	else if( (key == SDLK_RIGHT) && (game->State == BattleTech::State::WEAPON_ATTACK) )
	{
		if( (selected->TorsoTwist < 1) && ! selected->Prone && selected->ReadyAndAble() )
		{
			game->Snd.Play( game->Res.GetSound("m_twist.wav") );
			selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
			selected->TorsoTwist ++;
			if( target && Path.size() )
				UpdateWeaponsInRange( selected, target );
		}
	}
	else if( ((key == SDLK_DELETE) || ((key == SDLK_BACKSPACE) && shift)) && ! enemy_turn )
	{
		if( game->State != BattleTech::State::MOVEMENT )
			game->Snd.Play( game->Res.GetSound( selected->Destroyed() ? "i_blip.wav" : "b_eject.wav" ) );
		
		Packet remove_mech( BattleTech::Packet::REMOVE_MECH );
		remove_mech.AddUInt( selected->ID );
		game->Net.Send( &remove_mech );
		
		selected->Steps.clear();
		selected = NULL;
		game->SelectedID = 0;
		game->TargetID = 0;
		ClearPath();
	}
	
	// Only allow moving during movement and setup phases.
	else if( (game->State != BattleTech::State::MOVEMENT)
	&&       (game->State != BattleTech::State::SETUP) )
		return false;
	else if( (key == SDLK_UP) && selected->Prone && selected->ReadyAndAble() && selected->CanStand() && ! enemy_turn && ! playing_events )
	{
		selected->Steps.clear();
		selected->StandAttempts ++;
		
		Packet stand( BattleTech::Packet::STAND );
		stand.AddUInt( selected->ID );
		game->Net.Send( &stand );
	}
	else if( (key == SDLK_UP) && (! selected->Prone) && selected->ReadyAndAble() && ! playing_events )
	{
		selected->Animate( 0.05 );
		if( selected->Step(BattleTech::Move::WALK) )
		{
			game->Snd.Play( game->Res.GetSound("m_step.wav") );
			game->TargetID = 0;
			ClearPath();
		}
		else
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
	}
	else if( (key == SDLK_DOWN) && (! selected->Prone) && selected->ReadyAndAble() && ! playing_events )
	{
		selected->Animate( 0.05 );
		if( selected->Step(BattleTech::Move::REVERSE) )
		{
			game->Snd.Play( game->Res.GetSound("m_step.wav") );
			game->TargetID = 0;
			ClearPath();
		}
		else
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
	}
	else if( (key == SDLK_LEFT) && selected->ReadyAndAble() && ! playing_events )
	{
		selected->Animate( 0.05 );
		if( selected->Turn(-1) )
		{
			game->Snd.Play( game->Res.GetSound("m_turn.wav") );
			game->TargetID = 0;
			ClearPath();
		}
		else
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
	}
	else if( (key == SDLK_RIGHT) && selected->ReadyAndAble() && ! playing_events )
	{
		selected->Animate( 0.05 );
		if( selected->Turn(1) )
		{
			game->Snd.Play( game->Res.GetSound("m_turn.wav") );
			game->TargetID = 0;
			ClearPath();
		}
		else
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
	}
	else if( (key == SDLK_BACKSPACE) && selected->Steps.size() )
	{
		selected->Animate( 0.05 );
		selected->Steps.pop_back();
		game->Snd.Play( game->Res.GetSound("i_blip.wav") );
		game->TargetID = 0;
		ClearPath();
	}
	else
		return false;
	return true;
}


void HexBoard::ClearPath( bool remove_menu )
{
	Path.clear();
	WeaponsInRange.clear();
	PossibleMelee.clear();
	if( remove_menu )
		RemoveAllElements();
}


void HexBoard::UpdateWeaponsInRange( Mech *selected, Mech *target )
{
	WeaponsInRange.clear();
	
	if( !(selected && target) )
		return;
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	if( (game->State == BattleTech::State::WEAPON_ATTACK)
	||  (game->State == BattleTech::State::TAG) )
	{
		for( size_t index = 0; index < selected->Equipment.size(); index ++ )
		{
			const MechEquipment *eq = &(selected->Equipment[ index ]);
			if( ! eq->Weapon )
				continue;
			if( eq->Weapon->Defensive )
				continue;
			if( eq->Weapon->TAG != (game->State == BattleTech::State::TAG) )
				continue;
			if( eq->Damaged || eq->Jammed )
				continue;
			if( eq->Location && eq->Location->IsLeg && Path.LegWeaponsBlocked )
				continue;
			if( eq->Weapon->AmmoPerTon && ! selected->FindAmmo(eq->ID) )
				continue;
			if( eq->WithinFiringArc( target->X, target->Y ) && (TargetDist <= eq->Weapon->RangeLong) )
			{
				bool lrm = strstr( eq->Weapon->Name.c_str(), "LRM " );
				bool semiguided = false;
				uint8_t defense = (semiguided && target->Tagged) ? 0 : target->Defense;
				int8_t spotted  = (semiguided && target->Tagged) ? 0 : target->Spotted;
				
				int8_t extra = 0;
				if( target->Prone )
					extra += (TargetDist <= 1) ? -2 : 1;
				if( selected->Sensors )
					extra += selected->Sensors->Damaged * 2;
				
				int8_t difficulty = selected->GunnerySkill + selected->HeatFire + selected->Attack + defense + Path.Modifier + extra + eq->ShotModifier( TargetDist, target->ActiveStealth );
				
				bool ecm = Path.TeamECMs.size() > Path.TeamECMs.count(selected->Team);
				
				if( Path.LineOfSight )
				{
					if( selected->WeaponsToFire[ 0xFF ] && (game->State == BattleTech::State::WEAPON_ATTACK) && ! selected->FiredTAG() )
						difficulty ++;
					
					int8_t trees = Path.PartialCover ? (Path.Modifier - 1) : Path.Modifier;
					if( trees && (! ecm) && (TargetDist <= selected->ActiveProbeRange()) )
						difficulty --;
				}
				else if( lrm && (target->Spotted < 99) )
					difficulty = selected->GunnerySkill + selected->HeatFire + selected->Attack + defense + spotted + eq->ShotModifier( TargetDist, target->ActiveStealth );
				else if( lrm && target->Narced() && ! ecm )
					difficulty = selected->GunnerySkill + selected->HeatFire + selected->Attack + defense + 1 + extra + eq->ShotModifier( TargetDist, target->ActiveStealth );
				else
					continue;
				
				if( target->Immobile() )
					difficulty -= 4;
				
				if( difficulty > 12 )
					continue;
				
				WeaponsInRange[ index ] = difficulty;
			}
		}
	}
	else if( game->State == BattleTech::State::PHYSICAL_ATTACK )
	{
		int8_t modifier = target->Defense + Path.Modifier;
		
		if( target->Prone )
			modifier += (TargetDist <= 1) ? -2 : 1;
		
		if( target->Immobile() )
			modifier -= 4;
		
		bool ecm = Path.TeamECMs.size() > Path.TeamECMs.count(selected->Team);
		
		int8_t trees = Path.PartialCover ? (Path.Modifier - 1) : Path.Modifier;
		if( trees && (! ecm) && (TargetDist <= selected->ActiveProbeRange()) )
			modifier --;
		
		PossibleMelee = selected->PhysicalAttacks( target, modifier );
		
		selected->SelectedMelee.clear(); // FIXME: Remember previous choice?
		
		if( PossibleMelee.size() && ! selected->SelectedMelee.size() )
			selected->SelectedMelee.insert( *(PossibleMelee.begin()) );
	}
	else
		return;
	
	if( ! Elements.size() )
		AddElement( new WeaponMenu( this ) );
	else
	{
		WeaponMenu *wm = (WeaponMenu*) Elements.back();
		wm->Update();
	}
	
	if( ! IsTop() )
		game->Layers.RemoveTop();
}
