/*
 *  ShotPath.cpp
 */

#include "ShotPath.h"


ShotPath::ShotPath( void ) : std::vector<const Hex*>()
{
	Clear();
}


ShotPath::~ShotPath()
{
}


void ShotPath::Clear( void )
{
	clear();
	ECM.clear();
	Modifier = 0;
	Distance = 0;
	LineOfSight = true;
	PartialCover = false;
	LegWeaponsBlocked = false;
	DamageFromX = 0;
	DamageFromY = 0;
}


bool ShotPath::ECMvsTeam( uint8_t team ) const
{
	for( std::map<uint32_t,uint8_t>::const_iterator ecm = ECM.begin(); ecm != ECM.end(); ecm ++ )
	{
		if( ecm->second != team )
			return true;
	}
	return false;
}
