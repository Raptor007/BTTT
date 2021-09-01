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
		/*
		Misc |= (prone_fire & 0x0F) << 4;
		*/
		if( prone_fire == BattleTech::Loc::LEFT_ARM )
			Misc |= 0x10;
		else if( prone_fire == BattleTech::Loc::RIGHT_ARM )
			Misc |= 0x20;
		else if( prone_fire != BattleTech::Loc::UNKNOWN )
			Misc |= 0x30;
	}
	else
	{
		/*
		// This encoding allowed increased torso twist, but couldn't fit turned arms.
		Misc |= (twist & 0x07) << 4;
		if( arm_flip )
			Misc |= 0x80;
		*/
		if( twist == 1 )
			Misc |= 0x10;
		else if( twist == -1 )
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
		/*
		mech->ProneFire = (Misc & 0xF0) >> 4;
		if( mech->ProneFire >= BattleTech::Loc::COUNT )
			mech->ProneFire = BattleTech::Loc::UNKNOWN;
		*/
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
		/*
		// This encoding allowed increased torso twist, but couldn't fit turned arms.
		mech->ArmsFlipped =  Misc & 0x80;
		mech->TorsoTwist  = (Misc & 0x70) >> 4;
		if( mech->TorsoTwist & 0x04 )
			mech->TorsoTwist |= 0xF8;
		*/
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
}
