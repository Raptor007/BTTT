/*
 *  SpawnMenu.cpp
 */

#include "SpawnMenu.h"

#include "BattleTechGame.h"
#include "HexBoard.h"
#include "GroupBox.h"
#include "Num.h"
#include <cctype>


SpawnMenu::SpawnMenu( void )
{
	Name = "SpawnMenu";
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	// If the previously selected variant is missing, find the closest match.
	std::string mech = game->Cfg.SettingAsString("mech");
	if( game->Variants.size() && (game->Variants.find(mech) == game->Variants.end()) )
	{
		int best_match = -1;
		for( std::map<std::string,Variant>::const_iterator var = game->Variants.begin(); var != game->Variants.end(); var ++ )
		{
			int match = 0;
			for( ; tolower(mech[ match ]) == tolower(var->first[ match ]); match ++ ) {}
			if( match > best_match )
			{
				game->Cfg.Settings["mech"] = var->first;
				best_match = match;
			}
		}
	}
	
	int teams = game->Data.PropertyAsInt("teams",2);
	int teams_w = 20 + 110 * teams + 10 * (teams - 1);
	
	Rect.w = 390 + teams_w;
	Rect.x = game->Gfx.W/2 - Rect.w/2;
	Rect.y = 10;
	Rect.h = 80;
	
	Red = 0.4f;
	Green = 0.4f;
	Blue = 0.4f;
	Alpha = 0.75f;
	
	Font *large_font = game->Res.GetFont( "ProFont.ttf", 18 );
	Font *small_font = game->Res.GetFont( "ProFont.ttf", 14 );
	
	SDL_Rect rect, group_rect;
	GroupBox *group = NULL;
	
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new SpawnMenuCloseButton( &rect, small_font ) );
	
	// ----------------------------------------------------------------------------------
	
	group_rect.x = 10;
	group_rect.y = 10;
	group_rect.w = teams_w;
	group_rect.h = 300;
	group = new GroupBox( &group_rect, " MechWarrior, Choose Your Clan ", small_font );
	AddElement( group );
	
	rect.x = 10;
	rect.w = (group_rect.w - rect.x * (teams + 1)) / teams;
	
	for( int team = 1; team <= teams; team ++ )
	{
		std::string team_str = Num::ToString(team);
		
		rect.x = 10 + 120 * (team - 1);
		rect.y = 10 + group->TitleFont->GetAscent();
		rect.h = large_font->GetAscent();
		TeamButtons[ team_str ] = new SpawnMenuJoinButton( &rect, large_font, team_str, std::string("Team ") + team_str );
		group->AddElement( TeamButtons[ team_str ] );
		
		rect.y += rect.h + 5;
		rect.h = Rect.h - rect.y;
		TeamLists[ team_str ] = new Label( &rect, "", small_font, Font::ALIGN_TOP_CENTER );
		group->AddElement( TeamLists[ team_str ] );
	}
	
	// ----------------------------------------------------------------------------------
	
	group_rect.x += group_rect.w;
	group_rect.w = Rect.w - group_rect.x - 10;
	group = new GroupBox( &group_rect, "and Select Your BattleMech ", small_font );
	AddElement( group );
	
	/*
	rect.x = 10;
	rect.y = 10 + group->TitleFont->GetAscent();
	rect.w = (group_rect.w - rect.x * 3) / 2;
	rect.h = small_font->GetHeight();
	SpawnMenuDropDown *gunnery = new SpawnMenuDropDown( &rect, small_font, Font::ALIGN_MIDDLE_CENTER, 0, "gunnery" );
	gunnery->AddItem( "4", "Gunnery 4 (IS)  " );
	gunnery->AddItem( "3", "Gunnery 3 (Clan)" );
	gunnery->Update();
	group->AddElement( gunnery );
	
	rect.x += rect.x + rect.w;
	SpawnMenuDropDown *piloting = new SpawnMenuDropDown( &rect, small_font, Font::ALIGN_MIDDLE_CENTER, 0, "piloting" );
	piloting->AddItem( "5", "Piloting 5 (IS)  " );
	piloting->AddItem( "4", "Piloting 4 (Clan)" );
	piloting->Update();
	group->AddElement( piloting );
	
	rect.y += rect.h;
	*/
	
	rect.x = 7;
	rect.y = 5 + group->TitleFont->GetAscent();
	rect.w = group_rect.w - 10;
	rect.h = small_font->GetLineSkip();
	Label *search = new Label( &rect, "Search:", small_font, Font::ALIGN_MIDDLE_LEFT );
	search->SizeToText();
	group->AddElement( search );
	
	rect.x = search->Rect.x + search->Rect.w + 3;
	rect.w = group_rect.w - rect.x - 5;
	SearchBox = new SpawnMenuSearchBox( &rect, small_font, Font::ALIGN_MIDDLE_LEFT );
	group->AddElement( SearchBox );
	
	rect.y += rect.h + 5;
	
	rect.x = 5;
	rect.w = group_rect.w - 10;
	rect.h = small_font->GetLineSkip();
	MechList = new SpawnMenuDropDown( &rect, small_font, Font::ALIGN_MIDDLE_LEFT, 16, "mech" );
	for( std::map<std::string,Variant>::const_iterator var = game->Variants.begin(); var != game->Variants.end(); var ++ )
		MechList->AddItem( var->first, var->first );
	MechList->Update();
	group->AddElement( MechList );
	
	rect.x += 10;
	rect.w -= 20;
	rect.y += rect.h + 10;
	rect.h = 0;
	MechDetails = new Label( &rect, "", small_font, Font::ALIGN_TOP_LEFT );
	group->AddElement( MechDetails );
	
	// ----------------------------------------------------------------------------------
	
	Update();
}


SpawnMenu::~SpawnMenu()
{
}


void SpawnMenu::Draw( void )
{
	UpdateTeams();
	Window::Draw();
}


bool SpawnMenu::KeyDown( SDLKey key )
{
	HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
	if( hex_board && (hex_board->Selected == hex_board->MessageInput) )
		return false;
	else if( (key == SDLK_ESCAPE) || (key == SDLK_TAB) || (key == SDLK_RETURN) || (key == SDLK_KP_ENTER) )
		Remove();
	else if( key == SDLK_UP )
		MechList->Clicked( SDL_BUTTON_WHEELUP );
	else if( key == SDLK_DOWN )
		MechList->Clicked( SDL_BUTTON_WHEELDOWN );
	else if( (key == SDLK_PAGEUP) || (key == SDLK_LEFT) )
		PrevMech();
	else if( (key == SDLK_PAGEDOWN) || (key == SDLK_RIGHT) )
		NextMech();
	else if( (key >= 'a') && (key <= 'z') )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		bool shift = game->Keys.KeyDown(SDLK_LSHIFT) || game->Keys.KeyDown(SDLK_RSHIFT);
		size_t max_loop = game->Variants.size();
		for( size_t i = 0; i < max_loop; i ++ )
		{
			std::string mech = Raptor::Game->Cfg.SettingAsString("mech");
			if( shift )
			{
				PrevMech();
				if( mech == Raptor::Game->Cfg.SettingAsString("mech") )
					MechList->Select( game->Variants.rbegin()->first );
			}
			else
			{
				NextMech();
				if( mech == Raptor::Game->Cfg.SettingAsString("mech") )
					MechList->Select( game->Variants.begin()->first );
			}
			mech = Raptor::Game->Cfg.SettingAsString("mech");
			if( (mech.length() >= 10) && (tolower(mech[9]) == key) )
				break;
		}
	}
	else if( (key >= '0') && (key <= '9') )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		bool shift = game->Keys.KeyDown(SDLK_LSHIFT) || game->Keys.KeyDown(SDLK_RSHIFT);
		std::string prev = Raptor::Game->Cfg.SettingAsString("mech");
		size_t max_loop = game->Variants.size();
		for( size_t i = 0; i < max_loop; i ++ )
		{
			std::string mech = Raptor::Game->Cfg.SettingAsString("mech");
			if( shift )
			{
				PrevMech();
				if( mech == Raptor::Game->Cfg.SettingAsString("mech") )
					MechList->Select( game->Variants.rbegin()->first );
			}
			else
			{
				NextMech();
				if( mech == Raptor::Game->Cfg.SettingAsString("mech") )
					MechList->Select( game->Variants.begin()->first );
			}
			mech = Raptor::Game->Cfg.SettingAsString("mech");
			if( (mech.length() >= 7) && (mech[1] == key) && ((mech[1] != prev[1]) || (mech[2] != prev[2]) || (mech[6] != prev[6])) )
				break;
		}
	}
	else if( (key >= SDLK_F1) && (key <= SDLK_F15) )
	{
		int teams = Raptor::Game->Data.PropertyAsInt("teams",2);
		int team = key + 1 - SDLK_F1;
		if( team > teams )
			return false;
		
		std::string team_str = Num::ToString(team);
		Raptor::Game->Data.Players[ Raptor::Game->PlayerID ]->Properties[ "team" ] = team_str;
		
		Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
		
		Packet player_properties = Packet( Raptor::Packet::PLAYER_PROPERTIES );
		player_properties.AddUShort( Raptor::Game->PlayerID );
		player_properties.AddUInt( 1 );
		player_properties.AddString( "team" );
		player_properties.AddString( team_str );
		Raptor::Game->Net.Send( &player_properties );
	}
	else
		return false;
	return true;
}


bool SpawnMenu::MouseDown( Uint8 button )
{
	MoveToTop();
	
	if( (button == SDL_BUTTON_WHEELUP) || (button == SDL_BUTTON_WHEELDOWN) )
	{
		if( MechDetails->WithinCalcRect( Raptor::Game->Mouse.X, Raptor::Game->Mouse.Y ) )
		{
			if( Raptor::Game->Keys.KeyDown(SDLK_LSHIFT) || Raptor::Game->Keys.KeyDown(SDLK_RSHIFT) )
			{
				if( button == SDL_BUTTON_WHEELUP )
					PrevMech();
				else
					NextMech();
			}
			else
				MechList->Clicked( button );
		}
		
		return true;
	}
	
	if( (button == SDL_BUTTON_LEFT) && SearchBox->IsSelected() )
		SearchBox->Container->Selected = NULL;
	
	Draggable = (button == SDL_BUTTON_LEFT);
	return Window::MouseDown( button );
}


void SpawnMenu::NextMech( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	if( ! game->Variants.size() )
		return;
	
	std::string mech = Raptor::Game->Cfg.SettingAsString("mech");
	
	if( mech == game->Variants.rbegin()->first )
		return;
	
	std::map<std::string,Variant>::const_iterator var = game->Variants.find( mech );
	if( var == game->Variants.end() )
	{
		MechList->Select( game->Variants.rbegin()->first );
		return;
	}
	
	while( strncmp( var->first.c_str(), mech.c_str(), 13 ) == 0 )
	{
		if( var->first == game->Variants.rbegin()->first )
			break;
		var ++;
	}
	
	MechList->Select( var->first );
}


void SpawnMenu::PrevMech( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	if( ! game->Variants.size() )
		return;
	
	std::string mech = Raptor::Game->Cfg.SettingAsString("mech");
	
	if( mech == game->Variants.begin()->first )
		return;
	
	std::map<std::string,Variant>::const_iterator var = game->Variants.find( mech );
	if( var == game->Variants.end() )
	{
		MechList->Select( game->Variants.begin()->first );
		return;
	}
	
	while( strncmp( var->first.c_str(), mech.c_str(), 13 ) == 0 )
	{
		if( var == game->Variants.begin() )
			break;
		var --;
	}
	
	if( var != game->Variants.begin() )
	{
		// Select the first variant.
		std::string match = var->first;
		while( (var != game->Variants.begin()) && (strncmp( var->first.c_str(), match.c_str(), 13 ) == 0) )
			var --;
		if( strncmp( var->first.c_str(), match.c_str(), 13 ) != 0 )
			var ++;
	}
	
	MechList->Select( var->first );
}


void SpawnMenu::Update( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	std::map<std::string,Variant>::const_iterator var = game->Variants.find( Raptor::Game->Cfg.SettingAsString("mech") );
	if( var != game->Variants.end() )
	{
		MechDetails->Visible = true;
		MechDetails->LabelText = var->second.Details();
		MechDetails->Rect.h = MechDetails->LabelFont->TextHeight( MechDetails->LabelText );
		MechDetails->Container->Rect.h = MechDetails->Rect.y + MechDetails->Rect.h + 10;
	}
	else
	{
		MechDetails->Visible = false;
		MechDetails->LabelText = "";
		MechDetails->Rect.h = 0;
		MechDetails->Container->Rect.h = MechDetails->Rect.y;
	}
	
	TeamButtons[ "1" ]->Container->Rect.h = MechDetails->Container->Rect.h;
	Rect.h = MechDetails->Container->Rect.y + MechDetails->Container->Rect.h + 10;

	for( std::map<std::string,Label*>::iterator list = TeamLists.begin(); list != TeamLists.end(); list ++ )
		list->second->Rect.h = list->second->Container->Rect.h - list->second->Rect.y;
	
	UpdateTeams();
}


void SpawnMenu::UpdateTeams( void )
{
	for( std::map<std::string,Label*>::iterator list = TeamLists.begin(); list != TeamLists.end(); list ++ )
		list->second->LabelText = "";
	
	for( std::map<uint16_t,Player*>::const_iterator player = Raptor::Game->Data.Players.begin(); player != Raptor::Game->Data.Players.end(); player ++ )
	{
		std::map<std::string,Label*>::iterator list = TeamLists.find( player->second->PropertyAsString("team") );
		if( list != TeamLists.end() )
			list->second->LabelText += player->second->Name + std::string("\n");
	}
	
	for( std::map<std::string,SpawnMenuJoinButton*>::iterator button = TeamButtons.begin(); button != TeamButtons.end(); button ++ )
		button->second->LabelText = Raptor::Game->Data.PropertyAsString( std::string("team") + button->first, (std::string("Team ") + button->first).c_str() );
}


// ---------------------------------------------------------------------------


SpawnMenuDropDown::SpawnMenuDropDown( SDL_Rect *rect, Font *font, uint8_t align, int scroll_bar_size, std::string variable )
: DropDown( rect, font, align, scroll_bar_size, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	Variable = variable;
	Value = Raptor::Game->Cfg.SettingAsString( Variable );
}


SpawnMenuDropDown::~SpawnMenuDropDown()
{
}


void SpawnMenuDropDown::Changed( void )
{
	bool is_mech = (Variable == "mech");
	
	if( Value.length() || ! is_mech )
		Raptor::Game->Cfg.Settings[ Variable ] = Value;
	
	if( is_mech )
		((SpawnMenu*)( Container->Container ))->Update();
	
	DropDown::Changed();
}


// ---------------------------------------------------------------------------


SpawnMenuSearchBox::SpawnMenuSearchBox( SDL_Rect *rect, Font *font, uint8_t align )
: TextBox( rect, font, align )
{
	PassReturn = false;
	PassEsc = false;
	ReturnDeselects = true;
	EscDeselects = true;
	
	SelectedRed = 1.f;
	SelectedGreen = 1.f;
	SelectedBlue = 0.f;
}


SpawnMenuSearchBox::~SpawnMenuSearchBox()
{
}


bool SpawnMenuSearchBox::KeyDown( SDLKey key )
{
	bool handled = TextBox::KeyDown( key );
	
	if( (key >= SDLK_F1) && (key <= SDLK_F15) )
		return false;
	
	return handled;
}


void SpawnMenuSearchBox::Changed( void )
{
	SpawnMenuDropDown *mech_list = ((SpawnMenu*)( Container->Container ))->MechList;
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	std::list<std::string> words = Str::SplitToList( Text, " " );
	
	mech_list->Clear();
	for( std::map<std::string,Variant>::const_iterator var = game->Variants.begin(); var != game->Variants.end(); var ++ )
	{
		bool match = true;
		for( std::list<std::string>::const_iterator word = words.begin(); word != words.end(); word ++ )
		{
			if( word->length() && (Str::FindInsensitive( var->first, *word ) < 0) )
				match = false;
		}
		
		if( match )
			mech_list->AddItem( var->first, var->first );
	}
	
	// If the previously selected variant does not match this search, select something that does.
	std::string mech = game->Cfg.SettingAsString("mech");
	if( mech_list->Items.size() && (mech_list->FindItem(mech) < 0) )
	{
		int best_index = 0;
		int best_match = -1;
		for( size_t i = 0; i < mech_list->Items.size(); i ++ )
		{
			const char *item = mech_list->Items[ i ].Value.c_str();
			int match = 0;
			for( ; tolower(mech[ match ]) == tolower(item[ match ]); match ++ ) {}
			if( match > best_match )
			{
				best_match = match;
				best_index = i;
			}
		}
		
		mech_list->Select( best_index );
	}
	
	mech_list->Update();
	
	if( ! mech_list->Items.size() )
		mech_list->LabelText = "";
}


// ---------------------------------------------------------------------------


SpawnMenuJoinButton::SpawnMenuJoinButton( SDL_Rect *rect, Font *button_font, std::string team, std::string name ) : LabelledButton( rect, button_font, name, Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	Team = team;
}


SpawnMenuJoinButton::~SpawnMenuJoinButton()
{
}


void SpawnMenuJoinButton::Clicked( Uint8 button )
{
	Player *p = Raptor::Game->Data.GetPlayer( Raptor::Game->PlayerID );
	if( ! p )
		return;
	
	p->Properties[ "team" ] = Team;
	
	Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
	
	Packet player_properties = Packet( Raptor::Packet::PLAYER_PROPERTIES );
	player_properties.AddUShort( Raptor::Game->PlayerID );
	player_properties.AddUInt( 1 );
	player_properties.AddString( "team" );
	player_properties.AddString( Team );
	Raptor::Game->Net.Send( &player_properties );
}


// ---------------------------------------------------------------------------


SpawnMenuCloseButton::SpawnMenuCloseButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, "x", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


SpawnMenuCloseButton::~SpawnMenuCloseButton()
{
}


void SpawnMenuCloseButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
		Container->Remove();
}
