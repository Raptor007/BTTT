/*
 *  HexBoard.cpp
 */

#include "HexBoard.h"

#include "BattleTechDefs.h"
#include "BattleTechGame.h"
#include "SpawnMenu.h"
#include "WeaponMenu.h"
#include "GameMenu.h"
#include "RecordSheet.h"
#include "Num.h"
#include <cmath>


HexBoard::HexBoard( void )
{
	Name = "HexBoard";
	
	TurnFont = Raptor::Game->Res.GetFont( "ProFont.ttf", 22 );
	
	MessageOutput = new MessageOverlay( TurnFont );
	AddElement( MessageOutput );
	
	MessageInput = new TextBox( NULL, TurnFont, Font::ALIGN_TOP_LEFT );
	MessageInput->ReturnDeselects = false;
	MessageInput->PassReturn = true;
	MessageInput->EscDeselects = false;
	MessageInput->PassEsc = true;
	MessageInput->Visible = false;
	MessageInput->Enabled = false;
	MessageInput->TextRed = 1.f;
	MessageInput->TextGreen = 1.f;
	MessageInput->TextBlue = 1.f;
	MessageInput->TextAlpha = 0.5f;
	MessageInput->SelectedTextRed = 1.f;
	MessageInput->SelectedTextGreen = 1.f;
	MessageInput->SelectedTextBlue = 0.5f;
	MessageInput->SelectedTextAlpha = 1.f;
	MessageInput->Red = 0.f;
	MessageInput->Green = 0.f;
	MessageInput->Blue = 1.f;
	MessageInput->Alpha = 0.75f;
	MessageInput->SelectedRed = MessageInput->Red;
	MessageInput->SelectedGreen = MessageInput->Green;
	MessageInput->SelectedBlue = MessageInput->Blue;
	MessageInput->SelectedAlpha = MessageInput->Alpha;
	AddElement( MessageInput );
	
	Rect.x = 0;
	Rect.y = 0;
	UpdateRects();
}


HexBoard::~HexBoard()
{
}


void HexBoard::UpdateRects( void )
{
	if( Rect.x || Rect.y )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		game->X -= Rect.x / game->Zoom;
		game->Y -= Rect.y / game->Zoom;
	}
	
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
	
	if( Raptor::Game->Console.IsActive() )
	{
		Selected = NULL;
		MessageInput->Visible = MessageInput->Enabled = false;
	}
	else if( MessageInput->IsSelected() )
	{
		MessageInput->Rect.x = 3;
		MessageInput->Rect.y = MessageInput->Rect.x;
		MessageInput->Rect.w = Rect.w - (MessageInput->Rect.x * 2);
		MessageInput->Rect.h = MessageInput->TextFont->GetHeight();
	}
	
	HexMap *map = game->Map();
	if( map )
		map->Draw();
	else
		return;
	
	uint8_t my_team = game->MyTeam();
	
	Mech *selected = game->SelectedMech();
	Mech *target = game->TargetMech();
	
	bool playing_events = (game->EventClock.RemainingSeconds() > -0.1) || game->Events.size();
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			if( game->TeamTurn && (game->TeamTurn == my_team) && ! playing_events
			&& (mech->Team == my_team) && mech->ReadyAndAble() && (mech != selected) )
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
		
		float a = ((game->TeamTurn == my_team) || ! game->TeamTurn) ? 0.9f : 0.6f;
		
		if( selected->Destroyed() )
			map->DrawHexOutline( x, y, 3.f, 0.5f,0.5f,0.5f,a );
		else if( selected->Team == my_team )
		{
			if( selected->Ready() )
				map->DrawHexOutline( x, y, 3.f, 0.0f,1.0f,0.0f,a );
			else
				map->DrawHexOutline( x, y, 3.f, 1.0f,1.0f,1.0f,a );
		}
		else if( selected->Team && my_team )
			map->DrawHexOutline( x, y, 3.f, 1.0f,0.0f,0.0f,a );
		else
			map->DrawHexOutline( x, y, 3.f, 1.0f,1.0f,0.0f,a );
		
		if( (selected->X != x) || (selected->Y != y) )
			map->DrawHexOutline( selected->X, selected->Y, 2.f, 0.0f,0.9f,0.0f,a );
		
		if( Path.size() )
		{
			float r = 0.0f, g = 0.0f, b = 0.7f, a = 0.4f;
			if( selected->ReadyAndAble() && (selected->Team == game->TeamTurn) && (selected->Team == my_team)
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
				if( target && ((target->Spotted < 99) || ( target->Narced() && ! Path.ECMvsTeam(selected->Team) )) )
					g = 0.9f;
			}
			if( target && (selected->DeclaredTarget == target->ID) )
			{
				r = g = b = 0.4f;
				a = 0.9f;
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
	
	
	std::map< uint8_t, std::vector<GameObject*> > draw_order;
	
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second == map )
			continue;  // Map was already drawn.
		else if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			Mech *mech = (Mech*) obj_iter->second;
			if( mech->MoveClock.Progress() < 1. )
				draw_order[ 4 ].push_back( mech );
			else if( ! mech->Destroyed() )
				draw_order[ 3 ].push_back( mech );
			else if( map->MechAt_const( mech->X, mech->Y ) == mech )
				draw_order[ 2 ].push_back( mech );
			else
				draw_order[ 1 ].push_back( mech );
		}
		else
			draw_order[ 5 ].push_back( obj_iter->second );
	}
	
	for( std::map< uint8_t, std::vector<GameObject*> >::iterator draw = draw_order.begin(); draw != draw_order.end(); draw ++ )
		for( std::vector<GameObject*>::iterator obj_iter = draw->second.begin(); obj_iter != draw->second.end(); obj_iter ++ )
			(*obj_iter)->Draw();
	
	
	for( std::list<Effect>::iterator effect = game->Data.Effects.begin(); effect != game->Data.Effects.end(); effect ++ )
		effect->Draw();
	
	
	#define SHOW_HIT_TIME 3.5
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			
			uint8_t x, y;
			mech->GetPosition( &x, &y );
			Pos3D pos = map->Center( x, y );
			std::string att_str, def_str, spot_str;
			Vec3D shadow( 0.01, 0.02 );
			float alpha = 1.f;
			
			if( playing_events || (game->State >= BattleTech::State::END) )
				;
			else if( mech->TookTurn || ((game->State > BattleTech::State::MOVEMENT) && ! mech->Destroyed()) )
			{
				att_str = std::string("+") + Num::ToString(mech->Attack);
				def_str = std::string(/*mech->Tagged?" +":*/"+") + Num::ToString(mech->Defense) + std::string(/*mech->Tagged?"*":*/" ");
				if( mech->Spotted < 99 )
					spot_str = std::string((mech->Spotted >= 0)?"+":"") + Num::ToString(mech->Spotted);
			}
			else if( (game->State == BattleTech::State::MOVEMENT) && mech->Steps.size() )
			{
				uint8_t move_mode = mech->SpeedNeeded(true);
				if( move_mode != BattleTech::Speed::INVALID )
				{
					uint8_t entered = mech->EnteredHexes( move_mode );
					att_str = std::string("+") + Num::ToString(mech->AttackPenalty(move_mode));
					def_str = std::string("+") + Num::ToString(mech->DefenseBonus(move_mode,entered));
					alpha = 0.25f + 0.75f * fabsf(cosf( mech->Lifetime.ElapsedSeconds() * M_PI ));
				}
			}
			
			if( att_str.length() )
			{
				Pos3D att = pos + Vec2D( 0.5, 0.1 );
				Pos3D att2 = att + shadow;
				TurnFont->DrawText3D( att_str, &att2, Font::ALIGN_MIDDLE_CENTER, 0.f,0.f,0.f,alpha*0.6f, 0.016 );
				TurnFont->DrawText3D( att_str, &att, Font::ALIGN_MIDDLE_CENTER, 1.0f,0.4f,0.1f,alpha*1.f, 0.016 );
			}
			if( def_str.length() )
			{
				Pos3D def = pos + Vec2D( 0.3, 0.4 );
				Pos3D def2 = def + shadow;
				TurnFont->DrawText3D( def_str, &def2, Font::ALIGN_MIDDLE_CENTER, 0.f,0.f,0.f,alpha*0.6f, 0.016 );
				TurnFont->DrawText3D( def_str, &def, Font::ALIGN_MIDDLE_CENTER, 0.1f,0.6f,1.0f,alpha*1.f, 0.016 );
			}
			if( spot_str.length() )
			{
				Pos3D spot = pos + Vec2D( -0.3, 0.4 );
				Pos3D spot2 = spot + shadow;
				TurnFont->DrawText3D( spot_str, &spot2, Font::ALIGN_MIDDLE_CENTER, 0.f,0.f,0.f,alpha*0.6f, 0.016 );
				TurnFont->DrawText3D( spot_str, &spot, Font::ALIGN_MIDDLE_CENTER, 1.0f,0.8f,0.1f,alpha*1.f, 0.016 );
			}
			
			if( (mech->HitClock.ElapsedSeconds() <= SHOW_HIT_TIME) && (mech->Lifetime.ElapsedSeconds() > SHOW_HIT_TIME) && ! game->GetRecordSheet(mech) )
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
	
	double mx = (game->Mouse.X - game->Gfx.W/2) / game->Zoom + game->X;
	double my = (game->Mouse.Y - game->Gfx.H/2) / game->Zoom + game->Y;
	Hex *hex = map->Nearest( mx, my );
	const Mech *moused = hex ? map->MechAt_const( hex->X, hex->Y ) : NULL;
	
	if( moused )
	{
		if( (moused->HitClock.ElapsedSeconds() > SHOW_HIT_TIME) && ! game->GetRecordSheet(moused) )
			moused->DrawHealth( game->Mouse.X - 64, game->Mouse.Y + 35, game->Mouse.X + 64, game->Mouse.Y + 163 );
		
		std::string name = moused->ShortFullName();
		if( game->State == BattleTech::State::SETUP )
		{
			name += std::string(" [") + Num::ToString(moused->Tons) + std::string("T]");
			if( (moused->GunnerySkill != 3) || (moused->PilotSkill != 4) )
				name += std::string(" [G") + Num::ToString(moused->GunnerySkill) + std::string("/P") + Num::ToString(moused->PilotSkill) + std::string("]");
		}
		if( moused->Destroyed() )
			name += " [DESTROYED]";
		else
		{
			if( moused->Shutdown )
				name += " [SHUTDOWN]";
			if( moused->Unconscious )
				name += " [UNCONSCIOUS]";
			if( moused->Prone )
				name += " [PRONE]";
			if( moused->Narced() )
				name += " [NARC]";
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
	
	if( hex && selected && ! playing_events )
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
	
	std::string status;
	if( (game->State != BattleTech::State::SETUP) && playing_events )
	{
		for( std::deque<TextConsoleMessage*>::reverse_iterator msg = game->Console.Messages.rbegin(); msg != game->Console.Messages.rend(); msg ++ )
		{
			if( (*msg)->Type == TextConsole::MSG_NORMAL )
			{
				status = (*msg)->Text;
				break;
			}
		}
	}
	else if( Selected == MessageInput )
	{
		status = "Press Enter to send chat message or Esc to cancel.";
	}
	else if( (game->State == BattleTech::State::MOVEMENT) && selected && selected->Ready() && (selected->Team == game->TeamTurn) && (game->TeamTurn == my_team) )
	{
		uint8_t move_mode = selected->MoveSpeed ? selected->MoveSpeed : selected->SpeedNeeded(true);
		uint8_t step_cost = selected->StepCost();
		if( move_mode == BattleTech::Speed::JUMP )
		{
			uint8_t x, y;
			selected->GetPosition( &x, &y );
			step_cost = map->HexDist( selected->X, selected->Y, x, y );
		}
		
		status = selected->ShortName();
		
		if( selected->Steps.size() )
			status += std::string(" moves ") + Num::ToString(step_cost) + std::string(" of ");
		else if( step_cost )
			status += std::string(" moved ") + Num::ToString(step_cost) + std::string(" of ");
		else
			status += " can move ";
		
		status += selected->MPString( selected->MoveSpeed );
		
		if( move_mode == BattleTech::Speed::WALK )
			status += " walking.";
		else if( (move_mode == BattleTech::Speed::RUN) && (step_cost > selected->RunDist()) )
			status += " running with Minimum Movement rule.";
		else if( move_mode == BattleTech::Speed::RUN )
			status += " running.";
		else if( move_mode == BattleTech::Speed::MASC )
			status += " with MASC.";
		else if( move_mode == BattleTech::Speed::SUPERCHARGE )
			status += " with Supercharger.";
		else if( move_mode == BattleTech::Speed::MASC_SUPERCHARGE )
			status += " with MASC and Supercharger.";
		else if( move_mode == BattleTech::Speed::JUMP )
			status += " jumping.";
		else if( move_mode == BattleTech::Speed::INVALID )
			status += ", an invalid move.";
		else
			status += ".";
		
		if( selected->Prone )
		{
			if( ! selected->CanStand() )
				status += "  It cannot stand.";
			else if( selected->MoveSpeed || (selected->WalkDist() == 1) )
				status += "  Up arrow to stand.";
			else
				status += "  Up arrow to stand running, down for walking.";
		}
		
		if( selected->Shutdown || selected->Unconscious )
			status = selected->ShortName() + std::string(" cannot move this turn.  Press Enter to skip.");
		else if( selected->StandAttempts && (! selected->Prone) && (selected->WalkDist() == 1) )
			status = selected->ShortName() + std::string(" stood using Minimum Movement rule.");
		
		if( (move_mode >= BattleTech::Speed::WALK) && game->Cfg.SettingAsBool("debug") )
		{
			uint8_t entered = selected->EnteredHexes( move_mode );
			status += std::string("  [A+") + Num::ToString(selected->AttackPenalty(move_mode))
			       +  std::string(" H+") + Num::ToString(selected->MovementHeat(move_mode,entered))
			       +  std::string(" E") + Num::ToString(entered)
			       +  std::string(" D+") + Num::ToString(selected->DefenseBonus(move_mode,entered)) + std::string("]");
		}
	}
	else if( (game->State >= BattleTech::State::TAG) && (game->State <= BattleTech::State::PHYSICAL_ATTACK) && selected && selected->Ready() && (selected->Team == game->TeamTurn) && (game->TeamTurn == my_team) )
	{
		if( ! selected->ReadyAndAble() )
		{
			status = selected->ShortName() + std::string(" cannot ") + game->PhaseName() + std::string(" this turn.");
			if( (game->State == BattleTech::State::MOVEMENT) || (game->State == BattleTech::State::WEAPON_ATTACK) )
				status += std::string("  Press Enter to skip.");
		}
		else if( target && Path.LineOfSight && (WeaponsInRange.size() || (game->State == BattleTech::State::PHYSICAL_ATTACK)) )
			status = std::string("Press Enter to submit ") + game->PhaseName() + std::string(" target: ") + target->ShortName();
		else if( Path.size() )
			status = std::string("Press Enter to skip ") + game->PhaseName() + std::string(" (no target).");
		else
			status = std::string("Right-click target for ") + game->PhaseName() + std::string(", or press Enter to skip.");
	}
	else if( (game->State == BattleTech::State::SETUP) || ! playing_events )
	{
		bool hotseat = game->Hotseat();
		bool ai = game->Data.PropertyAsInt("ai_team");
		status = game->PhaseName();
		if( game->TeamTurn && my_team )
		{
			if( (game->TeamTurn == my_team) && ! hotseat )
				status += " (your team)";
			else if( (game->TeamsAlive() <= 2) && ! hotseat )
				status += " (enemy team)";
			else
			{
				std::string team_name = game->TeamName( game->TeamTurn );
				if( ! team_name.empty() )
					status += std::string(" (") + team_name + std::string(")");
			}
		}
		else if( game->State == BattleTech::State::SETUP )
		{
			if( selected && selected->Steps.size() )
				status = "Press Enter to submit new spawn position.";
			else if( game->Layers.Find("GameMenu") )
			{
				if( game->ReadyToBegin() )
				{
					if( game->Admin() )
						status = "When everyone is ready, click Initiate Combat.";
					else
						status = "Waiting for host to Initiate Combat.";
				}
				else
					status = "Set game options, then click Team/Mech to begin.";
			}
			else if( game->Layers.Find("SpawnMenu") )
			{
				if( my_team )
					status = "Select a BattleMech, then right-click to drop.";
				else
					status = "Select a team and BattleMech, then right-click to drop.";
			}
			else if( selected && (selected->PlayerID == game->PlayerID) )
			{
				if( (hotseat || ai) && ! game->ReadyToBegin() )
					status = "Press Tab to spawn for the other team.  When done, press Enter.";
				else if( game->ReadyToBegin() )
				{
					if( game->Admin() )
						status = "When everyone is ready to play, press Enter.";
					else
						status = "Waiting for host to Initiate Combat.";
				}
				else
					status = "Use arrows to move spawn point and Enter to submit.";
			}
			else if( my_team )
				status = "Right-click to drop.  Press Tab to change team/Mech.";
			else
				status = "Press Tab to select a team and BattleMech.";
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
	
	Draggable = false;
	
	if( (button == SDL_BUTTON_LEFT) && ! shift )
	{
		Mech *clicked = hex ? map->MechAt( hex->X, hex->Y ) : NULL;
		
		Draggable = (! clicked) && game->Cfg.SettingAsBool("map_drag");
		
		if( clicked != selected )
		{
			if( selected )
				selected->Steps.clear();
			selected = clicked;
			game->SelectedID = selected ? selected->ID : 0;
			target = NULL;
			game->TargetID = 0;
			
			if( selected )
				game->Snd.Play( game->Res.GetSound("i_select.wav") );
			
			if( selected && selected->DeclaredTarget )
			{
				ClearPath();  // FIXME: This should be ClearPath( false ) when UpdateWeaponsInRange no longer shows to-hit for undeclared weapons.
				game->TargetID = selected->DeclaredTarget;
				target = game->TargetMech();
				Path = map->Path( selected->X, selected->Y, target->X, target->Y );
				//UpdateWeaponsInRange( selected, target );  // FIXME: Only show to-hit for declared weapons.
			}
			else
				ClearPath();
		}
	}
	else if( button == SDL_BUTTON_MIDDLE )
		Draggable = true;
	else if( (button == SDL_BUTTON_WHEELUP) && ! game->Console.IsActive() )
	{
		game->Zoom *= 1.1;
		double new_mx = (game->Mouse.X - game->Gfx.W/2) / game->Zoom + game->X;
		double new_my = (game->Mouse.Y - game->Gfx.H/2) / game->Zoom + game->Y;
		game->X += (mx - new_mx);
		game->Y += (my - new_my);
	}
	else if( (button == SDL_BUTTON_WHEELDOWN) && ! game->Console.IsActive() )
	{
		game->Zoom /= 1.1;
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
			{
				game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
				return true;
			}
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
			RemoveSetupMenus();
		}
		/*
		// Prevent trying other targets when a Mech has already declared its shot.
		// FIXME: Commented-out because this currently prevents seeing the to-hit rolls of declared shots.
		else if( (game->State >= BattleTech::State::TAG) && selected && selected->TookTurn )
			;
		*/
		else if( selected )
		{
			uint8_t x = 0, y = 0, facing = BattleTech::Dir::UP;
			selected->GetPosition( &x, &y, &facing );
			if( hex )
			{
				ClearPath( false );
				Path = map->Path( x, y, hex->X, hex->Y );
				target = map->MechAt( hex->X, hex->Y );
			}
			else
			{
				ClearPath();
				target = NULL;
			}
			if( ! target )
				;
			else if( (target == selected) || target->Destroyed() )
				target = NULL;
			else if( selected->Team && target->Team && (selected->Team == target->Team) && ! game->FF() )
				target = NULL;
			game->TargetID = target ? target->ID : 0;
			
			if( target )
			{
				UpdateWeaponsInRange( selected, target );
				
				if( selected->ReadyAndAble()
				&& ((game->State == BattleTech::State::WEAPON_ATTACK)
				 || (game->State == BattleTech::State::PHYSICAL_ATTACK)
				 || (game->State == BattleTech::State::TAG)) )
					game->Snd.Play( game->Res.GetSound("i_select.wav") );
			}
			else
				RemoveWeaponMenu();
			
			if( game->Cfg.SettingAsBool("debug") )
			{
				bool ecm = Path.ECMvsTeam( selected->Team );
				std::string msg = "Dist from ";
				msg += Num::ToString((*(Path.begin()))->X) + std::string(" ") + Num::ToString((*(Path.begin()))->Y);
				msg += std::string(" to ");
				msg += Num::ToString((*(Path.rbegin()))->X) + std::string(" ") + Num::ToString((*(Path.rbegin()))->Y);
				if( target )
					msg += std::string(" (") + target->ShortName() + std::string(")");
				msg += std::string(" is ") + Num::ToString(Path.Distance) + std::string(" hexes");
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
	bool alt = game->Keys.KeyDown(SDLK_LALT) || game->Keys.KeyDown(SDLK_RALT);
	
	if( MessageInput->IsSelected() )
	{
		if( (key == SDLK_RETURN) || (key == SDLK_KP_ENTER) )
		{
			std::string msg = MessageInput->Text;
			MessageInput->Text = "";
			Selected = NULL;
			MessageInput->Visible = MessageInput->Enabled = false;
			Player *player = Raptor::Game->Data.GetPlayer( Raptor::Game->PlayerID );
			if( player && ! msg.empty() )
			{
				Packet message = Packet( Raptor::Packet::MESSAGE );
				message.AddString( player->Name + std::string(": ") + msg );
				message.AddUInt( TextConsole::MSG_CHAT );
				Raptor::Game->Net.Send( &message );
			}
		}
		else if( key == SDLK_ESCAPE )
		{
			MessageInput->Text = "";
			Selected = NULL;
			MessageInput->Visible = MessageInput->Enabled = false;
		}
		return MessageInput->KeyDown( key );
	}
	else if( (key == SDLK_t) || (key == SDLK_INSERT) )
	{
		Selected = MessageInput;
		MessageInput->Visible = MessageInput->Enabled = true;
	}
	else if( key == SDLK_i )
	{
		RecordSheet *rs = game->GetRecordSheet( selected );
		if( rs )
			rs->Close();
		else
			game->ShowRecordSheet( selected );
	}
	else if( (key == SDLK_o) && target )
	{
		RecordSheet *rs = game->GetRecordSheet( target );
		if( rs )
			rs->Close();
		else
			game->ShowRecordSheet( target );
	}
	else if( key == SDLK_p )
	{
		bool playing_events = (game->EventClock.RemainingSeconds() > -0.1) || game->Events.size();
		RecordSheet *rs = (RecordSheet*) game->Layers.Find("RecordSheet");
		if( playing_events )
		{
			if( rs )
			{
				rs->Remove();
				game->Cfg.Settings[ "record_sheet_popup" ] = "false";
			}
			else
				game->Cfg.Settings[ "record_sheet_popup" ] = "true";
		}
		else while( rs )
		{
			rs->Close();
			rs = (RecordSheet*) game->Layers.Find("RecordSheet");
		}
	}
	else if( (key == SDLK_q) || (key == SDLK_HOME) )
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
		game->Zoom *= 1.21;
	else if( (key == SDLK_f) || (key == SDLK_MINUS) || (key == SDLK_PAGEDOWN) )
		game->Zoom /= 1.21;
	else if( key == SDLK_w )
		game->Y -= 150. / game->Zoom;
	else if( key == SDLK_s )
		game->Y += 150. / game->Zoom;
	else if( key == SDLK_a )
		game->X -= 150. / game->Zoom;
	else if( key == SDLK_d )
		game->X += 150. / game->Zoom;
	
	else if( (key >= SDLK_F1) && (key <= SDLK_F15) && ((game->State == BattleTech::State::SETUP) || ! game->MyTeam()) )
	{
		int teams = game->Data.PropertyAsInt("teams",2);
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
	else if( key == SDLK_TAB )
	{
		RemoveWeaponMenu();
		game->Layers.Add( new SpawnMenu() );
	}
	else if( ((key == SDLK_RETURN) || (key == SDLK_KP_ENTER))
	&&       ((game->State == BattleTech::State::SETUP) || (game->State == BattleTech::State::GAME_OVER))
	&&       (! selected || ! selected->Steps.size()) )
	{
		RemoveWeaponMenu();
		game->Layers.Add( new GameMenu() );
	}
	else if( (key == SDLK_ESCAPE) && (playing_events || ! selected) )
	{
		RemoveWeaponMenu();
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
		uint8_t speed = (game->State == BattleTech::State::SETUP) ? (uint8_t) BattleTech::Speed::JUMP : selected->SpeedNeeded(true);
		if( (game->State == BattleTech::State::MOVEMENT) && selected->Steps.size() && (speed == BattleTech::Speed::INVALID) )
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
			
			if( game->State == BattleTech::State::MOVEMENT )
				selected->MoveSpeed = speed;
			
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
				if( Path.ECMvsTeam( selected->Team ) )
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
			
			if( target && Path.LineOfSight && (game->State == BattleTech::State::WEAPON_ATTACK) && selected->SpottingWithoutTAG() )
			{
				// Spot for Indirect Fire
				
				// NOTE: WeaponRollNeeded already adds +1 for SpottingWithoutTAG.
				int8_t spotted = selected->WeaponRollNeeded( target, &Path ) + (weapons.size() ? 1 : 0) - selected->GunnerySkill - selected->HeatFire;
				
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
	else if( (key == SDLK_BACKSPACE) && (game->State == BattleTech::State::WEAPON_ATTACK) && selected->TorsoTwist && selected->ReadyAndAble() )
	{
		game->Snd.Play( game->Res.GetSound("m_twist.wav") );
		selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
		selected->TorsoTwist = 0;
		if( target && Path.size() )
			UpdateWeaponsInRange( selected, target );
	}
	else if( ((key == SDLK_DELETE) || ((key == SDLK_BACKSPACE) && shift)) && ! enemy_turn )
	{
		if( game->State != BattleTech::State::MOVEMENT )
			game->Snd.Play( game->Res.GetSound( (game->State == BattleTech::State::GAME_OVER) || selected->Destroyed() ? "i_blip.wav" : "b_eject.wav" ) );
		
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
	else if( ((key == SDLK_UP) || (key == SDLK_DOWN)) && selected->Prone && selected->ReadyAndAble() && selected->CanStand() && ! enemy_turn && ! playing_events )
	{
		selected->Steps.clear();
		if( ! selected->MoveSpeed )
		{
			if( key == SDLK_UP )
			{
				bool masc = shift && selected->MASCDist( BattleTech::Speed::MASC );
				bool supercharge = alt && selected->MASCDist( BattleTech::Speed::SUPERCHARGE );
				if( masc && supercharge )
					selected->MoveSpeed = BattleTech::Speed::MASC_SUPERCHARGE;
				else if( masc )
					selected->MoveSpeed = BattleTech::Speed::MASC;
				else if( supercharge )
					selected->MoveSpeed = BattleTech::Speed::SUPERCHARGE;
				else
					selected->MoveSpeed = BattleTech::Speed::RUN;
			}
			else if( selected->WalkDist() < 2 ) // Minimum Movement Rule
				selected->MoveSpeed = BattleTech::Speed::RUN;
			else
				selected->MoveSpeed = BattleTech::Speed::WALK;
		}
		
		Packet stand( BattleTech::Packet::STAND );
		stand.AddUInt( selected->ID );
		stand.AddUChar( selected->MoveSpeed );
		game->Net.Send( &stand );
	}
	else if( (key == SDLK_UP) && (! selected->Prone) && selected->ReadyAndAble() && ! playing_events )
	{
		selected->Animate( 0.05 );
		if( selected->Step( BattleTech::Move::FORWARD ) )
		{
			game->Snd.Play( game->Res.GetSound("m_step.wav") );
			UpdateAim();
		}
		else
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
	}
	else if( (key == SDLK_DOWN) && (! selected->Prone) && selected->ReadyAndAble() && ! playing_events )
	{
		selected->Animate( 0.05 );
		if( selected->Step( BattleTech::Move::REVERSE ) )
		{
			game->Snd.Play( game->Res.GetSound("m_step.wav") );
			UpdateAim();
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
			UpdateAim();
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
			UpdateAim();
		}
		else
			game->Snd.Play( game->Res.GetSound("i_toofar.wav") );
	}
	else if( (key == SDLK_BACKSPACE) && selected->Steps.size() )
	{
		selected->Animate( 0.05 );
		selected->Steps.pop_back();
		game->Snd.Play( game->Res.GetSound("i_blip.wav") );
		UpdateAim();
	}
	else
		return false;
	return true;
}


void HexBoard::ClearPath( bool remove_menu )
{
	Path.Clear();
	WeaponsInRange.clear();
	PossibleMelee.clear();
	if( remove_menu )
		RemoveWeaponMenu();
}


void HexBoard::RemoveWeaponMenu( void )
{
	Layer *wm = Raptor::Game->Layers.Find("WeaponMenu");
	if( wm )
		wm->Remove();
}


void HexBoard::RemoveSetupMenus( void ) const
{
	Layer *spawn_menu = Raptor::Game->Layers.Find("SpawnMenu");
	if( spawn_menu )
		spawn_menu->Remove();
	
	Layer *game_menu = Raptor::Game->Layers.Find("GameMenu");
	if( game_menu )
		game_menu->Remove();
}


void HexBoard::UpdateAim( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	Mech *selected = game->SelectedMech();
	const Mech *target = game->TargetMech();
	if( selected && target && ! selected->Destroyed() && ! target->Destroyed() )
	{
		uint8_t x1, y1, x2, y2;
		selected->GetPosition( &x1, &y1 );
		target->GetPosition(   &x2, &y2 );
		bool repath = true;
		if( Path.size() )
		{
			const Hex *hex1 = *(Path.begin());
			const Hex *hex2 = *(Path.rbegin());
			repath = (x1 != hex1->X) || (y1 != hex1->Y) || (x2 != hex2->X) || (y2 != hex2->Y);
		}
		if( repath )
			Path = game->Map()->Path( x1, y1, x2, y2 );
		UpdateWeaponsInRange( selected, target );
	}
	else
		ClearPath();
}


void HexBoard::UpdateWeaponsInRange( Mech *selected, const Mech *target )
{
	HexBoardAim::UpdateWeaponsInRange( selected, target, Raptor::Game->State );
	
	if( Raptor::Game->State == BattleTech::State::PHYSICAL_ATTACK )
	{
		selected->SelectedMelee.clear(); // FIXME: Remember previous choice?
		
		if( ! PossibleMelee.size() )
		{
			RemoveWeaponMenu();
			return;
		}
		else if( ! selected->SelectedMelee.size() )
			selected->SelectedMelee.insert( *(PossibleMelee.begin()) );
	}
	
	RemoveSetupMenus();
	
	WeaponMenu *wm = (WeaponMenu*) Raptor::Game->Layers.Find("WeaponMenu");
	if( ! wm )
		Raptor::Game->Layers.Add( new WeaponMenu() );
	else
	{
		wm->Update();
		wm->MoveToTop();
	}
}


// -----------------------------------------------------------------------------


void HexBoardAim::UpdateWeaponsInRange( const Mech *selected, const Mech *target, int state )
{
	WeaponsInRange.clear();
	PossibleMelee.clear();
	
	if( !(selected && target) )
		return;
	if( selected->Unconscious || selected->Shutdown )
		return;
	
	int8_t modifier = 0;
	if( state == BattleTech::State::MOVEMENT )
	{
		uint8_t speed = selected->SpeedNeeded();
		if( speed == BattleTech::Speed::WALK )
			modifier = 1;
		else if( (speed >= BattleTech::Speed::RUN) && (speed <= BattleTech::Speed::MASC_SUPERCHARGE) )
			modifier = 2;
		else if( speed == BattleTech::Speed::JUMP )
			modifier = 3;
	}
	
	if( state != BattleTech::State::PHYSICAL_ATTACK )
	{
		for( size_t index = 0; index < selected->Equipment.size(); index ++ )
		{
			const MechEquipment *eq = &(selected->Equipment[ index ]);
			
			if( eq->Weapon && eq->Weapon->TAG && (state == BattleTech::State::WEAPON_ATTACK) )
				continue;
			if( eq->Weapon && (! eq->Weapon->TAG) && (state == BattleTech::State::TAG) )
				continue;
			
			int8_t difficulty = selected->WeaponRollNeeded( target, &Path, eq ) + modifier;
			
			if( difficulty <= 12 )
				WeaponsInRange[ index ] = difficulty;
		}
	}
	
	if( (state == BattleTech::State::PHYSICAL_ATTACK) || (state == BattleTech::State::MOVEMENT) )
	{
		modifier += target->Defense + Path.Modifier;
		
		if( target->Prone )
			modifier += (Path.Distance <= 1) ? -2 : 1;
		
		if( target->Immobile() )
			modifier -= 4;
		
		bool ecm = Path.ECMvsTeam( selected->Team );
		
		int8_t trees = Path.PartialCover ? (Path.Modifier - 1) : Path.Modifier;
		if( trees && (! ecm) && (Path.Distance <= selected->ActiveProbeRange()) )
			modifier --;
		
		PossibleMelee = selected->PhysicalAttacks( target, modifier );
	}
}


Packet *HexBoardAim::CreatePacket( const Mech *selected, const Mech *target, int state ) const
{
	uint8_t speed = (state == BattleTech::State::SETUP) ? (uint8_t) BattleTech::Speed::JUMP : selected->SpeedNeeded(true);
	if( (state == BattleTech::State::MOVEMENT) && selected->Steps.size() && (speed == BattleTech::Speed::INVALID) )
		return NULL;
	else if( (state == BattleTech::State::MOVEMENT)
	||      ((state == BattleTech::State::SETUP) && selected->Steps.size()) )
	{
		Packet *p = new Packet( BattleTech::Packet::MOVEMENT );
		p->AddUInt( selected->ID );
		p->AddUChar( speed );
		p->AddUChar( selected->Steps.size() );
		for( std::vector<MechStep>::const_iterator step = selected->Steps.begin(); step != selected->Steps.end(); step ++ )
			p->AddUChar( step->Move );
		
		/*
		if( state == BattleTech::State::MOVEMENT )
			selected->MoveSpeed = speed;
		
		selected->Steps.clear();
		*/
		
		return p;
	}
	else if( (state == BattleTech::State::WEAPON_ATTACK)
	||       (state == BattleTech::State::TAG) )
	{
		std::map<uint8_t,int8_t> weapons;
		if( target )
		{
			for( std::map<uint8_t,int8_t>::const_iterator weap = WeaponsInRange.begin(); weap != WeaponsInRange.end(); weap ++ )
			{
				std::map<uint8_t,uint8_t>::const_iterator fire = selected->WeaponsToFire.find( weap->first );
				if( (fire != selected->WeaponsToFire.end()) && fire->second )
					weapons[ weap->first ] = weap->second;
			}
		}
		
		uint8_t firing_arc = target ? selected->FiringArc( target->X, target->Y ) : (uint8_t) BattleTech::Arc::STRUCTURE;
		
		int8_t turned_arm = BattleTech::Loc::UNKNOWN;
		bool arms_flipped = false;
		if( (! selected->TorsoTwist) && (firing_arc == BattleTech::Arc::REAR) )
		{
			for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
			{
				const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
				if( eq->Location && eq->Location->IsArm() )
				{
					arms_flipped = true;
					break;
				}
			}
		}
		else if( weapons.size() && (firing_arc == BattleTech::Arc::LEFT_SIDE) )
			turned_arm = BattleTech::Loc::LEFT_ARM;
		else if( weapons.size() && (firing_arc == BattleTech::Arc::RIGHT_SIDE) )
			turned_arm = BattleTech::Loc::RIGHT_ARM;
		
		int8_t prone_fire = BattleTech::Loc::UNKNOWN;
		if( selected->Prone && weapons.size() )
		{
			prone_fire = BattleTech::Loc::CENTER_TORSO;
			for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
			{
				const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
				if( eq->Location && eq->Location->IsArm() )
				{
					prone_fire = eq->Location->Loc;
					break;
				}
			}
		}
		
		Packet *p = new Packet( BattleTech::Packet::SHOTS );
		p->AddUInt( selected->ID );
		p->AddChar( selected->TorsoTwist );
		p->AddChar( turned_arm );
		p->AddChar( prone_fire );
		
		uint8_t count_and_flags = weapons.size();
		if( arms_flipped )
			count_and_flags |= 0x80;
		p->AddUChar( count_and_flags );
		
		for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
		{
			p->AddUInt( target->ID );
			
			const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
			uint8_t arc_and_flags = target->DamageArc( selected->X, selected->Y );
			if( ! Path.LineOfSight )
				arc_and_flags |= 0x80; // Indirect Fire
			else if( Path.PartialCover )
				arc_and_flags |= 0x40; // Leg Partial Cover
			if( Path.ECMvsTeam( selected->Team ) )
				arc_and_flags |= 0x20; // Within Friendly ECM
			if( target->Narced() && (Path.LineOfSight || (target->Spotted < 99)) )
				arc_and_flags |= 0x10; // NARC Cluster Bonus (with exceptions in MechEquipment::ClusterHits)
			if( eq->WeaponFCS && ! eq->WeaponFCS->Damaged )
				arc_and_flags |= 0x08; // Artemis IV FCS
			
			std::map<uint8_t,uint8_t>::const_iterator fire = selected->WeaponsToFire.find( weap->first );
			uint8_t count = (fire != selected->WeaponsToFire.end()) ? fire->second : 0;
			
			p->AddUChar( arc_and_flags );
			p->AddUChar( weap->first );
			p->AddUChar( count );
			p->AddChar( weap->second );
			
			/*
			if( !(eq->Weapon && eq->Weapon->TAG) ) // TAG will only count as "fired" if it hits (reported in SPOT packet).
				eq->Fired = weap->second;
			*/
		}
		
		if( target && Path.LineOfSight && (state == BattleTech::State::WEAPON_ATTACK) && selected->SpottingWithoutTAG() )
		{
			// Spot for Indirect Fire
			
			// NOTE: WeaponRollNeeded already adds +1 for SpottingWithoutTAG.
			int8_t spotted = selected->WeaponRollNeeded( target, &Path ) + (weapons.size() ? 1 : 0) - selected->GunnerySkill - selected->HeatFire;
			
			p->AddUInt( target->ID );
			p->AddChar( spotted );
		}
		else
		{
			p->AddUInt( 0 );
			p->AddChar( 99 );
		}
		
		return p;
	}
	else if( state == BattleTech::State::PHYSICAL_ATTACK )
	{
		Packet *p = new Packet( BattleTech::Packet::MELEE );
		p->AddUInt( selected->ID );
		if( target )
		{
			p->AddUChar( selected->SelectedMelee.size() );
			for( std::set<MechMelee>::const_iterator chosen = selected->SelectedMelee.begin(); chosen != selected->SelectedMelee.end(); chosen ++ )
			{
				p->AddUInt( target->ID );
				p->AddUChar( target->DamageArc( selected->X, selected->Y ) | (chosen->HitTable << 2) );
				p->AddUChar( chosen->Attack );
				p->AddChar( chosen->Difficulty );
				p->AddUChar( chosen->Damage );
				p->AddUChar( chosen->Limb );
			}
		}
		else
			p->AddUChar( 0 );
		
		return p;
	}
	
	return NULL;
}
