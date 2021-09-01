/*
 *  BattleTechServer.h
 */

#pragma once
class BattleTechServer;

#include "PlatformSpecific.h"

#include "RaptorServer.h"
#include "Event.h"
#include "HexMap.h"


class BattleTechServer : public RaptorServer
{
public:
	std::queue<Event> Events;
	std::vector<uint8_t> Initiative;
	std::queue<uint8_t> TeamTurns;
	std::map< uint8_t, std::set<Mech*> > UnmovedUnits;
	uint32_t UnitTurn;
	bool UnitCheckedMASC;
	
	BattleTechServer( std::string version );
	virtual ~BattleTechServer();
	
	void Started( void );
	void Stopped( void );
	bool ProcessPacket( Packet *packet, ConnectedClient *from_client );
	void AcceptedClient( ConnectedClient *client );
	void DroppedClient( ConnectedClient *client );
	void ChangeState( int state );
	
	void Update( double dt );
	
	bool MechTurn( const Mech *mech );
	void TookTurn( Mech *mech = NULL );
	double SendEvents( void );
	
	std::string TeamName( uint8_t team_num ) const;
	HexMap *Map( void );
	bool FF( void ) const;
};
