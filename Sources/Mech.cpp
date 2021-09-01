/*
 *  Mech.cpp
 */

#include "Mech.h"

#include "BattleTechGame.h"
#include "BattleTechServer.h"
#include "HeavyMetal.h"
#include "Roll.h"
#include "Rand.h"
#include "Event.h"
#include "Num.h"
#include "Math2D.h"
#include <cmath>
#include <algorithm>

using namespace BattleTech::Loc;
using namespace BattleTech::Arc;


// -----------------------------------------------------------------------------


bool MechMelee::operator < ( const MechMelee &other ) const
{
	// Desired sorting characteristics.
	if( Difficulty < other.Difficulty )
		return true;
	if( Difficulty > other.Difficulty )
		return false;
	if( Damage > other.Damage )
		return true;
	if( Damage < other.Damage )
		return false;
	
	// Other variables just for uniqueness.
	if( Attack < other.Attack )
		return true;
	if( Attack > other.Attack )
		return false;
	return (Limb > other.Limb);
}


// -----------------------------------------------------------------------------


MechEquipment::MechEquipment( void )
{
	Init( 0, false, 0 );
}


MechEquipment::MechEquipment( uint16_t eq_id, bool clan, uint16_t ammo )
{
	Init( eq_id, clan, ammo );
}


MechEquipment::MechEquipment( Packet *packet, Mech *mech )
{
	ReadFromInitPacket( packet, mech );
}


MechEquipment::~MechEquipment()
{
}


void MechEquipment::Init( uint16_t eq_id, bool clan, uint16_t ammo )
{
	ID = eq_id;
	Location = NULL;
	
	ExplosionDamage = 0;
	Ammo = 0;
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	const std::map< short, HMWeapon > *weapons = clan ? &(game->ClanWeap) : &(game->ISWeap);
	
	Weapon = NULL;
	std::map< short, HMWeapon >::const_iterator witer = weapons->find( ID );
	if( witer != weapons->end() )
	{
		Weapon = &(witer->second);
		ExplosionDamage = Weapon->ExplodeDamage;
	}
	
	if( ID > 400 )
	{
		Ammo = ammo;
		std::map< short, HMWeapon >::const_iterator witer = weapons->find( ID - 400 );
		if( witer != weapons->end() )
		{
			if( witer->second.AmmoExplodes )
			{
				if( witer->second.Narc || strstr( witer->second.AmmoName.c_str(), "AMS" ) )
					ExplosionDamage = 2;
				else
				{
					uint16_t shot_damage = witer->second.Flamer ? witer->second.Heat : witer->second.Damage;
					ExplosionDamage = shot_damage * std::max<uint16_t>( 1, witer->second.ClusterWeaponSize );
				}
			}
		}
	}
	
	Name = HeavyMetal::CritName( eq_id, weapons );
	
	WeaponTC = NULL;
	WeaponFCS = NULL;
	
	Damaged = 0;
	Jammed = false;
	Fired = 0;
}


void MechEquipment::AddToInitPacket( Packet *packet ) const
{
	const Mech *mech = MyMech();
	
	packet->AddUShort( ID );
	packet->AddUShort( Ammo );
	
	uint8_t loc = Location ? Location->Loc : 0;
	if( WeaponArcs.count( BattleTech::Arc::REAR ) && (WeaponArcs.size() == 1) )
		loc |= 0x80;
	
	packet->AddUChar( loc );
	
	if( ID == BattleTech::Equipment::ARTEMIS_IV_FCS )
	{
		uint8_t weapon_index = 0;
		if( mech )
		{
			for( uint8_t i = 0; i < mech->Equipment.size(); i ++ )
			{
				if( mech->Equipment[ i ].WeaponFCS == this )
				{
					weapon_index = i;
					break;
				}
			}
		}
		
		packet->AddUChar( weapon_index );
	}
	else if( ID == BattleTech::Equipment::TARGETING_COMPUTER )
	{
		std::set<uint8_t> weapons;
		if( mech )
		{
			for( uint8_t i = 0; i < mech->Equipment.size(); i ++ )
			{
				if( mech->Equipment[ i ].WeaponTC == this )
					weapons.insert( i );
			}
		}
		
		packet->AddUChar( weapons.size() );
		
		for( std::set<uint8_t>::const_iterator w = weapons.begin(); w != weapons.end(); w ++ )
			packet->AddUChar( *w );
	}
}


void MechEquipment::ReadFromInitPacket( Packet *packet, Mech *mech )
{
	uint16_t eq_id = packet->NextUShort();
	uint16_t ammo  = packet->NextUShort();
	uint8_t  loc   = packet->NextUChar();
	
	Init( eq_id, (mech ? mech->Clan : false), ammo );
	
	if( mech )
		Location = &(mech->Locations[ loc & 0x7F ]);
	
	WeaponArcs.clear();
	if( loc & 0x80 )
		WeaponArcs.insert( BattleTech::Arc::REAR );
	else if( Weapon )
	{
		WeaponArcs.insert( BattleTech::Arc::FRONT );
		
		if( ((loc & 0x7F) == LEFT_ARM) && ! Location->IsLeg )
			WeaponArcs.insert( BattleTech::Arc::LEFT_SIDE );
		else if( ((loc & 0x7F) == RIGHT_ARM) && ! Location->IsLeg )
			WeaponArcs.insert( BattleTech::Arc::RIGHT_SIDE );
	}
	
	if( ID == BattleTech::Equipment::ARTEMIS_IV_FCS )
	{
		uint8_t weapon_index = packet->NextUChar();
		
		if( mech && (weapon_index < mech->Equipment.size()) )
			mech->Equipment[ weapon_index ].WeaponFCS = this;
	}
	else if( ID == BattleTech::Equipment::TARGETING_COMPUTER )
	{
		uint8_t weapon_count = packet->NextUChar();
		
		for( uint8_t i = 0; i < weapon_count; i ++ )
		{
			uint8_t weapon_index = packet->NextUChar();
			
			if( mech && (weapon_index < mech->Equipment.size()) )
				mech->Equipment[ weapon_index ].WeaponTC = this;
		}
	}
}


Mech *MechEquipment::MyMech( void ) const
{
	return Location ? Location->MyMech : NULL;
}


int8_t MechEquipment::Index( void ) const
{
	const Mech *mech = MyMech();
	for( size_t i = 0; i < mech->Equipment.size(); i ++ )
	{
		if( this == &(mech->Equipment[ i ]) )
			return i;
	}
	return -1;
}


bool MechEquipment::WithinFiringArc( uint8_t x, uint8_t y ) const
{
	const Mech *mech = MyMech();
	bool leg_mounted = Location && Location->IsLeg;
	uint8_t firing_arc = mech->FiringArc( x, y, ! leg_mounted );
	
	// Don't allow arm flipping with torso twist or when prone.
	if( (firing_arc == BattleTech::Arc::REAR) && Location && Location->IsArm() && (WeaponArcs.size() > 1) && (mech->TorsoTwist || mech->Prone) )
		return false;
	
	return WeaponArcs.count( firing_arc );
}


int8_t MechEquipment::ShotModifier( uint8_t range, bool stealth ) const
{
	const Mech *mech = MyMech();
	
	if( ! Weapon )
		return 99;
	if( range > Weapon->RangeLong )
		return 99;
	if( mech->Sensors->Damaged >= 2 )
		return 99;
	
	uint8_t arm_count = 0, arms_destroyed = 0, legs_intact = 0;
	if( mech->Prone )
	{
		if( Location && Location->IsLeg )
			return 99;
		
		for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
		{
			if( mech->Locations[ i ].IsLeg && mech->Locations[ i ].Structure )
				legs_intact ++;
			if( ! mech->Locations[ i ].IsArm() )
				continue;
			arm_count ++;
			if( ! mech->Locations[ i ].Structure )
				arms_destroyed ++;
		}
		
		if( arms_destroyed >= arm_count )
			return 99;
		if( Location && Location->IsArm() && arms_destroyed )
			return 99;
	}
	
	if( ! range )  // 0 is not a valid range, so assume this means no range was specified.
		range = Weapon->RangeShort;
	
	int8_t modifier = Weapon->ToHit;
	
	if( range > Weapon->RangeMedium )
		modifier += stealth ? 6 : 4;
	else if( range > Weapon->RangeShort )
		modifier += stealth ? 3 : 2;
	else if( range <= Weapon->RangeMin )
		modifier += Weapon->RangeMin - range + 1;
	
	if( WeaponTC && ! WeaponTC->Damaged )
		modifier --;
	
	if( Location && Location->DamagedCriticals.count( BattleTech::Equipment::SHOULDER ) )
		modifier += 4;
	else
	{
		if( Location && Location->DamagedCriticals.count( BattleTech::Equipment::UPPER_ARM_ACTUATOR ) )
			modifier ++;
		if( Location && Location->DamagedCriticals.count( BattleTech::Equipment::LOWER_ARM_ACTUATOR ) )
			modifier ++;
	}
	
	if( mech->Sensors->Damaged )
		modifier += 2;
	
	if( mech->Prone )
	{
		if( legs_intact < 4 )
			modifier += 2;
		
		if( arms_destroyed )
		{
			modifier ++;
			
			std::map<std::string,std::string>::const_iterator prone_1arm = mech->Data->Properties.find("prone_1arm");
			if( (prone_1arm != mech->Data->Properties.end()) && ! Str::AsBool(prone_1arm->second) )
				return 99;
		}
	}
	
	return modifier;
}


uint8_t MechEquipment::ClusterHits( uint8_t roll, bool fcs, bool narc, bool ecm, bool indirect, MechEquipment *ams ) const
{
	const Mech *mech = MyMech();
	
	if( ! Weapon )
		return 0;
	uint8_t cluster = Weapon->ClusterWeaponSize;
	if( cluster < 2 )
		return 1;
	
	if( ! roll )
		roll = Roll::Dice( 2 );  // FIXME: (Weapon->Type == HMWeapon::MISSILE_3D6) means what?
	
	if( mech->ActiveStealth )
		ecm = true;
	
	uint8_t roll_bonus = 0;
	std::string bonus_type = "";
	
	if( Weapon->ClusterRollBonus )
	{
		bonus_type = "Integrated Artemis IV";
		roll_bonus = Weapon->ClusterRollBonus; // Integrated FCS such as Artemis IV in ATMs.
	}
	else if( WeaponFCS )
	{
		bonus_type = WeaponFCS->Name;
		if( fcs )
			roll_bonus = 2;
	}
	else if( narc && (Weapon->CanUseArtemisLRM || Weapon->CanUseArtemisSRM) )
	{
		bonus_type = "Narc-seeking ammo";
		roll_bonus = 2;
	}
	
	if( roll_bonus )
	{
		BattleTechServer *server = (BattleTechServer*) Raptor::Server;
		Event e( mech );
		
		if( ecm )
			e.Text = bonus_type + std::string(" cluster bonus blocked by ECM.");
		else if( indirect && ! narc )
			e.Text = bonus_type + std::string(" does not apply to indirect fire.");
		else
		{
			e.Text = bonus_type + std::string(" adds +") + Num::ToString((int)roll_bonus) + std::string(" to cluster roll ") + Num::ToString((int)roll) + std::string(".");
			roll += roll_bonus;
		}
		
		server->Events.push( e );
	}
	
	if( ams )
	{
		BattleTechServer *server = (BattleTechServer*) Raptor::Server;
		Event e( ams->MyMech() );
		e.Effect = BattleTech::Effect::BLINK;
		e.Sound = ams->Weapon->Sound;
		e.Text = ams->Name + std::string(" takes -4 from cluster roll ") + Num::ToString((int)roll);
		
		roll = std::max<int8_t>( 0, roll - 4 );
		if( ams->Weapon->AmmoPerTon && (roll < 2) )
		{
			std::map<std::string,std::string>::const_iterator enhanced_ams = mech->Data->Properties.find("enhanced_ams");
			if( (enhanced_ams == mech->Data->Properties.end()) || ! Str::AsBool(enhanced_ams->second) )
				roll = 2;
		}
		
		if( roll )
			e.Text += std::string(" to ") + Num::ToString((int)roll) + std::string((roll == 2)?" (minimum).":".");
		else
			e.Text += " and no missiles hit the target.";
		server->Events.push( e );
	}
	
	return (roll >= 2) ? Roll::ClusterWithEvent( cluster, roll, Weapon->Damage ) : 0;
}


void MechEquipment::Hit( bool directly, bool find_slot )
{
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	Mech *mech = MyMech();
	
	Damaged ++;
	
	if( Location )
	{
		Location->DamagedCriticals.insert( ID );
		
		if( find_slot )
		{
			for( std::vector<MechEquipment*>::iterator crit = Location->CriticalSlots.begin(); crit != Location->CriticalSlots.end(); crit ++ )
			{
				if( *crit == this )
				{
					Location->CriticalSlots.erase( crit );
					break;
				}
			}
		}
	}
	
	if( (ID == BattleTech::Equipment::COCKPIT) && (Damaged == 1) )
	{
		Event e( mech );
		e.Effect = BattleTech::Effect::EXPLOSION;
		e.Sound = "m_expl.wav";
		e.Text = mech->ShortFullName() + std::string(" destroyed by cockpit hit!");
		e.Duration = 2.f;
		e.ShowCritHit( this );
		server->Events.push( e );
	}
	
	else if( ID == BattleTech::Equipment::ENGINE )
	{
		if( mech )
			mech->HeatDissipation -= 5;
		
		if( Damaged == 3 )
		{
			Event e( mech );
			e.Effect = BattleTech::Effect::EXPLOSION;
			e.Sound = "m_expl.wav";
			e.Text = mech->ShortFullName() + std::string(" destroyed by engine failure!");
			e.Duration = 2.f;
			e.ShowCritHit( this );
			server->Events.push( e );
			
			std::map<std::string,std::string>::const_iterator engine_explosions = mech->Data->Properties.find("engine_explosions");
			if( (engine_explosions != mech->Data->Properties.end()) && Str::AsBool(engine_explosions->second) )
			{
				Event e2( mech );
				uint8_t roll = Roll::Dice( 2 );
				if( roll >= 10 )
				{
					e2.Effect = BattleTech::Effect::BLINK;
					e2.Misc = BattleTech::RGB332::ORANGE;
					e2.Sound = "i_warn.wav";
				}
				e2.Text = std::string("Reactor will go critical on roll 10 or higher!  Rolled ") + Num::ToString((int)roll) + std::string(".");
				server->Events.push( e2 );
				if( roll >= 10 )
					mech->EngineExplode();
			}
		}
	}
	
	else if( (ID == BattleTech::Equipment::SINGLE_HEAT_SINK) && (Damaged == 1) && mech )
		mech->HeatDissipation -= 1;
	
	else if( (ID == BattleTech::Equipment::DOUBLE_HEAT_SINK) && (Damaged == 1) && mech )
		mech->HeatDissipation -= 2;
	
	else if( (ID == BattleTech::Equipment::GYRO) && mech && ! mech->Prone )
	{
		mech->PSRs ++;
		if( Damaged == 2 )
			mech->AutoFall = "gyro destruction";
	}
	
	else if( ((ID == BattleTech::Equipment::HIP)
	||        (ID == BattleTech::Equipment::UPPER_LEG_ACTUATOR)
	||        (ID == BattleTech::Equipment::LOWER_LEG_ACTUATOR)
	||        (ID == BattleTech::Equipment::FOOT_ACTUATOR))
	&& mech && ! mech->Prone )
		mech->PSRs ++;
	
	else if( directly && ExplosionDamage && Location )
	{
		uint16_t explosion = ExplosionDamage;
		if( Ammo )
		{
			explosion *= Ammo;
			Ammo = 0;
		}
		ExplosionDamage = 0;
		
		Event e( mech );
		/*
		e.Effect = BattleTech::Effect::BLINK;
		e.Misc = BattleTech::RGB332::RED;
		*/
		e.ShowCritHit( this );
		e.Sound = "b_ammoex.wav";
		e.Text = Name + std::string(" exploded for ") + Num::ToString((int)explosion) + std::string(" damage!");
		e.Duration = 2.f;
		server->Events.push( e );
		
		mech->HitPilot( 2 );
		
		Location->Damage( explosion, STRUCTURE, 0 );
	}
}


// -----------------------------------------------------------------------------


MechLocation::MechLocation( void )
{
	DamageTransfer = NULL;
	CritTransfer = NULL;
	AttachedLimb = NULL;
	Structure = MaxStructure = 3;
	Armor = MaxArmor = 0;
	RearArmor = MaxRearArmor = 0;
	IsTorso = false;
	IsLeg = false;
	IsHead = false;
	CASE = false;
	Narced = false;
	HitWhere = BattleTech::Loc::UNKNOWN;
	Destroyed = false;
}


MechLocation::~MechLocation()
{
}


bool MechLocation::IsArm( void ) const
{
	return ! (IsTorso || IsHead || IsLeg);
}


uint16_t *MechLocation::ArmorPtr( uint8_t arc )
{
	if( arc == STRUCTURE )
		return &Structure;
	else if( (arc == REAR) && IsTorso )
		return &RearArmor;
	else
		return &Armor;
}


size_t MechLocation::IntactCriticalCount( uint16_t crit_id ) const
{
	size_t count = 0;
	for( std::vector<MechEquipment*>::const_iterator crit = CriticalSlots.begin(); crit != CriticalSlots.end(); crit ++ )
	{
		if( (*crit)->ID == crit_id )
			count ++;
	}
	return count;
}


void MechLocation::Damage( uint16_t damage, uint8_t arc, uint8_t crit )
{
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	if( crit && ! damage )
	{
		// Critical Hit Checks (rolled 2 location without damage)
		CriticalHitCheck( crit );
		crit = 0;
	}
	
	if( damage && Structure )
	{
		// Armor
		uint16_t armor_damage = 0;
		uint16_t *armor = ArmorPtr( arc );
		if( *armor && (arc != STRUCTURE) ) // Ammo explosions don't hit armor.
		{
			armor_damage = std::min<uint16_t>( damage, *armor );
			*armor -= armor_damage;
			damage -= armor_damage;
			
			if( MyMech )
				MyMech->PhaseDamage += armor_damage;
			
			Event e( MyMech );
			e.Text = Num::ToString((int)armor_damage) + std::string(" damage soaked by armor; ") + Num::ToString((int)*armor) + std::string(" armor remaining.");
			e.ShowHealth( Loc, arc );
			server->Events.push( e );
		}
		
		// Structure
		uint16_t structure_damage = 0;
		if( damage )
		{
			// Structure hit adds a critical check.
			crit ++;
			
			structure_damage = std::min<uint16_t>( damage, Structure );
			Structure -= structure_damage;
			damage    -= structure_damage;
			
			if( MyMech )
				MyMech->PhaseDamage += structure_damage;
			
			Event e( MyMech );
			e.Sound = "m_struct.wav";
			e.Text = Num::ToString((int)structure_damage) + std::string(" damage to structure; ") + Num::ToString((int)Structure) + std::string(" structure remaining.");
			e.ShowHealth( Loc, STRUCTURE );
			server->Events.push( e );
		}
		bool structure_destroyed = structure_damage && ! Structure;
		
		// Critical Hit Checks (structure damage and/or rolled 2 locaiton)
		CriticalHitCheck( crit );
		crit = 0;
		
		if( MyMech && MyMech->Destroyed() )
			return;
		
		// Check Location Destroyed
		if( structure_destroyed )
		{
			Event e( MyMech );
			e.Effect = BattleTech::Effect::BLINK;
			e.Misc = BattleTech::RGB332::RED;
			e.Sound = "m_alarm.wav";
			e.Text = Name + std::string(" destroyed!");
			server->Events.push( e );
			
			// Don't allow using equipment on destroyed locations.
			uint8_t engine_hits = 0;
			for( std::vector<MechEquipment*>::iterator slot = CriticalSlots.begin(); slot != CriticalSlots.end(); slot ++ )
			{
				(*slot)->Hit( false, false );
				if( (*slot)->ID == BattleTech::Equipment::ENGINE )
					engine_hits ++;
			}
			
			// Remove all critical slots on destroyed regions.
			CriticalSlots.clear();
			
			if( engine_hits && !(MyMech && MyMech->Destroyed()) )
			{
				Event e( MyMech );
				e.Text = std::string("Engine takes ") + Num::ToString(engine_hits) + std::string((engine_hits == 1)?" hit.":" hits.");
				e.ShowCritHit( MyMech->Engine, Loc );
				server->Events.push( e );
			}
			
			// Lose the attached limb if side torso was just destroyed.
			if( AttachedLimb )
			{
				if( AttachedLimb->Structure && !(MyMech && MyMech->Destroyed()) )
				{
					Event e( MyMech );
					/*
					e.Effect = BattleTech::Effect::BLINK;
					e.Misc = BattleTech::RGB332::RED;
					*/
					e.Sound = "m_struct.wav";
					e.Text = AttachedLimb->Name + std::string(" blown off!");
					e.ShowHealth( AttachedLimb->Loc, STRUCTURE, 0 );
					server->Events.push( e );
				}
				
				AttachedLimb->Structure = 0;
				AttachedLimb->Armor = 0;
				for( std::vector<MechEquipment*>::iterator slot = AttachedLimb->CriticalSlots.begin(); slot != AttachedLimb->CriticalSlots.end(); slot ++ )
					(*slot)->Hit( false, false );
				AttachedLimb->CriticalSlots.clear();
				
				AttachedLimb = NULL;
			}
			
			// Destroyed arm should no longer be considered attached.
			else if( DamageTransfer && (DamageTransfer->AttachedLimb == this) )
				DamageTransfer->AttachedLimb = NULL;
			
			// Losing a leg is an automatic fall.
			if( IsLeg && MyMech && ! MyMech->Prone && ! MyMech->Destroyed() )
			{
				MyMech->PSRs ++;
				MyMech->AutoFall = "leg destruction";
			}
		}
		else if( IsHead && MyMech )
			// Pilot takes 1 point of damage for any head hit.
			MyMech->HitPilot();
	}
	
	if( MyMech && MyMech->Destroyed() )
		return;
	
	// Damage Transfer
	if( (arc == STRUCTURE) && CASE )
	{
		Event e( MyMech );
		e.Text = "CASE prevented ammo explosion damage transfer.";
		server->Events.push( e );
		
		// Count untransferred ammo explosion damage towards total this phase (PSR at 20+).
		MyMech->PhaseDamage += damage;
		
		return;
	}
	if( damage && DamageTransfer )
	{
		Event e( MyMech );
		e.Text = std::string("Transferring ") + Num::ToString((int)damage) + std::string(" damage to ") + DamageTransfer->Name;
		if( arc == STRUCTURE )
			e.Text += std::string(" structure (ammo explosion).");
		else if( IsTorso && (arc == REAR) )
			e.Text += std::string(" rear.");
		else
			e.Text += std::string(".");
		e.Duration = 0.6f;
		server->Events.push( e );
		
		DamageTransfer->Damage( damage, arc, 0 );
	}
}


void MechLocation::CriticalHitCheck( uint8_t crit )
{
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	for( uint8_t i = 0; i < crit; i ++ )
	{
		int roll = Roll::Dice( 2 );
		
		uint8_t hits = 0;
		if( (roll == 12) && ! IsTorso )
			Structure = 0;
		else if( roll == 12 )
			hits = 3;
		else if( roll >= 10 )
			hits = 2;
		else if( roll >= 8 )
			hits = 1;
		
		Event e( MyMech );
		e.Text = Name + std::string(" rolled ") + Num::ToString(roll) + std::string(" on critical check");
		if( (roll == 12) && ! IsTorso )
		{
			e.Sound = "m_alarm.wav";
			e.Text += std::string(", destroying the limb!");
			e.ShowHealth( Loc, STRUCTURE );
		}
		else
			e.Text += std::string(" for ") + Num::ToString((int)hits) + std::string((hits == 1)?" hit.":" hits.");
		server->Events.push( e );
		
		if( hits )
			CriticalHits( hits );
		
		if( MyMech && MyMech->Destroyed() )
			return;
	}
}


void MechLocation::CriticalHits( uint8_t hits )
{
	// This will only be non-NULL on a phase after this location's crits are all hit.
	if( CritTransfer )
	{
		CritTransfer->CriticalHits( hits );
		return;
	}
	
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	// Apply damage to equipment, and strike the lines from crit table.
	for( uint8_t i = 0; i < hits; i ++ )
	{
		// If we get too many critical hits in one phase, the extras are lost.
		if( ! CriticalSlots.size() )
			break;
		
		int index = Rand::Int( 0, CriticalSlots.size() - 1 );
		MechEquipment *crit = CriticalSlots[ index ];
		CriticalSlots.erase( CriticalSlots.begin() + index );
		
		Event e( MyMech );
		/*
		e.Effect = BattleTech::Effect::BLINK;
		e.Misc = BattleTech::RGB332::DARK_RED;
		*/
		e.ShowCritHit( crit, Loc );
		e.Sound = "b_crit.wav";
		e.Text = std::string("Critical hit: ") + crit->Name;
		if( (crit->ID == BattleTech::Equipment::ENGINE) || (crit->ID == BattleTech::Equipment::GYRO) )
			e.Text += std::string(" (") + Num::ToString( crit->Damaged + 1 ) + std::string(" total)");
		server->Events.push( e );
		
		// Apply hit to equipment, and check for weapon or ammo explosion.
		crit->Hit( true, false );
	}
}


// -----------------------------------------------------------------------------


std::string MechLocationRoll::String( const char *verb )
{
	std::string arc_name = "Front";
	if( Arc == REAR )
		arc_name = "Rear";
	else if( Arc == LEFT_SIDE )
		arc_name = "Left Side";
	else if( Arc == RIGHT_SIDE )
		arc_name = "Right Side";
	
	if( Table == BattleTech::HitTable::KICK )
		arc_name += " Kick Table";
	else if( Table == BattleTech::HitTable::PUNCH )
		arc_name += " Punch Table";
	
	std::string loc_name = Loc ? Loc->Name : "Mech";
	if( Loc && Loc->IsTorso && (Arc == REAR) )
		loc_name += " (Rear)";
	if( (Crit || (Roll == 2)) && ((Table == BattleTech::HitTable::STANDARD) || ! Table) )
		loc_name += " (Critical)";
	
	return std::string("Location roll ") + Num::ToString((int)Roll)
		+ std::string(" on ") + arc_name
		+ std::string(" ") + std::string(verb) + std::string(" ") + loc_name;
}


// -----------------------------------------------------------------------------


Mech::Mech( uint32_t id ) : GameObject( id, BattleTech::Object::MECH )
{
	SmoothPos = false;
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		Locations[ i ].MyMech = this;
		Locations[ i ].Loc = i;
	}
	
	Locations[ CENTER_TORSO ].IsTorso = true;
	Locations[ LEFT_TORSO   ].IsTorso = true;
	Locations[ RIGHT_TORSO  ].IsTorso = true;
	
	Locations[ LEFT_LEG  ].IsLeg = true;
	Locations[ RIGHT_LEG ].IsLeg = true;
	
	Locations[ HEAD ].IsHead = true;
	
	Locations[ LEFT_TORSO  ].DamageTransfer = &(Locations[ CENTER_TORSO ]);
	Locations[ RIGHT_TORSO ].DamageTransfer = &(Locations[ CENTER_TORSO ]);
	Locations[ LEFT_LEG    ].DamageTransfer = &(Locations[ LEFT_TORSO   ]);
	Locations[ RIGHT_LEG   ].DamageTransfer = &(Locations[ RIGHT_TORSO  ]);
	Locations[ LEFT_ARM    ].DamageTransfer = &(Locations[ LEFT_TORSO   ]);
	Locations[ RIGHT_ARM   ].DamageTransfer = &(Locations[ RIGHT_TORSO  ]);
	
	Locations[ LEFT_TORSO  ].AttachedLimb = &(Locations[ LEFT_ARM  ]);
	Locations[ RIGHT_TORSO ].AttachedLimb = &(Locations[ RIGHT_ARM ]);
	
	Locations[ HEAD         ].Name = "Head";
	Locations[ CENTER_TORSO ].Name = "Center Torso";
	Locations[ LEFT_TORSO   ].Name = "Left Torso";
	Locations[ RIGHT_TORSO  ].Name = "Right Torso";
	Locations[ LEFT_LEG     ].Name = "Left Leg";
	Locations[ RIGHT_LEG    ].Name = "Right Leg";
	Locations[ LEFT_ARM     ].Name = "Left Arm";
	Locations[ RIGHT_ARM    ].Name = "Right Arm";
	
	Locations[ HEAD         ].ShortName = "HD";
	Locations[ CENTER_TORSO ].ShortName = "CT";
	Locations[ LEFT_TORSO   ].ShortName = "LT";
	Locations[ RIGHT_TORSO  ].ShortName = "RT";
	Locations[ LEFT_LEG     ].ShortName = "LL";
	Locations[ RIGHT_LEG    ].ShortName = "RL";
	Locations[ LEFT_ARM     ].ShortName = "LA";
	Locations[ RIGHT_ARM    ].ShortName = "RA";
	
	LifeSupport = NULL;
	Sensors = NULL;
	Cockpit = NULL;
	Engine = NULL;
	Gyro = NULL;
	MASC = NULL;
	ECM = NULL;
	
	Tons = 100;
	Walk = 5;
	Jump = 0;
	HeatDissipation = 10;
	
	Clan = false;
	Quad = false;
	TSM = false;
	Stealth = false;
	
	PilotSkill   = 5;
	GunnerySkill = 4;
	
	Team = 0;
	
	X = 0;
	Y = 0;
	Facing = 0;
	TorsoTwist = 0;
	ArmsFlipped = false;
	Prone = false;
	ProneFire = BattleTech::Loc::UNKNOWN;
	TurnedArm = BattleTech::Loc::UNKNOWN;
	
	MoveEffect = BattleTech::Effect::NONE;
	FromX = 0;
	FromY = 0;
	FromFacing = 0;
	FromTorsoTwist = 0;
	FromProne = false;
	FromProneFire = BattleTech::Loc::UNKNOWN;
	FromTurnedArm = BattleTech::Loc::UNKNOWN;
	
	MASCTurns = 0;
	PilotDamage = 0;
	Unconscious = 0;
	Heat = 0;
	OutsideHeat = 0;
	Shutdown = false;
	Attack = 0;
	Defense = 0;
	HeatMove = 0;
	HeatFire = 0;
	ActiveTSM = false;
	ActiveStealth = false;
	Spotted = 99;
	Tagged = false;
	PhaseDamage = 0;
	PSRs = 0;
	PSRModifier = 0;
	StandAttempts = 0;
	TookTurn = false;
}


Mech::~Mech()
{
}


void Mech::ClientInit( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	game->Snd.Play( game->Res.GetSound("m_start.wav") );
	
	
	std::string fullname = FullName();
	std::map< std::string, std::map<uint8_t,const Animation*> >::const_iterator best = game->MechTex.end();
	int best_at = fullname.length();
	
	for( std::map< std::string, std::map<uint8_t,const Animation*> >::const_iterator tex = game->MechTex.begin(); tex != game->MechTex.end(); tex ++ )
	{
		int at = Str::FindInsensitive( fullname, tex->first );
		if( at >= 0 )
		{
			if( (best == game->MechTex.end())
			||  (! best->first.length())
			||  (at < best_at)
			||  ((at == best_at) && (tex->first.length() > best->first.length())) )
			{
				best = tex;
				best_at = at;
			}
		}
	}
	
	if( best != game->MechTex.end() )
	{
		for( std::map<uint8_t,const Animation*>::const_iterator ani = best->second.begin(); ani != best->second.end(); ani ++ )
			Locations[ ani->first ].Tex.BecomeInstance( ani->second );
	}
	
	
	if( PlayerID == game->PlayerID )
	{
		game->SelectedID = ID;
		//game->Snd.Play( game->Res.GetSound("b_start.wav") );
	}
}


bool Mech::PlayerShouldUpdateServer( void ) const
{
	return false;
}

bool Mech::ServerShouldUpdatePlayer( void ) const
{
	return false;
}

bool Mech::ServerShouldUpdateOthers( void ) const
{
	return false;
}

bool Mech::CanCollideWithOwnType( void ) const
{
	return false;
}

bool Mech::CanCollideWithOtherTypes( void ) const
{
	return false;
}

bool Mech::IsMoving( void ) const
{
	return false;
}


void Mech::AddToInitPacket( Packet *packet, int8_t precision )
{
	packet->AddUShort( PlayerID );
	
	packet->AddUChar( X );
	packet->AddUChar( Y );
	packet->AddUChar( Facing );
	
	packet->AddString( Name );
	packet->AddString( Var );
	
	packet->AddUChar( Tons );
	
	uint8_t flags = 0;
	if( Clan )
		flags |= 0x01;
	if( Quad )
		flags |= 0x02;
	if( TSM )
		flags |= 0x04;
	if( Stealth )
		flags |= 0x08;
	packet->AddUChar( flags );
	
	packet->AddUChar( Walk );
	packet->AddUChar( Jump );
	packet->AddChar( HeatDissipation );
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		packet->AddUChar( Locations[ i ].MaxStructure );
		packet->AddUChar( Locations[ i ].Structure );
		packet->AddUChar( Locations[ i ].MaxArmor );
		packet->AddUChar( Locations[ i ].Armor );
		if( Locations[ i ].IsTorso )
		{
			packet->AddUChar( Locations[ i ].MaxRearArmor );
			packet->AddUChar( Locations[ i ].RearArmor );
		}
	}
	
	packet->AddUChar( Equipment.size() );
	for( std::vector<MechEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
		eq->AddToInitPacket( packet );
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		packet->AddUChar( Locations[ i ].CriticalSlots.size() );
		for( std::vector<MechEquipment*>::const_iterator slot = Locations[ i ].CriticalSlots.begin(); slot != Locations[ i ].CriticalSlots.end(); slot ++ )
			packet->AddChar( (*slot)->Index() );
	}
	
	packet->AddChar( PilotSkill );
	packet->AddChar( GunnerySkill );
	packet->AddUChar( Team );
	
	AddToUpdatePacketFromServer( packet, 127 );
}


void Mech::ReadFromInitPacket( Packet *packet, int8_t precision )
{
	PlayerID = packet->NextUShort();
	
	X = packet->NextUChar();
	Y = packet->NextUChar();
	Facing = packet->NextUChar();
	
	Name = packet->NextString();
	Var  = packet->NextString();
	
	Tons = packet->NextUChar();
	
	uint8_t flags = packet->NextUChar();
	Clan    = flags & 0x01;
	Quad    = flags & 0x02;
	TSM     = flags & 0x04;
	Stealth = flags & 0x08;
	
	Walk = packet->NextUChar();
	Jump = packet->NextUChar();
	HeatDissipation = packet->NextChar();
	
	Locations[ LEFT_ARM  ].IsLeg = Quad;
	Locations[ RIGHT_ARM ].IsLeg = Quad;
	Locations[ LEFT_TORSO  ].AttachedLimb = Quad ? NULL : &(Locations[ LEFT_ARM  ]);
	Locations[ RIGHT_TORSO ].AttachedLimb = Quad ? NULL : &(Locations[ RIGHT_ARM ]);
	Locations[ LEFT_ARM  ].Name = Quad ? "Left Front Leg"  : "Left Arm";
	Locations[ RIGHT_ARM ].Name = Quad ? "Right Front Leg" : "Right Arm";
	Locations[ LEFT_LEG  ].Name = Quad ? "Left Rear Leg"   : "Left Leg";
	Locations[ RIGHT_LEG ].Name = Quad ? "Right Rear Leg"  : "Right Arm";
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		Locations[ i ].MaxStructure = packet->NextUChar();
		Locations[ i ].Structure    = packet->NextUChar();
		Locations[ i ].MaxArmor     = packet->NextUChar();
		Locations[ i ].Armor        = packet->NextUChar();
		if( Locations[ i ].IsTorso )
		{
			Locations[ i ].MaxRearArmor = packet->NextUChar();
			Locations[ i ].RearArmor    = packet->NextUChar();
		}
	}
	
	size_t equipment = packet->NextUChar();
	
	// First create the complete Equipment list so pointers to its elements will remain valid.
	for( size_t i = 0; i < equipment; i ++ )
		Equipment.push_back( MechEquipment() );
	
	for( size_t i = 0; i < equipment; i ++ )
		Equipment[ i ].ReadFromInitPacket( packet, this );
	
	bool arms_can_flip = true;
	for( size_t i = 0; i < equipment; i ++ )
	{
		if( Equipment[ i ].ID == BattleTech::Equipment::LIFE_SUPPORT )
			LifeSupport = &(Equipment[ i ]);
		if( Equipment[ i ].ID == BattleTech::Equipment::SENSORS )
			Sensors = &(Equipment[ i ]);
		if( Equipment[ i ].ID == BattleTech::Equipment::COCKPIT )
			Cockpit = &(Equipment[ i ]);
		if( Equipment[ i ].ID == BattleTech::Equipment::ENGINE )
			Engine = &(Equipment[ i ]);
		if( Equipment[ i ].ID == BattleTech::Equipment::GYRO )
			Gyro = &(Equipment[ i ]);
		if( Equipment[ i ].ID == BattleTech::Equipment::MASC )
			MASC = &(Equipment[ i ]);
		if( strstr( Equipment[ i ].Name.c_str(), "ECM" ) )
			ECM = &(Equipment[ i ]);
		if( (Equipment[ i ].ID == BattleTech::Equipment::LOWER_ARM_ACTUATOR)
		||  (Equipment[ i ].ID == BattleTech::Equipment::HAND_ACTUATOR) )
			arms_can_flip = false;
	}
	
	for( size_t i = 0; i < equipment; i ++ )
	{
		if( Equipment[ i ].Weapon && ! Equipment[ i ].Weapon->Defensive )
		{
			WeaponsToFire[ i ] = Equipment[ i ].Weapon->RapidFire;
			
			// FIXME: Arm flipping will need to be done differently when multiple attack targets are possible!
			if( arms_can_flip && (Equipment[ i ].WeaponArcs.count( BattleTech::Arc::LEFT_SIDE ) || Equipment[ i ].WeaponArcs.count( BattleTech::Arc::RIGHT_SIDE )) )
				Equipment[ i ].WeaponArcs.insert( BattleTech::Arc::REAR );
		}
	}
	
	WeaponsToFire[ 0xFF ] = 0;  // Spot for Indirect Fire
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		uint8_t count = packet->NextUChar();
		for( uint8_t j = 0; j < count; j ++ )
		{
			int8_t eq_index = packet->NextChar();
			if( eq_index >= 0 )
				Locations[ i ].CriticalSlots.push_back( &(Equipment[ eq_index ]) );
		}
	}
	
	PilotSkill   = packet->NextChar();
	GunnerySkill = packet->NextChar();
	Team         = packet->NextUChar();
	
	BeginPhase( BattleTech::State::SETUP );
	
	ReadFromUpdatePacketFromServer( packet, 127 );
}


void Mech::AddToUpdatePacketFromServer( Packet *packet, int8_t precision )
{
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( precision == 127 )
		{
			packet->AddUChar( Locations[ i ].Structure );
			packet->AddUChar( Locations[ i ].Armor );
			if( Locations[ i ].IsTorso )
				packet->AddUChar( Locations[ i ].RearArmor );
			
			uint8_t crits_and_narc = Locations[ i ].DamagedCriticals.size();
			if( Locations[ i ].Narced )
				crits_and_narc |= 0x80;
			packet->AddUChar( crits_and_narc );
			for( std::multiset<uint16_t>::const_iterator crit = Locations[ i ].DamagedCriticals.begin(); crit != Locations[ i ].DamagedCriticals.end(); crit ++ )
				packet->AddUShort( *crit );
			
			packet->AddUChar( Locations[ i ].CriticalSlots.size() );
			for( std::vector<MechEquipment*>::const_iterator slot = Locations[ i ].CriticalSlots.begin(); slot != Locations[ i ].CriticalSlots.end(); slot ++ )
				packet->AddChar( (*slot)->Index() );
		}
		else
			packet->AddUChar( Locations[ i ].Narced ? 1 : 0 );
	}
	
	for( std::vector<MechEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
	{
		uint8_t damage_and_jam = eq->Damaged;
		if( eq->Jammed )
			damage_and_jam |= 0x80;
		packet->AddUChar( damage_and_jam );
		packet->AddUShort( eq->Ammo );
	}
	
	packet->AddUChar( X );
	packet->AddUChar( Y );
	
	uint8_t facing_and_flags = Facing;
	if( Prone )
		facing_and_flags |= 0x80;
	if( Shutdown )
		facing_and_flags |= 0x40;
	if( ActiveTSM )
		facing_and_flags |= 0x20;
	if( ActiveStealth )
		facing_and_flags |= 0x10;
	packet->AddUChar( facing_and_flags );
	
	uint8_t pilot = PilotDamage;
	if( Unconscious )
		pilot |= 0x80;
	packet->AddUChar( pilot );
	
	uint8_t modifiers = Defense | (Attack << 4);
	if( Tagged )
		modifiers |= 0x80;
	packet->AddUChar( modifiers );
	
	packet->AddChar( Spotted );
	
	packet->AddUChar( Heat );
	packet->AddUChar( HeatDissipation );
	packet->AddUChar( HeatMove | (HeatFire << 4) );
}


void Mech::ReadFromUpdatePacketFromServer( Packet *packet, int8_t precision )
{
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( precision == 127 )
		{
			Locations[ i ].Structure = packet->NextUChar();
			Locations[ i ].Armor     = packet->NextUChar();
			if( Locations[ i ].IsTorso )
				Locations[ i ].RearArmor = packet->NextUChar();
			
			uint8_t crits_and_narc = packet->NextUChar();
			Locations[ i ].Narced = crits_and_narc & 0x80;
			uint8_t crit_count    = crits_and_narc & 0x7F;
			Locations[ i ].DamagedCriticals.clear();
			for( uint8_t j = 0; j < crit_count; j ++ )
				Locations[ i ].DamagedCriticals.insert( packet->NextUShort() );
			
			uint8_t count = packet->NextUChar();
			Locations[ i ].CriticalSlots.clear();
			for( uint8_t j = 0; j < count; j ++ )
			{
				int8_t eq_index = packet->NextChar();
				if( eq_index >= 0 )
					Locations[ i ].CriticalSlots.push_back( &(Equipment[ eq_index ]) );
			}
		}
		else
			Locations[ i ].Narced = packet->NextUChar();
	}
	
	for( std::vector<MechEquipment>::iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
	{
		uint8_t damage_and_jam = packet->NextUChar();
		if( precision == 127 )
			eq->Damaged = damage_and_jam & 0x7F;
		eq->Jammed = damage_and_jam & 0x80;
		
		eq->Ammo = packet->NextUShort();
	}
	
	uint8_t x = packet->NextUChar();
	uint8_t y = packet->NextUChar();
	uint8_t facing_and_flags = packet->NextUChar();
	
	Shutdown      = facing_and_flags & 0x40;
	ActiveTSM     = facing_and_flags & 0x20;
	ActiveStealth = facing_and_flags & 0x10;
	if( precision >= 0 )
	{
		X = x;
		Y = y;
		Facing = facing_and_flags & 0x07;
		Prone  = facing_and_flags & 0x80;
	}
	
	uint8_t pilot = packet->NextUChar();
	PilotDamage = pilot & 0x07;
	Unconscious = pilot & 0x80;
	
	uint8_t modifiers = packet->NextUChar();
	Defense = modifiers & 0x0F;
	Attack = (modifiers & 0x30) >> 4;
	Tagged  = modifiers & 0x80;
	
	Spotted = packet->NextChar();
	
	Heat = packet->NextUChar();
	HeatDissipation = packet->NextUChar();
	
	uint8_t heat_effects = packet->NextUChar();
	HeatMove =  heat_effects & 0x0F;
	HeatFire = (heat_effects & 0xF0) >> 4;
}


void Mech::SetVariant( const Variant &variant )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	Name = variant.Name;
	Var  = variant.Var;
	if( Var[0] == ' ' )
		Var = Var.substr( 1 );
	
	Tons = variant.Tons;
	HMStructure structure = game->Struct[ Tons ];
	Locations[ HEAD         ].Structure = Locations[ HEAD         ].MaxStructure = structure.Head;
	Locations[ CENTER_TORSO ].Structure = Locations[ CENTER_TORSO ].MaxStructure = structure.CenterTorso;
	Locations[ LEFT_TORSO   ].Structure = Locations[ LEFT_TORSO   ].MaxStructure = structure.SideTorso;
	Locations[ RIGHT_TORSO  ].Structure = Locations[ RIGHT_TORSO  ].MaxStructure = structure.SideTorso;
	Locations[ LEFT_ARM     ].Structure = Locations[ LEFT_ARM     ].MaxStructure = structure.Arm;
	Locations[ RIGHT_ARM    ].Structure = Locations[ RIGHT_ARM    ].MaxStructure = structure.Arm;
	Locations[ LEFT_LEG     ].Structure = Locations[ LEFT_LEG     ].MaxStructure = structure.Leg;
	Locations[ RIGHT_LEG    ].Structure = Locations[ RIGHT_LEG    ].MaxStructure = structure.Leg;
	
	Clan = variant.Clan;
	
	Quad = variant.Quad;
	Locations[ LEFT_ARM  ].IsLeg = Quad;
	Locations[ RIGHT_ARM ].IsLeg = Quad;
	Locations[ LEFT_TORSO  ].AttachedLimb = Quad ? NULL : &(Locations[ LEFT_ARM  ]);
	Locations[ RIGHT_TORSO ].AttachedLimb = Quad ? NULL : &(Locations[ RIGHT_ARM ]);
	Locations[ LEFT_ARM  ].Name = Quad ? "Left Front Leg"  : "Left Arm";
	Locations[ RIGHT_ARM ].Name = Quad ? "Right Front Leg" : "Right Arm";
	Locations[ LEFT_LEG  ].Name = Quad ? "Left Rear Leg"   : "Left Leg";
	Locations[ RIGHT_LEG ].Name = Quad ? "Right Rear Leg"  : "Right Leg";
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		Locations[ i ].Armor = Locations[ i ].MaxArmor = variant.Armor[ i ];
		Locations[ i ].CASE = variant.CASE & (1 << i);
	}
	Locations[ LEFT_TORSO   ].RearArmor = Locations[ LEFT_TORSO   ].MaxRearArmor = variant.Armor[ LEFT_TORSO_REAR   ];
	Locations[ RIGHT_TORSO  ].RearArmor = Locations[ RIGHT_TORSO  ].MaxRearArmor = variant.Armor[ RIGHT_TORSO_REAR  ];
	Locations[ CENTER_TORSO ].RearArmor = Locations[ CENTER_TORSO ].MaxRearArmor = variant.Armor[ CENTER_TORSO_REAR ];
	
	Stealth = variant.Stealth;
	ActiveStealth = Stealth;  // FIXME: This should be a choice.
	
	Walk = variant.Walk;
	Jump = variant.Jump;
	TSM  = variant.TSM;
	
	HeatDissipation = variant.Heatsinks;
	if( variant.HeatsinksDouble )
		HeatDissipation *= 2;
	
	Equipment.clear();
	LifeSupport = NULL;
	Sensors = NULL;
	Cockpit = NULL;
	Engine = NULL;
	Gyro = NULL;
	MASC = NULL;
	ECM = NULL;
	
	MechEquipment *targeting_computer = NULL;
	std::map< MechLocation*, std::queue<MechEquipment*> > weapons_with_fcs;
	
	// First create the complete Equipment list so pointers to its elements will remain valid.
	for( size_t i = 0; i < variant.Equipment.size(); i ++ )
		Equipment.push_back( MechEquipment( variant.Equipment[ i ].ID, Clan, variant.Equipment[ i ].Ammo ) );
	
	for( size_t i = 0; i < variant.Equipment.size(); i ++ )
	{
		MechEquipment *equipment = &(Equipment[ i ]);
		
		if( variant.Equipment[ i ].CritLocs.size() )
			equipment->Location = &(Locations[ variant.Equipment[ i ].CritLocs.at(0) ]);
		
		if( variant.Equipment[ i ].Rear )
			equipment->WeaponArcs.insert( BattleTech::Arc::REAR );
		else if( equipment->Weapon )
		{
			equipment->WeaponArcs.insert( BattleTech::Arc::FRONT );
			
			//bool arm_mounted = false;
			if( variant.Equipment[ i ].CritLocs.size() && (variant.Equipment[ i ].CritLocs.at(0) == LEFT_ARM) && ! Quad )
			{
				equipment->WeaponArcs.insert( BattleTech::Arc::LEFT_SIDE );
				//arm_mounted = true;
			}
			else if( variant.Equipment[ i ].CritLocs.size() && (variant.Equipment[ i ].CritLocs.at(0) == RIGHT_ARM) && ! Quad )
			{
				equipment->WeaponArcs.insert( BattleTech::Arc::RIGHT_SIDE );
				//arm_mounted = true;
			}
		}
		
		if( variant.Equipment[ i ].HasFCS && equipment->Location )
			weapons_with_fcs[ equipment->Location ].push( equipment );
		else if( (equipment->ID == BattleTech::Equipment::ARTEMIS_IV_FCS) && equipment->Location && weapons_with_fcs[ equipment->Location ].size() )
		{
			weapons_with_fcs[ equipment->Location ].front()->WeaponFCS = equipment;
			weapons_with_fcs[ equipment->Location ].pop();
		}
		
		if( equipment->ID == BattleTech::Equipment::TARGETING_COMPUTER )
			targeting_computer = equipment;
		
		if( equipment->ID == BattleTech::Equipment::LIFE_SUPPORT )
			LifeSupport = equipment;
		if( equipment->ID == BattleTech::Equipment::SENSORS )
			Sensors = equipment;
		if( equipment->ID == BattleTech::Equipment::COCKPIT )
			Cockpit = equipment;
		if( equipment->ID == BattleTech::Equipment::ENGINE )
			Engine = equipment;
		if( equipment->ID == BattleTech::Equipment::GYRO )
			Gyro = equipment;
		if( equipment->ID == BattleTech::Equipment::MASC )
			MASC = equipment;
		if( strstr( equipment->Name.c_str(), "ECM" ) )
			ECM = equipment;
		
		for( std::vector<uint8_t>::const_iterator crit = variant.Equipment[ i ].CritLocs.begin(); crit != variant.Equipment[ i ].CritLocs.end(); crit ++ )
			Locations[ *crit ].CriticalSlots.push_back( equipment );
	}
	
	if( targeting_computer )
	{
		for( std::vector<MechEquipment>::iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
		{
			if( eq->Weapon && eq->Weapon->CanUseTargetingComputer )
				eq->WeaponTC = targeting_computer;
		}
	}
	
	PilotSkill   = Clan ? 4 : 5;
	GunnerySkill = Clan ? 3 : 4;
	
	BeginPhase( BattleTech::State::SETUP );
}


std::string Mech::FullName( void ) const
{
	std::string name = Name;
	if( Var.length() )
		name += std::string(" ") + Var;
	return name;
}


std::string Mech::ShortName( void ) const
{
	size_t alternate_name = Name.find(" (");
	if( alternate_name != std::string::npos )
		return Name.substr( 0, alternate_name );
	return Name;
}


std::string Mech::ShortFullName( void ) const
{
	std::string name = ShortName();
	if( Var.length() )
		name += std::string(" ") + Var;
	return name;
}


uint8_t Mech::WalkDist( void ) const
{
	int8_t walk = Walk;
	
	uint8_t leg_count = 0, legs_destroyed = 0, damaged_hips = 0, damaged_actuators = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( ! Locations[ i ].IsLeg )
			continue;
		leg_count ++;
		if( ! Locations[ i ].Structure )
			legs_destroyed ++;
		else if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::HIP ) )
			damaged_hips ++;
		else
		{
			if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::UPPER_LEG_ACTUATOR ) )
				damaged_actuators ++;
			if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::LOWER_LEG_ACTUATOR ) )
				damaged_actuators ++;
			if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::FOOT_ACTUATOR ) )
				damaged_actuators ++;
		}
	}
	
	if( legs_destroyed >= leg_count )
		return 0;
	else if( legs_destroyed >= (leg_count / 2) )
		walk = 1;
	else if( (leg_count == 3) && (legs_destroyed == 1) )
		walk --;
	
	if( damaged_hips >= leg_count )
		return 0;
	else if( damaged_hips )
		walk = ceilf( walk / 2.f );
	
	walk -= damaged_actuators;
	
	if( ActiveTSM )
		walk += 2;
	
	walk -= HeatMove;
	
	return std::max<int8_t>( 0, walk );
}


uint8_t Mech::RunDist( void ) const
{
	uint8_t leg_count = 0, legs_destroyed = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( ! Locations[ i ].IsLeg )
			continue;
		leg_count ++;
		if( ! Locations[ i ].Structure )
			legs_destroyed ++;
	}
	
	if( legs_destroyed >= (leg_count / 2) )
		return 0;
	
	return ceilf( WalkDist() * 1.5f );
}


uint8_t Mech::MASCDist( void ) const
{
	uint8_t leg_count = 0, legs_destroyed = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( ! Locations[ i ].IsLeg )
			continue;
		leg_count ++;
		if( ! Locations[ i ].Structure )
			legs_destroyed ++;
	}
	
	if( legs_destroyed >= (leg_count / 2) )
		return 0;
	
	if( MASC && ! MASC->Damaged )
		return WalkDist() * 2;
	
	return 0;
}


uint8_t Mech::JumpDist( void ) const
{
	int8_t jump = Jump;
	
	if( Prone || StandAttempts )
		return 0;
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
		jump -= Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::JUMP_JET );
	
	return std::max<int8_t>( 0, jump );
}


MechLocationRoll Mech::LocationRoll( uint8_t arc, uint8_t table, bool leg_cover )
{
	MechLocationRoll lr;
	
	lr.Table = table;
	lr.Arc = arc;
	
	uint8_t dice = ((lr.Table == BattleTech::Melee::KICK) || (lr.Table == BattleTech::Melee::PUNCH)) ? 1 : 2;
	
	lr.Roll = Roll::Dice( dice );
	lr.Crit = (dice == 2) && (lr.Roll == 2);
	
	if( lr.Crit )
	{
		std::map<std::string,std::string>::const_iterator floating_crits = Data->Properties.find("floating_crits");
		if( (floating_crits != Data->Properties.end()) && Str::AsBool(floating_crits->second) )
		{
			BattleTechServer *server = (BattleTechServer*) Raptor::Server;
			
			Event e( this );
			e.Text = "Location roll 2 is Floating Critical.";
			server->Events.push( e );
			
			lr.Roll = Roll::Dice( dice );
			
			// Floating Criticals re-roll leg hits when the target is in partial cover.
			while( leg_cover && HitLocation( lr.Arc, lr.Roll )->IsLeg )
				lr.Roll = Roll::Dice( dice );
		}
	}
	
	if( table == BattleTech::Melee::KICK )
		lr.Loc = KickLocation( lr.Arc, lr.Roll );
	else if( table == BattleTech::Melee::PUNCH )
		lr.Loc = PunchLocation( lr.Arc, lr.Roll );
	else
		lr.Loc = HitLocation( lr.Arc, lr.Roll );
	
	return lr;
}


MechLocation *Mech::HitLocation( uint8_t arc, uint8_t roll )
{
	if( ! roll )
		roll = Roll::Dice( 2 );
	
	MechLocation *location = &(Locations[ HEAD ]);
	if( (arc == FRONT) || (arc == REAR) )
	{
		if( roll ==  2 ) location = &(Locations[ CENTER_TORSO ]);
		if( roll ==  3 ) location = &(Locations[ RIGHT_ARM    ]);
		if( roll ==  4 ) location = &(Locations[ RIGHT_ARM    ]);
		if( roll ==  5 ) location = &(Locations[ RIGHT_LEG    ]);
		if( roll ==  6 ) location = &(Locations[ RIGHT_TORSO  ]);
		if( roll ==  7 ) location = &(Locations[ CENTER_TORSO ]);
		if( roll ==  8 ) location = &(Locations[ LEFT_TORSO   ]);
		if( roll ==  9 ) location = &(Locations[ LEFT_LEG     ]);
		if( roll == 10 ) location = &(Locations[ LEFT_ARM     ]);
		if( roll == 11 ) location = &(Locations[ LEFT_ARM     ]);
	}
	else
	{
		bool left_side = (arc == LEFT_SIDE);
		if( roll ==  2 ) location = &(Locations[ left_side ? LEFT_TORSO  : RIGHT_TORSO ]);
		if( roll ==  3 ) location = &(Locations[ left_side ? LEFT_LEG    : RIGHT_LEG   ]);
		if( roll ==  4 ) location = &(Locations[ left_side ? LEFT_ARM    : RIGHT_ARM   ]);
		if( roll ==  5 ) location = &(Locations[ left_side ? LEFT_ARM    : RIGHT_ARM   ]);
		if( roll ==  6 ) location = &(Locations[ left_side ? LEFT_LEG    : RIGHT_LEG   ]);
		if( roll ==  7 ) location = &(Locations[ left_side ? LEFT_TORSO  : RIGHT_TORSO ]);
		if( roll ==  8 ) location = &(Locations[             CENTER_TORSO              ]);
		if( roll ==  9 ) location = &(Locations[ left_side ? RIGHT_TORSO : LEFT_TORSO  ]);
		if( roll == 10 ) location = &(Locations[ left_side ? RIGHT_ARM   : LEFT_ARM    ]);
		if( roll == 11 ) location = &(Locations[ left_side ? RIGHT_LEG   : LEFT_LEG    ]);
	}
	
	return location;
}


MechLocation *Mech::KickLocation( uint8_t arc, uint8_t roll )
{
	// FIXME: Quads have a different table.
	
	MechLocation *location = &(Locations[ LEFT_LEG ]);
	if( arc == RIGHT_SIDE )
		location = &(Locations[ RIGHT_LEG ]);
	else if( arc != LEFT_SIDE )
	{
		if( ! roll )
			roll = Roll::Die();
		if( roll <= 3 )
			location = &(Locations[ RIGHT_LEG ]);
	}
	
	return location;
}


MechLocation *Mech::PunchLocation( uint8_t arc, uint8_t roll )
{
	if( ! roll )
		roll = Roll::Die();
	
	// FIXME: Quads have a different table.
	
	MechLocation *location = &(Locations[ HEAD ]);
	if( (arc == FRONT) || (arc == REAR) )
	{
		if( roll == 1 ) location = &(Locations[ LEFT_ARM     ]);
		if( roll == 2 ) location = &(Locations[ LEFT_TORSO   ]);
		if( roll == 3 ) location = &(Locations[ CENTER_TORSO ]);
		if( roll == 4 ) location = &(Locations[ RIGHT_TORSO  ]);
		if( roll == 5 ) location = &(Locations[ RIGHT_ARM    ]);
	}
	else
	{
		bool left_side = (arc == LEFT_SIDE);
		if( roll == 1 ) location = &(Locations[ left_side ? LEFT_TORSO  : RIGHT_TORSO ]);
		if( roll == 2 ) location = &(Locations[ left_side ? LEFT_TORSO  : RIGHT_TORSO ]);
		if( roll == 3 ) location = &(Locations[             CENTER_TORSO              ]);
		if( roll == 4 ) location = &(Locations[ left_side ? LEFT_ARM    : RIGHT_ARM   ]);
		if( roll == 5 ) location = &(Locations[ left_side ? LEFT_ARM    : RIGHT_ARM   ]);
	}
	
	return location;
}


void Mech::EngineExplode( void )
{
	if( ClientSide() )
		return;
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	HexMap *map = server->Map();
	if( ! map )
		return;
	
	Event e( this );
	e.Effect = BattleTech::Effect::EXPLOSION;
	e.Misc = BattleTech::Effect::EXPLOSION;
	e.Sound = "m_expl.wav";
	e.Text = ShortFullName() + std::string(" reactor went critical!");
	e.Duration = 2.f;
	server->Events.push( e );
	
	std::map< uint8_t, std::set<Mech*> > ranges;
	
	for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
	{
		if( obj_iter->second == this )
			continue;
		if( obj_iter->second->Type() == BattleTech::Object::MECH )
		{
			Mech *mech = (Mech*) obj_iter->second;
			if( mech->Destroyed() )
				continue;
			
			uint8_t dist = map->HexDist( X, Y, mech->X, mech->Y );
			
			if( dist <= 3 )
				ranges[ dist ].insert( mech );
		}
	}
	
	for( std::map< uint8_t, std::set<Mech*> >::iterator range = ranges.begin(); range != ranges.end(); range ++ )
	{
		uint16_t damage = Tons * Walk;
		if( range->first == 1 )
			damage /= 10;
		else if( range->first == 2 )
			damage /= 20;
		else if( range->first == 3 )
			damage /= 40;
		else
			continue;
		
		for( std::set<Mech*>::iterator mech = range->second.begin(); mech != range->second.end(); mech ++ )
		{
			uint8_t hit_arc = (*mech)->DamageArc( X, Y );
			MechLocationRoll lr = (*mech)->LocationRoll( hit_arc );
			
			Event e( *mech );
			e.Effect = BattleTech::Effect::EXPLOSION;
			e.Misc = BattleTech::Effect::FLAMER;
			e.Text = lr.String() + std::string(" for ") + Num::ToString((int)damage) + std::string(" damage from engine explosion.");
			e.Loc = lr.Loc->Loc;
			server->Events.push( e );
			
			lr.Loc->Damage( damage, hit_arc, 0 );
		}
	}
}


bool Mech::PilotSkillCheck( int8_t modifier, bool fall )
{
	uint8_t need = PilotSkill + modifier;
	
	if( Gyro && (Gyro->Damaged >= 2) )
		need += 6;
	else if( Gyro && Gyro->Damaged )
		need += 3;
	
	uint8_t leg_count = 0, legs_destroyed = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( Locations[ i ].IsLeg )
		{
			leg_count ++;
			if( ! Locations[ i ].Structure )
				legs_destroyed ++;
			else if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::HIP ) )
				need += 2;
			else
			{
				if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::UPPER_LEG_ACTUATOR ) )
					need ++;
				if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::LOWER_LEG_ACTUATOR ) )
					need ++;
				if( Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::FOOT_ACTUATOR ) )
					need ++;
			}
		}
	}
	if( legs_destroyed >= (leg_count / 2) )
		need += 5;
	
	// Shutdown 'Mechs automatically fail piloting checks.
	if( fall && Shutdown )
		need = 99;
	// Prone quads do not need to make a pilot check to stand unless a leg has been destroyed.
	else if( fall && Prone && (leg_count == 4) && ! legs_destroyed )
		need = 0;
	
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	bool passed = false;
	if( (need <= 12) && (! Unconscious) && !(fall && AutoFall.length()) )
	{
		uint8_t roll = Roll::Dice( 2 );
		passed = roll >= need;
		
		Event e( this );
		e.Effect = BattleTech::Effect::BLINK;
		e.Misc = passed ? BattleTech::RGB332::BLACK : BattleTech::RGB332::DARK_RED;
		e.Text = std::string("PSR for ") + ShortName()
			+ std::string(fall ? (Prone ? " to stand needs " : " to avoid fall needs ") : " to avoid pilot hit needs ")
			+ Num::ToString((int)need) + std::string(", rolled ") + Num::ToString((int)roll)
			+ std::string(passed ? ": PASSED!" : ": FAILED!");
		server->Events.push( e );
	}
	else
	{
		Event e( this );
		e.Effect = BattleTech::Effect::BLINK;
		e.Misc = BattleTech::RGB332::DARK_RED;
		e.Text = "PSR roll automatically FAILED";
		if( AutoFall.length() )
			e.Text += std::string(" due to ") + AutoFall;
		else if( Unconscious )
			e.Text += std::string(" because pilot is unconscious");
		else if( Shutdown )
			e.Text += std::string(" because of shutdown");
		e.Text += "!";
		server->Events.push( e );
	}
	
	if( fall && ! passed )
		Fall( modifier );
	
	return passed;
}


void Mech::Fall( int8_t psr_modifier )
{
	Prone = true;
	
	uint8_t dir = Roll::Die();
	
	Facing += dir - 1;
	Facing %= 6;
	
	uint8_t hit_arc = FRONT;
	if( (dir == 2) || (dir == 3) )
		hit_arc = RIGHT_SIDE;
	else if( dir == 4 )
		hit_arc = REAR;
	else if( (dir == 5) || (dir == 6) )
		hit_arc = LEFT_SIDE;
	
	uint16_t damage = ceilf(Tons/10.f);
	
	MechLocationRoll lr = LocationRoll( hit_arc );
	
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	Event e1( this );
	e1.Effect = BattleTech::Effect::FALL;
	e1.Sound = "m_fall.wav";
	e1.ShowFacing( Facing, true );
	e1.Text = ShortFullName() + std::string(" fell!  Direction roll is ") + Num::ToString((int)dir) + std::string(".");
	server->Events.push( e1 );
	
	Event e2( this );
	e2.Text = lr.String() + std::string(" for ") + Num::ToString((int)damage) + std::string(" fall damage.");
	server->Events.push( e2 );
	
	lr.Loc->Damage( damage, hit_arc, lr.Crit );
	
	if( ! PilotSkillCheck( psr_modifier, false ) )
		HitPilot();
}


void Mech::HitPilot( uint8_t hits )
{
	if( ! hits )
		return;
	
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	Event e( this );
	e.Effect = BattleTech::Effect::BLINK;
	e.Misc = BattleTech::RGB332::PURPLE;
	e.Text = std::string("Pilot takes ") + Num::ToString((int)hits) + std::string((hits == 1)?" hit (":" hits (") + Num::ToString((int)(PilotDamage + hits)) + std::string(" total).");
	server->Events.push( e );
	
	for( uint8_t hit = 0; hit < hits; hit ++ )
	{
		if(	PilotDamage >= 6 )  // Already dead.
			return;
		
		PilotDamage ++;
		
		if( PilotDamage >= 6 )
		{
			Unconscious = 1;
			
			Event e( this );
			e.Effect = BattleTech::Effect::EXPLOSION;  // FIXME: Should this be a different effect?
			e.Sound = "m_expl.wav";
			e.Text = ShortFullName() + std::string(" pilot killed!");
			e.Duration = 2.f;
			e.Loc = BattleTech::Loc::HEAD;
			server->Events.push( e );
		}
		else if( PilotDamage && ! ConsciousnessRoll() )
			Unconscious = 1;
	}
}


bool Mech::ConsciousnessRoll( bool wake )
{
	if( Unconscious && ! wake )
		return false;
	
	uint8_t need = 0;
	if( PilotDamage == 1 )
		need = 3;
	else if( PilotDamage == 2 )
		need = 5;
	else if( PilotDamage == 3 )
		need = 7;
	else if( PilotDamage == 4 )
		need = 10;
	else if( PilotDamage == 5 )
		need = 11;
	
	if( need )
	{
		uint8_t roll = Roll::Dice( 2 );
		bool passed = roll >= need;
		
		BattleTechServer *server = (BattleTechServer*) Raptor::Server;
		Event e( this );
		if( ! passed )
		{
			e.Effect = BattleTech::Effect::BLINK;
			e.Misc = BattleTech::RGB332::VIOLET;
		}
		e.Text = std::string("Consciousness roll needs ") + Num::ToString((int)need) + std::string(", rolled ") + Num::ToString((int)roll) + std::string(passed ? ": PASSED!" : ": FAILED!");
		server->Events.push( e );
		
		return passed;
	}
	else
		return ! PilotDamage;  // Damage 0 is always conscious, damage 6+ is dead.
}


bool Mech::Destroyed( void ) const
{
	if( ClientSide() )
		return Locations[ CENTER_TORSO ].Destroyed;
	
	if( Cockpit && Cockpit->Damaged )
		return true;
	if( Engine && (Engine->Damaged >= 3) )
		return true;
	if( PilotDamage >= 6 )
		return true;
	return false;
}


bool Mech::Immobile( void ) const
{
	if( Shutdown )
		return true;
	if( Unconscious )
		return true;
	if( Quad && !( Locations[ LEFT_LEG ].Structure || Locations[ RIGHT_LEG ].Structure || Locations[ LEFT_ARM ].Structure || Locations[ RIGHT_ARM ].Structure ) )
		return true;
	return false;
}


size_t Mech::DamagedCriticalCount( uint16_t crit_id ) const
{
	size_t count = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
		count += Locations[ i ].DamagedCriticals.count( crit_id );
	return count;
}


bool Mech::Narced( void ) const
{
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( Locations[ i ].Narced && Locations[ i ].Structure )
			return true;
	}
	return false;
}


bool Mech::CanStand( void ) const
{
	if( Gyro && (Gyro->Damaged >= 2) )
		return false;
	
	uint8_t leg_count = 0, legs_destroyed = 0, arm_count = 0, arms_destroyed = 0;
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( Locations[ i ].IsLeg )
		{
			leg_count ++;
			if( ! Locations[ i ].Structure )
				legs_destroyed ++;
		}
		else if( Locations[ i ].IsArm() )
		{
			arm_count ++;
			if( ! Locations[ i ].Structure )
				arms_destroyed ++;
		}
	}
	
	if( legs_destroyed >= leg_count )
		return false;
	if( legs_destroyed >= 3 )  // Quads can't stand on one leg.
		return false;
	if( legs_destroyed && (arms_destroyed >= 2) )
		return false;
	
	// Minimum Movement Rule allows one stand attempt with a single movement point.
	if( (! StandAttempts) && WalkDist() )
		return true;
	
	return ( (StandAttempts + 1) * 2 <= std::max<uint8_t>( RunDist(), MASCDist() ) );
}


bool Mech::Ready( void ) const
{
	return !( TookTurn || Destroyed() );
}


bool Mech::ReadyAndAble( int phase ) const
{
	if( ! Ready() )
		return false;
	
	if( Unconscious || Shutdown )
		return false;
	
	if( ! phase )
		phase = ClientSide() ? Raptor::Game->State : Raptor::Server->State;
	
	if( (phase == BattleTech::State::MOVEMENT) || (phase == BattleTech::State::SETUP) )
		return WalkDist() || (JumpDist() && ! Prone);
	
	if( phase == BattleTech::State::TAG )
	{
		std::map<std::string,std::string>::const_iterator skip_tag = Data->Properties.find("skip_tag");
		if( (skip_tag != Data->Properties.end()) && Str::AsBool(skip_tag->second) )
			return false;
		
		for( std::vector<MechEquipment>::const_iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
		{
			if( equip->Weapon && equip->Weapon->TAG && ! equip->Damaged )
				return true;
		}
		return false;
	}
	
	if( phase == BattleTech::State::WEAPON_ATTACK )
	{
		// FIXME: See if weapon attacks are possible.
		return true;
	}
	
	if( phase == BattleTech::State::PHYSICAL_ATTACK )
	{
		bool ff = ClientSide() ? ((const BattleTechGame*)( Raptor::Game ))->FF() : ((const BattleTechServer*)( Raptor::Server ))->FF();
		
		for( std::map<uint32_t,GameObject*>::const_iterator obj_iter = Data->GameObjects.begin(); obj_iter != Data->GameObjects.end(); obj_iter ++ )
		{
			if( obj_iter->second->Type() == BattleTech::Object::MECH )
			{
				const Mech *target = (const Mech*) obj_iter->second;
				if( Team && (target->Team == Team) && ! ff )
					continue;
				if( PhysicalAttacks( target ).size() )
					return true;
			}
		}
		return false;
	}
	
	// Other phases don't have user actions to take.
	return false;
}


std::set<MechMelee> Mech::PhysicalAttacks( const Mech *target, int8_t modifier ) const
{
	std::set<MechMelee> attacks;
	
	if( Destroyed() || Unconscious || Shutdown || Prone )
		return attacks;
	if( ! target )
		return attacks;
	if( target == this )
		return attacks;
	if( target->Destroyed() )
		return attacks;
	
	HexMap *map = ClientSide() ? ((BattleTechGame*)( Raptor::Game ))->Map() : ((BattleTechServer*)( Raptor::Server ))->Map();
	if( ! map )
		return attacks;
	if( map->HexDist( X, Y, target->X, target->Y ) > 1 )
		return attacks;
	
	int8_t my_legs     = map->Hexes[ X ][ Y ].Height + 1;
	int8_t my_arms     = my_legs + 1;
	int8_t target_legs = map->Hexes[ target->X ][ target->Y ].Height + 1;
	int8_t target_body = target->Prone ? target_legs : (target_legs + 1);
	
	uint8_t upper_arc = FiringArc( target->X, target->Y, true );
	uint8_t lower_arc = FiringArc( target->X, target->Y, false );
	
	bool hip_damage = DamagedCriticalCount( BattleTech::Equipment::HIP );
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		if( ! Locations[ i ].Structure )
			continue;
		
		if( Locations[ i ].IsLeg && ! hip_damage
		&&  (my_legs >= target_legs) && (my_legs <= target_body)
		&&  (lower_arc == BattleTech::Arc::FRONT) )
		{
			bool fired = false;
			for( std::vector<MechEquipment>::const_iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
			{
				if( equip->Location != &(Locations[ i ]) )
					continue;
				if( equip->Weapon && (equip->Weapon->Defensive || equip->Weapon->TAG) )
					continue;
				if( equip->Fired )
				{
					fired = true;
					break;
				}
			}
			if( fired )
				continue;
			
			size_t damaged_leg_actuators = Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::UPPER_LEG_ACTUATOR ) + Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::LOWER_LEG_ACTUATOR );
			
			MechMelee kick;
			kick.Attack = BattleTech::Melee::KICK;
			kick.Limb = i;
			kick.Difficulty = PilotSkill + Attack + modifier - 2 + (2 * damaged_leg_actuators) + Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::FOOT_ACTUATOR );
			kick.Damage = (Tons / 5) * (ActiveTSM ? 2 : 1) / (1 << damaged_leg_actuators);
			kick.HitTable = target->PhysicalHitTable( kick.Attack, this );
			attacks.insert( kick );
		}
		
		if( Locations[ i ].IsArm()
		&&  ! Locations[ i ].DamagedCriticals.count( BattleTech::Equipment::SHOULDER ) )
		{
			if( (upper_arc == BattleTech::Arc::REAR) != ArmsFlipped )
				continue;
			if( (i == BattleTech::Loc::LEFT_ARM) && (upper_arc == BattleTech::Arc::RIGHT_SIDE) )
				continue;
			if( (i == BattleTech::Loc::RIGHT_ARM) && (upper_arc == BattleTech::Arc::LEFT_SIDE) )
				continue;
			
			uint8_t weapon_id = 0;
			bool fired = false;
			for( std::vector<MechEquipment>::const_iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
			{
				if( equip->Location != &(Locations[ i ]) )
					continue;
				if( equip->Weapon && (equip->Weapon->Defensive || equip->Weapon->TAG) )
					continue;
				if( equip->Fired )
				{
					fired = true;
					break;
				}
				
				if( equip->Damaged )
					continue;
				if( (equip->ID == BattleTech::Equipment::HATCHET)
				||  (equip->ID == BattleTech::Equipment::SWORD) )
					weapon_id = equip->ID;
			}
			if( fired )
				continue;
			
			int8_t missing_hand_actuator = 1 - Locations[ i ].IntactCriticalCount( BattleTech::Equipment::HAND_ACTUATOR );
			int8_t missing_arm_actuators = 2 - Locations[ i ].IntactCriticalCount( BattleTech::Equipment::UPPER_ARM_ACTUATOR ) - Locations[ i ].IntactCriticalCount( BattleTech::Equipment::LOWER_ARM_ACTUATOR );
			
			if( (my_arms >= target_legs) && (my_arms <= target_body) && ! weapon_id )
			{
				MechMelee punch;
				punch.Attack = BattleTech::Melee::PUNCH;
				punch.Limb = i;
				punch.Difficulty = PilotSkill + Attack + modifier + (2 * missing_arm_actuators) + missing_hand_actuator;
				punch.Damage = std::max<uint8_t>( 1, (Tons / 10) * (ActiveTSM ? 2 : 1) / (1 << missing_arm_actuators) );
				punch.HitTable = target->PhysicalHitTable( punch.Attack, this );
				attacks.insert( punch );
			}
			
			if( weapon_id && (my_arms >= target_legs) && (my_legs <= target_body) && ! missing_hand_actuator )
			{
				bool hatchet = (weapon_id == BattleTech::Equipment::HATCHET);
				
				MechMelee weapon;
				weapon.Attack = weapon_id;
				weapon.Limb = i;
				weapon.Difficulty = PilotSkill + Attack + modifier - (hatchet ? 1 : 2) + (2 * missing_arm_actuators);
				weapon.Damage = (hatchet ? (Tons / 5) : (Tons / 10 + 1)) * (ActiveTSM ? 2 : 1);
				weapon.HitTable = target->PhysicalHitTable( weapon.Attack, this );
				attacks.insert( weapon );
			}
		}
	}
	
	return attacks;
}


uint8_t Mech::PhysicalHitTable( uint8_t attack, const Mech *attacker ) const
{
	// This function assumes we already know the attack will hit.
	
	if( Prone )
		return BattleTech::HitTable::STANDARD;
	if( ! attacker )
		attacker = this;
	
	int8_t my_level = 0;
	int8_t attacker_level = 0;
	HexMap *map = ClientSide() ? ((BattleTechGame*)( Raptor::Game ))->Map() : ((BattleTechServer*)( Raptor::Server ))->Map();
	if( map )
	{
		my_level = map->Hexes[ X ][ Y ].Height;
		attacker_level = map->Hexes[ attacker->X ][ attacker->Y ].Height;
	}
	
	if( attack == BattleTech::Melee::KICK )
		return (attacker_level > my_level) ? BattleTech::HitTable::PUNCH : BattleTech::HitTable::KICK;
	
	if( (my_level == attacker_level) && ((attack == BattleTech::Melee::HATCHET) || (attack == BattleTech::Melee::SWORD)) )
		return BattleTech::HitTable::STANDARD;
	
	return (attacker_level < my_level) ? BattleTech::HitTable::KICK : BattleTech::HitTable::PUNCH;
}


MechEquipment *Mech::FindAmmo( uint16_t eq_id )
{
	if( eq_id && (eq_id < 400) )
		eq_id += 400;
	
	MechEquipment *ammo = NULL;
	
	for( std::vector<MechEquipment>::iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
	{
		if( ! equip->Ammo )
			continue;
		if( eq_id && (equip->ID != eq_id) )
			continue;
		
		// These rules correctly prioritize which bin explodes first from heat, and also work for finding ammo to shoot.
		if( ! ammo )
			ammo = &*equip;
		else if( equip->ExplosionDamage > ammo->ExplosionDamage )
			ammo = &*equip;
		else if( (equip->ExplosionDamage == ammo->ExplosionDamage)
		&&       (equip->Ammo > ammo->Ammo) )
			ammo = &*equip;
		else if( (equip->ExplosionDamage == ammo->ExplosionDamage)
		&&       (equip->Ammo == ammo->Ammo)
		&&       Rand::Bool() )
			ammo = &*equip;
	}
	
	return ammo;
}


uint16_t Mech::TotalAmmo( uint16_t eq_id )
{
	if( eq_id && (eq_id < 400) )
		eq_id += 400;
	
	uint16_t total = 0;
	
	for( std::vector<MechEquipment>::iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
	{
		if( eq_id && (equip->ID != eq_id) )
			continue;
		
		total += equip->Ammo;
	}
	
	return total;
}


bool Mech::SpendAmmo( uint16_t eq_id )
{
	MechEquipment *ammo = FindAmmo( eq_id );
	if( ammo && ammo->Ammo )
	{
		ammo->Ammo --;
		return true;
	}
	return false;
}


uint8_t Mech::ActiveProbeRange( void ) const
{
	uint8_t range = 0;
	for( std::vector<MechEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
	{
		if( eq->Weapon && ! eq->Damaged && strstr( eq->Weapon->Name.c_str(), "Active Probe" ) && (eq->Weapon->RangeLong > range) )
			range = eq->Weapon->RangeLong;
	}
	return range;
}


MechEquipment *Mech::AvailableAMS( uint8_t arc )
{
	MechEquipment *ams = NULL;
	for( std::vector<MechEquipment>::iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
	{
		if( eq->Weapon && eq->Weapon->Defensive && ! eq->Damaged && ! eq->Fired && eq->WeaponArcs.count( arc )
		&& (strstr( eq->Weapon->Name.c_str(), "AMS" ) || strstr( eq->Weapon->Name.c_str(), "Anti-Missile" )) )
		{
			if( eq->Weapon->AmmoPerTon && ! FindAmmo(eq->ID) )
				continue;
			if( (! ams) || (eq->Weapon->Heat < ams->Weapon->Heat) )
				ams = &*eq;
		}
	}
	return ams;
}


bool Mech::FiredTAG( void ) const
{
	for( std::vector<MechEquipment>::const_iterator eq = Equipment.begin(); eq != Equipment.end(); eq ++ )
	{
		if( eq->Weapon && eq->Weapon->TAG && eq->Fired )
			return true;
	}
	return false;
}


void Mech::GetPosition( uint8_t *x, uint8_t *y, uint8_t *facing ) const
{
	if( Steps.size() )
	{
		*x = Steps.back().X;
		*y = Steps.back().Y;
		if( facing )
			*facing = Steps.back().Facing;
	}
	else
	{
		*x = X;
		*y = Y;
		if( facing )
			*facing = Facing;
	}
}


double Mech::RelativeAngle( uint8_t x, uint8_t y, int8_t twist ) const
{
	if( (X == x) && (Y == y) )
		return 0.;
	HexMap *map = ClientSide() ? ((BattleTechGame*)( Raptor::Game ))->Map() : ((BattleTechServer*)( Raptor::Server ))->Map();
	if( ! map )
		return 0.;
	
	Pos3D me = map->Center( X, Y );
	Pos3D them = map->Center( x, y );
	Vec3D vec = them - me;
	
	double angle = Num::RadToDeg( Math2D::Angle( vec.X, vec.Y ) ) - ((Facing + twist) * 60.);
	while( angle > 180. )
		angle -= 360.;
	while( angle <= -180. )
		angle += 360.;
	return angle;
}


uint8_t Mech::FiringArc( uint8_t x, uint8_t y, bool twist ) const
{
	double angle = RelativeAngle( x, y, twist ? TorsoTwist : 0 );
	double abs_angle = fabs(angle);
	if( abs_angle < 60.01 )
		return BattleTech::Arc::FRONT;
	if( abs_angle < 120.01 )
		return (angle < 0.) ? BattleTech::Arc::LEFT_SIDE : BattleTech::Arc::RIGHT_SIDE;
	return BattleTech::Arc::REAR;
}


uint8_t Mech::DamageArc( uint8_t x, uint8_t y ) const
{
	double angle = RelativeAngle( x, y );
	double abs_angle = fabs(angle);
	if( abs_angle < 90.01 )
		return BattleTech::Arc::FRONT;
	if( abs_angle < 150.01 )
		return (angle < 0.) ? BattleTech::Arc::LEFT_SIDE : BattleTech::Arc::RIGHT_SIDE;
	return BattleTech::Arc::REAR;
}


void Mech::BeginPhase( int phase )
{
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		// If no slots remain, allow critical hits to transfer next turn.
		if( ! Locations[ i ].CriticalSlots.size() )
			Locations[ i ].CritTransfer = Locations[ i ].DamageTransfer;
	}
	
	Steps.clear();
	StandAttempts = 0;
	TookTurn = false;
	
	if( Destroyed() )
		return;
	
	if( PhaseDamage >= 20 )
	{
		PSRs ++;
		PSRModifier ++;
	}
	PhaseDamage = 0;
	
	if( ! ClientSide() )
	{
		for( uint8_t i = 0; i < PSRs; i ++ )
		{
			if( Prone || Destroyed() )
				break;
			PilotSkillCheck( PSRModifier );
		}
	}
	PSRs = 0;
	PSRModifier = 0;
	AutoFall = "";
	
	if( phase == BattleTech::State::INITIATIVE )
	{
		if( Unconscious )
			Unconscious = 2;
	}
	
	if( (phase == BattleTech::State::HEAT) && ! ClientSide() )
	{
		BattleTechServer *server = (BattleTechServer*) Raptor::Server;
		
		bool any_fired = false;
		for( std::vector<MechEquipment>::iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
		{
			if( equip->Fired && equip->Weapon )
			{
				Heat += equip->Fired * equip->Weapon->Heat;
				any_fired = true;
			}
			equip->Fired = 0;
		}
		
		if( OutsideHeat > 15 )
			OutsideHeat = 15;
		Heat += OutsideHeat;
		OutsideHeat = 0;
		
		if( ActiveStealth )
			Heat += 10;
		
		Heat -= HeatDissipation;
		if( Heat < 0 )
			Heat = 0;
		
		HeatMove = std::min<uint8_t>( 5, Heat / 5 );
		
		if( Heat >= 24 )
			HeatFire = 4;
		else if( Heat >= 17 )
			HeatFire = 3;
		else if( Heat >= 13 )
			HeatFire = 2;
		else if( Heat >= 8 )
			HeatFire = 1;
		else
			HeatFire = 0;
		
		if( HeatMove || HeatFire )
		{
			Event e( this );
			e.Effect = BattleTech::Effect::SMOKE;
			e.Misc = Heat;
			e.Sound = "b_heat.wav";
			e.Duration = 1.8f;
			e.Text = ShortName() + std::string(" heat level ") + Num::ToString((int)Heat) + std::string(" has movement penalty -") + Num::ToString((int)HeatMove) + std::string(" and firing penalty +") + Num::ToString((int)HeatFire) + std::string(".");
			server->Events.push( e );
		}
		
		if( TSM && (Heat >= 9) && (Heat < 30) )
		{
			Event e( this );
			e.Effect = BattleTech::Effect::BLINK;
			e.Duration = 1.6f;
			if( HeatMove > 2 )
				e.Text = std::string("Heat activates Triple-Strength Myomer: movement +2, total penalty -") + Num::ToString(((int)HeatMove)-2) + std::string(".");
			else
				e.Text = std::string("Heat activates Triple-Strength Myomer: movement +2, total bonus +") + Num::ToString(2-(int)HeatMove) + std::string(".");
			server->Events.push( e );
		}
		else if( ActiveTSM && (Heat < 9) )
		{
			Event e( this );
			e.Effect = BattleTech::Effect::BLINK;
			e.Text = "Triple-Strength Myomer deactivates without heat.";
			server->Events.push( e );
		}
		
		if( LifeSupport && LifeSupport->Damaged )
		{
			if( Heat >= 26 )
				HitPilot( 2 );
			else if( Heat >= 15 )
				HitPilot();
		}
		
		uint8_t ammo_explosion_check = 0;
		if( Heat >= 28 )
			ammo_explosion_check = 8;
		else if( Heat >= 23 )
			ammo_explosion_check = 6;
		else if( Heat >= 19 )
			ammo_explosion_check = 4;
		
		if( ammo_explosion_check )
		{
			MechEquipment *ammo = FindAmmo();
			if( ammo )
			{
				uint8_t ammo_explosion_roll = Roll::Dice( 2 );
				
				Event e( this );
				e.Text = std::string("Must roll ") + Num::ToString((int)ammo_explosion_check) + std::string(" to avoid ammo explosion, rolled ") + Num::ToString((int)ammo_explosion_roll) + std::string(".");
				server->Events.push( e );
				
				if( ammo_explosion_roll < ammo_explosion_check )
					ammo->Hit();
			}
		}
		
		if( ! Destroyed() )
		{
			bool was_shutdown = Shutdown;
			uint8_t shutdown_check = 0;
			uint8_t shutdown_roll = 0;
			if( Heat >= 30 )
				shutdown_check = 99;
			else if( Heat >= 26 )
				shutdown_check = 10;
			else if( Heat >= 22 )
				shutdown_check = 8;
			else if( Heat >= 18 )
				shutdown_check = 6;
			else if( Heat >= 14 )
				shutdown_check = 4;
			
			if( shutdown_check && Unconscious )
				shutdown_check = 99;
			
			bool shutdown_init = (shutdown_check && ! Shutdown);
			if( shutdown_init )
			{
				Event e( this );
				e.Sound = "b_sd_ini.wav";
				e.Duration = 1.7f;
				if( shutdown_check <= 12 )
					e.Text = std::string("Shutdown sequence initiated: need ") + Num::ToString((int)shutdown_check) + std::string(" to override.");
				else
					e.Text = "Shutdown sequence initiated...";
				server->Events.push( e );
			}
			
			if( shutdown_check > 12 )
				Shutdown = true;
			else if( shutdown_check )
			{
				shutdown_roll = Roll::Dice( 2 );
				Shutdown = (shutdown_roll < shutdown_check);
			}
			else
				Shutdown = false;
			
			if( shutdown_init )
			{
				Event e( this );
				if( Shutdown )
				{
					e.Sound = "b_sd.wav";
					if( shutdown_check <= 12 )
						e.Text = std::string("Rolled ") + Num::ToString((int)shutdown_roll) + std::string(": Shutting down.");
					else
						e.Text = "Shutting down.";
				}
				else
				{
					e.Effect = BattleTech::Effect::BLINK;
					e.Misc = BattleTech::RGB332::DARK_GREEN;
					e.Sound = "b_sd_ovr.wav";
					e.Duration = 1.8f;
					e.Text = std::string("Rolled ") + Num::ToString((int)shutdown_roll) + std::string(": Shutdown sequence overridden.");
				}
				server->Events.push( e );
			}
			
			if( Shutdown != was_shutdown )
			{
				Event e( this );
				e.Effect = BattleTech::Effect::BLINK;
				if( Shutdown )
					e.Sound = "m_shtdwn.wav";
				else
				{
					e.Misc = BattleTech::RGB332::DARK_GREEN;
					e.Sound = "m_start.wav";
					if( shutdown_check )
						e.Text = ShortName() + std::string(" startup at ") + Num::ToString((int)Heat) + std::string(" heat needs ") + Num::ToString((int)shutdown_check) + std::string(" and rolled ") + Num::ToString((int)shutdown_roll) + std::string(": starting up.");
					else
						e.Text = ShortName() + std::string(" is at heat level ") + Num::ToString((int)Heat) + std::string(": starting up.");
				}
				server->Events.push( e );
				
				if( Shutdown && ! Prone )
				{
					Shutdown = false;
					PilotSkillCheck( 3 );
					Shutdown = true;
				}
			}
			else if( Shutdown && (shutdown_check <= 12) )
			{
				Event e( this );
				e.Text = ShortName() + std::string(" startup at ") + Num::ToString((int)Heat) + std::string(" heat needs ") + Num::ToString((int)shutdown_check) + std::string(" but rolled ") + Num::ToString((int)shutdown_roll) + std::string(".");
				server->Events.push( e );
			}
			
			if( ! any_fired && (Attack <= 1) && ! Shutdown && ! was_shutdown )
			{
				for( std::vector<MechEquipment>::iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
				{
					if( equip->Jammed && ! equip->Damaged && equip->Weapon && (equip->Weapon->RapidFire > 2) )
					{
						int8_t unjam_need = GunnerySkill + 3;
						int8_t unjam_roll = Roll::Dice( 2 );
						if( unjam_roll >= unjam_need )
						{
							equip->Jammed = false;
							equip->ExplosionDamage = 0;
						}
						
						Event e( this );
						e.Effect = BattleTech::Effect::BLINK;
						if( ! equip->Jammed )
							e.Misc = BattleTech::RGB332::DARK_GREEN;
						e.Text = equip->Name + std::string(" needs ") + Num::ToString((int)unjam_need) + std::string(" to unjam, rolled ") + Num::ToString((int)unjam_roll) + std::string( equip->Jammed ? ": FAILED!" : ": SUCCESS!" );
						server->Events.push( e );
					}
				}
			}
		}
	}
	
	if( (phase == BattleTech::State::HEAT) && ClientSide() )
	{
		for( std::vector<MechEquipment>::iterator equip = Equipment.begin(); equip != Equipment.end(); equip ++ )
			equip->Fired = 0;
	}
	
	if( (phase == BattleTech::State::END) || (phase == BattleTech::State::SETUP) )
	{
		if( (phase == BattleTech::State::SETUP) || ! ClientSide() )
		{
			// Clients receive these through events.
			TorsoTwist = 0;
			ArmsFlipped = false;
			ProneFire = BattleTech::Loc::UNKNOWN;
			TurnedArm = BattleTech::Loc::UNKNOWN;
		}
		
		Attack = 0;
		Defense = 0;
		Spotted = 99;
		Tagged = false;
		
		if( phase == BattleTech::State::SETUP )
		{
			// When ending a game early, reset some Mech stats.
			Unconscious = 0;
			Shutdown = false;
			Heat = 0;
		}
		else if( ! ClientSide() && ! Destroyed() )
		{
			if( (Unconscious == 2) && ConsciousnessRoll(true) )
				Unconscious = 0;
		}
		
		ActiveTSM = TSM && (Heat >= 9);
		
		/*
		if( (! Stealth) || (! ECM) || ECM->Damaged || Shutdown )
			ActiveStealth = false;
		*/
		ActiveStealth = Stealth && ECM && ! ECM->Damaged && ! Shutdown;  // FIXME: This should be a choice.
	}
}


bool Mech::Step( uint8_t move )
{
	if( Unconscious || Shutdown )
		return false;
	
	if( move == BattleTech::Move::LEFT )
		return Turn( -1 );
	if( move == BattleTech::Move::RIGHT )
		return Turn( 1 );
	
	if( Prone )
		return false;
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexMap *map = game->Map();
	if( ! map )
		return false;
	
	uint8_t x, y, facing;
	GetPosition( &x, &y, &facing );
	
	int8_t step = (move == BattleTech::Move::REVERSE) ? -1 : 1;
	uint8_t rotate = (move == BattleTech::Move::REVERSE) ? 3 : 0;
	uint8_t dir = (facing + rotate + 6) % 6;
	
	std::map<uint8_t,const Hex*> adjacent = map->Adjacent_const( x, y );
	if( ! adjacent[ dir ] )
		return false;
	
	uint8_t next_x = adjacent[ dir ]->X;
	uint8_t next_y = adjacent[ dir ]->Y;
	Hex *here = &(map->Hexes[ x ][ y ]);
	int cost = map->CostToEnter( next_x, next_y, here, (move == BattleTech::Move::REVERSE) );
	if( ! cost )
	{
		if( JumpDist() || (game->State == BattleTech::State::SETUP) )
			cost = 99; // Make sure this path is only considered valid by jumping.
		else
			return false;
	}
	
	if( Steps.size() && (Steps.back().Step == step * -1) )
		// Undo previous opposite move.
		Steps.pop_back();
	else
	{
		Steps.push_back( MechStep() );
		Steps.back().Cost = cost;
		Steps.back().X = next_x;
		Steps.back().Y = next_y;
		Steps.back().Facing = facing;
		Steps.back().Step = step;
		Steps.back().Turn = 0;
		Steps.back().Jump = 0;
		Steps.back().Move = move;
		
		if( (game->State == BattleTech::State::MOVEMENT) && (SpeedNeeded() == BattleTech::Move::INVALID) )
		{
			Steps.pop_back();
			return false;
		}
	}
	
	return true;
}


bool Mech::Turn( int8_t rotate )
{
	if( Unconscious || Shutdown )
		return false;
	
	uint8_t x, y, facing;
	GetPosition( &x, &y, &facing );
	
	while( rotate < -2 )
		rotate += 6;
	while( rotate > 3 )
		rotate -= 6;
	
	if( Steps.size() && Steps.back().Jump )
	{
		// Apply rotation to previous jump landing.
		Steps.back().Facing = (facing + rotate + 6) % 6;
		Steps.back().Turn += rotate;
		
		while( Steps.back().Turn < -2 )
			Steps.back().Turn += 6;
		while( Steps.back().Turn > 3 )
			Steps.back().Turn -= 6;
	}
	else if( Steps.size() && (Steps.back().Turn == rotate * -1) )
		// Undo previous opposite rotation.
		Steps.pop_back();
	else
	{
		Steps.push_back( MechStep() );
		Steps.back().Cost = 1;
		Steps.back().X = x;
		Steps.back().Y = y;
		Steps.back().Facing = (facing + rotate + 6) % 6;
		Steps.back().Step = 0;
		Steps.back().Turn = rotate;
		Steps.back().Jump = 0;
		Steps.back().Move = (rotate & 0xFC) ? BattleTech::Move::LEFT : BattleTech::Move::RIGHT;
		
		// After successfully standing, select any facing at no cost.
		if( StandAttempts && (! Prone) && (StepCost() == StandAttempts * 2 + Steps.back().Cost) )
			Steps.back().Cost = 0;
		
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		if( (game->State == BattleTech::State::MOVEMENT) && (SpeedNeeded() == BattleTech::Move::INVALID) )
		{
			Steps.pop_back();
			return false;
		}
	}
	
	return true;
}


bool Mech::JumpTo( uint8_t next_x, uint8_t next_y, uint8_t next_facing )
{
	if( Unconscious || Shutdown || Prone )
		return false;
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexMap *map = game->Map();
	if( ! map )
		return false;
	
	uint8_t x, y, facing;
	GetPosition( &x, &y, &facing );
	
	uint8_t cost = map->JumpCost( x, y, next_x, next_y );
	if( cost > JumpDist() )
		return false;
	
	Steps.push_back( MechStep() );
	Steps.back().Cost = cost;
	Steps.back().X = next_x;
	Steps.back().Y = next_y;
	Steps.back().Facing = facing;
	Steps.back().Step = 0;
	Steps.back().Turn = 0;
	Steps.back().Jump = map->HexDist( x, y, next_x, next_y );
	Steps.back().Move = BattleTech::Move::JUMP;
	
	if( (game->State == BattleTech::State::MOVEMENT) && (SpeedNeeded() == BattleTech::Move::INVALID) )
	{
		Steps.pop_back();
		return false;
	}
	
	return true;
}


int Mech::StepCost( void ) const
{
	int cost = StandAttempts * 2;
	for( std::vector<MechStep>::const_iterator step = Steps.begin(); step != Steps.end(); step ++ )
		cost += step->Cost;
	return cost;
}


bool Mech::Reversed( void ) const
{
	for( std::vector<MechStep>::const_iterator step = Steps.begin(); step != Steps.end(); step ++ )
	{
		if( step->Step < 0 )
			return true;
	}
	return false;
}


uint8_t Mech::SpeedNeeded( bool final_position ) const
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexMap *map = game->Map();
	if( map )
	{
		uint8_t x, y;
		GetPosition( &x, &y );
		// Allow walking through team-mates but not ending movement on them.
		size_t mechs_in_hex = final_position ? map->MechsAt( x, y ) : map->TeamsAt( x, y );
		if( mechs_in_hex > 1 )
			return BattleTech::Move::INVALID;
	}
	
	int cost = StepCost();
	
	if( ! cost )
		return BattleTech::Move::STOP;
	if( cost <= WalkDist() )
		return BattleTech::Move::WALK;
	
	bool reversed = Reversed();
	
	if( (cost <= RunDist()) && ! reversed )
		return BattleTech::Move::RUN;
	if( (cost <= MASCDist()) && ! reversed )
		return BattleTech::Move::MASC;
	
	if( map && (! StandAttempts) && JumpDist() )
	{
		int jump_cost = final_position ? map->JumpCost( X, Y, Steps.back().X, Steps.back().Y ) : map->HexDist( X, Y, Steps.back().X, Steps.back().Y );
		if( jump_cost <= (int) JumpDist() )
			return BattleTech::Move::JUMP;
	}
	
	// Minimum Movement Rule allows entering the hex ahead using a single movement point.
	if( (Steps.size() == 1) && (Steps.back().Step == 1) && WalkDist() )
		return BattleTech::Move::RUN;
	
	// Minimum Movement Rule allows one stand attempt with a single movement point.
	if( (StandAttempts == 1) && (cost == 2) && WalkDist() )
		return BattleTech::Move::RUN;
	
	return BattleTech::Move::INVALID;
}


void Mech::Animate( double duration, uint8_t effect )
{
	GetPosition( &FromX, &FromY, &FromFacing );
	FromTorsoTwist = TorsoTwist;
	FromArmsFlipped = ArmsFlipped;
	FromProne = Prone;
	FromProneFire = ProneFire;
	FromTurnedArm = TurnedArm;
	MoveEffect = effect;
	MoveClock.Reset( duration );
}


void Mech::Draw( void )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexMap *map = game->Map();
	if( ! map )
		return;
	
	uint8_t x, y, facing;
	GetPosition( &x, &y, &facing );
	
	bool destroyed = Destroyed();
	if( destroyed && (map->MechAt_const(x,y) != this) )
		return;
	
	Pos3D pos = map->Center( x, y );
	Vec3D fwd = map->Direction( facing );
	Vec3D aim = map->Direction( facing + (Prone ? 0 : TorsoTwist) );
	Vec3D axis(0,0,1);
	
	double move_progress = MoveClock.Progress();
	if( move_progress < 1. )
	{
		Pos3D from = map->Center( FromX, FromY );
		pos.X = pos.X * move_progress + from.X * (1. - move_progress);
		pos.Y = pos.Y * move_progress + from.Y * (1. - move_progress);
		
		double from_facing = FromFacing;
		if( facing - FromFacing > 3 )
			from_facing += 6.;
		else if( facing - FromFacing < -3 )
			from_facing -= 6.;
		fwd = map->Direction( facing * move_progress + from_facing * (1. - move_progress) );
		
		double from_twist = FromProne ? 0. : FromTorsoTwist;
		double twist = TorsoTwist * move_progress + from_twist * (1. - move_progress);
		aim = fwd;
		aim.RotateAround( &axis, twist * 60. );
	}
	
	double radius = 0.5 * pow( Tons / (double) 100., 0.333 );
	
	bool friendly = false, enemy = false;
	if( Team && game->MyTeam() )
	{
		if( Team == game->MyTeam() )
			friendly = true;
		else
			enemy = true;
	}
	
	float r = 0.7f, g = 0.7f, b = 0.7f;
	if( friendly )
		b = 1.f;
	if( enemy )
		r = 1.f;
	
	if( destroyed )
	{
		float dim = std::max<float>( 0.4f, 1. - Locations[ BattleTech::Loc::CENTER_TORSO ].DestroyedClock.ElapsedSeconds() );
		r *= dim;
		g *= dim;
		b *= dim;
	}
	
	std::vector<uint8_t> locs;
	if( Locations[ BattleTech::Loc::LEFT_LEG ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::LEFT_LEG );
	if( Locations[ BattleTech::Loc::RIGHT_LEG ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::RIGHT_LEG );
	if( Locations[ BattleTech::Loc::LEFT_ARM ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::LEFT_ARM );
	if( Locations[ BattleTech::Loc::RIGHT_ARM ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::RIGHT_ARM );
	if( Locations[ BattleTech::Loc::CENTER_TORSO ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::CENTER_TORSO );
	if( Locations[ BattleTech::Loc::HEAD ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::HEAD );
	if( Locations[ BattleTech::Loc::LEFT_TORSO ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::LEFT_TORSO );
	if( Locations[ BattleTech::Loc::RIGHT_TORSO ].Tex.Frames.size() )
		locs.push_back( BattleTech::Loc::RIGHT_TORSO );
	
	for( std::vector<uint8_t>::const_iterator loc = locs.begin(); loc != locs.end(); loc ++ )
	{
		if( Locations[ *loc ].Tex.Frames.size() )
		{
			bool arm = Locations[ *loc ].IsArm();
			bool leg = Locations[ *loc ].IsLeg;
			bool limb = arm || leg;
			
			bool mirror = (*loc == BattleTech::Loc::LEFT_ARM) || (*loc == BattleTech::Loc::LEFT_LEG) || (*loc == BattleTech::Loc::LEFT_TORSO);
			bool upper = ! leg;
			float h = 1.f;
			if( Prone && (ProneFire != BattleTech::Loc::UNKNOWN) && upper && ! limb )
				h = 0.9f;
			else if( Prone && (*loc == ProneFire) )
				h = 0.85f;
			else if( Prone )
				h = limb ? 0.75f : 0.85f;
			else if( limb )
				h = upper ? 0.9f : 0.75f;
			
			double destroyed_secs = (limb && Locations[ *loc ].Destroyed) ? Locations[ *loc ].DestroyedClock.ElapsedSeconds() : 0.;
			float a = 1.f;
			if( destroyed_secs )
			{
				a = std::min<float>( 1.f, 2. - destroyed_secs );
				if( a < 0.f )
					continue;
				h *= std::max<float>( 0.5f, 1. - destroyed_secs * 0.5 );
			}
			
			Vec3D up( upper ? &aim : &fwd );
			Vec3D right( &up );
			right.RotateAround( &axis, mirror ? -90 : 90. );
			up.ScaleBy(    radius );
			right.ScaleBy( radius );
			Pos3D center = pos;
			
			if( Prone || ((move_progress < 1.) && (MoveEffect == BattleTech::Effect::STAND)) )
			{
				double prone = 1.;
				if( (move_progress < 1.) && (MoveEffect == BattleTech::Effect::FALL) )
					prone = move_progress;
				else if( (move_progress < 1.) && (MoveEffect == BattleTech::Effect::STAND) )
					prone = 1. - move_progress;
				
				if( upper )
				{
					double firing = 0.;
					if( (move_progress < 1.) && (ProneFire != FromProneFire) )
						firing = (ProneFire != BattleTech::Loc::UNKNOWN) ? move_progress : (1. - move_progress);
					else if( ProneFire != BattleTech::Loc::UNKNOWN )
						firing = 1.;
					
					double fwd = 0.15 * prone;
					double rot = (mirror ? 10. : -10.) * prone;
					
					fwd *= (1. - firing * 0.5);
					if( (*loc == ProneFire) || (*loc == FromProneFire) )
						rot *= (1. - firing);
					else if( limb )
						rot *= (1. + firing * 0.5);
					
					center += up * fwd;
					if( limb )
					{
						up.RotateAround(    &axis, rot );
						right.RotateAround( &axis, rot );
					}
				}
				else
				{
					center += up * -0.1 * prone;
					center += right * 0.2 * prone;
					double rotation = (mirror ? -30. : 30.) * prone;
					up.RotateAround(    &axis, rotation );
					right.RotateAround( &axis, rotation );
				}
			}
			
			if( (move_progress < 1.) && ((MoveEffect == BattleTech::Effect::STEP) || (MoveEffect == BattleTech::Effect::TURN)) )
			{
				if( leg && ! Prone )
					center.MoveAlong( &up, sin( move_progress * M_PI * 2. ) * (mirror ? -0.1 : 0.1) );
				else if( (leg || arm) && Prone )
					center.MoveAlong( &up, sin( move_progress * M_PI * 2. ) * ((mirror == leg) ? -0.05 : 0.05) );
			}
			else if( (move_progress < 1.) && (MoveEffect == BattleTech::Effect::JUMP) )
			{
				double height = sin( move_progress * M_PI );
				up.ScaleBy(    1. + height * 0.1 );
				right.ScaleBy( 1. + height * 0.1 );
				h = powf( h, 1.f - (float) height * 0.9f );
			}
			else if( (move_progress < 1.) && ((MoveEffect == BattleTech::Effect::LEFT_PUNCH) || (MoveEffect == BattleTech::Effect::RIGHT_PUNCH)) && upper )
			{
				double punch = sin( move_progress * M_PI );
				if( ArmsFlipped )
					punch *= -1.;
				else
					center.MoveAlong( &up, punch * 0.2 );
				double rotate = punch * 20.;
				if( arm && ((MoveEffect == BattleTech::Effect::LEFT_PUNCH) == mirror) )
					rotate = punch * 30.;
				if( MoveEffect == BattleTech::Effect::RIGHT_PUNCH )
					rotate *= -1.;
				up.RotateAround(    &axis, rotate );
				right.RotateAround( &axis, rotate );
			}
			else if( (move_progress < 1.) && ((MoveEffect == BattleTech::Effect::LEFT_KICK) || (MoveEffect == BattleTech::Effect::RIGHT_KICK)) )
			{
				if( leg && ((MoveEffect == BattleTech::Effect::LEFT_KICK) == mirror) )
					center.MoveAlong( &up, sin( move_progress * M_PI ) * 0.3 );
				else if( upper )
					center.MoveAlong( &up, sin( move_progress * M_PI ) * 0.1 );
			}
			
			if( (*loc == TurnedArm) || ((*loc == FromTurnedArm) && (move_progress < 1.)) )
			{
				double turned = 0.;
				if( (move_progress < 1.) && (TurnedArm != FromTurnedArm) )
					turned = (*loc == TurnedArm) ? move_progress : (1. - move_progress);
				else if( *loc == TurnedArm )
					turned = 1.;
				double rotate = turned * (mirror ? -15 : 15.);
				center.MoveAlong( &up,    turned * 0.08 );
				center.MoveAlong( &right, turned * 0.05 );
				up.RotateAround(    &axis, rotate );
				right.RotateAround( &axis, rotate );
				center.MoveAlong( &up,    turned * -0.02 );
			}
			
			bool arms_flipped = ArmsFlipped && ! Prone;
			bool arms_flipping = ((move_progress < 1.) && (arms_flipped != FromArmsFlipped));
			if( arm && (arms_flipped || arms_flipping) )
			{
				double flip = 1.;
				if( arms_flipping )
					flip = arms_flipped ? move_progress : (1. - move_progress);
				center.MoveAlong( &up, flip * -0.28 );
				if( flip < 0.5 )
					up.ScaleBy( 1. - flip );
				else
					up.ScaleBy( flip * -1. );
			}
			
			if( destroyed_secs )
			{
				center.MoveAlong(  &right, destroyed_secs * 0.5 );
				up.RotateAround(    &axis, destroyed_secs * (mirror ? 60. : -60.) );
				right.RotateAround( &axis, destroyed_secs * (mirror ? 60. : -60.) );
			}
			
			Pos3D tl = center + up - right;
			Pos3D tr = center + up + right;
			Pos3D bl = center - up - right;
			Pos3D br = center - up + right;
			
			glEnable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D, Locations[ *loc ].Tex.CurrentFrame() );
			glColor4f( r*h, g*h, b*h, a );
			
			glBegin( GL_QUADS );
				
				glTexCoord2i( 0, 0 );
				glVertex3d( tl.X, tl.Y, tl.Z );
				
				glTexCoord2i( 0, 1 );
				glVertex3d( bl.X, bl.Y, bl.Z );
				
				glTexCoord2i( 1, 1 );
				glVertex3d( br.X, br.Y, br.Z );
				
				glTexCoord2i( 1, 0 );
				glVertex3d( tr.X, tr.Y, tr.Z );
				
			glEnd();
			
			glDisable( GL_TEXTURE_2D );
			glColor4f( 1.f, 1.f, 1.f, 1.f );
		}
	}
	
	if( ! locs.size() )
	{
		radius = 0.22 + (Tons / (double) 1000.);
		
		if( Prone )
		{
			r *= 0.8f;
			g *= 0.8f;
			b *= 0.8f;
		}
		
		Raptor::Game->Gfx.DrawCircle2D( pos.X, pos.Y, radius, 12, 0, r,g,b,1.f );
		Raptor::Game->Gfx.DrawCircleOutline2D( pos.X, pos.Y, radius, 12, 0, 0,0,0,1 );
		
		if( ! destroyed )
		{
			Pos3D dir = pos + (map->Direction( facing ) * radius);
			Raptor::Game->Gfx.DrawLine2D( pos.X, pos.Y, dir.X, dir.Y, 4.f, 0,0,0,1 );
			dir = pos + (map->Direction( facing + TorsoTwist ) * 0.5);
			Raptor::Game->Gfx.DrawLine2D( pos.X, pos.Y, dir.X, dir.Y, 4.f, 0,0,0,1 );
		}
	}
	
	if( ECM && ! ECM->Damaged && ! Shutdown && ! Destroyed() )
	{
		if( friendly )
			glColor4f( 0.5f, 1.0f, 0.0f, 0.5f );
		else
			glColor4f( 1.0f, 1.0f, 0.0f, 0.4f );
		glLineWidth( 2.f );
		glBegin( GL_LINE_STRIP );
			glVertex2d( pos.X      , pos.Y - 6.57 );
			glVertex2d( pos.X + 5.7, pos.Y - 3.3  );
			glVertex2d( pos.X + 5.7, pos.Y + 3.3  );
			glVertex2d( pos.X      , pos.Y + 6.57 );
			glVertex2d( pos.X - 5.7, pos.Y + 3.3  );
			glVertex2d( pos.X - 5.7, pos.Y - 3.3  );
			glVertex2d( pos.X      , pos.Y - 6.57 );
		glEnd();
		glColor4f( 1.f, 1.f, 1.f, 1.f );
	}
}


static const char *HealthIcons[ BattleTech::Loc::COUNT * 3 ] = {
	"hp_la_s.png","hp_lt_s.png","hp_ll_s.png","hp_ra_s.png","hp_rt_s.png","hp_rl_s.png","hp_hd_s.png","hp_ct_s.png",
	"hp_la_a.png","hp_lt_f.png","hp_ll_a.png","hp_ra_a.png","hp_rt_f.png","hp_rl_a.png","hp_hd_a.png","hp_ct_f.png",
	"",           "hp_lt_r.png",""           ,"",           "hp_rt_r.png",""           ,"",           "hp_ct_r.png"
};


void Mech::DrawHealth( double x1, double y1, double x2, double y2 ) const
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	bool playing_events = (game->EventClock.RemainingSeconds() > -0.1) || game->Events.size();
	
	for( size_t i = 0; i < BattleTech::Loc::COUNT; i ++ )
	{
		double last_hit = Locations[ i ].HitClock.ElapsedSeconds();
		bool recent_hit = (last_hit <= 1.5);
		bool blink_hit = ((int)( last_hit * 8. )) % 2;
		
		if( Locations[ i ].Structure )
		{
			if( (Locations[ i ].HitWhere != STRUCTURE) || (! recent_hit) || blink_hit )
			{
				float portion = Locations[ i ].Structure / (float) Locations[ i ].MaxStructure;
				GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ i ] );
				Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 1.f,portion,floorf(portion),1.f );
			}
			
			if( Locations[ i ].Armor )
			{
				if( (Locations[ i ].IsTorso && (Locations[ i ].HitWhere == REAR)) || (Locations[ i ].HitWhere == STRUCTURE) || (! recent_hit) || blink_hit )
				{
					float portion = Locations[ i ].Armor / (float) Locations[ i ].MaxArmor;
					GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ i + BattleTech::Loc::COUNT ] );
					Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 1.f,portion,floorf(portion),1.f );
				}
			}
			else if( (Locations[ i ].HitWhere == FRONT) && recent_hit && blink_hit )
			{
				GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ i + BattleTech::Loc::COUNT ] );
				Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 0.f,0.f,0.f,1.f );
			}
			
			if( Locations[ i ].RearArmor )
			{
				if( (Locations[ i ].HitWhere != REAR) || (! recent_hit) || blink_hit )
				{
					float portion = Locations[ i ].RearArmor / (float) Locations[ i ].MaxRearArmor;
					GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ i + BattleTech::Loc::COUNT * 2 ] );
					Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 1.f,portion,floorf(portion),1.f );
				}
			}
			else if( Locations[ i ].IsTorso && (Locations[ i ].HitWhere == REAR) && recent_hit && blink_hit )
			{
				GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ i + BattleTech::Loc::COUNT * 2 ] );
				Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 0.f,0.f,0.f,1.f );
			}
		}
		else if( (Locations[ i ].HitWhere == STRUCTURE) && recent_hit && blink_hit )
		{
			GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ i ] );
			Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 0.f,0.f,0.f,1.f );
		}
	}
	
	if( Destroyed() && (! playing_events) && (((int)( Lifetime.ElapsedSeconds() * 2. )) % 2) )
	{
		if( Engine && (Engine->Damaged >= 3) )
		{
			GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ BattleTech::Loc::CENTER_TORSO ] );
			Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 0.f,0.f,0.f,1.f );
		}
		
		if( Cockpit && Cockpit->Damaged )
		{
			GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ BattleTech::Loc::HEAD ] );
			Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 0.f,0.f,0.f,1.f );
		}
		else if( PilotDamage >= 6 )
		{
			GLuint texture = Raptor::Game->Res.GetTexture( HealthIcons[ BattleTech::Loc::HEAD ] );
			Raptor::Game->Gfx.DrawRect2D( x1, y1, x2, y2, texture, 1.f,0.f,1.f,1.f );
		}
	}
}
