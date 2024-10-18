/*
 *  ShotPath.h
 */

#pragma once
class ShotPath;

#include "PlatformSpecific.h"

#include "HexMap.h"


class ShotPath : public std::vector<const Hex*>
{
public:
	int Distance;
	int Modifier;
	bool LineOfSight;
	bool PartialCover;
	bool LegWeaponsBlocked;
	std::map<uint32_t,uint8_t> ECM;
	uint8_t DamageFromX, DamageFromY;
	Vec2D Offset;
	
	ShotPath( void );
	virtual ~ShotPath();
	void Clear( void );
	bool ECMvsTeam( uint8_t team ) const;
};
