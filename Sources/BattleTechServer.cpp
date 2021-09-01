/*
 *  BattleTechServer.cpp
 */

#include "BattleTechServer.h"

#include <cstddef>
#include <cmath>
#include <stdint.h>
#include <dirent.h>
#include <list>
#include <algorithm>
#include "BattleTechDefs.h"
#include "BattleTechGame.h"
#include "Packet.h"
#include "Mech.h"
#include "Num.h"
#include "Str.h"
#include "Roll.h"


BattleTechServer::BattleTechServer( std::string version ) : RaptorServer( "BTTT", version )
{
	Port = 3050;
	AnnouncePort = 3050;
	
	UnitTurn = 0;
	UnitCheckedMASC = false;
}


BattleTechServer::~BattleTechServer()
{
}


void BattleTechServer::Started( void )
{
	State = BattleTech::State::SETUP;
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	if( ! game->Struct.size() )
		game->LoadDatabases();
	
	while( Events.size() )
		Events.pop();
	
	Data.Properties[ "teams" ] = "2";
	Data.Properties[ "team1" ] = "Wolf";
	Data.Properties[ "team2" ] = "Jade Falcon";
	Data.Properties[ "team3" ] = "Nova Cat";
	Data.Properties[ "team4" ] = "Ghost Bear";
	Data.Properties[ "team5" ] = "Smoke Jaguar";
	
	Data.Properties[ "engine_explosions" ] = "true";
	Data.Properties[ "prone_1arm" ] = "true";
	Data.Properties[ "enhanced_flamers" ] = "true";
	Data.Properties[ "enhanced_ams" ] = "true";
	Data.Properties[ "floating_crits" ] = "false";
	Data.Properties[ "skip_tag" ] = "false";
	Data.Properties[ "tag" ] = "false";
	Data.Properties[ "mech_limit" ] = "1";
	Data.Properties[ "hotseat" ] = "false";
	
	HexMap *map = new HexMap();
	map->Randomize();
	Data.AddObject( map );
	Console->Print( "Map seed: " + Num::ToString( (int) map->Seed ) );
	
	// The Windows random number generator was making me superstitious.
	for( size_t throwaway_rolls = 1 + (time(NULL) % 4); throwaway_rolls; throwaway_rolls -- )
		Roll::Die();
}


void BattleTechServer::Stopped( void )
{
	Data.Clear();
}


bool BattleTechServer::ProcessPacket( Packet *packet, ConnectedClient *from_client )
{
	packet->Rewind();
	PacketType type = packet->Type();
	
	if( type == BattleTech::Packet::SPAWN_MECH )
	{
		// Ignore packets that arrive during the wrong phase.
		if( (State != BattleTech::State::SETUP)
		&&  (State != BattleTech::State::END) )
			return true;
		
		uint8_t x = packet->NextUChar();
		uint8_t y = packet->NextUChar();
		uint8_t facing = packet->NextUChar();
		
		Variant v;
		v.ReadFromPacket( packet );
		
		Mech *m = new Mech();
		
		m->PlayerID = from_client->PlayerID;
		Player *p = Data.GetPlayer( m->PlayerID );
		m->Team = p ? atoi( p->Properties["team"].c_str() ) : 0;
		
		m->SetVariant( v );
		m->X = x;
		m->Y = y;
		m->Facing = facing;
		
		m->PilotSkill   = packet->NextUChar();
		m->GunnerySkill = packet->NextUChar();
		
		Data.AddObject( m );
		
		Packet objects_add = Packet( Raptor::Packet::OBJECTS_ADD );
		objects_add.AddUInt( 1 );
		objects_add.AddUInt( m->ID );
		objects_add.AddUInt( m->Type() );
		m->AddToInitPacket( &objects_add );
		Net.SendAll( &objects_add );
		
		size_t mech_limit = atoi(Data.Properties["mech_limit"].c_str());
		if( mech_limit )
		{
			bool hotseat = Str::AsBool(Data.Properties["hotseat"]);
			std::map<double,Mech*> mechs;
			for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
			{
				if( obj_iter->second->Type() == BattleTech::Object::MECH )
				{
					Mech *mech = (Mech*) obj_iter->second;
					if( hotseat ? (mech->Team == m->Team) : (mech->PlayerID == from_client->PlayerID) )
						mechs[ mech->Lifetime.ElapsedSeconds() * -1. ] = mech;
				}
			}
			
			std::set<Mech*> remove;
			while( mechs.size() > mech_limit )
			{
				std::map<double,Mech*>::iterator oldest = mechs.begin();
				remove.insert( oldest->second );
				mechs.erase( oldest );
			}
			
			if( remove.size() )
			{
				Packet objects_remove = Packet( Raptor::Packet::OBJECTS_REMOVE );
				objects_remove.AddUInt( remove.size() );
				for( std::set<Mech*>::iterator mech_iter = remove.begin(); mech_iter != remove.end(); mech_iter ++ )
					objects_remove.AddUInt( (*mech_iter)->ID );
				Net.SendAll( &objects_remove );
				
				for( std::set<Mech*>::iterator mech_iter = remove.begin(); mech_iter != remove.end(); mech_iter ++ )
					Data.RemoveObject( (*mech_iter)->ID );
			}
		}
	}
	else if( type == BattleTech::Packet::REMOVE_MECH )
	{
		uint32_t mech_id = packet->NextUInt();
		
		Mech *mech = (Mech*) Data.GetObject( mech_id );
		if( mech && (mech->Type() != BattleTech::Object::MECH) )
			mech = NULL;
		
		if( ! mech )
			return true;
		
		if( (State == BattleTech::State::SETUP) || (State == BattleTech::State::GAME_OVER) )
		{
			Packet objects_remove = Packet( Raptor::Packet::OBJECTS_REMOVE );
			objects_remove.AddUInt( 1 );
			objects_remove.AddUInt( mech->ID );
			Net.SendAll( &objects_remove );
			
			Data.RemoveObject( mech->ID );
			
			if( (State == BattleTech::State::GAME_OVER) && (Data.GameObjects.size() <= 1) )
				ChangeState( BattleTech::State::SETUP );
		}
		else if( MechTurn( mech ) )
		{
			Event e( mech );
			e.Effect = BattleTech::Effect::BLINK;
			e.Sound = "b_eject.wav";
			e.Duration = 0.5f;
			e.Text = mech->ShortFullName() + std::string(" ejected!");
			Events.push( e );
			
			if( mech->Cockpit )
				mech->Cockpit->Hit();
			
			if( State == BattleTech::State::MOVEMENT )
				SendEvents();
			
			TookTurn( mech );
		}
	}
	else if( type == BattleTech::Packet::CHANGE_MAP )
	{
		uint16_t w = packet->NextUShort();
		uint16_t h = packet->NextUShort();
		uint32_t seed = packet->NextUInt();
		
		if( State != BattleTech::State::SETUP )
			return true;
		
		HexMap *map = Map();
		if( map )
		{
			if( ! w )
				w = map->Hexes.size();
			if( ! h )
				h = map->Hexes[ 0 ].size();
			
			map->SetSize( w, h );
			map->Randomize( seed );
			Console->Print( "Map seed: " + Num::ToString( (int) map->Seed ) );
			
			std::set<Mech*> moved;
			for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
			{
				if( obj_iter->second->Type() == BattleTech::Object::MECH )
				{
					Mech *mech = (Mech*) obj_iter->second;
					if( mech->X >= map->Hexes.size() )
					{
						mech->X = map->Hexes.size();
						moved.insert( mech );
					}
					if( mech->Y >= map->Hexes[ mech->X ].size() )
					{
						mech->Y = map->Hexes[ mech->X ].size();
						moved.insert( mech );
					}
				}
			}
			// FIXME: Make sure moved Mechs are not positioned on top of other Mechs.
			
			Packet update = Packet( Raptor::Packet::UPDATE );
			update.AddChar( 127 );              // Precision
			update.AddUInt( moved.size() + 1 ); // Number of Objects
			update.AddUInt( map->ID );
			map->AddToUpdatePacketFromServer( &update, 127 );
			for( std::set<Mech*>::iterator mech_iter = moved.begin(); mech_iter != moved.end(); mech_iter ++ )
			{
				update.AddUInt( (*mech_iter)->ID );
				(*mech_iter)->AddToUpdatePacketFromServer( &update, 127 );
			}
			Net.SendAll( &update );
		}
	}
	else if( type == BattleTech::Packet::READY )
	{
		if( State == BattleTech::State::SETUP )
			// FIXME: Keep track ready state for each player, and begin when all are ready.
			ChangeState( BattleTech::State::INITIATIVE );
		else if( State == BattleTech::State::GAME_OVER )
			ChangeState( BattleTech::State::SETUP );
	}
	else if( type == BattleTech::Packet::MOVEMENT )
	{
		// Ignore packets that arrive during the wrong phase.
		if( (State != BattleTech::State::MOVEMENT)
		&&  (State != BattleTech::State::SETUP) )
			return true;
		
		uint32_t mech_id = packet->NextUInt();
		
		Mech *mech = (Mech*) Data.GetObject( mech_id );
		if( mech && (mech->Type() != BattleTech::Object::MECH) )
			mech = NULL;
		
		// Ignore packets when it isn't this Mech's turn.
		if( (State != BattleTech::State::SETUP) && ! MechTurn( mech ) )
			return true;
		
		mech->Attack = 0;
		mech->Defense = 0;
		
		uint8_t speed = packet->NextUChar();
		
		// If they failed a MASC roll this turn, their speed is still MASC.
		if( UnitCheckedMASC )
			speed = BattleTech::Move::MASC;
		
		if( State == BattleTech::State::SETUP )
			;
		else if( speed == BattleTech::Move::STOP )
		{
			Event e( mech );
			e.Effect = BattleTech::Effect::BLINK;
			e.Text = mech->ShortFullName() + std::string(" is not moving.");
			Events.push( e );
		}
		else if( speed == BattleTech::Move::WALK )
		{
			Event e( mech );
			e.Effect = BattleTech::Effect::BLINK;
			e.Sound = "m_walk.wav";
			e.Text = mech->ShortFullName() + std::string(" is walking.");
			Events.push( e );
			
			mech->Heat += 1;
			mech->Attack = 1;
		}
		else if( speed == BattleTech::Move::RUN )
		{
			Event e( mech );
			e.Effect = BattleTech::Effect::BLINK;
			e.Sound = "m_run.wav";
			e.Text = mech->ShortFullName() + std::string(" is running.");
			Events.push( e );
			
			mech->Heat += 2;
			mech->Attack = 2;
		}
		else if( speed == BattleTech::Move::MASC )
		{
			if( ! UnitCheckedMASC )
			{
				mech->MASCTurns ++;
				UnitCheckedMASC = true;
				
				uint8_t masc_need = 3;
				if( mech->MASCTurns >= 5 )
					masc_need = 99;
				else if( mech->MASCTurns >= 4 )
					masc_need = 11;
				else if( mech->MASCTurns >= 3 )
					masc_need = 7;
				else if( mech->MASCTurns >= 2 )
					masc_need = 5;
				uint8_t masc_roll = Roll::Dice( 2 );
				
				Event e( mech );
				e.Effect = BattleTech::Effect::BLINK;
				if( masc_roll < masc_need )
				{
					e.Misc = BattleTech::RGB332::DARK_RED;
					e.Duration = 1.f;
				}
				e.Sound = "m_run.wav";
				e.Text = mech->ShortFullName() + std::string(" is using MASC.  Must roll ") + Num::ToString((int)masc_need)
					+ std::string(" to avoid damage.  Rolled ") + Num::ToString((int)masc_roll) + std::string( (masc_roll >= masc_need) ? ": PASSED!" : ": FAILED!" );
				Events.push( e );
				
				if( masc_roll < masc_need )
				{
					uint8_t masc_dist = mech->MASCDist();
					
					for( uint8_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
					{
						if( mech->Locations[ i ].IsLeg )
							mech->Locations[ i ].CriticalHits( 1 );
					}
					
					// Immediately roll any PSRs caused by MASC damage.
					for( uint8_t i = 0; i < mech->PSRs; i ++ )
					{
						if( mech->Prone || mech->Destroyed() )
							break;
						mech->PilotSkillCheck();
						mech->AutoFall = "";
					}
					mech->PSRs = 0;
					
					SendEvents();
					
					if( mech->Destroyed() || mech->Unconscious )
					{
						TookTurn( mech );
						return true;
					}
					else if( mech->Prone || (mech->MASCDist() < masc_dist) )
					{
						UnitTurn = mech->ID;
						
						Packet update = Packet( Raptor::Packet::UPDATE );
						update.AddChar( -127 ); // Precision
						update.AddUInt( 1 );    // Number of Objects
						update.AddUInt( mech->ID );
						mech->AddToUpdatePacketFromServer( &update, -127 );
						Net.SendAll( &update );
						
						return true;
					}
				}
				
				mech->Heat += 2;
			}
			else
			{
				Event e( mech );
				e.Effect = BattleTech::Effect::BLINK;
				e.Sound = "m_run.wav";
				e.Text = mech->ShortFullName() + std::string(" is using MASC." );
				Events.push( e );
			}
			
			mech->Attack = 2;
		}
		
		if( mech->MASCTurns && (speed != BattleTech::Move::MASC) )
			mech->MASCTurns --;
		
		const HexMap *map = Map();
		uint8_t entered = 0;
		uint8_t old_x = mech->X, old_y = mech->Y;
		bool reversing = false;
		
		uint8_t steps = packet->NextUChar();
		
		for( uint8_t i = 0; i < steps; i ++ )
		{
			uint8_t move = packet->NextUChar();
			
			if( (move == BattleTech::Move::WALK)
			||  (move == BattleTech::Move::RUN)
			||  (move == BattleTech::Move::MASC)
			||  (move == BattleTech::Move::REVERSE) )
			{
				std::map<uint8_t,const Hex*> adjacent = map->Adjacent_const( mech->X, mech->Y );
				uint8_t dir = (move != BattleTech::Move::REVERSE) ? mech->Facing : (mech->Facing + 3) % 6;
				mech->X = adjacent[ dir ]->X;
				mech->Y = adjacent[ dir ]->Y;
				
				// Count entered hexes (last leg of movement if alternating forward/reverse).
				bool step_reverse = (move == BattleTech::Move::REVERSE);
				if( reversing == step_reverse )
					entered ++;
				else
				{
					entered = 1;
					reversing = step_reverse;
				}
				
				if( speed != BattleTech::Move::JUMP )
				{
					Event e( mech );
					e.Effect = BattleTech::Effect::STEP;
					e.ShowFacing( mech->Facing );
					if( State == BattleTech::State::SETUP )
						e.Duration = 0.f;
					else
					{
						e.Sound = "m_step.wav";
						e.Duration = (i == steps - 1) ? 0.2f : 0.5f;
					}
					Events.push( e );
				}
			}
			else if( (move == BattleTech::Move::LEFT)
			||       (move == BattleTech::Move::RIGHT) )
			{
				mech->Facing += (move == BattleTech::Move::LEFT) ? 5 : 1;
				mech->Facing %= 6;
				
				if( speed != BattleTech::Move::JUMP )
				{
					Event e( mech );
					e.Effect = BattleTech::Effect::TURN;
					e.ShowFacing( mech->Facing, mech->Prone );
					if( State == BattleTech::State::SETUP )
						e.Duration = 0.f;
					else
					{
						e.Sound = "m_turn.wav";
						e.Duration = (i == steps - 1) ? 0.2f : 0.5f;
					}
					Events.push( e );
				}
			}
		}
		
		if( speed == BattleTech::Move::JUMP )
		{
			Event e( mech );
			e.Effect = BattleTech::Effect::JUMP;
			if( State == BattleTech::State::SETUP )
			{
				e.ShowFacing( mech );
				e.Duration = 0.f;
			}
			else
			{
				e.ShowFacing( mech->Facing );
				e.Sound = "m_jump.wav";
				e.Duration = 2.f;
				e.Text = mech->ShortFullName() + std::string(" is jumping.");
			}
			Events.push( e );
			
			mech->Heat += std::max<uint8_t>( 3, map->JumpCost( old_x, old_y, mech->X, mech->Y ) );
			mech->Attack = 3;
			entered = map->HexDist( old_x, old_y, mech->X, mech->Y );
		}
		
		if( entered >= 25 )
			mech->Defense = 6;
		else if( entered >= 18 )
			mech->Defense = 5;
		else if( entered >= 10 )
			mech->Defense = 4;
		else if( entered >= 7 )
			mech->Defense = 3;
		else if( entered >= 5 )
			mech->Defense = 2;
		else if( entered >= 3 )
			mech->Defense = 1;
		
		if( speed == BattleTech::Move::JUMP )
			mech->Defense ++;
		
		if( State != BattleTech::State::SETUP )
		{
			if( mech->ReadyAndAble() )
			{
				Event e( mech );
				e.Sound = (speed == BattleTech::Move::JUMP) ? "m_land.wav" : "m_stop.wav";
				e.Duration = 0.5f;
				e.Text = mech->ShortName() + std::string(" entered ") + Num::ToString((int)entered) + std::string(" hexes for defense +")
					+ Num::ToString((int)(mech->Defense)) + std::string(" and attack +") + Num::ToString((int)(mech->Attack)) + std::string(".");
				Events.push( e );
				
				if( (speed == BattleTech::Move::RUN)
				||  (speed == BattleTech::Move::MASC)
				||  (speed == BattleTech::Move::JUMP) )
				{
					if( mech->Gyro && mech->Gyro->Damaged )
						mech->PSRs ++;
					mech->PSRs += mech->DamagedCriticalCount( BattleTech::Equipment::HIP );
				}
				
				if( speed == BattleTech::Move::JUMP )
				{
					for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
					{
						if( mech->Locations[ i ].IntactCriticalCount( BattleTech::Equipment::HIP ) )
						{
							mech->PSRs += mech->Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::UPPER_LEG_ACTUATOR );
							mech->PSRs += mech->Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::LOWER_LEG_ACTUATOR );
							mech->PSRs += mech->Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::FOOT_ACTUATOR );
						}
					}
				}
				
				// Immediately roll any PSRs caused by running/jumping.
				for( uint8_t i = 0; i < mech->PSRs; i ++ )
				{
					if( mech->Prone || mech->Destroyed() )
						break;
					mech->PilotSkillCheck();
					mech->AutoFall = "";
				}
				mech->PSRs = 0;
			}
			
			SendEvents();
			TookTurn( mech );
		}
		else
		{
			// Moving during Setup should not generate heat or attack/defense bonus.
			mech->Heat = 0;
			mech->Attack = 0;
			mech->Defense = 0;
		}
		
		SendEvents();
	}
	else if( type == BattleTech::Packet::STAND )
	{
		// Ignore packets that arrive during the wrong phase.
		if( (State != BattleTech::State::MOVEMENT)
		&&  (State != BattleTech::State::SETUP) )
			return true;
		
		uint32_t mech_id = packet->NextUInt();
		
		Mech *mech = (Mech*) Data.GetObject( mech_id );
		if( mech && (mech->Type() != BattleTech::Object::MECH) )
			mech = NULL;
		
		// Ignore packets when it isn't this Mech's turn.
		if( (State != BattleTech::State::SETUP) && ! MechTurn( mech ) )
			return true;
		
		// Make sure it has a leg to stand on.
		if( ! mech->CanStand() )
			return true;
		
		Event e( mech );
		e.Effect = BattleTech::Effect::STAND;
		e.ShowFacing( mech->Facing );
		e.Sound = "m_stand.wav";
		if( State == BattleTech::State::SETUP )
			e.Duration = 0.f;
		else
		{
			e.Text = mech->ShortFullName() + std::string(" stands up.");
			e.Duration = 0.5f;
		}
		Events.push( e );
		
		if( State == BattleTech::State::SETUP )
		{
			mech->Prone = false;
			SendEvents();
		}
		else
		{
			// Each attempt to stand generates 1 heat.
			mech->Heat ++;
			
			if( mech->PilotSkillCheck() )
				mech->Prone = false;
			
			SendEvents();
			
			if( mech->Destroyed() || mech->Unconscious )
			{
				TookTurn( mech );
				return true;
			}
			
			// This 'Mech must now complete its turn before anyone else can move.
			UnitTurn = mech->ID;
		}
	}
	else if( type == BattleTech::Packet::SHOTS )
	{
		// Ignore packets that arrive during the wrong phase.
		if( (State != BattleTech::State::WEAPON_ATTACK)
		&&  (State != BattleTech::State::TAG) )
			return true;
		
		uint32_t from_id = packet->NextUInt();
		
		Mech *from = (Mech*) Data.GetObject( from_id );
		if( from && (from->Type() != BattleTech::Object::MECH) )
			from = NULL;
		
		// Ignore packets when it isn't this Mech's turn.
		if( ! MechTurn( from ) )
			return true;
		
		bool enhanced_flamers = Str::AsBool(Data.Properties["enhanced_flamers"]);
		
		from->TorsoTwist = packet->NextChar();
		from->TurnedArm  = packet->NextChar();
		from->ProneFire  = packet->NextChar();
		
		uint8_t count_and_flags = packet->NextUChar();
		uint8_t count     = count_and_flags & 0x7F;
		from->ArmsFlipped = count_and_flags & 0x80;
		
		Event e( from );
		e.Effect = BattleTech::Effect::BLINK;
		if( State != BattleTech::State::TAG )
		{
			e.Text = from->ShortFullName() + std::string(" has declared ") + Num::ToString((int)count) + std::string(" weapons to fire.");
			Packet events( BattleTech::Packet::EVENTS );
			events.AddUShort( 1 );
			e.AddToPacket( &events );
			Net.SendAll( &events );
		}
		
		if( State == BattleTech::State::TAG )
			e.Text = from->ShortFullName() + std::string(count?" is using TAG.":" is not using TAG.");
		else
			e.Text = from->ShortFullName() + std::string(" is firing ") + Num::ToString((int)count) + std::string(" weapons.");
		Events.push( e );
		
		Event torso( from );
		torso.Effect = BattleTech::Effect::TORSO_TWIST;
		torso.ShowFacing( from );
		if( from->TorsoTwist || (from->TurnedArm != BattleTech::Loc::UNKNOWN) || from->ArmsFlipped || (from->ProneFire != BattleTech::Loc::UNKNOWN) )
		{
			torso.Sound = "m_twist.wav";
			torso.Duration = 0.4f;
		}
		else
			torso.Duration = 0.f;
		Events.push( torso );
		
		for( uint8_t i = 0; i < count; i ++ )
		{
			uint32_t target_id = packet->NextUInt();
			
			Mech *target = (Mech*) Data.GetObject( target_id );
			if( target && (target->Type() != BattleTech::Object::MECH) )
				target = NULL;
			
			uint8_t arc_and_flags = packet->NextUChar();
			
			uint8_t hit_arc = arc_and_flags & 0x03; // 2-bit location arc (values 0-3).
			bool indirect   = arc_and_flags & 0x80;
			bool leg_cover  = arc_and_flags & 0x40;
			bool ecm        = arc_and_flags & 0x20;
			bool narc       = arc_and_flags & 0x10;
			bool fcs        = arc_and_flags & 0x08;
			
			// FIXME: This only checks if the target has Angel ECM, not nearby friendlies.
			bool angel_ecm = false;
			if( ecm && target && target->ECM && ! target->ECM->Damaged && ! target->Shutdown )
				angel_ecm = strstr( target->ECM->Name.c_str(), "Angel" );
			
			uint8_t eq_index   = packet->NextUChar();
			uint8_t fire_count = packet->NextUChar();
			int8_t  difficulty = packet->NextChar();
			
			if( (! from) || (! target) || (eq_index >= from->Equipment.size()) )
				continue;
			MechEquipment *eq = &(from->Equipment[ eq_index ]);
			if( (! eq) || (! eq->Weapon) )
				continue;
			
			if( fire_count && eq->Weapon->AmmoPerTon )
			{
				uint16_t total_ammo = from->TotalAmmo( eq->ID );
				if( total_ammo < fire_count )
					fire_count = total_ammo;
				if( ! fire_count )
					continue;
			}
			
			eq->Fired += fire_count;
			
			uint8_t jam_on = 0;
			if( (eq->Weapon->SnakeEyesEffect & 0x01) && (fire_count > 1) )
			{
				jam_on = 2;
				if( fire_count >= 4 )
					jam_on += (fire_count - 2) / 2;
			}
			
			// FIXME: This is accurate for Hypervelocity AC but not Caseless AC.
			uint8_t explode_on = (eq->Weapon->SnakeEyesEffect & 0x02) ? 2 : 0;
			
			uint8_t hit_roll = Roll::Dice( 2 );
			
			if( (hit_roll < difficulty) && (eq->Weapon->Type == HMWeapon::MISSILE_STREAK) )
				fire_count = 0;
			
			Event e( from, target );
			e.Misc = (eq->Weapon->ClusterWeaponSize > 1) ? eq->Weapon->ClusterWeaponSize : fire_count;
			if( hit_roll < difficulty )
				e.Misc |= 0x80; // Missed
			if( target->Destroyed() )
			{
				e.Text = eq->Weapon->Name + std::string(" fired at destroyed Mech.");
				e.Duration = 0.3f;
			}
			else
			{
				e.Text = eq->Weapon->Name + std::string(" needs ") + Num::ToString((int)difficulty) + std::string(" to hit");
				if( difficulty <= 2 )
					e.Text += ", which is an automatic HIT!";
				else
				{
					e.Text += std::string(", rolled ") + Num::ToString((int)hit_roll)
						+ ((hit_roll >= difficulty) ? std::string(": HIT!") : std::string(": MISS."));
				}
			}
			if( fire_count )
			{
				e.Sound = eq->Weapon->Sound;
				e.Effect = eq->Weapon->Effect;
				e.Loc = eq->Location->Loc;
			}
			else if( eq->Weapon->Type == HMWeapon::MISSILE_STREAK )
				e.Text += "  Streak held fire instead of missing.";
			Events.push( e );
			
			if( eq->Weapon->AmmoPerTon )
			{
				for( uint8_t j = 0; j < fire_count; j ++ )
					from->SpendAmmo( eq->ID );
			}
			else if( eq->Weapon->Type == HMWeapon::ONE_SHOT )
				eq->Jammed = true;
			
			if( (hit_roll >= difficulty) && ! target->Destroyed() )
			{
				int hits = 1;
				uint8_t cluster = 0;
				int8_t cluster_roll = 0;
				
				MechEquipment *ams = NULL;
				if( (eq->Weapon->Type == HMWeapon::MISSILE)
				||  (eq->Weapon->Type == HMWeapon::MISSILE_3D6)
				||  (eq->Weapon->Type == HMWeapon::MISSILE_STREAK) )
				{
					uint8_t ams_arc = target->FiringArc( from->X, from->Y, false );  // FIXME: This should probably use torso twist, but that data depends on turn order.
					ams = target->AvailableAMS( ams_arc );
				}
				
				if( fire_count > 1 )
				{
					cluster = fire_count;
					cluster_roll = Roll::Dice( 2 );
					hits = Roll::ClusterWithEvent( cluster, cluster_roll );
				}
				else if( eq->Weapon->ClusterWeaponSize > 1 )
				{
					cluster = eq->Weapon->ClusterWeaponSize;
					
					if( (eq->Weapon->Type == HMWeapon::MISSILE_STREAK) && angel_ecm )
					{
						Event e( target );
						e.Effect = BattleTech::Effect::BLINK;
						e.Text = "Angel ECM Suite forces Streak to roll on cluster hits table.";
						Events.push( e );
					}
					
					if( (eq->Weapon->Type == HMWeapon::MISSILE_STREAK) && ! angel_ecm )
					{
						hits = cluster;
						
						if( ams )
						{
							Event e( target );
							e.Effect = BattleTech::Effect::BLINK;
							e.Text = ams->Name + std::string(" reduces Streak to cluster roll 7.");
							Events.push( e );
							
							hits = Roll::ClusterWithEvent( cluster, 7, eq->Weapon->Damage );
						}
					}
					else
					{
						cluster_roll = Roll::Dice( 2 );  // FIXME: (eq->Weapon->Type == HMWeapon::MISSILE_3D6) means what?
						hits = eq->ClusterHits( cluster_roll, fcs, narc, ecm, indirect, ams );
					}
				}
				else if( ams )
				{
					uint8_t ams_roll = Roll::Die();
					hits = (ams_roll >= 4) ? 1 : 0;
					
					Event e( target );
					e.Effect = BattleTech::Effect::BLINK;
					e.Text = ams->Name + std::string(" shoots down missile on 1-3, rolled ") + Num::ToString((int)ams_roll) + std::string(hits?": MISSILE HITS!":" SHOT DOWN!");
					Events.push( e );
				}
				
				if( ams )
				{
					ams->Fired ++;
					if( ams->Weapon->AmmoPerTon )
						target->SpendAmmo( ams->ID );
				}
				
				int total_damage = hits * eq->Weapon->Damage;
				
				// Special case for different damage by range (Heavy Gauss Rifle, etc).
				if( Str::Count( eq->Weapon->DamageStr, "/" ) == 2 )
				{
					HexMap *map = Map();
					uint8_t hex_dist = map->HexDist( from->X, from->Y, target->X, target->Y );
					if( hex_dist > eq->Weapon->RangeMedium )
						total_damage = hits * atoi( strrchr( eq->Weapon->DamageStr.c_str(), '/' ) + 1 );
					else if( hex_dist > eq->Weapon->RangeShort )
						total_damage = hits * atoi( strchr( eq->Weapon->DamageStr.c_str(), '/' ) + 1 );
				}
				
				while( hits || (total_damage > 0) )
				{
					hits = 0;  // Make sure we get into this scope once for non-damaging weapons.
					
					int damage = eq->Weapon->ClusterDamageGroup;
					if( (damage < 1) || (damage > total_damage) )
						damage = total_damage;
					total_damage -= damage;
					
					std::string damage_str = Num::ToString((int)damage) + std::string(" damage");
					
					if( eq->Weapon->Flamer && enhanced_flamers )
					{
						damage = eq->Weapon->Flamer;
						damage_str = Num::ToString((int)damage) + std::string(" heat and damage");
					}
					
					if( eq->Weapon->Narc )
						damage_str = "Narc Homing Pod";
					
					if( damage || eq->Weapon->Narc )
					{
						MechLocationRoll lr = target->LocationRoll( hit_arc, BattleTech::HitTable::STANDARD, leg_cover );
						
						if( leg_cover && lr.Loc->IsLeg )
						{
							Event e( target );
							e.Text = lr.String("would hit") + std::string(" for ") + damage_str + std::string(", but partial cover blocked it.");
							Events.push( e );
							
							continue;
						}
						
						Event e( target );
						e.Effect = BattleTech::Effect::EXPLOSION;
						e.Misc = eq->Weapon->Effect;
						e.Sound = eq->Weapon->HitSound;
						e.Text = lr.String() + std::string(" for ") + damage_str + std::string(".");
						e.Loc = lr.Loc->Loc;
						e.Arc = hit_arc;
						Events.push( e );
						
						if( damage )
							lr.Loc->Damage( damage, hit_arc, lr.Crit );
						
						if( target->Destroyed() )
							break;
						
						if( eq->Weapon->Narc )
							lr.Loc->Narced = true;
					}
					else if( eq->Weapon->HitSound.length() )
					{
						Event e( target );
						e.Effect = BattleTech::Effect::EXPLOSION;
						e.Misc = eq->Weapon->Effect;
						if( eq->Weapon->TAG )
						{
							e.Effect = BattleTech::Effect::BLINK;
							e.Misc = BattleTech::RGB332::YELLOW;
						}
						e.Sound = eq->Weapon->HitSound;
						if( eq->Weapon->Flamer )
							e.Text = std::string("Adding ") + Num::ToString((int)(eq->Weapon->Flamer)) + std::string(" heat to target.");
						Events.push( e );
					}
					
					if( eq->Weapon->Flamer )
						target->OutsideHeat += eq->Weapon->Flamer;
					
					if( eq->Weapon->TAG )
					{
						target->Tagged = true;
						
						const HexMap *map = Map();
						ShotPath path = map->Path( from->X, from->Y, target->X, target->Y );
						int target_dist = map->HexDist( from->X, from->Y, target->X, target->Y );
						
						int8_t spotted = from->Attack + path.Modifier + 1;
						if( from->Sensors )
							spotted += from->Sensors->Damaged * 2;
						if( target->Prone )
							spotted += (target_dist <= 1) ? -2 : 1;
						
						int8_t trees = path.PartialCover ? (path.Modifier - 1) : path.Modifier;
						if( trees && (! ecm) && (target_dist <= from->ActiveProbeRange()) )
							spotted --;
						
						if( spotted < target->Spotted )
							target->Spotted = spotted;
						
						Packet spot( BattleTech::Packet::SPOT );
						spot.AddUInt( target->ID );
						spot.AddChar( target->Spotted );
						spot.AddUChar( eq_index | 0x80 );
						spot.AddUInt( from->ID );
						Net.SendAll( &spot );
					}
				}
			}
			
			if( hit_roll <= jam_on )
			{
				Event e( from );
				e.Sound = "w_jam.wav";
				e.Text = eq->Weapon->Name + std::string(" is jammed!");
				Events.push( e );
				
				eq->Jammed = true;
				
				// Rotary AC explodes one round of ammo if critical hit while jammed.
				if( eq->Weapon->RapidFire > 2 )
					eq->ExplosionDamage = eq->Weapon->Damage;
			}
			
			if( hit_roll <= explode_on )
			{
				Event e( from );
				e.Sound = "b_wepdes.wav";
				e.Text = eq->Weapon->Name + std::string(" explodes!");
				e.ShowCritHit( eq );
				Events.push( e );
				
				eq->Hit();
			}
		}
		
		// See if we're also spotting for indirect fire.
		uint32_t spot_target = packet->NextUInt();
		int8_t spot_modifier = packet->NextChar();
		Mech *target = (Mech*) Data.GetObject( spot_target );
		if( target && (target->Type() != BattleTech::Object::MECH) )
			target = NULL;
		if( target && (spot_modifier < target->Spotted) )
		{
			target->Spotted = spot_modifier;
			
			Packet spot( BattleTech::Packet::SPOT );
			spot.AddUInt( target->ID );
			spot.AddChar( target->Spotted );
			spot.AddUChar( target->Tagged ? 0xFF : 0 );
			spot.AddUInt( from->ID );
			Net.SendAll( &spot );
			
			if( ! target->Destroyed() )
			{
				Event e( target );
				/*
				e.Effect = BattleTech::Effect::EXPLOSION;
				e.Misc = BattleTech::Effect::TAG;
				*/
				e.Effect = BattleTech::Effect::BLINK;
				e.Misc = BattleTech::RGB332::ORANGE;
				e.Sound = "w_lock.wav";
				e.Text = std::string("Spotted ") + target->ShortName() + std::string(" for indirect fire (+") + Num::ToString(spot_modifier) + std::string(").");
				Events.push( e );
			}
		}
		
		if( State == BattleTech::State::TAG )
			SendEvents();
		
		TookTurn( from );
	}
	else if( type == BattleTech::Packet::MELEE )
	{
		// Ignore packets that arrive during the wrong phase.
		if( State != BattleTech::State::PHYSICAL_ATTACK )
			return true;
		
		uint32_t from_id = packet->NextUInt();
		
		Mech *from = (Mech*) Data.GetObject( from_id );
		if( from && (from->Type() != BattleTech::Object::MECH) )
			from = NULL;
		
		// Ignore packets when it isn't this Mech's turn.
		if( ! MechTurn( from ) )
			return true;
		
		uint8_t count = packet->NextUChar();
		
		Event e( from );
		e.Effect = BattleTech::Effect::BLINK;
		if( count >= 2 )
			e.Text = from->ShortFullName() + std::string(" has declared 2 physical attacks.");
		else if( count == 1 )
			e.Text = from->ShortFullName() + std::string(" has declared 1 physical attack.");
		else
			e.Text = from->ShortFullName() + std::string(" is not attacking.");
		Packet events( BattleTech::Packet::EVENTS );
		events.AddUShort( 1 );
		e.AddToPacket( &events );
		Net.SendAll( &events );
		
		Event torso( from );
		torso.Effect = BattleTech::Effect::TORSO_TWIST;
		torso.ShowFacing( from );
		torso.Duration = 0.f;
		Events.push( torso );
		
		for( uint8_t i = 0; i < count; i ++ )
		{
			Mech *target = NULL;
			uint32_t target_id = packet->NextUInt();
			target = (Mech*) Data.GetObject( target_id );
			if( target && (target->Type() != BattleTech::Object::MECH) )
				target = NULL;
			
			uint8_t arc_and_table = packet->NextUChar();
			uint8_t hit_arc   =  arc_and_table & 0x03;
			uint8_t hit_table = (arc_and_table & 0x0C) >> 2;
			
			uint8_t melee = packet->NextUChar();
			int8_t difficulty = packet->NextChar();
			uint8_t damage = packet->NextUChar();
			uint8_t limb = packet->NextUChar();
			
			if( ! target )
				break;
			
			e.Sound = "w_melee.wav";
			e.Text = from->ShortFullName();
			if( melee == BattleTech::Melee::KICK )
				e.Text += " is kicking ";
			else if( melee == BattleTech::Melee::PUNCH )
				e.Text += " is punching ";
			else if( melee == BattleTech::Melee::PUSH )
				e.Text += " is pushing ";
			else if( melee == BattleTech::Melee::CHARGE )
				e.Text += " is charging ";
			else if( melee == BattleTech::Melee::DFA )
				e.Text += " is DFA'ing ";
			else if( melee == BattleTech::Melee::CLUB )
				e.Text += " is clubbing ";
			else if( melee == BattleTech::Melee::HATCHET )
				e.Text += " is swinging Hatchet at ";
			else if( melee == BattleTech::Melee::SWORD )
				e.Text += " is swinging Sword at ";
			else
				e.Text += " is resolving physical attack against ";
			e.Text += target->ShortName() + std::string(", need ") + Num::ToString((int)difficulty);
			bool hit = true;
			if( difficulty > 2 )
			{
				uint8_t roll = Roll::Dice( 2 );
				hit = (roll >= difficulty);
				e.Text += std::string(" and rolled ") + Num::ToString((int)roll) + std::string(hit?": HIT!":": MISS!");
			}
			else
				e.Text += " which is an automatic HIT!";
			if( limb == BattleTech::Loc::LEFT_ARM )
				e.Effect = BattleTech::Effect::LEFT_PUNCH;
			else if( limb == BattleTech::Loc::RIGHT_ARM )
				e.Effect = BattleTech::Effect::RIGHT_PUNCH;
			else if( limb == BattleTech::Loc::LEFT_LEG )
				e.Effect = BattleTech::Effect::LEFT_KICK;
			else if( limb == BattleTech::Loc::RIGHT_LEG )
				e.Effect = BattleTech::Effect::RIGHT_KICK;
			else
				e.Effect = BattleTech::Effect::BLINK;
			Events.push( e );
			
			if( hit )
			{
				MechLocationRoll lr = target->LocationRoll( hit_arc, hit_table );
				
				Event e2( target );
				e2.Effect = BattleTech::Effect::EXPLOSION;
				e2.Misc = e.Effect;
				e2.Sound = (limb == BattleTech::Loc::LEFT_LEG || limb == BattleTech::Loc::RIGHT_LEG) ? "w_kick.wav" : "w_punch.wav";
				e2.Text = lr.String() + std::string(" for ") + Num::ToString((int)damage) + std::string(" damage.");
				e2.Loc = lr.Loc->Loc;
				e2.Arc = hit_arc;
				Events.push( e2 );
				
				lr.Loc->Damage( damage, hit_arc );
				
				if( melee == BattleTech::Melee::KICK )
					target->PSRs ++;
			}
			else if( melee == BattleTech::Melee::KICK )
				from->PSRs ++;
		}
		
		TookTurn( from );
	}
	else if( type == BattleTech::Packet::ATTENTION )
	{
		uint8_t x = packet->NextUChar();
		uint8_t y = packet->NextUChar();
		
		Player *from_player = Data.GetPlayer( from_client->PlayerID );
		uint8_t from_team = from_player ? atoi( from_player->Properties["team"].c_str() ) : 0;
		
		for( std::map<uint16_t,Player*>::const_iterator player = Data.Players.begin(); player != Data.Players.end(); player ++ )
		{
			Packet attention( BattleTech::Packet::ATTENTION );
			Event e;
			e.Effect = BattleTech::Effect::ATTENTION;
			e.Duration = 0.f;
			e.X = x;
			e.Y = y;
			e.Misc = BattleTech::RGB332::WHITE;
			e.Sound = "i_nav.wav";
			
			uint8_t player_team = 0;
			std::map<std::string,std::string>::const_iterator team = player->second->Properties.find("team");
			if( team != player->second->Properties.end() )
				player_team = atoi( team->second.c_str() );
			if( player_team && (player_team != from_team) )
			{
				// Don't show attention markers to the enemy team during play.
				if( State != BattleTech::State::SETUP )
					continue;
				
				e.Misc = BattleTech::RGB332::ORANGE;
				e.Sound = "";
			}
			
			e.AddToPacket( &attention );
			Net.SendToPlayer( &attention, player->first );
		}
	}
	else
		return RaptorServer::ProcessPacket( packet, from_client );
	return true;
}


void BattleTechServer::AcceptedClient( ConnectedClient *client )
{
	// Localhost client gets admin rights.
	if( client->PlayerID && (((client->IP & 0xFF000000) >> 24) == 127) )
	{
		Player *player = Data.GetPlayer( client->PlayerID );
		if( player )
			player->Properties["admin"] = "true";
	}
	
	RaptorServer::AcceptedClient( client );
	
	Packet change_state( Raptor::Packet::CHANGE_STATE );
	change_state.AddInt( State );
	client->Send( &change_state );
}


void BattleTechServer::DroppedClient( ConnectedClient *client )
{
	RaptorServer::DroppedClient( client );
}


void BattleTechServer::ChangeState( int state )
{
	// Clear any unused turns.
	while( ! TeamTurns.empty() )
		TeamTurns.pop();
	UnmovedUnits.clear();
	UnitTurn = 0;
	UnitCheckedMASC = false;
	
	bool update = true;  // FIXME: Should this only happen for some phases?
	int8_t update_precision = (State == BattleTech::State::SETUP) ? 0 : -127;  // Avoid spoiling the surprise of a failed PSR roll before events show it happen.
	std::set<GameObject*> update_objects;
	std::map< uint8_t, std::set<Mech*> > team_mechs;
	
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			Mech *mech = (Mech*) obj_iter->second;
			mech->BeginPhase( state );
			
			if( (state == BattleTech::State::SETUP) && mech->Destroyed() )
				continue;
			
			if( update )
				update_objects.insert( obj_iter->second );
			
			if( mech->Ready() )
				team_mechs[ mech->Team ].insert( mech );
		}
	}
	
	
	if( state == BattleTech::State::SETUP )
	{
		// Clear dead Mechs from the board when going back to Setup.
		std::set<uint32_t> remove;
		for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
		{
			if( obj_iter->second->Type() == BattleTech::Object::MECH )
			{
				const Mech *mech = (const Mech*) obj_iter->second;
				if( mech->Destroyed() )
					remove.insert( mech->ID );
			}
		}
		if( remove.size() )
		{
			Packet objects_remove = Packet( Raptor::Packet::OBJECTS_REMOVE );
			objects_remove.AddUInt( remove.size() );
			for( std::set<uint32_t>::const_iterator rem_iter = remove.begin(); rem_iter != remove.end(); rem_iter ++ )
			{
				objects_remove.AddUInt( *rem_iter );
				Data.RemoveObject( *rem_iter );
			}
			Net.SendAll( &objects_remove );
		}
		team_mechs.clear();
	}
	
	if( state == BattleTech::State::INITIATIVE )
	{
		Initiative.clear();
		
		if( team_mechs.size() >= 1 )
		{
			for( std::map< uint8_t, std::set<Mech*> >::const_iterator team = team_mechs.begin(); team != team_mechs.end(); team ++ )
				Initiative.push_back( team->first );
			
			if( Initiative.size() >= 2 )
			{
				// Randomize initiative order.
				std::random_shuffle( Initiative.begin(), Initiative.end() );
				
				Event e;
				e.Text = TeamName(*(Initiative.rbegin())) + std::string(" has won the initiative roll, so ") + TeamName(*(Initiative.begin())) + std::string(" must move first.");
				Events.push( e );
			}
		}
	}
	
	if( (state == BattleTech::State::MOVEMENT)
	||  (state == BattleTech::State::WEAPON_ATTACK) )
	{
		UnmovedUnits = team_mechs;
	}
	else
	{
		// FIXME: This modifies turn order.  Should do UnmovedUnits=team_mechs and just skip unable unit turns.
		for( std::map< uint8_t, std::set<Mech*> >::iterator team = team_mechs.begin(); team != team_mechs.end(); team ++ )
		{
			for( std::set<Mech*>::iterator mech = team->second.begin(); mech != team->second.end(); mech ++ )
			{
				if( (*mech)->ReadyAndAble( state ) )
					UnmovedUnits[ team->first ].insert( *mech );
			}
		}
	}
	
	
	if( UnmovedUnits.size() >= 1 )
	{
		// First gather everything we will be assigning.
		std::map<uint8_t,size_t> team_turns_remaining;
		size_t total_turns_remaining = 0;
		for( std::map< uint8_t, std::set<Mech*> >::const_iterator team = UnmovedUnits.begin(); team != UnmovedUnits.end(); team ++ )
		{
			team_turns_remaining[ team->first ] = team->second.size();
			total_turns_remaining += team_turns_remaining[ team->first ];
		}
		
		// Keep assigning sets of turns until none remain.
		while( total_turns_remaining )
		{
			// Determine which team has the fewest to move.
			size_t fewest_to_move = total_turns_remaining;
			for( std::map<uint8_t,size_t>::const_iterator team = team_turns_remaining.begin(); team != team_turns_remaining.end(); team ++ )
			{
				if( team->second < fewest_to_move )
					fewest_to_move = team->second;
			}
			
			// Assign sets of turns in initiative order.
			for( std::vector<uint8_t>::const_iterator team = Initiative.begin(); team != Initiative.end(); team ++ )
			{
				size_t team_moves_this_turn = team_turns_remaining[ *team ] / fewest_to_move;
				
				for( size_t i = 0; i < team_moves_this_turn; i ++ )
					TeamTurns.push( *team );
				
				team_turns_remaining[ *team ] -= team_moves_this_turn;
				total_turns_remaining -= team_moves_this_turn;
			}
		}
		
		// FIXME: Automatically take turns for units unable to act?
	}
	
	
	RaptorServer::ChangeState( state );
	
	Packet change_state( Raptor::Packet::CHANGE_STATE );
	change_state.AddInt( State );
	Net.SendAll( &change_state );
	
	
	if( update_objects.size() )
	{
		Packet update = Packet( Raptor::Packet::UPDATE );
		update.AddChar( update_precision );
		update.AddUInt( update_objects.size() );
		
		for( std::set<GameObject*>::iterator obj = update_objects.begin(); obj != update_objects.end(); obj ++ )
		{
			update.AddUInt( (*obj)->ID );
			(*obj)->AddToUpdatePacketFromServer( &update, update_precision );
			
			if( (state == BattleTech::State::END) && (*obj)->Type() == BattleTech::Object::MECH )
			{
				const Mech *mech = (const Mech*) *obj;
				if( ! mech->Destroyed() )
				{
					// Clear TorsoTwist, TurnedArm, ArmsFlipped, and ProneFire at end of turn.
					Event torso( mech );
					torso.Effect = BattleTech::Effect::TORSO_TWIST;
					torso.ShowFacing( mech->Facing, mech->Prone );
					torso.Duration = 0.f;
					Events.push( torso );
				}
			}
		}
		
		Net.SendAll( &update );
	}
	
	SendEvents();
	
	if( state > BattleTech::State::SETUP )
		TookTurn();
}


void BattleTechServer::Update( double dt )
{
	RaptorServer::Update( dt );
}


bool BattleTechServer::MechTurn( const Mech *mech )
{
	if( UnitTurn )
		return mech && (mech->ID == UnitTurn);
	return mech && mech->Team && TeamTurns.size() && (mech->Team == TeamTurns.front()) && ! mech->TookTurn;
}


void BattleTechServer::TookTurn( Mech *mech )
{
	uint32_t mech_turn = 0;
	if( MechTurn( mech ) )
	{
		mech_turn = mech->ID;
		mech->TookTurn = true;
		UnitTurn = 0;
		UnitCheckedMASC = false;
		TeamTurns.pop();
		UnmovedUnits[ mech->Team ].erase( mech );
		
		Packet update = Packet( Raptor::Packet::UPDATE );
		update.AddChar( -127 ); // Precision
		update.AddUInt( 1 );    // Number of Objects
		update.AddUInt( mech->ID );
		mech->AddToUpdatePacketFromServer( &update, -127 );
		Net.SendAll( &update );
	}
	
	if( TeamTurns.size() )
	{
		Packet team_turn( BattleTech::Packet::TEAM_TURN );
		team_turn.AddUChar( TeamTurns.front() );
		team_turn.AddUInt( mech_turn );
		Net.SendAll( &team_turn );
	}
	else
	{
		bool game_over = (Initiative.size() < 2) && ! FF();
		
		int next_state = State;
		if( (State == BattleTech::State::END) && ! game_over )
			next_state = BattleTech::State::INITIATIVE;
		else if( (State == BattleTech::State::INITIATIVE) && game_over )
			next_state = BattleTech::State::GAME_OVER;
		else if( (State == BattleTech::State::GAME_OVER) && (Data.GameObjects.size() <= 1) )
			next_state = BattleTech::State::SETUP;
		else if( State > BattleTech::State::GAME_OVER )
			next_state = BattleTech::State::SETUP;
		else if( (State != BattleTech::State::SETUP) && (State != BattleTech::State::GAME_OVER) )
			next_state = State + 1;
		
		if( State != next_state )
			ChangeState( next_state );
	}
}


double BattleTechServer::SendEvents( void )
{
	if( ! Events.size() )
		return 0.;
	
	double duration = 0.;
	
	Packet events( BattleTech::Packet::EVENTS );
	events.AddUShort( Events.size() );
	while( Events.size() )
	{
		Events.front().AddToPacket( &events );
		duration += Events.front().Duration;
		Events.pop();
	}
	Net.SendAll( &events );
	
	return duration;
}


std::string BattleTechServer::TeamName( uint8_t team_num ) const
{
	std::map<std::string,std::string>::const_iterator name = Raptor::Game->Data.Properties.find( std::string("team") + Num::ToString((int)team_num) );
	if( name != Raptor::Game->Data.Properties.end() )
		return name->second;
	
	return std::string("Team ") + Num::ToString((int)team_num);
}


HexMap *BattleTechServer::Map( void )
{
	for( std::map<uint32_t,GameObject*>::iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MAP )
			return (HexMap*) obj_iter->second;
	}
	
	return NULL;
}


bool BattleTechServer::FF( void ) const
{
	std::map<std::string,std::string>::const_iterator ff = Data.Properties.find("ff");
	if( ff != Data.Properties.end() )
	{
		if( Str::AsBool(ff->second) )
			return true;
	}
	
	/*
	// Allow friendly fire after all enemies are eliminated.
	// Commented-out to go to Game Over state when a friendly physical attack is possible.
	std::set<uint8_t> teams_alive;
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data.GameObjects.begin(); obj_iter != Data.GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			const Mech *mech = (const Mech*) obj_iter->second;
			if( ! mech->Destroyed() )
				teams_alive.insert( mech->Team );
		}
	}
	return (teams_alive.size() < 2);
	*/
	return false;
}