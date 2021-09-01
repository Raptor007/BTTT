/*
 *  HeavyMetal.h
 */

#pragma once
class HMStructure;
class HMWeapon;

#include "PlatformSpecific.h"

#include <map>
#include <string>
#include <stdint.h>


class HMStructure
{
public:
	short Head;
	short CenterTorso;
	short SideTorso;
	short Arm;
	short Leg;
};


class HMWeapon
{
public:
	short Type;
	short ID;
	std::string Name;
	std::string AmmoName;
	std::string HeatStr;
	std::string DamageStr;
	short DamageMin;
	short DamageAvg;
	short ExplodeDamage;
	bool AmmoExplodes;
	bool CanUseTargetingComputer;
	bool CanUseArtemisLRM;
	bool CanUseArtemisSRM;
	short SnakeEyesEffect;
	short ToHit;
	short RangeMin;
	short RangeShort;
	short RangeMedium;
	short RangeLong;
	float Tons;
	short Crits;
	short AmmoPerTon;
	int Price;
	int AmmoPrice;
	short TechLevel;
	std::string Era;
	bool Defensive;
	short BV;
	short AmmoBV;
	
	uint8_t Damage;
	uint8_t Heat;
	uint8_t Flamer;
	uint8_t ClusterWeaponSize;
	uint8_t ClusterDamageGroup;
	int8_t ClusterRollBonus;
	uint8_t RapidFire;
	bool TAG;
	bool Narc;
	std::string Sound;
	std::string HitSound;
	uint8_t Effect;
	
	enum
	{
		ENERGY = 0,
		BALLISTIC,
		ULTRA_AC,
		MISSILE,
		MISSILE_3D6,
		MISSILE_STREAK,
		MISC,
		ONE_SHOT
	};
};


namespace HeavyMetal
{
	unsigned char *hmstrcpy( char *dst, unsigned char *src );
	std::map< short, HMStructure > Structures( const char *filename = "INTRNAL.DAT" );
	std::map< short, HMWeapon > Weapons( const char *filename );
	std::string CritName( short wid, const std::map< short, HMWeapon > *weapons );
	bool HittableCrit( short wid );
	uint8_t CritSlots( short wid, bool clan, const std::map< short, HMWeapon > *weapons );
}
