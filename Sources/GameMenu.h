/*
 *  GameMenu.h
 */

#pragma once
class GameMenu;
class GameMenuCloseButton;
class GameMenuDisconnectButton;
class GameMenuEndButton;
class GameMenuSpawnButton;
class GameMenuCommandButton;
class GameMenuCheckBox;
class GameMenuSvCheckBox;
class GameMenuDropDown;
class GameMenuSvDropDown;

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
	GameMenuSvCheckBox *Hotseat;
	GameMenuSvDropDown *AITeam;
	
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
};


class GameMenuEndButton : public LabelledButton
{
public:
	GameMenuEndButton( SDL_Rect *rect, Font *button_font );
	virtual ~GameMenuEndButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
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
	void Draw( void );
	virtual void SetChecked( void );
	virtual void Changed( void );
};


class GameMenuSvCheckBox : public GameMenuCheckBox
{
public:
	GameMenuSvCheckBox( SDL_Rect *rect, Font *font, std::string variable, std::string label, std::string true_str = "true", std::string false_str = "false" );
	virtual ~GameMenuSvCheckBox();
	void SetChecked( void );
	void Changed( void );
};


class GameMenuDropDown : public DropDown
{
public:
	std::string Variable;
	Mix_Chunk *Sound;
	
	GameMenuDropDown( SDL_Rect *rect, Font *font, std::string variable );
	virtual ~GameMenuDropDown();
	virtual void Changed( void );
};


class GameMenuSvDropDown : public GameMenuDropDown
{
public:
	GameMenuSvDropDown( SDL_Rect *rect, Font *font, std::string variable );
	virtual ~GameMenuSvDropDown();
	void Changed( void );
};
