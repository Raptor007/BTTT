/*
 *  GameMenu.cpp
 */

#include "GameMenu.h"

#include "BattleTechGame.h"
#include "Num.h"
#include "HexBoard.h"
#include "SpawnMenu.h"


GameMenu::GameMenu( void )
{
	Name = "GameMenu";
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	Rect.w = 350;
	Rect.x = game->Gfx.W/2 - Rect.w/2;
	
	Red = 0.4f;
	Green = 0.4f;
	Blue = 0.4f;
	Alpha = 0.75f;
	
	ItemFont = game->Res.GetFont( "ProFont.ttf", 24 );
	
	SDL_Rect rect;
	
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new GameMenuCloseButton( &rect, game->Res.GetFont( "ProFont.ttf", 14 ) ) );
	
	rect.x = 20;
	rect.w = Rect.w - rect.x * 2;
	rect.y = 40;
	rect.h = ItemFont->GetAscent() + 2;
	if( Raptor::Server->IsRunning() )
	{
		AddElement( new GameMenuCheckBox( &rect, ItemFont, "hotseat", "Hotseat Mode" ) );
		rect.y += rect.h + 10;
		
		if( Raptor::Server->State == BattleTech::State::SETUP )
		{
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "mech_limit", "1 Mech Per Player", "1", "0" ) );
			rect.y += rect.h + 10;
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "engine_explosions", "Engine Explosions" ) );
			rect.y += rect.h + 10;
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "prone_1arm", "One-Armed Prone Fire" ) );
			rect.y += rect.h + 10;
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "enhanced_flamers", "Enhanced Flamers" ) );
			rect.y += rect.h + 10;
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "enhanced_ams", "Enhanced Missile Defense" ) );
			rect.y += rect.h + 10;
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "floating_crits", "Floating Criticals" ) );
			rect.y += rect.h + 10;
			AddElement( new GameMenuCheckBox( &rect, ItemFont, "skip_tag", "Skip TAG Phase" ) );
			rect.y += rect.h + 10;
			
			rect.h = ItemFont->GetAscent() + 6;
			AddElement( new GameMenuCommandButton( &rect, ItemFont, "map", "Randomize Map Terrain" ) );
			rect.y += rect.h + 10;
		}
		
		rect.y += 5;
	}
	
	rect.h = ItemFont->GetAscent() + 6;
	GameMenuDropDown *event_speed = new GameMenuDropDown( &rect, ItemFont, "event_speed" );
	event_speed->AddItem( "0.5", " Event Speed: Slow" );
	event_speed->AddItem( "0.7", " Event Speed: Medium" );
	event_speed->AddItem( "1",   " Event Speed: Fast" );
	event_speed->AddItem( "2",   " Event Speed: Ludicrous" );
	event_speed->Update();
	AddElement( event_speed );
	
	rect.y += rect.h + 10;
	GameMenuDropDown *s_volume = new GameMenuDropDown( &rect, ItemFont, "s_volume" );
	s_volume->AddItem( "0",   " Sound Volume: Mute" );
	s_volume->AddItem( "0.1", " Sound Volume: Lowest" );
	s_volume->AddItem( "0.2", " Sound Volume: Low" );
	s_volume->AddItem( "0.3", " Sound Volume: Medium" );
	s_volume->AddItem( "0.5", " Sound Volume: Loud" );
	s_volume->AddItem( "0.7", " Sound Volume: Louder" );
	s_volume->AddItem( "1",   " Sound Volume: Loudest" );
	s_volume->Update();
	s_volume->Sound = Raptor::Game->Res.GetSound("i_target.wav");
	AddElement( s_volume );
	
	rect.y += rect.h + 20;
	rect.h = ItemFont->GetAscent() + 10;
	rect.x = 10;
	rect.w = (Rect.w - rect.x * 3) / 2;
	AddElement( new GameMenuDisconnectButton( &rect, ItemFont ) );
	
	std::set<uint8_t> teams_with_mechs;
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = game->Data.GameObjects.begin(); obj_iter != game->Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			if( mech->Team && ! mech->Destroyed() )
				teams_with_mechs.insert( mech->Team );
		}
	}
	
	DefaultButton = NULL;
	rect.x += rect.x + rect.w;
	
	if( game->State <= BattleTech::State::SETUP )
	{
		DefaultButton = new GameMenuSpawnButton( &rect, ItemFont );
		AddElement( DefaultButton );
		
		if( teams_with_mechs.count( game->MyTeam() ) && (teams_with_mechs.size() >= 2) )
		{
			rect.x = 10;
			rect.w = Rect.w - rect.x * 2;
			rect.y += rect.h + 10;
			DefaultButton = new GameMenuCommandButton( &rect, ItemFont, "ready", "Initiate Combat" );
			AddElement( DefaultButton );
		}
	}
	else
	{
		DefaultButton = new GameMenuCloseButton( &rect, ItemFont );
		DefaultButton->LabelText = "OK";
		AddElement( DefaultButton );
	}
	
	DefaultButton->Blue = 0.5f;
	
	Rect.h = rect.y + rect.h + 10;
	Rect.y = game->Gfx.H/2 - Rect.h/2;
}


GameMenu::~GameMenu()
{
}


void GameMenu::Draw( void )
{
	Window::Draw();
	ItemFont->DrawText( "BTTT Game Settings", Rect.w/2 + 2, 7, Font::ALIGN_TOP_CENTER, 0,0,0,0.8f );
	ItemFont->DrawText( "BTTT Game Settings", Rect.w/2,     5, Font::ALIGN_TOP_CENTER );
}


bool GameMenu::KeyDown( SDLKey key )
{
	if( key == SDLK_ESCAPE )
		Remove();
	else if( key == SDLK_TAB )
	{
		Remove();
		Raptor::Game->Layers.Add( new SpawnMenu() );
	}
	else if( DefaultButton && ((key == SDLK_RETURN) || (key == SDLK_KP_ENTER)) )
		DefaultButton->Clicked();
	else
		return false;
	return true;
}


bool GameMenu::MouseDown( Uint8 button )
{
	return true;
}


// ---------------------------------------------------------------------------


GameMenuCloseButton::GameMenuCloseButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, "x", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


GameMenuCloseButton::~GameMenuCloseButton()
{
}


void GameMenuCloseButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
		Container->Remove();
}


// ---------------------------------------------------------------------------


GameMenuDisconnectButton::GameMenuDisconnectButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, (Raptor::Server->IsRunning() && (Raptor::Server->State > BattleTech::State::SETUP)) ? "End Game" : "Disconnect", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


GameMenuDisconnectButton::~GameMenuDisconnectButton()
{
}


void GameMenuDisconnectButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
	{
		if( Raptor::Server->IsRunning() && (Raptor::Server->State > BattleTech::State::SETUP) )
			Raptor::Server->ChangeState( BattleTech::State::SETUP );
		else
			Raptor::Game->Net.Disconnect();
	}
}


// ---------------------------------------------------------------------------


GameMenuSpawnButton::GameMenuSpawnButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, "Team/Mech", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


GameMenuSpawnButton::~GameMenuSpawnButton()
{
}


void GameMenuSpawnButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
	{
		Container->Remove();
		Raptor::Game->Layers.Add( new SpawnMenu() );
	}
}


// ---------------------------------------------------------------------------


GameMenuCommandButton::GameMenuCommandButton( SDL_Rect *rect, Font *button_font, std::string command, std::string label )
: LabelledButton( rect, button_font, label, Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Command = command;
	
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


GameMenuCommandButton::~GameMenuCommandButton()
{
}


void GameMenuCommandButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
	{
		Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
		
		Raptor::Game->HandleCommand( Command );
	}
}


// ---------------------------------------------------------------------------


GameMenuCheckBox::GameMenuCheckBox( SDL_Rect *rect, Font *font, std::string variable, std::string label, std::string true_str, std::string false_str )
: CheckBox( rect, font, std::string(" ") + label, false, NULL, NULL, NULL, NULL, NULL, NULL )
{
	Red = 1.f;
	Green = 1.f;
	Blue = 1.f;
	Alpha = 0.75f;
	
	Variable = variable;
	TrueStr = true_str;
	FalseStr = false_str;
	
	std::map<std::string,std::string>::const_iterator value = Raptor::Game->Data.Properties.find( variable );
	if( value != Raptor::Game->Data.Properties.end() )
		Checked = Str::AsBool( value->second );
}


GameMenuCheckBox::~GameMenuCheckBox()
{
}


void GameMenuCheckBox::Changed( void )
{
	Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
	
	Packet info = Packet( Raptor::Packet::INFO );
	info.AddUShort( 1 );
	info.AddString( Variable );
	info.AddString( Checked ? TrueStr : FalseStr );
	Raptor::Game->Net.Send( &info );
}


void GameMenuCheckBox::Draw( void )
{
	CheckBox::Draw();
	Raptor::Game->Gfx.DrawRect2D( 1, 1, Rect.h - 1, Rect.h - 1, 0, 0.f,0.f,0.f,1.f );
	if( Checked )
		LabelFont->DrawText( "X", Rect.h/2, Rect.h/2, Font::ALIGN_MIDDLE_CENTER );
}


// ---------------------------------------------------------------------------


GameMenuDropDown::GameMenuDropDown( SDL_Rect *rect, Font *font, std::string variable )
: DropDown( rect, font, Font::ALIGN_MIDDLE_LEFT, 0, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	Variable = variable;
	Value = Raptor::Game->Cfg.SettingAsString( Variable );
	
	Sound = Raptor::Game->Res.GetSound("i_select.wav");
}


GameMenuDropDown::~GameMenuDropDown()
{
}


void GameMenuDropDown::Changed( void )
{
	Raptor::Game->Cfg.Settings[ Variable ] = Value;
	
	if( Sound )
		Raptor::Game->Snd.Play( Sound );
}
