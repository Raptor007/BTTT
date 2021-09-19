/*
 *  GameMenu.h
 */

#pragma once
class GameMenu;
class GameMenuCloseButton;
class GameMenuDisconnectButton;
class GameMenuReadyButton;
class GameMenuCheckBox;

#include "PlatformSpecific.h"

#include <SDL/SDL.h>

#include "Window.h"
#include "Font.h"
#include "LabelledButton.h"
#include "Label.h"
#include "DropDown.h"
#include "CheckBox.h"
#include "SoundOut.h"


class GameMenu : public Window
{
public:
	Font *ItemFont;
	LabelledButton *DefaultButton;
	
	GameMenu( void );
	virtual ~GameMenu();
	void Draw( void );
	bool KeyDown( SDLKey key );
	bool MouseDown( Uint8 button );
};


class GameMenuCloseButton : public LabelledButton
{
public:
	GameMenuCloseButton( SDL_Rect *rect, Font *button_font );
	virtual ~GameMenuCloseButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};


class GameMenuDisconnectButton : public LabelledButton
{
public:
	GameMenuDisconnectButton( SDL_Rect *rect, Font *button_font );
	virtual ~GameMenuDisconnectButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
	bool EndGame( void ) const;
};


class GameMenuSpawnButton : public LabelledButton
{
public:
	GameMenuSpawnButton( SDL_Rect *rect, Font *button_font );
	virtual ~GameMenuSpawnButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};


class GameMenuCommandButton : public LabelledButton
{
public:
	std::string Command;
	
	GameMenuCommandButton( SDL_Rect *rect, Font *button_font, std::string command, std::string label );
	virtual ~GameMenuCommandButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};


class GameMenuCheckBox : public CheckBox
{
public:
	std::string Variable, TrueStr, FalseStr;
	
	GameMenuCheckBox( SDL_Rect *rect, Font *font, std::string variable, std::string label, std::string true_str = "true", std::string false_str = "false" );
	virtual ~GameMenuCheckBox();
	void Changed( void );
	void Draw( void );
};


class GameMenuDropDown : public DropDown
{
public:
	std::string Variable;
	Mix_Chunk *Sound;
	
	GameMenuDropDown( SDL_Rect *rect, Font *font, std::string variable );
	virtual ~GameMenuDropDown();
	void Changed( void );
};
