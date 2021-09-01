/*
 *  HexBoard.h
 */

#pragma once
class HexBoard;

#include "PlatformSpecific.h"

#include "Layer.h"
#include "HexMap.h"
#include "Font.h"


class HexBoard : public Layer
{
public:
	int TargetDist;
	ShotPath Path;
	std::map<uint8_t,int8_t> WeaponsInRange;
	std::set<MechMelee> PossibleMelee;
	Font *TurnFont;
	
	HexBoard( void );
	virtual ~HexBoard();
	
	void UpdateRects( void );
	void Draw( void );
	
	bool MouseDown( Uint8 button = SDL_BUTTON_LEFT );
	bool KeyDown( SDLKey key );
	void ClearPath( bool remove_menu = true );
	void UpdateWeaponsInRange( Mech *selected, Mech *target );
};
