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
#include "RecordSheet.h"


class BattleTechGame : public RaptorGame
{
public:
	std::map< short, HMStructure > Struct;
	std::map< short, HMWeapon > ClanWeap, ISWeap;
	std::map< std::string, Variant > Variants;
	std::map< std::string, std::map<uint8_t,const Animation*> > MechTex;
	std::vector< std::pair<std::string,std::string> > Biomes;
	std::map< uint32_t, bool > ReadyAndAbleCache;
	
	double X, Y, Zoom;
	uint8_t TeamTurn;
	uint32_t SelectedID, TargetID; // FIXME: Remove TargetID?
	std::vector< uint32_t > TargetIDs;
	std::queue< Event > Events;
	Clock EventClock;
	Clock SoundClock;
	
	BattleTechGame( std::string version );
	virtual ~BattleTechGame();
	
	void SetDefaults( void );
	void Setup( int argc, char **argv );
	void LoadDatabases( void );
	void LoadMechTex( void );
	void LoadBiomes( void );
	size_t LoadVariants( std::string dir );
	bool HasMechTex( std::string name ) const;
	void Precache( void );
	
	std::map< std::string, const Variant* > VariantSearch( std::string text, bool search_equipment = true ) const;
	
	void Update( double dt );
	void PlayEvent( const Event *e );
	void PlayEvent( const Event *e, double play_duration );
	void ShowRecordSheet( const Mech *mech, double duration = 0. );
	RecordSheet *GetRecordSheet( const Mech *mech = NULL );
	
	bool HandleEvent( SDL_Event *event );
	bool HandleCommand( std::string cmd, std::vector<std::string> *params = NULL );
	void MessageReceived( std::string text, uint32_t type = TextConsole::MSG_NORMAL );
	bool ProcessPacket( Packet *packet );
	
	void ChangeState( int state );
	void Disconnected( void );
	
	GameObject *NewObject( uint32_t id, uint32_t type );
	
	HexMap *Map( void );
	bool PlayingEvents( void ) const;
	bool Hotseat( void );
	bool FF( void );
	bool Admin( void );
	bool ReadyToBegin( void );
	
	Mech *MyMech( void );
	uint8_t TeamsAlive( void ) const;
	uint8_t MyTeam( void );
	std::string TeamName( uint8_t team_num );
	bool BotControlsTeam( uint8_t team_num );
	
	Mech *GetMech( uint32_t mech_id );
	Mech *SelectedMech( void );
	Mech *TargetMech( void );
	std::vector<Mech*> TargetMechs( void );
	int TargetNum( uint32_t target_id ) const;
	void SelectMech( Mech *mech );
	void DeselectMech( void );
	void SetPrimaryTarget( uint32_t target_id = 0 );
	
	void GetPixel( const Pos3D *pos, int *x, int *y );
	std::string PhaseName( int state = -1 ) const;
};
