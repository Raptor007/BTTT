/*
 *  WeaponMenu.h
 */

#pragma once
class WeaponMenu;
class WeaponMenuCloseButton;
class WeaponMenuCheckBox;
class WeaponMenuDropDown;

#include "PlatformSpecific.h"

#include <SDL/SDL.h>

#include "Window.h"
#include "Font.h"
#include "LabelledButton.h"
#include "Label.h"
#include "CheckBox.h"
#include "DropDown.h"
#include "Mech.h"
#include "HexBoard.h"
#include <map>


class WeaponMenu : public Window
{
public:
	Font *ItemFont;
	Mech *Selected;
	Label *HeatTotal;
	
	WeaponMenu( HexBoard *container );
	virtual ~WeaponMenu();
	void Draw( void );
	bool KeyDown( SDLKey key );
	bool MouseDown( Uint8 button );
	
	bool Update( bool force = true );
	void UpdateWeapons( void );
	void UpdateMelee( void );
	void UpdateHeatTotal( void );
	
	void SetCount( uint8_t index, uint8_t count );
	void SetWeapon( uint8_t index, uint8_t count );
	void SetMelee( uint8_t index, uint8_t count );
};


class WeaponMenuCloseButton : public LabelledButton
{
public:
	WeaponMenuCloseButton( SDL_Rect *rect, Font *button_font );
	virtual ~WeaponMenuCloseButton();
	void Clicked( Uint8 button = SDL_BUTTON_LEFT );
};


class WeaponMenuCheckBox : public CheckBox
{
public:
	uint8_t EquipmentIndex;
	
	WeaponMenuCheckBox( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, bool checked );
	virtual ~WeaponMenuCheckBox();
	void Changed( void );
	void Draw( void );
};


class WeaponMenuDropDown : public DropDown
{
public:
	uint8_t EquipmentIndex;
	
	WeaponMenuDropDown( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, uint8_t value, uint8_t maximum );
	virtual ~WeaponMenuDropDown();
	void Changed( void );
};
