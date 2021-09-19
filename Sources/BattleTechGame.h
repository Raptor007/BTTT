/*
 *  BattleTechGame.h
 */

#pragma once
class BattleTechGame;

#include "PlatformSpecific.h"

#include "RaptorGame.h"
#include "HeavyMetal.h"
#include "Variant.h"
#include "Animation.h"
#include "HexMap.h"
#include "Event.h"


class BattleTechGame : public RaptorGame
{
public:
	std::map< short, HMStructure > Struct;
	std::map< short, HMWeapon > ClanWeap, ISWeap;
	std::map< std::string, Variant > Variants;
	std::map< std::string, std::map<uint8_t,const Animation*> > MechTex;
	std::map< uint32_t, bool > ReadyAndAbleCache;
	
	double X, Y, Zoom;
	uint8_t TeamTurn;
	uint32_t SelectedID, TargetID;
	std::queue< Event > Events;
	Clock EventClock;
	
	BattleTechGame( std::string version );
	virtual ~BattleTechGame();
	
	void SetDefaults( void );
	void Setup( int argc, char **argv );
	void LoadDatabases( void );
	void LoadMechTex( void );
	size_t LoadVariants( std::string dir );
	void Precache( void );
	
	void Update( double dt );
	void PlayEvent( const Event *e );
	void PlayEvent( const Event *e, double play_duration );
	
	bool HandleEvent( SDL_Event *event );
	bool HandleCommand( std::string cmd, std::vector<std::string> *params = NULL );
	bool ProcessPacket( Packet *packet );
	
	void ChangeState( int state );
	void Disconnected( void );
	
	GameObject *NewObject( uint32_t id, uint32_t type );
	
	HexMap *Map( void );
	bool Hotseat( void ) const;
	bool FF( void ) const;
	bool Admin( void );
	uint8_t MyTeam( void );
	Mech *GetMech( uint32_t mech_id );
	Mech *SelectedMech( void );
	Mech *TargetMech( void );
	std::string PhaseName( int state = -1 ) const;
};
