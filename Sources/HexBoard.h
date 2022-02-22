/*
 *  HexBoard.h
 */

#pragma once
class HexBoard;
class HexBoardAim;

#include "PlatformSpecific.h"

#include "Layer.h"
#include "HexMap.h"
#include "Font.h"
#include "MessageOverlay.h"
#include "TextBox.h"


class HexBoardAim
{
public:
	ShotPath Path;
	std::map<uint8_t,int8_t> WeaponsInRange;
	std::set<MechMelee> PossibleMelee;
	
	void UpdateWeaponsInRange( const Mech *selected, const Mech *target, int state );
	Packet *CreatePacket( const Mech *selected, const Mech *target, int state ) const;
};


class HexBoard : public HexBoardAim, public Layer
{
public:
	Font *TurnFont;
	MessageOverlay *MessageOutput;
	TextBox *MessageInput;
	
	HexBoard( void );
	virtual ~HexBoard();
	
	void UpdateRects( void );
	void Draw( void );
	
	bool MouseDown( Uint8 button = SDL_BUTTON_LEFT );
	bool KeyDown( SDLKey key );
	void ClearPath( bool remove_menu = true );
	void RemoveWeaponMenu( void );
	void RemoveSetupMenus( void ) const;
	void UpdateWeaponsInRange( Mech *selected, const Mech *target );
};
