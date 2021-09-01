/*
 *  Roll.cpp
 */

#include "Roll.h"

#include "Rand.h"
#include "BattleTechServer.h"
#include "Num.h"


namespace Roll
{
	uint8_t DieFaces[ 6 ] = { 5, 2, 1, 6, 4, 3 };
}


uint8_t Roll::Die( void )
{
	// Compensate for tiny amount of rand modulo bias.
	return DieFaces[ Rand::Int() % 6 ];
}


uint8_t Roll::Dice( uint8_t count )
{
	uint8_t total = 0;
	for( uint8_t i = 0; i < count; i ++ )
		total += Die();
	return total;
}


uint8_t Roll::Cluster( uint8_t cluster, uint8_t roll )
{
	if( cluster < 2 )
		return 1;
	
	if( ! roll )
		roll = Dice( 2 );
	
	if( cluster == 2 )
		return (roll <= 7) ? 1: 2;
	if( cluster == 3 )
	{
		if( roll <= 4 ) return 1;
		if( roll <= 9 ) return 2;
		return cluster;
	}
	if( cluster == 4 )
	{
		if( roll <=  2 ) return 1;
		if( roll <=  6 ) return 2;
		if( roll <= 10 ) return 3;
		return cluster;
	}
	if( cluster == 5 )
	{
		if( roll <=  2 ) return 1;
		if( roll <=  4 ) return 2;
		if( roll <=  8 ) return 3;
		if( roll <= 10 ) return 4;
		return cluster;
	}
	if( cluster == 6 )
	{
		if( roll <=  3 ) return 2;
		if( roll <=  5 ) return 3;
		if( roll <=  8 ) return 4;
		if( roll <= 10 ) return 5;
		return cluster;
	}
	if( cluster == 7 )
	{
		if( roll <=  3 ) return 2;
		if( roll <=  4 ) return 3;
		if( roll <=  8 ) return 4;
		if( roll <= 10 ) return 6;
		return cluster;
	}
	if( cluster == 8 )
	{
		if( roll <=  3 ) return 3;
		if( roll <=  5 ) return 4;
		if( roll <=  8 ) return 5;
		if( roll <= 10 ) return 6;
		return cluster;
	}
	if( cluster == 9 )
	{
		if( roll <=  3 ) return 3;
		if( roll <=  4 ) return 4;
		if( roll <=  8 ) return 5;
		if( roll <= 10 ) return 7;
		return cluster;
	}
	if( cluster == 10 )
	{
		if( roll <=  3 ) return 3;
		if( roll <=  4 ) return 4;
		if( roll <=  8 ) return 6;
		if( roll <= 10 ) return 8;
		return cluster;
	}
	if( cluster == 11 )
	{
		if( roll <=  3 ) return 4;
		if( roll <=  4 ) return 5;
		if( roll <=  8 ) return 7;
		if( roll <= 10 ) return 9;
		return cluster;
	}
	if( cluster == 12 )
	{
		if( roll <=  3 ) return  4;
		if( roll <=  4 ) return  5;
		if( roll <=  8 ) return  8;
		if( roll <= 10 ) return 10;
		return cluster;
	}
	if( cluster == 13 )
	{
		if( roll <=  3 ) return  4;
		if( roll <=  4 ) return  5;
		if( roll <=  8 ) return  8;
		if( roll <= 10 ) return 11;
		return cluster;
	}
	if( cluster == 14 )
	{
		if( roll <=  3 ) return  5;
		if( roll <=  4 ) return  6;
		if( roll <=  8 ) return  9;
		if( roll <= 10 ) return 11;
		return cluster;
	}
	if( cluster == 15 )
	{
		if( roll <=  3 ) return  5;
		if( roll <=  4 ) return  6;
		if( roll <=  8 ) return  9;
		if( roll <= 10 ) return 12;
		return cluster;
	}
	if( cluster == 16 )
	{
		if( roll <=  3 ) return  5;
		if( roll <=  4 ) return  7;
		if( roll <=  8 ) return 10;
		if( roll <= 10 ) return 13;
		return cluster;
	}
	if( cluster == 17 )
	{
		if( roll <=  3 ) return  5;
		if( roll <=  4 ) return  7;
		if( roll <=  8 ) return 10;
		if( roll <= 10 ) return 14;
		return cluster;
	}
	if( cluster == 18 )
	{
		if( roll <=  3 ) return  6;
		if( roll <=  4 ) return  8;
		if( roll <=  8 ) return 11;
		if( roll <= 10 ) return 14;
		return cluster;
	}
	if( cluster == 19 )
	{
		if( roll <=  3 ) return  6;
		if( roll <=  4 ) return  8;
		if( roll <=  8 ) return 11;
		if( roll <= 10 ) return 15;
		return cluster;
	}
	if( cluster == 20 )
	{
		if( roll <=  3 ) return  6;
		if( roll <=  4 ) return  9;
		if( roll <=  8 ) return 12;
		if( roll <= 10 ) return 16;
		return cluster;
	}
	if( cluster == 21 )
	{
		if( roll <=  3 ) return  7;
		if( roll <=  4 ) return  9;
		if( roll <=  8 ) return 13;
		if( roll <= 10 ) return 17;
		return cluster;
	}
	if( cluster == 22 )
	{
		if( roll <=  3 ) return  7;
		if( roll <=  4 ) return  9;
		if( roll <=  8 ) return 14;
		if( roll <= 10 ) return 18;
		return cluster;
	}
	if( cluster == 23 )
	{
		if( roll <=  3 ) return  7;
		if( roll <=  4 ) return 10;
		if( roll <=  8 ) return 15;
		if( roll <= 10 ) return 19;
		return cluster;
	}
	if( cluster == 24 )
	{
		if( roll <=  3 ) return  8;
		if( roll <=  4 ) return 10;
		if( roll <=  8 ) return 16;
		if( roll <= 10 ) return 20;
		return cluster;
	}
	if( cluster == 25 )
	{
		if( roll <=  3 ) return  8;
		if( roll <=  4 ) return 10;
		if( roll <=  8 ) return 16;
		if( roll <= 10 ) return 21;
		return cluster;
	}
	if( cluster == 26 )
	{
		if( roll <=  3 ) return  9;
		if( roll <=  4 ) return 11;
		if( roll <=  8 ) return 17;
		if( roll <= 10 ) return 21;
		return cluster;
	}
	if( cluster == 27 )
	{
		if( roll <=  3 ) return  9;
		if( roll <=  4 ) return 11;
		if( roll <=  8 ) return 17;
		if( roll <= 10 ) return 22;
		return cluster;
	}
	if( cluster == 28 )
	{
		if( roll <=  3 ) return  9;
		if( roll <=  4 ) return 11;
		if( roll <=  8 ) return 17;
		if( roll <= 10 ) return 23;
		return cluster;
	}
	if( cluster == 29 )
	{
		if( roll <=  3 ) return 10;
		if( roll <=  4 ) return 12;
		if( roll <=  8 ) return 18;
		if( roll <= 10 ) return 23;
		return cluster;
	}
	if( cluster == 30 )
	{
		if( roll <=  3 ) return 10;
		if( roll <=  4 ) return 12;
		if( roll <=  8 ) return 18;
		if( roll <= 10 ) return 24;
		return cluster;
	}
	if( cluster == 40 )
	{
		if( roll <=  3 ) return 12;
		if( roll <=  4 ) return 18;
		if( roll <=  8 ) return 24;
		if( roll <= 10 ) return 32;
		return cluster;
	}
	
	return Rand::Int( cluster * 0.29 + 1.33, cluster );  // Should never happen, but just in case.
}


uint8_t Roll::ClusterWithEvent( uint8_t cluster, uint8_t roll, uint8_t damage )
{
	uint8_t hits = Cluster( cluster, roll );
	
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	Event e;
	e.Text = std::string("Cluster roll ") + Num::ToString((int)roll) + std::string(" in column ") + Num::ToString((int)cluster) + std::string(" gives ") + Num::ToString((int)hits) + std::string((hits == 1)?" hit":" hits");
	if( damage > 1 )
		e.Text += std::string(" (") + Num::ToString(hits * (int)damage) + std::string(" damage)");
	e.Text += std::string(".");
	server->Events.push( e );
	
	return hits;
}
