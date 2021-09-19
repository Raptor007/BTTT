/*
 *  Variant.cpp
 */

#include "Variant.h"

#include "BattleTechGame.h"
#include "Num.h"
#include <map>
#include <algorithm>
#include <cstring>
#include <cmath>


VariantEquipment::VariantEquipment( uint16_t wid )
{
	ID = wid;
	Ammo = 0;
	Rear = false;
	HasFCS = false;
}


VariantEquipment::VariantEquipment( Packet *packet )
{
	ReadFromPacket( packet );
}


VariantEquipment::~VariantEquipment()
{
}


void VariantEquipment::AddToPacket( Packet *packet ) const
{
	packet->AddUShort( ID );
	packet->AddUShort( Ammo );
	
	uint8_t flags = 0;
	if( Rear )
		flags |= 0x01;
	if( HasFCS )
		flags |= 0x02;
	packet->AddUChar( flags );
	
	packet->AddUChar( CritLocs.size() );
	for( std::vector<uint8_t>::const_iterator crit = CritLocs.begin(); crit != CritLocs.end(); crit ++ )
		packet->AddUChar( *crit );
}


void VariantEquipment::ReadFromPacket( Packet *packet )
{
	ID   = packet->NextUShort();
	Ammo = packet->NextUShort();
	
	uint8_t flags = packet->NextUChar();
	Rear   = flags & 0x01;
	HasFCS = flags & 0x02;
	
	size_t crits = packet->NextUChar();
	for( size_t i = 0; i < crits; i ++ )
		CritLocs.push_back( packet->NextUChar() );
}


// -----------------------------------------------------------------------------


Variant::Variant( void )
{
	Tons = 0;
	Clan = false;
	Quad = false;
	Walk = 0;
	Jump = 0;
	MASC = false;
	TSM = false;
	Heatsinks = 0;
	HeatsinksDouble = false;
	memset( &Armor, 0, sizeof(Armor) );
	Stealth = false;
	CASE = 0;
}


Variant::~Variant()
{
}


bool Variant::Load( const char *filename )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	unsigned char buf[ 1024 ] = {0};
	char name[ 128 ] = {0};
	
	FILE *f = fopen( filename, "rb" );
	if( ! f )
		return false;
	fread( buf, 1, sizeof(buf), f );
	fclose( f );
	f = NULL;
	if( strncmp( (char*) buf, "V5.00\1", 6 ) != 0 )
		return false;
	
	Quad = buf[ 0x07 ] & 0x10;
	Tons = buf[ 0x11 ];
	
	unsigned char *ptr = buf + 0x13;
	ptr = HeavyMetal::hmstrcpy( name, ptr );
	Name = name;
	ptr = HeavyMetal::hmstrcpy( name, ptr );
	Var = name;
	
	ptr = HeavyMetal::hmstrcpy( NULL, ptr + 30 );
	
	Clan = ptr[ 0 ] & 0x01;
	if( ptr[ 0 ] & 0xFE ) // Mixed tech base.
	{
		Clan = ptr[ 2 ] & 0x01;
		ptr += 14;
	}
	const std::map< short, HMWeapon > *weapons = Clan ? &(game->ClanWeap) : &(game->ISWeap);
	CASE = Clan ? 0xFF : 0x00;
	
	Walk = ptr[ 10 ];
	Jump = ptr[ 12 ];
	MASC = (ptr[ 74 ] & 0x02);
	TSM  = (ptr[ 74 ] & 0x01);
	
	Heatsinks = ptr[ 14 ];
	HeatsinksDouble = ptr[ 16 ];
	
	Stealth = (ptr[ 18 ] == 0x08);
	
	if( ptr[ 18 ] == 0x07 ) // Patchwork armor.
		ptr += 22;
	Armor[ BattleTech::Loc::LEFT_ARM          ] = ptr[ 22 ];
	Armor[ BattleTech::Loc::LEFT_TORSO        ] = ptr[ 28 ];
	Armor[ BattleTech::Loc::LEFT_LEG          ] = ptr[ 34 ];
	Armor[ BattleTech::Loc::RIGHT_ARM         ] = ptr[ 40 ];
	Armor[ BattleTech::Loc::RIGHT_TORSO       ] = ptr[ 46 ];
	Armor[ BattleTech::Loc::RIGHT_LEG         ] = ptr[ 52 ];
	Armor[ BattleTech::Loc::HEAD              ] = ptr[ 58 ];
	Armor[ BattleTech::Loc::CENTER_TORSO      ] = ptr[ 64 ];
	Armor[ BattleTech::Loc::LEFT_TORSO_REAR   ] = ptr[ 68 ];
	Armor[ BattleTech::Loc::RIGHT_TORSO_REAR  ] = ptr[ 70 ];
	Armor[ BattleTech::Loc::CENTER_TORSO_REAR ] = ptr[ 72 ];
	
	// Weapons
	size_t wcount = ptr[ 76 ];
	ptr += 78;
	for( size_t i = 0; i < wcount; i ++ )
	{
		int qty = ptr[ 0 ];
		
		uint16_t wid = 0x33;
		Endian::CopyLittle( ptr + 2, &wid, 2 );
		
		int loc = ptr[ 4 ] - 1;
		bool rear = false;
		if( loc >= BattleTech::Loc::COUNT )
		{
			loc -= BattleTech::Loc::COUNT;
			rear = true;
		}
		
		//short ammo = ptr[ 6 ];
		//Endian::CopyLittle( ptr + 6, &ammo, 2 );
		
		for( int i = 0; i < qty; i ++ )
		{
			Equipment.push_back( VariantEquipment(wid) );
			Equipment.back().Rear = rear;
		}
		
		ptr = HeavyMetal::hmstrcpy( NULL, ptr + 10 );
	}
	
	// Critital Slots
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		VariantEquipment *prev = NULL;
		
		for( size_t j = 0; j < 12; j ++ )
		{
			uint16_t crit_id = 0;
			Endian::CopyLittle( ptr, &crit_id, 2 );
			
			if( crit_id == BattleTech::Equipment::CASE )
				CASE |= (1 << i); // Set CASE bit per location.
			
			if( (crit_id == BattleTech::Equipment::ARTEMIS_IV_FCS) && prev )
				prev->HasFCS = true;
			
			if( HeavyMetal::HittableCrit(crit_id) )
			{
				size_t slots = HeavyMetal::CritSlots( crit_id, Clan, weapons );
				VariantEquipment *equipment = NULL;
				bool rear = (crit_id <= 400) && ptr[ 2 ];
				
				for( std::vector<VariantEquipment>::iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
				{
					if( (eq->ID == crit_id) && (eq->CritLocs.size() < slots) && (eq->Rear == rear) )
					{
						equipment = &*eq;
						break;
					}
				}
				
				if( ! equipment )
				{
					Equipment.push_back( VariantEquipment(crit_id) );
					equipment = &(Equipment.back());
					if( crit_id > 400 )
						equipment->Ammo = ptr[ 2 ];
				}
				
				equipment->CritLocs.push_back( i );
				
				// Equipment that spans multiple locations should sort by most crit slots.
				if( std::count( equipment->CritLocs.begin(), equipment->CritLocs.end(), i ) > std::count( equipment->CritLocs.begin(), equipment->CritLocs.end(), *(equipment->CritLocs.begin()) ) )
				{
					while( *(equipment->CritLocs.rbegin()) == i )
					{
						equipment->CritLocs.pop_back();
						equipment->CritLocs.insert( equipment->CritLocs.begin(), i );
					}
				}
				
				prev = equipment;
			}
			else
				prev = NULL;
			
			ptr += 4;
		}
	}
	
	return true;
}


void Variant::AddToPacket( Packet *packet ) const
{
	packet->AddUChar( Tons );
	
	packet->AddString( Name );
	packet->AddString( Var );
	
	uint8_t flags = 0;
	if( Clan )
		flags |= 0x01;
	if( Quad )
		flags |= 0x02;
	if( MASC )
		flags |= 0x04;
	if( TSM )
		flags |= 0x08;
	if( HeatsinksDouble )
		flags |= 0x10;
	if( Stealth )
		flags |= 0x20;
	packet->AddUChar( flags );
	
	packet->AddUChar( Walk );
	packet->AddUChar( Jump );
	
	packet->AddUChar( Heatsinks );
	
	for( size_t i = 0; i < sizeof(Armor); i ++ )
		packet->AddUChar( Armor[ i ] );
	
	packet->AddUChar( CASE );
	
	packet->AddUChar( Equipment.size() );
	for( std::vector<VariantEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
		eq->AddToPacket( packet );
}


void Variant::ReadFromPacket( Packet *packet )
{
	Tons = packet->NextUChar();
	
	Name = packet->NextString();
	Var  = packet->NextString();
	
	uint8_t flags = packet->NextUChar();
	Clan            = flags & 0x01;
	Quad            = flags & 0x02;
	MASC            = flags & 0x04;
	TSM             = flags & 0x08;
	HeatsinksDouble = flags & 0x10;
	Stealth         = flags & 0x20;
	
	Walk = packet->NextUChar();
	Jump = packet->NextUChar();
	
	Heatsinks = packet->NextUChar();
	
	for( size_t i = 0; i < sizeof(Armor); i ++ )
		Armor[ i ] = packet->NextUChar();
	
	CASE = packet->NextUChar();
	
	Equipment.clear();
	size_t eq_count = packet->NextUChar();
	for( size_t i = 0; i < eq_count; i ++ )
		Equipment.push_back( VariantEquipment( packet ) );
}


std::string Variant::Details( void ) const
{
	std::string details = Name + std::string(" ") + Var;
	
	details += std::string("\n") + Num::ToString(Tons) + std::string("-Ton");
	details += Clan ? " Clan" : " Inner Sphere";
	details += Quad ? " Quad" : " Mech";
	
	uint16_t total_armor = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT + 3; i ++ )
		total_armor += Armor[ i ];
	details += std::string("\nTotal Armor ") + Num::ToString(total_armor);
	//details += std::string(", Structure ") + Num::ToString(total_structure);
	
	details += std::string("\nWalk ") + Num::ToString(Walk);
	if( TSM )
		details += std::string(" (") + Num::ToString(Walk+2) + std::string(")");
	details += std::string(", Run ") + Num::ToString(ceilf(Walk*1.5f));
	if( TSM )
		details += std::string(" (") + Num::ToString(ceilf((Walk+2)*1.5f)) + std::string(")");
	if( MASC )
		details += std::string(" [") + Num::ToString(Walk*2) + std::string("]");
	details += std::string(", Jump ") + Num::ToString(Jump);
	
	details += std::string("\nHeatsinks ") + Num::ToString(Heatsinks);
	if( HeatsinksDouble )
		details += std::string(" (") + Num::ToString(Heatsinks*2) + std::string(")");
	
	std::set<uint16_t> special_equipment;
	for( std::vector<VariantEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
	{
		switch( eq->ID )
		{
			case BattleTech::Equipment::TARGETING_COMPUTER:
			case BattleTech::Equipment::HATCHET:
			case BattleTech::Equipment::SWORD:
				special_equipment.insert( eq->ID );
		}
	}
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	const std::map< short, HMWeapon > *weapons = Clan ? &(game->ClanWeap) : &(game->ISWeap);
	if( weapons )
	{
		std::map<uint16_t,uint8_t> weapon_counts;
		std::map<uint16_t,bool> weapon_fcs;
		std::map<uint16_t,bool> weapon_tc;
		//uint16_t alpha_damage = 0, alpha_heat = 0;
		
		for( std::vector<VariantEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
		{
			std::map< short, HMWeapon >::const_iterator witer = weapons->find( eq->ID );
			if( witer != weapons->end() )
			{
				uint16_t wid = eq->ID;
				if( eq->Rear )
					wid |= 0x8000;
				weapon_counts[ wid ] ++;
				weapon_fcs[ wid ] = eq->HasFCS;
				weapon_tc[ wid ] = witer->second.CanUseTargetingComputer && special_equipment.count( BattleTech::Equipment::TARGETING_COMPUTER );
				//alpha_damage += witer->second.MaximumDamage();
				//alpha_heat   += witer->second.MaximumHeat();
			}
		}
		for( std::map<uint16_t,uint8_t>::const_iterator wc = weapon_counts.begin(); wc != weapon_counts.end(); wc ++ )
		{
			details += std::string("\n") + Num::ToString(wc->second) + std::string(" x ") + HeavyMetal::CritName( wc->first & 0x7FFF, weapons );
			if( weapon_fcs[ wc->first ] )
				details += " + Artemis IV FCS";
			if( weapon_tc[ wc->first ] )
				details += " + Targeting Computer";
			if( wc->first & 0x8000 )
				details += " (Rear)";
		}
	}
	
	for( std::set<uint16_t>::const_iterator seq = special_equipment.begin(); seq != special_equipment.end(); seq ++ )
	{
		if( *seq != BattleTech::Equipment::TARGETING_COMPUTER )
			details += std::string("\n") + HeavyMetal::CritName( *seq, weapons );
	}
	
	if( Stealth )
		details += std::string("\nStealth Armor System");
	
	return details;
}
