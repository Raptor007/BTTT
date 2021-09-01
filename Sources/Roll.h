/*
 *  Roll.h
 */

#pragma once

#include "PlatformSpecific.h"

#include <stdint.h>


namespace Roll
{
	uint8_t Die( void );
	uint8_t Dice( uint8_t count = 2 );
	uint8_t Cluster( uint8_t cluster, uint8_t roll = 0 );
	uint8_t ClusterWithEvent( uint8_t cluster, uint8_t roll = 0, uint8_t damage = 0 );
}
