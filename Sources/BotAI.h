/*
 *  BattleTechServer.h
 */

#pragma once
class BotAI;
class BotAIAim;

#include "PlatformSpecific.h"

#include "Packet.h"
#include "HexBoard.h"
#include <map>


class BotAI
{
public:
	double Waited;
	std::map<uint32_t,Packet*> PreparedTurns;
	
	BotAI( void );
	virtual ~BotAI();
	
	void Update( double dt );
	bool ControlsTeam( uint8_t team = 0 ) const;
	void BeginPhase( int state );
	void BeginTurn( void );
	
private:
	void TakeTurn( void );
};


class BotAIAim : public HexBoardAim
{
public:
	const Mech *From, *Target;
	int State;
	std::map<uint8_t,uint8_t> WeaponsToFire;
	std::set<MechMelee> SelectedMelee;
	int8_t TorsoTwist;
	double ValueBonus;
	
	void Initialize( Mech *from, const Mech *target, int state, const HexMap *map );
	void ChooseWeapons( void );
	int16_t ShotHeat( void ) const;
	double Value( void ) const;
	double ValueWithTwist( Mech *from, int8_t twist );
	bool operator < ( const BotAIAim &other ) const;
};
