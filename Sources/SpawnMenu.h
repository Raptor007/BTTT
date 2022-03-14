/*
 *  SpawnMenu.h
 */

#pragma once
class SpawnMenu;
class SpawnMenuDropDown;
class SpawnMenuSearchBox;
class SpawnMenuJoinButton;
class SpawnMenuCloseButton;

#include "PlatformSpecific.h"

#include <SDL/SDL.h>

#include "Window.h"
#include "Font.h"
#include "LabelledButton.h"
#include "DropDown.h"
#include "TextBox.h"
#include "Label.h"
#include <map>
#include <string>


class SpawnMenu : public Window
{
public:
	SpawnMenuDropDown *MechList;
	SpawnMenuSearchBox *SearchBox;
	Label *MechDetails;
	std::map<std::string,Label*> TeamLists;
	std::map<std::string,SpawnMenuJoinButton*> TeamButtons;
	
	SpawnMenu( void );
	virtual ~SpawnMenu();
	void Draw( void );
	bool KeyDown( SDLKey key );
	bool MouseDown( Uint8 button );
	void NextMech( void );
	void PrevMech( void );
	void Update( void );
	void UpdateTeams( void );
};


class SpawnMenuDropDown : public DropDown
{
public:
	std::string Variable;
	
	SpawnMenuDropDown( SDL_Rect *rect, Font *font, uint8_t align, int scroll_bar_size, std::string variable );
	virtual ~SpawnMenuDropDown();
	void Changed( void );
};


class SpawnMenuSearchBox : public TextBox
{
public:
	SpawnMenuSearchBox( SDL_Rect *rect, Font *font, uint8_t align );
	virtual ~SpawnMenuSearchBox();
	bool KeyDown( SDLKey key );
	void Changed( void );
};


class SpawnMenuJoinButton : public LabelledButton
{
public:
	std::string Team;
	
	SpawnMenuJoinButton( SDL_Rect *rect, Font *button_font, std::string team, std::string name );
	virtual ~SpawnMenuJoinButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};


class SpawnMenuCloseButton : public LabelledButton
{
public:
	SpawnMenuCloseButton( SDL_Rect *rect, Font *button_font );
	virtual ~SpawnMenuCloseButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};
