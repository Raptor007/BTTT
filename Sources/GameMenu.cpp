/*
 *  GameMenu.cpp
 */

#include "GameMenu.h"

#include <dirent.h>
#include <fstream>
#include "BattleTechGame.h"
#include "GroupBox.h"
#include "Num.h"
#include "HexBoard.h"
#include "SpawnMenu.h"
#ifndef DT_DIR
#include <sys/stat.h>
#endif


GameMenu::GameMenu( void )
{
	Name = "GameMenu";
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	Rect.w = 320;
	Rect.x = game->Gfx.W/2 - Rect.w/2;
	
	Red = 0.4f;
	Green = 0.4f;
	Blue = 0.4f;
	Alpha = 0.75f;
	
	ItemFont = game->Res.GetFont( "ProFont.ttf", 18 );
	Font *small_font = game->Res.GetFont( "ProFont.ttf", 14 );
	
	SDL_Rect rect, group_rect;
	GroupBox *group = NULL;
	
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new GameMenuCloseButton( &rect, small_font ) );
	
	group_rect.x = 10;
	group_rect.y = ItemFont->GetHeight() + 7;
	group_rect.w = Rect.w - group_rect.x * 2;
	group_rect.h = 0;
	
	#define SPACING 7
	
	Hotseat = NULL;
	AITeam = NULL;
	
	if( game->Admin() )
	{
		group = new GroupBox( &group_rect, "Game Options", small_font );
		AddElement( group );
		
		rect.x = 10;
		rect.y = 20;
		rect.w = group_rect.w - rect.x * 2;
		rect.h = ItemFont->GetAscent() + 2;
		
		Hotseat = new GameMenuSvCheckBox( &rect, ItemFont, "hotseat", "Hotseat Mode" );
		group->AddElement( Hotseat );
		rect.y += rect.h + SPACING;
		
		if( (Raptor::Game->State <= BattleTech::State::SETUP) || (Raptor::Game->State == BattleTech::State::GAME_OVER) )
		{
			group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "mech_limit", "Limit to 1 Mech Per Player", "1", "0" ) );
			rect.y += rect.h + SPACING;
			
			rect.h = ItemFont->GetAscent() + 6;
			GameMenuSvDropDown *teams = new GameMenuSvDropDown( &rect, ItemFont, "teams" );
			for( int i = 2; i <= 5; i ++ )
				teams->AddItem( Num::ToString(i), std::string(" Number of Teams: ") + Num::ToString(i) );
			std::string current_teams = game->Data.PropertyAsString("teams","2");
			if( teams->FindItem(current_teams) < 0 )
				teams->AddItem( current_teams, std::string(" Number of Teams: ") + current_teams );
			teams->Update();
			group->AddElement( teams );
			rect.y += rect.h + SPACING;
		}
		
		rect.h = ItemFont->GetAscent() + 6;
		AITeam = new GameMenuSvDropDown( &rect, ItemFont, "ai_team" );
		AITeam->AddItem( "0", " AI Team: None" );
		int teams = game->Data.PropertyAsInt("teams",2);
		for( int i = 1; i <= teams; i ++ )
			AITeam->AddItem( Num::ToString(i), std::string(" AI Team: ") + game->TeamName(i) );
		AITeam->AddItem( "1,2,3,4,5", " AI Team: All (No Humans)" );
		std::string current_ai = game->Data.PropertyAsString("ai_team","0");
		if( AITeam->FindItem(current_ai) < 0 )
			AITeam->AddItem( current_ai, std::string(" AI Team: ") + current_ai );
		AITeam->Update();
		group->AddElement( AITeam );
		rect.y += rect.h + SPACING;
		
		rect.h = ItemFont->GetAscent() + 2;
		group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "engine_explosions", "Engine Explosions" ) );
		rect.y += rect.h + SPACING;
		group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "prone_1arm", "One-Armed Prone Fire" ) );
		rect.y += rect.h + SPACING;
		group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "enhanced_flamers", "Enhanced Flamers" ) );
		rect.y += rect.h + SPACING;
		group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "enhanced_ams", "Enhanced Missile Defense" ) );
		rect.y += rect.h + SPACING;
		group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "floating_crits", "Floating Criticals" ) );
		rect.y += rect.h + SPACING;
		group->AddElement( new GameMenuSvCheckBox( &rect, ItemFont, "skip_tag", "Skip TAG Phase" ) );
		rect.y += rect.h + SPACING;
		
		if( Raptor::Game->State <= BattleTech::State::SETUP )
		{
			rect.h = ItemFont->GetAscent() + 6;
			group->AddElement( new GameMenuCommandButton( &rect, ItemFont, "map", "Randomize Map Terrain" ) );
			rect.y += rect.h + SPACING;
		}
		
		rect.h = ItemFont->GetAscent() + 6;
		GameMenuSvDropDown *biome = new GameMenuSvDropDown( &rect, ItemFont, "biome" );
		for( std::vector< std::pair<std::string,std::string> >::const_iterator b = game->Biomes.begin(); b != game->Biomes.end(); b ++ )
			biome->AddItem( b->second, std::string(" Biome: ") + b->first );
		if( ! game->Biomes.size() )
			biome->AddItem( "", " Biome: Default" );
		std::string current_biome = game->Data.PropertyAsString("biome");
		if( biome->FindItem(current_biome) < 0 )
			biome->AddItem( current_biome, current_biome.length() ? " Biome: Custom" : " Biome: Default" );
		biome->Update();
		group->AddElement( biome );
		rect.y += rect.h + SPACING;
		
		if( Raptor::Game->State <= BattleTech::State::SETUP )
		{
			group->AddElement( new GameMenuScenarioDropDown( &rect, ItemFont ) );
			rect.y += rect.h + SPACING;
		}
		
		group->SizeToElements();
		group->Rect.w = group_rect.w;
		group_rect.y += group->Rect.h + 10;
	}
	
	group = new GroupBox( &group_rect, "Local Settings", small_font );
	AddElement( group );
	
	rect.x = 10;
	rect.y = 20;
	rect.w = group_rect.w - rect.x * 2;
	
	rect.h = ItemFont->GetAscent() + 6;
	GameMenuDropDown *event_speed = new GameMenuDropDown( &rect, ItemFont, "event_speed" );
	event_speed->AddItem( "0.5", " Event Speed: Slow" );
	event_speed->AddItem( "0.7", " Event Speed: Medium" );
	event_speed->AddItem( "1",   " Event Speed: Fast" );
	event_speed->AddItem( "2",   " Event Speed: Ludicrous" );
	std::string current_speed = game->Cfg.SettingAsString("event_speed","1");
	if( event_speed->FindItem(current_speed) < 0 )
		event_speed->AddItem( current_speed, std::string(" Event Speed: ") + current_speed );
	event_speed->Update();
	group->AddElement( event_speed );
	rect.y += rect.h + SPACING;
	
	GameMenuDropDown *s_volume = new GameMenuDropDown( &rect, ItemFont, "s_volume" );
	s_volume->AddItem( "0",   " Sound Volume: Mute" );
	s_volume->AddItem( "0.1", " Sound Volume: Lowest" );
	s_volume->AddItem( "0.2", " Sound Volume: Low" );
	s_volume->AddItem( "0.3", " Sound Volume: Medium" );
	s_volume->AddItem( "0.5", " Sound Volume: Loud" );
	s_volume->AddItem( "0.7", " Sound Volume: Louder" );
	s_volume->AddItem( "1",   " Sound Volume: Loudest" );
	std::string current_volume = game->Cfg.SettingAsString("s_volume","1");
	if( s_volume->FindItem(current_volume) < 0 )
		s_volume->AddItem( current_volume, std::string(" Sound Volume: ") + current_volume );
	s_volume->Update();
	s_volume->Sound = Raptor::Game->Res.GetSound("i_target.wav");
	group->AddElement( s_volume );
	rect.y += rect.h + SPACING;
	
	rect.h = ItemFont->GetAscent() + 2;
	group->AddElement( new GameMenuCheckBox( &rect, ItemFont, "record_sheet_popup", "Show Record Sheet for Damage" ) );
	rect.y += rect.h + SPACING;
	group->AddElement( new GameMenuCheckBox( &rect, ItemFont, "show_ecm", "Show Relevant ECM Ranges" ) );
	rect.y += rect.h + SPACING;
	group->AddElement( new GameMenuCheckBox( &rect, ItemFont, "map_drag", "Left-Click to Drag Map" ) );
	rect.y += rect.h + SPACING;
	
	group->SizeToElements();
	group->Rect.w = group_rect.w;
	
	rect.y = group->Rect.y + group->Rect.h + 15;
	rect.h = ItemFont->GetAscent() + 10;
	rect.x = 10;
	
	DefaultButton = NULL;
	
	if( game->Admin() && (game->State == BattleTech::State::GAME_OVER) )
	{
		rect.w = Rect.w - rect.x * 2;
		
		DefaultButton = new GameMenuEndButton( &rect, ItemFont );
		AddElement( DefaultButton );
	}
	else
	{
		rect.w = (Rect.w - rect.x * 3) / 2;
		
		if( Raptor::Server->IsRunning() && (Raptor::Server->State > BattleTech::State::SETUP) )
			AddElement( new GameMenuEndButton( &rect, ItemFont ) );
		else
			AddElement( new GameMenuDisconnectButton( &rect, ItemFont ) );
		
		rect.x += rect.x + rect.w;
		
		if( game->State <= BattleTech::State::SETUP )
		{
			DefaultButton = new GameMenuSpawnButton( &rect, ItemFont );
			AddElement( DefaultButton );
			
			if( game->ReadyToBegin() && game->Admin() )
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
	}
	
	DefaultButton->Blue = 0.5f;
	
	Rect.h = rect.y + rect.h + 10;
	Rect.y = game->Gfx.H/2 - Rect.h/2;
	
	Draggable = true;
}


GameMenu::~GameMenu()
{
}


void GameMenu::Draw( void )
{
	if( RefreshClock.CountUpToSecs && (RefreshClock.Progress() >= 1.) )
	{
		GameMenu *gm = Refresh();
		gm->Draw();
		return;
	}
	
	bool is_top = IsTop();
	if( ! is_top )
	{
		const Layer *top = Raptor::Game->Layers.TopLayer();
		if( top && ! top->Name.length() )  // FIXME: Hack for DropDownListBox.
			is_top = true;
	}
	Alpha = is_top ? 0.75f : 0.66f;
	if( DefaultButton )
		DefaultButton->Blue = is_top ? 0.5f : 0.f;
	
	Window::Draw();
	ItemFont->DrawText( "BTTT: BattleTech TableTop", Rect.w/2 + 2, 7, Font::ALIGN_TOP_CENTER, 0,0,0,0.8f );
	ItemFont->DrawText( "BTTT: BattleTech TableTop", Rect.w/2,     5, Font::ALIGN_TOP_CENTER );
}


bool GameMenu::KeyDown( SDLKey key )
{
	HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
	if( hex_board && (hex_board->Selected == hex_board->MessageInput) )
		return false;
	else if( (key == SDLK_ESCAPE) || (key == SDLK_F10) )
		Remove();
	else if( DefaultButton && ((key == SDLK_RETURN) || (key == SDLK_KP_ENTER)) && IsTop() )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		const Mech *selected = game->SelectedMech();
		if( selected && selected->Steps.size() )
			return false;
		DefaultButton->Clicked();
	}
	else
		return false;
	return true;
}


bool GameMenu::MouseDown( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
		MoveToTop();
	
	return true;
}


GameMenu *GameMenu::Refresh( double delay )
{
	if( delay )
	{
		RefreshClock.Reset( delay );
		return NULL;
	}
	else
	{
		GameMenu *gm = new GameMenu();
		Raptor::Game->Layers.Add( gm );
		gm->Rect.x = Rect.x;
		gm->Rect.y = Rect.y;
		Remove();
		return gm;
	}
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
: LabelledButton( rect, button_font, "Disconnect", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
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
		Raptor::Game->Net.Disconnect();
}


// ---------------------------------------------------------------------------


GameMenuEndButton::GameMenuEndButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, "End Game", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


GameMenuEndButton::~GameMenuEndButton()
{
}


void GameMenuEndButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
	{
		if( Raptor::Server->IsRunning() && (Raptor::Server->State > BattleTech::State::SETUP) )
			Raptor::Server->ChangeState( BattleTech::State::SETUP );
		else
			Raptor::Game->Cfg.Command( "ready" );
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
		
		Raptor::Game->Cfg.Command( Command );
	}
}


// ---------------------------------------------------------------------------


GameMenuCheckBox::GameMenuCheckBox( SDL_Rect *rect, Font *font, std::string variable, std::string label, std::string true_str, std::string false_str )
: CheckBox( rect, font, std::string(" ") + label, false, NULL, NULL, NULL, NULL, NULL, NULL )
{
	Name = variable;
	
	Red = 1.f;
	Green = 1.f;
	Blue = 1.f;
	Alpha = 0.75f;
	
	Variable = variable;
	TrueStr = true_str;
	FalseStr = false_str;
	
	SizeToText();
	
	SetChecked();
}


GameMenuCheckBox::~GameMenuCheckBox()
{
}


void GameMenuCheckBox::Draw( void )
{
	CheckBox::Draw();
	Raptor::Game->Gfx.DrawRect2D( 1, 1, Rect.h - 1, Rect.h - 1, 0, 0.f,0.f,0.f,1.f );
	if( Checked )
		LabelFont->DrawText( "X", (Rect.h+1)/2, (Rect.h+1)/2, Font::ALIGN_MIDDLE_CENTER );
}


void GameMenuCheckBox::SetChecked( void )
{
	std::map<std::string,std::string>::const_iterator value = Raptor::Game->Cfg.Settings.find( Variable );
	if( value != Raptor::Game->Cfg.Settings.end() )
	{
		if( value->second == TrueStr )
			Checked = true;
		else if( value->second == FalseStr )
			Checked = false;
		else
			Checked = Str::AsBool( value->second );
	}
	else
		Checked = false;
}


void GameMenuCheckBox::Changed( void )
{
	Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
	Raptor::Game->Cfg.Settings[ Variable ] = Checked ? TrueStr : FalseStr;
}


// ---------------------------------------------------------------------------


GameMenuSvCheckBox::GameMenuSvCheckBox( SDL_Rect *rect, Font *font, std::string variable, std::string label, std::string true_str, std::string false_str )
: GameMenuCheckBox( rect, font, variable, label, true_str, false_str )
{
	SetChecked();
}


GameMenuSvCheckBox::~GameMenuSvCheckBox()
{
}


void GameMenuSvCheckBox::SetChecked( void )
{
	std::map<std::string,std::string>::const_iterator value = Raptor::Game->Data.Properties.find( Variable );
	if( value != Raptor::Game->Data.Properties.end() )
	{
		if( value->second == TrueStr )
			Checked = true;
		else if( value->second == FalseStr )
			Checked = false;
		else
			Checked = Str::AsBool( value->second );
	}
	else
		Checked = false;
}


void GameMenuSvCheckBox::Changed( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	game->Snd.Play( game->Res.GetSound("i_select.wav") );
	
	Packet info = Packet( Raptor::Packet::INFO );
	info.AddUShort( 1 );
	info.AddString( Variable );
	info.AddString( Checked ? TrueStr : FalseStr );
	game->Net.Send( &info );
	
	if( Variable == "hotseat" )
	{
		GameMenu *gm = (GameMenu*) game->Layers.Find("GameMenu");
		int teams = game->Data.PropertyAsInt("teams",2);
		
		if( (teams == 2) && gm && gm->AITeam && (gm->AITeam->Value != "0") )
			gm->AITeam->Select( "0" );  // FIXME: Prevent double sound.
		
		if( Checked && (game->State >= BattleTech::State::INITIATIVE) )
		{
			uint8_t my_team = game->MyTeam();
			if( my_team && game->TeamTurn && (my_team != game->TeamTurn) )
				game->Cfg.Command( std::string("team ") + Num::ToString(game->TeamTurn) );
		}
	}
}


// ---------------------------------------------------------------------------


GameMenuDropDown::GameMenuDropDown( SDL_Rect *rect, Font *font, std::string variable )
: DropDown( rect, font, Font::ALIGN_MIDDLE_LEFT, 0, NULL, NULL )
{
	Name = variable;
	
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


// ---------------------------------------------------------------------------


GameMenuSvDropDown::GameMenuSvDropDown( SDL_Rect *rect, Font *font, std::string variable )
: GameMenuDropDown( rect, font, variable )
{
	Value = Raptor::Game->Data.PropertyAsString( variable );
}


GameMenuSvDropDown::~GameMenuSvDropDown()
{
}


void GameMenuSvDropDown::Changed( void )
{
	if( Sound )
		Raptor::Game->Snd.Play( Sound );
	
	Packet info = Packet( Raptor::Packet::INFO );
	info.AddUShort( 1 );
	info.AddString( Variable );
	info.AddString( Value );
	Raptor::Game->Net.Send( &info );
	
	GameMenu *gm = (GameMenu*) Raptor::Game->Layers.Find("GameMenu");
	int teams = Raptor::Game->Data.PropertyAsInt("teams",2);
	if( gm && gm->AITeam && (Variable == "teams") )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		teams = Str::AsInt( Value );
		gm->AITeam->Clear();
		gm->AITeam->AddItem( "0", " AI Team: None" );
		for( int i = 1; i <= teams; i ++ )
			gm->AITeam->AddItem( Num::ToString(i), std::string(" AI Team: ") + game->TeamName(i) );
		gm->AITeam->AddItem( "1,2,3,4,5", " AI Team: All (No Humans)" );
		if( gm->AITeam->FindItem(gm->AITeam->Value) < 0 )
			gm->AITeam->Select( 0 );
	}
	else if( gm && (Variable == "ai_team") )
	{
		if( gm->Hotseat && gm->Hotseat->Checked && (teams == 2) && Str::AsInt(Value) )
			gm->Hotseat->Clicked();  // FIXME: Prevent double sound.
		/*
		if( Container && Container->Container && (Container->Container->Name == "GameMenu") )
			((GameMenu*)( Container->Container ))->Refresh( 0.3 );
		*/
	}
}


// ---------------------------------------------------------------------------


GameMenuScenarioDropDown::GameMenuScenarioDropDown( SDL_Rect *rect, Font *font )
: DropDown( rect, font, Font::ALIGN_MIDDLE_CENTER, 0, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	LoadedNames = false;
	
	AddItem( "", "Load Scenario..." );
	if( DIR *dir_p = opendir("Scenarios") )
	{
		while( struct dirent *dir_entry_p = readdir(dir_p) )
		{
			if( ! dir_entry_p->d_name )
				continue;
			if( dir_entry_p->d_name[ 0 ] == '.' )
				continue;
			AddItem( dir_entry_p->d_name, dir_entry_p->d_name );
		}
		closedir( dir_p );
	}
	Select( "" );
	
	if( Items.size() == 1 )
	{
		Enabled = false;
		AlphaNormal = AlphaOver = 0.5;
	}
}


GameMenuScenarioDropDown::~GameMenuScenarioDropDown()
{
}


void GameMenuScenarioDropDown::Changed( void )
{
	if( ! Value.empty() )
	{
		Raptor::Game->Cfg.Command( std::string("exec \"Scenarios/") + Value + std::string("\"") );
		
		Player *player = Raptor::Game->Data.GetPlayer( Raptor::Game->PlayerID );
		if( player )
		{
			Packet message = Packet( Raptor::Packet::MESSAGE );
			message.AddString( player->Name + std::string(" loaded scenario: ") + LabelText );
			message.AddUInt( TextConsole::MSG_NORMAL );
			Raptor::Game->Net.Send( &message );
		}
		
		Select( "" );
		
		if( Container && Container->Container && (Container->Container->Name == "GameMenu") )
			((GameMenu*)( Container->Container ))->Refresh( 0.3 );
	}
}


void GameMenuScenarioDropDown::Clicked( Uint8 button )
{
	if( (button == SDL_BUTTON_WHEELUP) || (button == SDL_BUTTON_WHEELDOWN) )
		return;
	
	if( ! LoadedNames )
	{
		for( std::vector<ListBoxItem>::iterator item = Items.begin(); item != Items.end(); item ++ )
		{
			if( item->Value.empty() )
				continue;
			
			std::ifstream input( (std::string("Scenarios/") + item->Value).c_str() );
			if( input.is_open() && ! input.eof() )
			{
				char buffer[ 1024 ] = "";
				input.getline( buffer, sizeof(buffer) );
				if( (buffer[0] == '/') && (buffer[1] == '/') )
				{
					const char *name = buffer + 2;
					while( (*name == ' ') || (*name == '\t') )
						name ++;
					if( strlen(name) )
						item->Text = name;
				}
			}
		}
		
		LoadedNames = true;
	}
	
	DropDown::Clicked( button );
}
