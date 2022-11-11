/*
 *  Event.cpp
 */

#include "Event.h"

#include "BattleTechDefs.h"
#include "RaptorServer.h"


Event::Event( const Mech *m1, const Mech *m2 )
{
	Duration = 0.8f;
	Effect = BattleTech::Effect::NONE;
	MechID = 0;
	X = 0;
	Y = 0;
	Misc = 0;
	
	HealthUpdate = false;
	CriticalHit = false;
	Loc = BattleTech::Loc::CENTER_TORSO;
	Arc = BattleTech::Arc::STRUCTURE;
	HP = 0;
	Eq = 0xFF;
	
	Stat = BattleTech::Stat::NONE;
	Val1 = 0;
	Val2 = 0;
	
	if( m1 )
	{
		MechID = m1->ID;
		X = m1->X;
		Y = m1->Y;
	}
	
	if( m2 )
	{
		X = m2->X;
		Y = m2->Y;
	}
}


Event::Event( Packet *packet )
{
	ReadFromPacket( packet );
}


Event::~Event()
{
}


void Event::ShowFacing( void )
{
	ShowFacing( (const Mech*) Raptor::Server->Data.GetObject( MechID ) );
}


void Event::ShowFacing( const Mech *mech )
{
	if( mech )
		ShowFacing( mech->Facing, mech->Prone, mech->TorsoTwist, mech->TurnedArm, mech->ArmsFlipped, mech->ProneFire );
}


void Event::ShowFacing( uint8_t facing, bool prone, int8_t twist, int8_t turned_arm, bool arm_flip, int8_t prone_fire )
{
	Misc = facing & 0x07;
	if( prone )
	{
		Misc |= 0x08;
		if( prone_fire == BattleTech::Loc::LEFT_ARM )
			Misc |= 0x10;
		else if( prone_fire == BattleTech::Loc::RIGHT_ARM )
			Misc |= 0x20;
		else if( prone_fire != BattleTech::Loc::UNKNOWN )
			Misc |= 0x30;
	}
	else
	{
		if( twist >= 1 )
			Misc |= 0x10;
		else if( twist <= -1 )
			Misc |= 0x20;
		else if( arm_flip )
			Misc |= 0x30;
	}
	if( turned_arm == BattleTech::Loc::LEFT_ARM )
		Misc |= 0x40;
	if( turned_arm == BattleTech::Loc::RIGHT_ARM )
		Misc |= 0x80;
}


void Event::ReadFacing( Mech *mech ) const
{
	mech->Facing  = Misc & 0x07;
	mech->Prone   = Misc & 0x08;
	if( mech->Prone )
	{
		mech->ArmsFlipped = false;
		mech->TorsoTwist = 0;
		uint8_t prone_fire = Misc & 0x30;
		if( prone_fire == 0x30 )
			mech->ProneFire = BattleTech::Loc::CENTER_TORSO;
		else if( prone_fire == 0x10 )
			mech->ProneFire = BattleTech::Loc::LEFT_ARM;
		else if( prone_fire == 0x20 )
			mech->ProneFire = BattleTech::Loc::RIGHT_ARM;
		else
			mech->ProneFire = BattleTech::Loc::UNKNOWN;
	}
	else
	{
		mech->ProneFire = BattleTech::Loc::UNKNOWN;
		uint8_t twist_flip = Misc & 0x30;
		if( twist_flip == 0x10 )
			mech->TorsoTwist = 1;
		else if( twist_flip == 0x20 )
			mech->TorsoTwist = -1;
		else
			mech->TorsoTwist = 0;
		mech->ArmsFlipped = (twist_flip == 0x30);
	}
	mech->TurnedArm = BattleTech::Loc::UNKNOWN;
	if( Misc & 0x40 )
		mech->TurnedArm = BattleTech::Loc::LEFT_ARM;
	if( Misc & 0x80 )
		mech->TurnedArm = BattleTech::Loc::RIGHT_ARM;
}


void Event::ShowHealth( int8_t loc, uint8_t arc )
{
	ShowHealth( (Mech*) Raptor::Server->Data.GetObject( MechID ), loc, arc );
}


void Event::ShowHealth( Mech *mech, int8_t loc, uint8_t arc )
{
	if( loc < 0 )
		return;
	if( mech )
		ShowHealth( loc, arc, *(mech->Locations[ loc ].ArmorPtr( arc )) );
}


void Event::ShowHealth( int8_t loc, uint8_t arc, uint16_t hp )
{
	if( loc < 0 )
		return;
	HealthUpdate = true;
	Loc = loc;
	Arc = ((arc == BattleTech::Arc::LEFT_SIDE) || (arc == BattleTech::Arc::RIGHT_SIDE)) ? (uint8_t) BattleTech::Arc::FRONT : arc;
	HP = hp;
}


void Event::ShowCritHit( const MechEquipment *eq, int8_t loc )
{
	if( ! eq )
		return;
	CriticalHit = true;
	Loc = (loc == BattleTech::Loc::UNKNOWN) ? eq->Location->Loc : loc;
	Arc = BattleTech::Arc::STRUCTURE;
	Eq = eq->Index();
}


void Event::ShowEquipment( const MechEquipment *eq )
{
	uint8_t damage_and_flags = eq->Damaged;
	if( eq->Jammed )
		damage_and_flags |= 0x80;
	ShowStat( BattleTech::Stat::EQUIPMENT, eq->Index(), damage_and_flags );
}


void Event::ShowAmmo( const MechEquipment *ammo )
{
	ShowStat( BattleTech::Stat::AMMO, ammo->Index(), std::min<uint16_t>( 255, ammo->Ammo ) );
}


void Event::ShowStat( int8_t stat, int8_t val1, int8_t val2 )
{
	Stat = stat;
	Val1 = val1;
	Val2 = val2;
}


void Event::ShowStat( int8_t stat )
{
	ShowStat( (Mech*) Raptor::Server->Data.GetObject( MechID ), stat );
}


void Event::ShowStat( Mech *mech, int8_t stat )
{
	if( ! mech )
		return;
	else if( stat == BattleTech::Stat::NONE )
		ShowStat( stat, 0, 0 );
	else if( stat == BattleTech::Stat::SHUTDOWN )
		ShowStat( stat, mech->Shutdown ? 1 : 0 );
	else if( stat == BattleTech::Stat::PILOT )
		ShowStat( stat, mech->PilotDamage, mech->Unconscious );
	else if( stat == BattleTech::Stat::HEAT )
		ShowStat( stat, mech->Heat, mech->HeatMove | (mech->HeatFire << 4) );
	else if( stat == BattleTech::Stat::HEAT_MASC_SC )
		ShowStat( stat, mech->Heat, mech->MASCTurns | (mech->SuperchargerTurns << 4) );
	else if( stat == BattleTech::Stat::HEATSINKS )
		ShowStat( stat, mech->HeatDissipation );
	else if( stat == BattleTech::Stat::ACTIVE_SPECIAL )
		ShowStat( stat, mech->ActiveStealth ? 1 : 0, mech->ActiveTSM ? 1 : 0 );
	else if( stat == BattleTech::Stat::NARC )
	{
		uint8_t narc = 0;
		for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
			if( mech->Locations[ i ].Narced )
				narc |= (1 << i);
		ShowStat( stat, narc );
	}
}


void Event::ReadStat( Mech *mech ) const
{
	if( ! mech )
		return;
	else if( (Stat == BattleTech::Stat::AMMO) && (Val1 >= 0) && ((size_t) Val1 < mech->Equipment.size()) )
	{
		MechEquipment *eq = &(mech->Equipment.at( Val1 ));
		eq->Ammo = Val2;
	}
	else if( (Stat == BattleTech::Stat::EQUIPMENT) && (Val1 >= 0) && ((size_t) Val1 < mech->Equipment.size()) )
	{
		MechEquipment *eq = &(mech->Equipment.at( Val1 ));
		eq->Jammed  = Val2 & 0x80;
		eq->Damaged = Val2 & 0x7F;
	}
	else if( Stat == BattleTech::Stat::SHUTDOWN )
		mech->Shutdown = Val1;
	else if( Stat == BattleTech::Stat::PILOT )
	{
		mech->PilotDamage = Val1;
		mech->Unconscious = Val2;
	}
	else if( Stat == BattleTech::Stat::HEAT )
	{
		mech->Heat = Val1;
		mech->HeatMove =  Val2 & 0x0F;
		mech->HeatFire = (Val2 & 0xF0) >> 4;
	}
	else if( Stat == BattleTech::Stat::HEAT_MASC_SC )
	{
		mech->Heat = Val1;
		mech->MASCTurns         =  Val2 & 0x0F;
		mech->SuperchargerTurns = (Val2 & 0xF0) >> 4;
	}
	else if( Stat == BattleTech::Stat::HEAT_ADD )
		mech->Heat += Val1;
	else if( Stat == BattleTech::Stat::HEATSINKS )
		mech->HeatDissipation = Val1;
	else if( Stat == BattleTech::Stat::ACTIVE_SPECIAL )
	{
		mech->ActiveStealth = Val1;
		mech->ActiveTSM = Val2;
	}
	else if( Stat == BattleTech::Stat::NARC )
	{
		for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
			mech->Locations[ i ].Narced = Val1 & (1 << i);
	}
	else if( Stat == BattleTech::Stat::WEAPON_FIRED )
	{
		MechEquipment *eq = &(mech->Equipment.at( Val1 ));
		eq->Fired += Val2;
		mech->Heat += eq->Weapon->Heat * Val2;
	}
}


void Event::AddToPacket( Packet *packet ) const
{
	packet->AddFloat( Duration );
	packet->AddString( Text );
	packet->AddString( Sound );
	packet->AddUChar( Effect );
	packet->AddUInt( MechID );
	packet->AddUChar( X );
	packet->AddUChar( Y );
	packet->AddUChar( Misc );
	
	uint8_t loc_arc = (Arc << 4) | (Loc & 0x07);
	if( HealthUpdate )
		loc_arc |= 0x80;
	if( CriticalHit )
		loc_arc |= 0x08;
	packet->AddUChar( loc_arc );
	
	if( HealthUpdate || CriticalHit )
		packet->AddUChar( HP );
	if( CriticalHit )
		packet->AddUChar( Eq );
	
	packet->AddUChar( Stat );
	if( Stat )
	{
		packet->AddChar( Val1 );
		packet->AddChar( Val2 );
	}
}


void Event::ReadFromPacket( Packet *packet )
{
	Duration = packet->NextFloat();
	Text     = packet->NextString();
	Sound    = packet->NextString();
	Effect   = packet->NextUChar();
	MechID   = packet->NextUInt();
	X        = packet->NextUChar();
	Y        = packet->NextUChar();
	Misc     = packet->NextUChar();
	
	uint8_t loc_arc = packet->NextUChar();
	HealthUpdate =  loc_arc & 0x80;
	CriticalHit  =  loc_arc & 0x08;
	Loc          =  loc_arc & 0x07;
	Arc          = (loc_arc & 0x70) >> 4;
	
	HP = 0;
	Eq = 0xFF;
	
	if( HealthUpdate || CriticalHit )
		HP = packet->NextUChar();
	if( CriticalHit )
		Eq = packet->NextUChar();
	
	Stat = packet->NextUChar();
	if( Stat )
	{
		Val1 = packet->NextChar();
		Val2 = packet->NextChar();
	}
}
