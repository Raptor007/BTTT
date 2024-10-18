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
#include <algorithm>


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
	MessageInput->ClickOutDeselects = false;
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
	
	ClearPath( false );
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
	
	int overlay_lines = game->Cfg.SettingAsInt( "overlay_lines", 10 );
	MessageOutput->MaxMessages = (overlay_lines > 0) ? overlay_lines : 10;  // Overlay is always used for chat.
	MessageOutput->SetTypeToDraw( TextConsole::MSG_NORMAL, (overlay_lines > 0) );
	MessageOutput->ScrollTime = game->Cfg.SettingAsDouble( "overlay_scroll", 0.2 );
	
	HexMap *map = game->Map();
	if( map )
		map->Draw();
	else
		return;
	
	uint8_t my_team = game->MyTeam();
	
	Mech *selected = game->SelectedMech();
	Mech *target = game->TargetMech();
	
	bool playing_events = game->PlayingEvents();
	
	if( game->TeamTurn && (game->TeamTurn == my_team) && ! playing_events )
	{
		for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
		{
			if( obj_iter->second->Type() == BattleTech::Object::MECH )
			{
				const Mech *mech = (const Mech*) obj_iter->second;
				if( (mech->Team == my_team) && mech->ReadyAndAble() && (mech != selected) )
				{
					uint8_t x, y;
					mech->GetPosition( &x, &y );
					map->DrawHexOutline( x, y, selected?1.f:2.f, 0.f,0.2f,1.f );
				}
			}
		}
	}
	
	if( selected && (! playing_events) && ! game->Cfg.SettingAsBool("screensaver") )
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
		
		// FIXME: Sort by path length to better handle paths that overlap other targets.
		for( size_t i = 0; i < Paths.size(); i ++ )
		{
			if( Paths[ i ].size() )
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
				if( ! Paths[ i ].LineOfSight )
				{
					r = 1.0f;
					b = 0.0f;
					if( target && ((target->Spotted < 99) || ( target->Narced() && ! Paths[ i ].ECMvsTeam(selected->Team) && ! selected->ActiveStealth )) )
						g = 0.9f;
				}
				if( target && (selected->DeclaredTarget == target->ID) )
				{
					r = g = b = 0.4f;
					a = 0.9f;
				}
				for( ShotPath::const_iterator hex = Paths[ i ].begin(); hex != Paths[ i ].end(); hex ++ )
				{
					if( hex == Paths[ i ].begin() )
						continue;
					map->DrawHexOutline( (*hex)->X, (*hex)->Y, 2.f, r,g,b,a );
				}
				if( Paths[ i ].size() >= 2 )
				{
					Pos3D here = (*(Paths[ i ].begin()))->Center()  + Paths[ i ].Offset;
					Pos3D dest = (*(Paths[ i ].rbegin()))->Center() + Paths[ i ].Offset;
					game->Gfx.DrawLine2D( here.X, here.Y, dest.X, dest.Y, 2.f, r,g,b,a*0.9f );
				}
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
			int bonus = mech->Destroyed() ? 0 : 5;
			if( mech->MoveClock.Progress() < 1. )
				draw_order[ 5 + bonus ].push_back( mech );
			else if( mech->MoveClock.ElapsedSeconds() < 0.5 )
				draw_order[ 4 + bonus ].push_back( mech );
			else if( mech->ID == game->SelectedID )
				draw_order[ 3 + bonus ].push_back( mech );
			else if( map->MechAt_const( mech->X, mech->Y ) == mech )
				draw_order[ 2 + bonus ].push_back( mech );
			else
				draw_order[ 1 + bonus ].push_back( mech );
		}
		else
			draw_order[ 11 ].push_back( obj_iter->second );
	}
	
	for( std::map< uint8_t, std::vector<GameObject*> >::iterator draw = draw_order.begin(); draw != draw_order.end(); draw ++ )
		for( std::vector<GameObject*>::iterator obj_iter = draw->second.begin(); obj_iter != draw->second.end(); obj_iter ++ )
			(*obj_iter)->Draw();
	
	
	for( std::list<Effect>::iterator effect = game->Data.Effects.begin(); effect != game->Data.Effects.end(); effect ++ )
		effect->Draw();
	
	
	double show_hit_time = game->Cfg.SettingAsDouble( "show_hit", 3.5, 3.5 );
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			
			uint8_t x, y;
			mech->GetPosition( &x, &y );
			Pos3D pos = map->Center( x, y );
			std::string att_str, def_str, spot_str, num_str;
			Vec3D shadow( 0.01, 0.02 );
			float alpha = 1.f;
			
			if( playing_events || (game->State >= BattleTech::State::END) )
				;
			else
			{
				if( mech->TookTurn || ((game->State > BattleTech::State::MOVEMENT) && ! mech->Destroyed()) )
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
				
				int num_targets = game->TargetIDs.size();
				if( num_targets > 1 )
				{
					for( int i = 0; i < num_targets; i ++ )
					{
						if( mech->ID == game->TargetIDs.at( i ) )
						{
							num_str = std::string("#") + Num::ToString(i+1);
							break;
						}
					}
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
			if( num_str.length() )
			{
				Pos3D num = pos;
				Pos3D num2 = num + shadow;
				TurnFont->DrawText3D( num_str, &num2, Font::ALIGN_MIDDLE_CENTER, 0.f,0.f,0.f,alpha*0.6f, 0.016 );
				TurnFont->DrawText3D( num_str, &num, Font::ALIGN_MIDDLE_CENTER, 1.0f,1.0f,1.0f,alpha*1.f, 0.016 );
			}
			
			if( (mech->HitClock.ElapsedSeconds() <= show_hit_time) && (mech->Lifetime.ElapsedSeconds() > show_hit_time) && ! game->GetRecordSheet(mech) )
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
	Hex *hex = game->Mouse.ShowCursor ? map->Nearest( mx, my ) : NULL;
	const Mech *moused = hex ? map->MechAt_const( hex->X, hex->Y ) : NULL;
	
	if( moused )
	{
		if( (moused->HitClock.ElapsedSeconds() > show_hit_time) && ! game->GetRecordSheet(moused) )
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
	float r = 1.f, g = 1.f, b = 1.f;
	if( (game->State >= Raptor::State::CONNECTED) && (game->Net.PingClock.ElapsedSeconds() > 5.) && ! Raptor::Server->IsRunning() )
	{
		status = "LOST NETWORK CONNECTION";
		g = b = fabsf(sinf( game->Net.PingClock.ElapsedSeconds() * M_PI ));
	}
	else if( (game->State != BattleTech::State::SETUP) && playing_events )
	{
		if( MessageOutput->DrawsType(TextConsole::MSG_NORMAL) )
			status = "";
		else for( std::deque<TextConsoleMessage*>::reverse_iterator msg = game->Msg.Messages.rbegin(); msg != game->Msg.Messages.rend(); msg ++ )
		{
			if( (*msg)->Type == TextConsole::MSG_NORMAL )
			{
				status = (*msg)->Text;
				break;
			}
		}
	}
	else if( Selected == MessageInput )
		status = "Press Enter to send chat message or Esc to cancel.";
	else if( (game->State >= BattleTech::State::MOVEMENT) && game->TeamTurn && game->BotControlsTeam(game->TeamTurn) )
	{
		status = game->TeamName(game->TeamTurn) + std::string(" is thinking.");
		r = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
	}
	else if( (game->State == BattleTech::State::MOVEMENT) && selected && selected->Ready() && (selected->Team == game->TeamTurn) && (game->TeamTurn == my_team) )
	{
		uint8_t move_mode = selected->MoveSpeed ? selected->MoveSpeed : selected->SpeedNeeded(true);
		uint8_t step_cost = selected->StepCost();
		if( (move_mode == BattleTech::Speed::JUMP) || (step_cost >= 99) )
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
		{
			status += " can move ";
			b = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
		}
		
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
		{
			status = selected->ShortName() + std::string(" cannot move this turn.  Press Enter to skip.");
			b = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
		}
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
			{
				status += std::string("  Press Enter to skip.");
				b = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
			}
		}
		else if( target && (Paths[0].LineOfSight || (target->Spotted < 99)) && (WeaponsInRange.size() || (game->State == BattleTech::State::PHYSICAL_ATTACK)) ) // FIXME: Path(s)
			status = std::string("Press Enter to submit ") + game->PhaseName() + std::string(" target: ") + target->ShortName();
		else if( Paths[0].size() ) // FIXME: Path(s)
			status = std::string("Press Enter to skip ") + game->PhaseName() + std::string(" (no target).");
		else
		{
			status = std::string("Right-click target for ") + game->PhaseName() + std::string(", or press Enter to skip.");
			b = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
		}
	}
	else if( (game->State == BattleTech::State::SETUP) || ! playing_events )
	{
		bool hotseat = game->Hotseat();
		bool ai = game->Data.PropertyAsInt("ai_team");
		status = game->PhaseName();
		if( game->TeamTurn && my_team )
		{
			if( (game->TeamTurn == my_team) && ! hotseat )
			{
				status += " (your team)";
				b = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
			}
			else if( (game->TeamsAlive() <= 2) && ! hotseat )
				status += " (enemy team)";
			else
			{
				std::string team_name = game->TeamName( game->TeamTurn );
				if( ! team_name.empty() )
					status += std::string(" (") + team_name + std::string(")");
				if( hotseat )
					b = fabsf(sinf( game->EventClock.ElapsedSeconds() * M_PI ));
			}
		}
		else if( game->State == BattleTech::State::SETUP )
		{
			bool ready_to_begin = game->ReadyToBegin();
			bool admin = game->Admin();
			if( selected && selected->Steps.size() )
				status = "Press Enter to submit new drop position.";
			else if( ready_to_begin && ! admin )
				status = "Waiting for host to initiate combat.";
			else if( game->Layers.Find("GameMenu") )
			{
				if( ready_to_begin && admin )
					status = "When everyone is ready, click Initiate Combat.";
				else
					status = "Set game options, then click Team/Mech to continue.";
			}
			else if( game->Layers.Find("SpawnMenu") )
			{
				if( ready_to_begin )
				{
					if( ai && game->BotControlsTeam(my_team) && (game->Data.PropertyAsInts("ai_team").size() == 1) )
						status = "Change to a non-AI team when ready to play.";
					else if( game->Data.PropertyAsInt("mech_limit") == 1 )
						status = "Right-click to move drop point, or press F10 when done.";
					else
						status = "Right-click to drop more Mechs, or press F10 when done.";
				}
				else if( ai && (! ready_to_begin) && game->MyMech() )
				{
					if( game->Data.PropertyAsInts("ai_team").size() > 1 )
						status = "Change to another AI team to drop Mechs for them.";
					else if( game->BotControlsTeam(my_team) )
						status = "Change to a non-AI team to drop your Mechs.";
					else
						status = "Change to the AI team to drop Mechs for them.";
				}
				else if( my_team )
					status = "Select a BattleMech, then right-click to drop.";
				else
					status = "Select a team and BattleMech, then right-click to drop.";
			}
			else if( ready_to_begin && admin )
			{
				if( game->BotControlsTeam(my_team) && (game->Data.PropertyAsInts("ai_team").size() == 1) )
					status = "Press Tab to switch back to your own team.";
				else
					status = "When everyone is ready to play, press F10.";
			}
			else if( (hotseat || ai) && game->MyMech() )
				status = "Press Tab to drop Mechs for the other team.  When done, press F10.";
			else if( game->MyMech() )
				status = "Use arrows to move drop point and Enter to submit.";
			else if( my_team )
				status = "Right-click to drop.  Press Tab to change team/Mech.";
			else
				status = "Press Tab to select a team and BattleMech.";
		}
	}
	TurnFont->DrawText( status, Rect.w/2 + 2, Rect.h,     Font::ALIGN_BOTTOM_CENTER, 0,0,0,0.8f );
	TurnFont->DrawText( status, Rect.w/2,     Rect.h - 2, Font::ALIGN_BOTTOM_CENTER, r,g,b,1.0f );
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
	
	bool playing_events = game->PlayingEvents();
	
	bool shift = game->Keys.KeyDown(SDLK_LSHIFT) || game->Keys.KeyDown(SDLK_RSHIFT);
	
	Draggable = false;
	
	if( (button == SDL_BUTTON_LEFT) && ! shift )
	{
		Mech *clicked = hex ? map->MechAt( hex->X, hex->Y ) : NULL;
		
		Draggable = (! clicked) && game->Cfg.SettingAsBool("map_drag");
		
		if( clicked && selected && (clicked != selected) && ! selected->DeclaredTargets.size() && (std::find( game->TargetIDs.begin(), game->TargetIDs.end(), clicked->ID ) != game->TargetIDs.end()) )
		{
			uint32_t prev_primary = game->TargetID;
			game->SetPrimaryTarget( clicked->ID );
			target = game->TargetMech();
			UpdateWeaponsInRange( selected, target );
			
			if( prev_primary != clicked->ID )
				game->Snd.Play( game->Res.GetSound( (game->TargetID == clicked->ID) ? "i_select.wav" : "i_error.wav" ) );
		}
		else if( clicked != selected )
		{
			game->SelectMech( clicked );
			selected = clicked;
			target = NULL;
			
			if( selected )
				game->Snd.Play( game->Res.GetSound("i_select.wav") );
			
			if( selected && selected->DeclaredTargets.size() )
			{
				ClearPath( false );
				Paths.clear();
				game->TargetID = selected->DeclaredTarget;  // Primary target.
				// NOTE: This does not retain sorting of secondary targets, but that has no game effect.
				for( std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->DeclaredTargets.begin(); target_iter != selected->DeclaredTargets.end(); target_iter ++ )
				{
					if( std::find( game->TargetIDs.begin(), game->TargetIDs.end(), target_iter->second ) == game->TargetIDs.end() )
					{
						game->TargetIDs.push_back( target_iter->second );
						target = (Mech*) game->Data.GetObject( target_iter->second );
						if( target )
						{
							Paths.push_back( map->Path( selected->X, selected->Y, target->X, target->Y ) );
							// FIXME: Paths.back().UpdateWeaponsInRange( selected, target );  // Path(s) mess?
						}
					}
				}
				target = game->TargetMech();
				if( ! Paths.size() )
					Paths.push_back( map->Path( selected->X, selected->Y, target->X, target->Y ) );
				UpdateWeaponsInRange( selected, target );
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
				game->Snd.Play( game->Res.GetSound("i_error.wav") );
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
			int8_t piloting = game->Cfg.SettingAsInt( "piloting", mech->second.Clan ? 4 : 5 );
			int8_t gunnery  = game->Cfg.SettingAsInt( "gunnery",  mech->second.Clan ? 3 : 4 );
			spawn_mech.AddChar( piloting );
			spawn_mech.AddChar( gunnery );
			game->Net.Send( &spawn_mech );
			
			GameMenu *gm = (GameMenu*) game->Layers.Find("GameMenu");
			if( gm )
				gm->Refresh( 0.3 );
			
			SpawnMenu *sm = (SpawnMenu*) game->Layers.Find("SpawnMenu");
			if( sm && sm->SearchBox && sm->SearchBox->Container )
				sm->SearchBox->Container->Selected = NULL;
		}
		// Prevent trying other targets when a Mech has already declared its shot.
		else if( (game->State >= BattleTech::State::TAG) && selected && selected->TookTurn )
			;
		else if( selected )
		{
			uint8_t x = 0, y = 0, facing = BattleTech::Dir::UP;
			selected->GetPosition( &x, &y, &facing );
			bool added_path = false;
			if( hex )
			{
				// FIXME: Lots of Path(s) mess here.
				if( (! shift) || (game->State == BattleTech::State::TAG) || ! (Paths.size() && Paths[ 0 ].size()) )  // FIXME: What if a Mech has multiple TAGs?
				{
					ClearPath( false );
					Paths[ 0 ] = map->Path( x, y, hex->X, hex->Y );
					added_path = true;
					target = map->MechAt( hex->X, hex->Y );
					game->TargetIDs.clear();
					if( target )
						game->TargetIDs.push_back( target->ID );
					selected->WeaponTargets.clear();
				}
				else
				{
					if( ! RemovePathTo( hex->X, hex->Y ) )
					{
						Paths.push_back( map->Path( x, y, hex->X, hex->Y ) );
						added_path = true;
					}
					
					target = NULL;
					game->TargetIDs.clear();
					for( size_t i = 0; i < Paths.size(); i ++ )
					{
						if( Paths[ i ].size() )
						{
							const Hex *target_hex = *(Paths[ i ].rbegin());
							Mech *target_mech = map->MechAt( target_hex->X, target_hex->Y );
							if( target_mech && ( (! selected->Team) || (selected->Team != target_mech->Team) || game->FF() ) )
							{
								if( ! target )
									target = target_mech;
								game->TargetIDs.push_back( target_mech->ID );
							}
						}
					}
				}
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
				if( std::find( game->TargetIDs.begin(), game->TargetIDs.end(), target->ID ) == game->TargetIDs.end() )
				{
					if( ! shift )
						game->TargetIDs.clear();
					game->TargetIDs.push_back( target->ID );
				}
				
				game->SetPrimaryTarget();
				target = game->TargetMech();
				
				for( std::map<uint8_t,uint32_t>::iterator target_iter = selected->WeaponTargets.begin(); target_iter != selected->WeaponTargets.end(); )
				{
					std::map<uint8_t,uint32_t>::iterator target_next = target_iter;
					target_next ++;
					
					// Clear any weapon targets that are no longer in our set of targets.
					if( std::find( game->TargetIDs.begin(), game->TargetIDs.end(), target_iter->second ) == game->TargetIDs.end() )
						selected->WeaponTargets.erase( target_iter );
					
					target_iter = target_next;
				}
				
				// Automatic arm flipping when only one target.
				if( (game->State == BattleTech::State::WEAPON_ATTACK) && (game->TargetIDs.size() < 2) && selected->ArmsCanFlip() )
				{
					selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
					selected->ArmsFlipped = target && (! target->TorsoTwist) && (selected->FiringArc( target->X, target->Y ) == BattleTech::Arc::REAR);
				}
				
				UpdateWeaponsInRange( selected, target );
				
				if( selected->ReadyAndAble()
				&& ((game->State == BattleTech::State::WEAPON_ATTACK)
				 || (game->State == BattleTech::State::PHYSICAL_ATTACK)
				 || (game->State == BattleTech::State::TAG)) )
					game->Snd.Play( game->Res.GetSound("i_select.wav") );
			}
			else
			{
				game->TargetIDs.clear();
				selected->WeaponTargets.clear();
				if( selected->ArmsFlipped && (game->State == BattleTech::State::WEAPON_ATTACK) )
				{
					selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
					selected->ArmsFlipped = false;
				}
				RemoveWeaponMenu();
			}
			
			if( added_path && game->Cfg.SettingAsBool("debug") )
			{
				bool ecm = Paths.back().ECMvsTeam( selected->Team ) || selected->ActiveStealth;
				std::string msg = "Dist from ";
				msg += Num::ToString((*(Paths.back().begin()))->X) + std::string(" ") + Num::ToString((*(Paths.back().begin()))->Y);
				msg += std::string(" to ");
				msg += Num::ToString((*(Paths.back().rbegin()))->X) + std::string(" ") + Num::ToString((*(Paths.back().rbegin()))->Y);
				if( target )
					msg += std::string(" (") + target->ShortName() + std::string(")");
				msg += std::string(" is ") + Num::ToString(Paths.back().Distance) + std::string(" hexes");
				if( ! Paths.back().LineOfSight )
				{
					msg += std::string(", NO line of sight.");
					if( target && (target->Spotted < 99) )
						msg += std::string("  Target spotted for indirect fire (+") + Num::ToString((int)( target->Spotted )) + std::string(").");
					else if( target && target->Narced() && ! ecm )
						msg += std::string("  Narc beacon allows indirect fire (+1).");
				}
				else if( Paths.back().Modifier )
					msg += std::string(", modifier +") + Num::ToString(Paths.back().Modifier) + std::string(".");
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
	bool playing_events = game->PlayingEvents();
	
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
			Player *player = game->Data.GetPlayer( game->PlayerID );
			if( player && ! msg.empty() )
			{
				Packet message = Packet( Raptor::Packet::MESSAGE );
				message.AddString( player->Name + std::string(": ") + msg );
				message.AddUInt( TextConsole::MSG_CHAT );
				game->Net.Send( &message );
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
		
		MessageInput->Rect.y = 3;
		MessageInput->UpdateCalcRects();
		int min_y = MessageInput->Rect.h + MessageInput->CalcRect.y * 2;
		for( std::list<Layer*>::iterator layer = game->Layers.Layers.begin(); layer != game->Layers.Layers.end(); layer ++ )
		{
			if( (*layer != this) && ((*layer)->Rect.y < min_y) )
				(*layer)->Rect.y = min_y;
		}
	}
	else if( (key == SDLK_i) || (key == SDLK_F9) )
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
		bool playing_events = game->PlayingEvents();
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
		else if( (game->Data.GameTime.ElapsedSeconds() > 0.1) && game->Cfg.SettingAsBool("screensaver") )
		{
			// Smooth map auto-scrolling.
			// FIXME: This shouldn't be in KeyDown; it should be a held input.  (That's why I'm only doing this in screensaver.)
			Vec3D center = (tl + br) / 2.;
			game->X = (game->X + center.X) / 2.;
			game->Y = (game->Y + center.Y) / 2.;
			double zoom_w = game->Gfx.W / std::max<double>( 4., br.X - tl.X );
			double zoom_h = game->Gfx.H / std::max<double>( 4., br.Y - tl.Y );
			double zoom = (zoom_w < zoom_h) ? zoom_w : zoom_h;
			game->Zoom = (game->Zoom + zoom) / 2.;
		}
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
	
	else if( (key >= SDLK_F1) && (key <= SDLK_F8) && ((game->State == BattleTech::State::SETUP) || ! game->MyTeam()) )
	{
#ifdef WIN32
		if( (key == SDLK_F4) && (game->Keys.KeyDown(SDLK_LALT) || game->Keys.KeyDown(SDLK_RALT)) )
			return false;
#endif
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
	else if( (key == SDLK_ESCAPE) && selected && ! playing_events )
	{
		game->DeselectMech();
		selected = NULL;
		ClearPath();
	}
	else if( (key == SDLK_ESCAPE) || (key == SDLK_F10) )
	{
		RemoveWeaponMenu();
		game->Layers.Add( new GameMenu() );
	}
	else if( (key == SDLK_TAB) || (key == SDLK_F11) )
	{
		RemoveWeaponMenu();
		game->Layers.Add( new SpawnMenu() );
	}
	else if( ((key == SDLK_RETURN) || (key == SDLK_KP_ENTER))
	&&       ((game->State == BattleTech::State::SETUP) || (game->State == BattleTech::State::GAME_OVER))
	&&       (! selected || ! selected->Steps.size()) )
	{
		RemoveWeaponMenu();
		if( ! game->Layers.Find("GameMenu") )
			game->Layers.Add( new GameMenu() );
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
			game->Snd.Play( game->Res.GetSound("i_error.wav") );
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
			game->TargetIDs.clear();
			ClearPath();
			game->EventClock.Reset( 0. );
			
			// Claim this Mech so it will automatically select on future turns.
			selected->PlayerID = game->PlayerID;
		}
		else if( (game->State == BattleTech::State::WEAPON_ATTACK)
		||       (game->State == BattleTech::State::TAG) )
		{
			// FIXME: Replace most of this section with a call to CreatePacket?
			
			game->Snd.Play( game->Res.GetSound("i_target.wav") );
			
			game->SetPrimaryTarget();
			target = game->TargetMech();
			
			uint8_t firing_arc = target ? selected->FiringArc( target->X, target->Y ) : (uint8_t) BattleTech::Arc::STRUCTURE;
			
			std::map<uint8_t,int8_t> weapons;
			bool left_arm_firing = false, right_arm_firing = false;
			if( target )
			{
				for( std::map<uint8_t,int8_t>::const_iterator weap = WeaponsInRange.begin(); weap != WeaponsInRange.end(); weap ++ )
				{
					if( selected->WeaponsToFire[ weap->first ] )
					{
						weapons[ weap->first ] = weap->second;
						
						const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
						if( eq->Location->Loc == BattleTech::Loc::LEFT_ARM )
							left_arm_firing = true;
						else if( eq->Location->Loc == BattleTech::Loc::RIGHT_ARM )
							right_arm_firing = true;
					}
				}
			}
			
			selected->Animate( 0.2, BattleTech::Effect::TORSO_TWIST );
			
			if( (game->TargetIDs.size() < 2) && selected->ArmsCanFlip() )
				selected->ArmsFlipped = (firing_arc == BattleTech::Arc::REAR) && (left_arm_firing || right_arm_firing) && ! selected->TorsoTwist;
			
			// FIXME: Move TurnedArm to per shot/target?
			selected->TurnedArm = BattleTech::Loc::UNKNOWN;
			if(      left_arm_firing  && (! selected->ArmsFlipped) && (firing_arc == BattleTech::Arc::LEFT_SIDE)  && selected->Locations[ BattleTech::Loc::LEFT_ARM ].Structure )
				selected->TurnedArm = BattleTech::Loc::LEFT_ARM;
			else if( right_arm_firing && (! selected->ArmsFlipped) && (firing_arc == BattleTech::Arc::RIGHT_SIDE) && selected->Locations[ BattleTech::Loc::RIGHT_ARM ].Structure )
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
			shots.AddChar( selected->TurnedArm );  // FIXME: Move TurnedArm to per shot/target?
			shots.AddChar( selected->ProneFire );
			
			uint8_t count_and_flags = weapons.size();
			if( selected->ArmsFlipped )
				count_and_flags |= 0x80;
			shots.AddUChar( count_and_flags );
			
			// Sort by target (primary, secondary, etc).
			std::vector< std::pair<uint8_t,int8_t> > sorted_weap;
			for( size_t i = 0; i < game->TargetIDs.size(); i ++ )
			{
				for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
				{
					const Mech *weap_target = target;
					std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->WeaponTargets.find( weap->first );
					if( target_iter != selected->WeaponTargets.end() )
						weap_target = game->GetMech( target_iter->second );
					
					if( ! weap_target )
						continue;
					if( (weap->first != 0xFF) && (game->State == BattleTech::State::WEAPON_ATTACK) )
					{
						// Keep arm-flipping honest by verifying weapon arcs again.
						if( weap->first >= selected->Equipment.size() )
							continue;
						if( ! selected->Equipment[ weap->first ].WithinFiringArc( weap_target->X, weap_target->Y, true ) )
							continue;
					}
					
					if( weap_target->ID == game->TargetIDs[ i ] )
						sorted_weap.push_back( std::pair<uint8_t,int8_t>( weap->first, weap->second ) );
				}
			}
			
			for( size_t i = 0; i < sorted_weap.size(); i ++ )
			{
				std::pair<uint8_t,int8_t> *weap = &(sorted_weap[ i ]);
				const Mech *weap_target = target;
				ShotPath path = Paths[0];
				std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->WeaponTargets.find( weap->first );
				if( target_iter != selected->WeaponTargets.end() )
				{
					weap_target = game->GetMech( target_iter->second );
					if( weap_target && (std::find( game->TargetIDs.begin(), game->TargetIDs.end(), weap_target->ID ) != game->TargetIDs.end()) )
						path = PathToTarget( weap_target->ID );
					else
						weap_target = target;
				}
				
				shots.AddUInt( weap_target->ID );
				
				MechEquipment *eq = &(selected->Equipment[ weap->first ]);
				uint8_t arc_and_flags = weap_target->DamageArc( path.DamageFromX, path.DamageFromY );
				if( ! path.LineOfSight )
					arc_and_flags |= 0x80; // Indirect Fire
				else if( path.PartialCover )
					arc_and_flags |= 0x40; // Leg Partial Cover
				if( path.ECMvsTeam( selected->Team ) || selected->ActiveStealth )  // When the stealth armor system is engaged, the Mech suffers effects as if in the radius of an enemy ECM suite. [BattleMech Manual p.114]
					arc_and_flags |= 0x20; // Within Friendly ECM
				if( weap_target->Narced() && (path.LineOfSight || (weap_target->Spotted < 99)) )
					arc_and_flags |= 0x10; // NARC Cluster Bonus (with exceptions in MechEquipment::ClusterHits)
				if( eq->WeaponFCS && ! eq->WeaponFCS->Damaged )
					arc_and_flags |= 0x08; // Artemis IV FCS
				
				shots.AddUChar( arc_and_flags );
				shots.AddUChar( weap->first );
				shots.AddUChar( selected->WeaponsToFire[ weap->first ] );
				shots.AddChar( weap->second );
			}
			
			const Mech *spot_target = target;
			ShotPath spot_path = Paths[0];
			std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->WeaponTargets.find( 0xFF );
			if( target_iter != selected->WeaponTargets.end() )
			{
				spot_target = game->GetMech( target_iter->second );
				if( spot_target && (std::find( game->TargetIDs.begin(), game->TargetIDs.end(), spot_target->ID ) != game->TargetIDs.end()) )
					spot_path = PathToTarget( spot_target->ID );
				else
					spot_target = target;
			}
			int8_t spotted = selected->SpottingModifier( spot_target, &spot_path );
			if( (spotted < 99) && spot_path.LineOfSight && (game->State == BattleTech::State::WEAPON_ATTACK) && selected->SpottingWithoutTAG() )
			{
				// Spot for Indirect Fire
				shots.AddUInt( spot_target->ID );
				shots.AddChar( spotted );
			}
			else
			{
				shots.AddUInt( 0 );
				shots.AddChar( 99 );
			}
			
			game->Net.Send( &shots );
			
			game->TargetID = 0;
			game->TargetIDs.clear();
			ClearPath();
			game->EventClock.Reset( 0. );
			
			// Claim this Mech so it will automatically select on future turns.
			selected->PlayerID = game->PlayerID;
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
			game->TargetIDs.clear();
			ClearPath();
			game->EventClock.Reset( 0. );
			
			// Claim this Mech so it will automatically select on future turns.
			selected->PlayerID = game->PlayerID;
		}
	}
	else if( (key == SDLK_LEFT) && (game->State == BattleTech::State::WEAPON_ATTACK) )
	{
		if( (selected->TorsoTwist > -1) && ! selected->Prone && selected->ReadyAndAble() )
		{
			game->Snd.Play( game->Res.GetSound("m_twist.wav") );
			selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
			selected->TorsoTwist --;
			if( selected->TorsoTwist )
				selected->ArmsFlipped = false;
			else if( (game->TargetIDs.size() < 2) && selected->ArmsCanFlip() )
				selected->ArmsFlipped = target && (selected->FiringArc( target->X, target->Y ) == BattleTech::Arc::REAR);
			if( target && Paths[0].size() ) // FIXME: Path(s)
				UpdateWeaponsInRange( selected, target );
			game->SetPrimaryTarget();
		}
	}
	else if( (key == SDLK_RIGHT) && (game->State == BattleTech::State::WEAPON_ATTACK) )
	{
		if( (selected->TorsoTwist < 1) && ! selected->Prone && selected->ReadyAndAble() )
		{
			game->Snd.Play( game->Res.GetSound("m_twist.wav") );
			selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
			selected->TorsoTwist ++;
			if( selected->TorsoTwist )
				selected->ArmsFlipped = false;
			else if( (game->TargetIDs.size() < 2) && selected->ArmsCanFlip() )
				selected->ArmsFlipped = target && (selected->FiringArc( target->X, target->Y ) == BattleTech::Arc::REAR);
			if( target && Paths[0].size() ) // FIXME: Path(s)
				UpdateWeaponsInRange( selected, target );
			game->SetPrimaryTarget();
		}
	}
	else if( (key == SDLK_BACKSPACE) && (game->State == BattleTech::State::WEAPON_ATTACK) && selected->TorsoTwist && selected->ReadyAndAble() )
	{
		game->Snd.Play( game->Res.GetSound("m_twist.wav") );
		selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
		selected->TorsoTwist = 0;
		if( (game->TargetIDs.size() < 2) && selected->ArmsCanFlip() )
			selected->ArmsFlipped = target && (selected->FiringArc( target->X, target->Y ) == BattleTech::Arc::REAR);
		if( target && Paths[0].size() ) // FIXME: Path(s)
			UpdateWeaponsInRange( selected, target );
		game->SetPrimaryTarget();
	}
	else if( ((key == SDLK_DELETE) || ((key == SDLK_BACKSPACE) && shift)) && ! enemy_turn )
	{
		if( game->State != BattleTech::State::MOVEMENT )
			game->Snd.Play( game->Res.GetSound( (game->State == BattleTech::State::GAME_OVER) || selected->Destroyed() ? "i_blip.wav" : "b_eject.wav" ) );
		
		Packet remove_mech( BattleTech::Packet::REMOVE_MECH );
		remove_mech.AddUInt( selected->ID );
		game->Net.Send( &remove_mech );
		
		game->DeselectMech();
		selected = NULL;
		ClearPath();
		
		if( game->State == BattleTech::State::SETUP )
		{
			GameMenu *gm = (GameMenu*) game->Layers.Find("GameMenu");
			if( gm )
				gm->Refresh( 0.3 );
		}
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
		
		// Claim this Mech so it will automatically select on future turns.
		selected->PlayerID = game->PlayerID;
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
	// FIXME: Path(s) minor mess here.
	if( Paths.size() == 1 )
		Paths[ 0 ].Clear();
	else
	{
		Paths.clear(); // FIXME: After every blind Paths[0] access is removed, this is the only line we need to keep!
		Paths.push_back( ShotPath() );
	}
	WeaponsInRange.clear();
	PossibleMelee.clear();
	if( remove_menu )
		RemoveWeaponMenu();
}


bool HexBoard::RemovePathTo( uint8_t x, uint8_t y )
{
	for( size_t i = 0; i < Paths.size(); i ++ )
	{
		if( Paths[ i ].size() )
		{
			const Hex *target_hex = *(Paths[ i ].rbegin());
			if( (x == target_hex->X) && (y == target_hex->Y) )
			{
				if( Paths.size() == 1 )
					ClearPath();
				else
					Paths.erase( Paths.begin() + i );
				return true;
			}
		}
	}
	
	return false;
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
		target->GetPosition(   &x2, &y2 ); // FIXME: Path(s) old code?
		const HexMap *map = game->Map();
		for( size_t i = 0; i < Paths.size(); i ++ )
		{
			bool repath = true;
			if( Paths[ i ].size() )
			{
				const Hex *hex1 = *(Paths[ i ].begin());
				const Hex *hex2 = *(Paths[ i ].rbegin());
				x2 = hex2->X;
				y2 = hex2->Y;
				repath = (x1 != hex1->X) || (y1 != hex1->Y) || (x2 != hex2->X) || (y2 != hex2->Y);
			}
			if( repath )
				Paths[ i ] = map->Path( x1, y1, x2, y2 );
		}
		UpdateWeaponsInRange( selected, target );
	}
	else
		ClearPath();
}


void HexBoard::UpdateWeaponsInRange( Mech *selected, const Mech *target, bool show_menu )
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
	if( wm )
	{
		wm->Update();
		wm->MoveToTop();
	}
	else if( show_menu )
		Raptor::Game->Layers.Add( new WeaponMenu() );
}


// -----------------------------------------------------------------------------


ShotPath HexBoardAim::PathTo( uint8_t x, uint8_t y ) const
{
	for( size_t i = 0; i < Paths.size(); i ++ )
	{
		if( Paths[ i ].size() )
		{
			const Hex *hex = *(Paths[ i ].rbegin());
			if( (hex->X == x) && (hex->Y == y) )
				return Paths[ i ];
		}
	}
	
	return ShotPath();
}


ShotPath HexBoardAim::PathToTarget( uint32_t target_id ) const
{
	HexMap *map = ((BattleTechGame*)( Raptor::Game ))->Map();
	
	for( size_t i = 0; i < Paths.size(); i ++ )
	{
		if( Paths[ i ].size() )
		{
			const Hex *hex = *(Paths[ i ].rbegin());
			const Mech *mech = map->MechAt_const( hex->X, hex->Y );
			uint32_t mech_id = mech ? mech->ID : 0;
			if( mech_id == target_id )
				return Paths[ i ];
		}
	}
	
	return ShotPath();
}


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
			const Mech *weap_target = target;
			ShotPath path = Paths[0];
			std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->WeaponTargets.find( index );
			if( target_iter != selected->WeaponTargets.end() )
			{
				weap_target = ((BattleTechGame*)( Raptor::Game ))->GetMech( target_iter->second );
				if( weap_target )
					path = PathToTarget( weap_target->ID );
				else
					weap_target = target;
			}
			
			if( eq->Weapon && eq->Weapon->TAG && (state == BattleTech::State::WEAPON_ATTACK) )
				continue;
			if( eq->Weapon && (! eq->Weapon->TAG) && (state == BattleTech::State::TAG) )
				continue;
			
			int8_t difficulty = selected->WeaponRollNeeded( weap_target, &path, eq ) + modifier;
			
			if( difficulty <= 12 )
				WeaponsInRange[ index ] = difficulty;
		}
	}
	
	if( (state == BattleTech::State::PHYSICAL_ATTACK) || (state == BattleTech::State::MOVEMENT) )
	{
		modifier += target->Defense + Paths[0].Modifier; // FIXME: Path(s)
		
		if( target->Prone )
			modifier += (Paths[0].Distance <= 1) ? -2 : 1; // FIXME: Path(s)
		
		if( target->Immobile() )
			modifier -= 4;
		
		bool ecm = Paths[0].ECMvsTeam( selected->Team ); // FIXME: Path(s)
		
		int8_t trees = Paths[0].PartialCover ? (Paths[0].Modifier - 1) : Paths[0].Modifier; // FIXME: Path(s)
		if( trees && (! ecm) && (Paths[0].Distance <= selected->ActiveProbeRange()) ) // FIXME: Path(s)
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
		
		// FIXME: Primary target must be in forward arc if any target is there. [BattleMech Manual p.109]
		
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
		else if( weapons.size() && (firing_arc == BattleTech::Arc::LEFT_SIDE) && selected->Locations[ BattleTech::Loc::LEFT_ARM ].Structure ) // FIXME: Make sure an arm weapon is firing?
			turned_arm = BattleTech::Loc::LEFT_ARM;
		else if( weapons.size() && (firing_arc == BattleTech::Arc::RIGHT_SIDE) && selected->Locations[ BattleTech::Loc::RIGHT_ARM ].Structure ) // FIXME: Make sure an arm weapon is firing?
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
		p->AddChar( turned_arm );  // FIXME: Move this to per shot?
		p->AddChar( prone_fire );
		
		uint8_t count_and_flags = weapons.size();
		if( arms_flipped )
			count_and_flags |= 0x80;
		p->AddUChar( count_and_flags );
		
		// FIXME: Sort by target (primary, secondary, etc).
		
		for( std::map<uint8_t,int8_t>::const_iterator weap = weapons.begin(); weap != weapons.end(); weap ++ )
		{
			const Mech *weap_target = target;
			ShotPath path = Paths[0];
			std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->WeaponTargets.find( weap->first );
			if( target_iter != selected->WeaponTargets.end() )
			{
				weap_target = (const Mech*) selected->Data->GetObject( target_iter->second );
				if( weap_target )
					path = PathToTarget( weap_target->ID );
				else
					weap_target = target;
			}
			
			p->AddUInt( weap_target->ID );
			
			const MechEquipment *eq = &(selected->Equipment[ weap->first ]);
			uint8_t arc_and_flags = weap_target->DamageArc( path.DamageFromX, path.DamageFromY );
			if( ! path.LineOfSight )
				arc_and_flags |= 0x80; // Indirect Fire
			else if( path.PartialCover )
				arc_and_flags |= 0x40; // Leg Partial Cover
			if( path.ECMvsTeam( selected->Team ) || selected->ActiveStealth )  // When the stealth armor system is engaged, the Mech suffers effects as if in the radius of an enemy ECM suite. [BattleMech Manual p.114]
				arc_and_flags |= 0x20; // Within Friendly ECM
			if( weap_target->Narced() && (path.LineOfSight || (weap_target->Spotted < 99)) )
				arc_and_flags |= 0x10; // NARC Cluster Bonus (with exceptions in MechEquipment::ClusterHits)
			if( eq->WeaponFCS && ! eq->WeaponFCS->Damaged )
				arc_and_flags |= 0x08; // Artemis IV FCS
			
			std::map<uint8_t,uint8_t>::const_iterator fire = selected->WeaponsToFire.find( weap->first );
			uint8_t count = (fire != selected->WeaponsToFire.end()) ? fire->second : 0;
			
			p->AddUChar( arc_and_flags );
			p->AddUChar( weap->first );
			p->AddUChar( count );
			p->AddChar( weap->second );
		}
		
		const Mech *spot_target = target;
		ShotPath spot_path = Paths[0];
		std::map<uint8_t,uint32_t>::const_iterator target_iter = selected->WeaponTargets.find( 0xFF );
		if( target_iter != selected->WeaponTargets.end() )
		{
			spot_target = (const Mech*) selected->Data->GetObject( target_iter->second );
			if( spot_target )
				spot_path = PathToTarget( spot_target->ID );
			else
				spot_target = target;
		}
		int8_t spotted = selected->SpottingModifier( spot_target, &spot_path );
		if( (spotted < 99) && spot_path.LineOfSight && (state == BattleTech::State::WEAPON_ATTACK) && selected->SpottingWithoutTAG() )
		{
			// Spot for Indirect Fire
			p->AddUInt( spot_target->ID );
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
