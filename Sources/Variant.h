/*
 *  Variant.h
 */

#pragma once
class VariantEquipment;
class Variant;

#include "PlatformSpecific.h"

#include "HeavyMetal.h"
#include "BattleTechDefs.h"
#include "Packet.h"
#include <vector>


class VariantEquipment
{
public:
	uint16_t ID;
	uint16_t Ammo;
	bool Rear;
	bool HasFCS;
	std::vector<uint8_t> CritLocs;
	
	VariantEquipment( uint16_t wid );
	VariantEquipment( Packet *packet );
	virtual ~VariantEquipment();
	
	void AddToPacket( Packet *packet ) const;
	void ReadFromPacket( Packet *packet );
};


class Variant
{
public:
	uint8_t Tons;
	std::string Name, Var;
	uint16_t Era, BV2;
	bool Clan, Quad;
	uint8_t Walk, Jump;
	bool MASC, TSM, Supercharger;
	uint8_t Heatsinks;
	bool HeatsinksDouble;
	uint8_t Armor[ BattleTech::Loc::COUNT + 3 ];
	bool Stealth;
	uint16_t CASE;
	std::vector<VariantEquipment> Equipment;
	
	Variant( void );
	virtual ~Variant();
	
	bool Load( const char *filename );
	
	void AddToPacket( Packet *packet ) const;
	void ReadFromPacket( Packet *packet );
	
	std::string Details( void ) const;
};
