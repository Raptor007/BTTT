/*
 *  RecordSheet.h
 */

#pragma once
class RecordSheet;
class RecordSheetLoc;
class RecordSheetHealth;

#include "PlatformSpecific.h"

#include "Window.h"
#include "Font.h"
#include "Label.h"
#include "LabelledButton.h"
#include "GroupBox.h"
#include "Clock.h"
#include "Mech.h"


class RecordSheet : public Window
{
public:
	Font *TitleFont;
	uint32_t MechID;
	bool AutoHide;
	Clock Lifetime;
	
	RecordSheet( const Mech *mech = NULL );
	virtual ~RecordSheet();
	
	void AutoPosition( void );
	Mech *GetMech( void );
	bool KeyDown( SDLKey key );
	bool MouseDown( Uint8 button = SDL_BUTTON_LEFT );
	void Draw( void );
	void Close( void );
};


class RecordSheetCloseButton : public LabelledButton
{
public:
	RecordSheetCloseButton( SDL_Rect *rect, Font *button_font );
	virtual ~RecordSheetCloseButton();
	
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};


class RecordSheetLoc : public GroupBox
{
public:
	int8_t Loc;
	Font *ItemFont;
	
	RecordSheetLoc( int8_t loc, SDL_Rect *rect, Font *title_font, Font *item_font = NULL );
	virtual ~RecordSheetLoc();
	
	void Draw( void );
};


class RecordSheetPilot : public GroupBox
{
public:
	Font *ItemFont;
	
	RecordSheetPilot( SDL_Rect *rect, Font *title_font, Font *item_font = NULL );
	virtual ~RecordSheetPilot();
	
	void Draw( void );
};


class RecordSheetStatus : public GroupBox
{
public:
	Font *ItemFont;
	
	RecordSheetStatus( SDL_Rect *rect, Font *title_font, Font *item_font = NULL );
	virtual ~RecordSheetStatus();
	
	void Draw( void );
};


class RecordSheetHealth : public Layer
{
public:
	RecordSheetHealth( SDL_Rect *rect );
	virtual ~RecordSheetHealth();
	
	void Draw( void );
};
