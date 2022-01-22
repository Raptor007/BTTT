/*
 *  Event.h
 */

#pragma once
class Event;

#include "PlatformSpecific.h"

#include "Packet.h"
#include "Mech.h"
#include <string>


class Event
{
public:
	float Duration;
	std::string Text;
	std::string Sound;
	uint8_t Effect;
	uint32_t MechID;
	uint8_t X, Y, Misc;
	bool HealthUpdate, CriticalHit;
	uint8_t Loc, Arc, HP, Eq;
	int8_t Stat, Val1, Val2;
	
	Event( const Mech *m1 = NULL, const Mech *m2 = NULL );
	Event( Packet *packet );
	virtual ~Event();
	
	void ShowFacing( void );
	void ShowFacing( const Mech *mech );
	void ShowFacing( uint8_t facing, bool prone = false, int8_t twist = 0, int8_t turned_arm = BattleTech::Loc::UNKNOWN, bool arm_flip = false, int8_t prone_fire = BattleTech::Loc::UNKNOWN );
	void ReadFacing( Mech *mech ) const;
	
	void ShowHealth( int8_t loc, uint8_t arc );
	void ShowHealth( Mech *mech, int8_t loc, uint8_t arc );
	void ShowHealth( int8_t loc, uint8_t arc, uint16_t hp );
	void ShowCritHit( const MechEquipment *eq, int8_t loc = BattleTech::Loc::UNKNOWN );
	
	void ShowEquipment( const MechEquipment *eq );
	void ShowAmmo( const MechEquipment *ammo );
	void ShowStat( int8_t stat, int8_t val1, int8_t val2 = 0 );
	void ShowStat( int8_t stat );
	void ShowStat( Mech *mech, int8_t stat );
	void ReadStat( Mech *mech ) const;
	
	void AddToPacket( Packet *packet ) const;
	void ReadFromPacket( Packet *packet );
};
