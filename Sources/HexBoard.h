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
#include "ShotPath.h"


class HexBoardAim
{
public:
	std::vector<ShotPath> Paths;
	std::map<uint8_t,int8_t> WeaponsInRange; // FIXME: Move into ShotPath?
	std::set<MechMelee> PossibleMelee;       // FIXME: Move into ShotPath?
	
	ShotPath PathTo( uint8_t x, uint8_t y ) const;
	ShotPath PathToTarget( uint32_t target_id ) const;
	void UpdateWeaponsInRange( const Mech *selected, const Mech *target, int state ); // FIXME: Move into ShotPath?
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
	bool RemovePathTo( uint8_t x, uint8_t y );
	void RemoveWeaponMenu( void );
	void RemoveSetupMenus( void ) const;
	void UpdateAim( void );
	void UpdateWeaponsInRange( Mech *selected, const Mech *target, bool show_menu = true );
};
