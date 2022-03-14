/*
 *  BotAI.cpp
 */

#include "BotAI.h"

#include "BattleTechServer.h"
#include "Rand.h"
#include <algorithm>
#include <cmath>


BotAI::BotAI( void )
{
	BeginPhase( Raptor::State::CONNECTED );
}


BotAI::~BotAI()
{
}


void BotAI::Update( double dt )
{
	Waited += dt;
	if( Waited > 0.5 )
	{
		Waited = 0.;
		
		if( ControlsTeam() )
			TakeTurn();
	}
}


bool BotAI::ControlsTeam( uint8_t team ) const
{
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	
	if( (! team) && server->TeamTurns.size() )
		team = server->TeamTurns.front();
	if( ! team )
		return false;
	
	std::vector<int> ai_teams = server->Data.PropertyAsInts("ai_team");
	if( std::find( ai_teams.begin(), ai_teams.end(), team ) != ai_teams.end() )
		return true;
	
	if( server->Data.PropertyAsString("ai_team") == "auto" )
	{
		for( std::map<uint16_t,Player*>::const_iterator player = server->Data.Players.begin(); player != server->Data.Players.end(); player ++ )
		{
			uint8_t player_team = player->second->PropertyAsInt("team");
			if( player_team == team )
				return false;
		}
		return true;
	}
	
	return false;
}


void BotAI::BeginPhase( int state )
{
	Waited = 0.;
	for( std::map<uint32_t,Packet*>::iterator pt = PreparedTurns.begin(); pt != PreparedTurns.end(); pt ++ )
		delete pt->second;
	PreparedTurns.clear();
	
	if( (state == BattleTech::State::TAG)
	||  (state == BattleTech::State::WEAPON_ATTACK)
	||  (state == BattleTech::State::PHYSICAL_ATTACK) )
	{
		// Shots should be declared without knowing the outcome of anyone else's shots,
		// so we prepare them at the start of the phase before any have been submitted.
		
		BattleTechServer *server = (BattleTechServer*) Raptor::Server;
		HexMap *map = server->Map();
		
		for( std::map< uint8_t, std::set<Mech*> >::iterator team_unmoved = server->UnmovedUnits.begin(); team_unmoved != server->UnmovedUnits.end(); team_unmoved ++ )
		{
			for( std::set<Mech*>::iterator mech_iter = team_unmoved->second.begin(); mech_iter != team_unmoved->second.end(); mech_iter ++ )
			{
				Mech *mech = *mech_iter;
				std::map<Mech*,BotAIAim> PossibleAim;
				
				for( std::map<uint32_t,GameObject*>::iterator obj_iter = server->Data.GameObjects.begin(); obj_iter != server->Data.GameObjects.end(); obj_iter ++ )
				{
					if( obj_iter->second->Type() != BattleTech::Object::MECH )
						continue;
					Mech *target = (Mech*) obj_iter->second;
					if( target->Team == mech->Team )
						continue;
					if( target->Destroyed() )
						continue;
					
					PossibleAim[ target ].Initialize( mech, target, state, map );
				}
				
				std::set<BotAIAim> sorted;
				for( std::map<Mech*,BotAIAim>::iterator aim = PossibleAim.begin(); aim != PossibleAim.end(); aim ++ )
					sorted.insert( aim->second );
				
				Packet *p = NULL;
				while( sorted.size() && ! p )
				{
					std::set<BotAIAim>::iterator best = sorted.begin();
					mech->WeaponsToFire = best->WeaponsToFire;
					mech->SelectedMelee = best->SelectedMelee;
					int8_t torso_twist = mech->TorsoTwist;
					mech->TorsoTwist = best->TorsoTwist;
					p = best->CreatePacket( mech, best->Target, state );
					mech->TorsoTwist = torso_twist;
					sorted.erase( best );
				}
				
				if( p )
					PreparedTurns[ mech->ID ] = p;
			}
		}
	}
}


void BotAI::BeginTurn( void )
{
	Waited = 0.;
}


void BotAI::TakeTurn( void )
{
	// Make sure the current team has any Mechs to move.
	BattleTechServer *server = (BattleTechServer*) Raptor::Server;
	if( ! server->TeamTurns.size() )
		return;
	std::map< uint8_t, std::set<Mech*> >::iterator team_unmoved = server->UnmovedUnits.find( server->TeamTurns.front() );
	if( team_unmoved == server->UnmovedUnits.end() )
		return;
	
	// Find all Mechs that can move now for the current team.
	std::vector<Mech*> mechs;
	for( std::set<Mech*>::iterator mech_iter = team_unmoved->second.begin(); mech_iter != team_unmoved->second.end(); mech_iter ++ )
	{
		if( server->MechTurn( *mech_iter ) )
			mechs.push_back( *mech_iter );
	}
	if( ! mechs.size() )
		return;
	
	// Select a viable Mech at random.
	// FIXME: Should this be selected intelligently?
	Mech *mech = mechs.at( rand() % mechs.size() );
	
	std::map<uint32_t,Packet*>::iterator prepared_turn = PreparedTurns.find( mech->ID );
	if( prepared_turn != PreparedTurns.end() )
	{
		Packet *p = prepared_turn->second;
		PreparedTurns.erase( prepared_turn );
		server->ProcessPacket( p, NULL );
		delete p;
	}
	else if( server->State == BattleTech::State::MOVEMENT )
	{
		if( mech->Prone && mech->CanStand() && ! mech->Unconscious && ! mech->Shutdown )
		{
			if( ! mech->MoveSpeed )
				mech->MoveSpeed = BattleTech::Move::RUN; // FIXME: Choose walk/run intelligently?
			
			Packet p( BattleTech::Packet::STAND );
			p.AddUInt( mech->ID );
			p.AddUChar( mech->MoveSpeed );
			server->ProcessPacket( &p, NULL );
		}
		else
		{
			HexMap *map = server->Map();
			
			// Find the set of steps that provides the best likely damage to any possible target.
			std::vector<MechStep> best_steps;
			double best_value = 0.;
			Mech *best_target = NULL;
			for( std::map<uint32_t,GameObject*>::iterator obj_iter = server->Data.GameObjects.begin(); obj_iter != server->Data.GameObjects.end(); obj_iter ++ )
			{
				if( obj_iter->second->Type() != BattleTech::Object::MECH )
					continue;
				Mech *target = (Mech*) obj_iter->second;
				if( target->Team == mech->Team )
					continue;
				if( target->Destroyed() )
					continue;
				
				mech->Steps.clear();
				
				while( (mech->SpeedNeeded() != BattleTech::Move::INVALID) && (mech->Steps.size() < 100) )
				{
					BotAIAim aim;
					aim.Initialize( mech, target, server->State, map );
					double value = aim.Value();
					
					if( (value > best_value) && (mech->SpeedNeeded(true) != BattleTech::Move::INVALID) )
					{
						best_steps = mech->Steps;
						best_value = value;
						best_target = target;
					}
					
					int8_t turn = 0;
					uint8_t firing_arc = mech->FiringArc( target->X, target->Y );
					if( firing_arc == BattleTech::Arc::REAR )
						turn = (mech->RelativeAngle( target->X, target->Y ) < 0.) ? -1 : 1;
					else if( firing_arc == BattleTech::Arc::LEFT_SIDE )
						turn = -1;
					else if( firing_arc == BattleTech::Arc::RIGHT_SIDE )
						turn = 1;
					
					if( turn )
					{
						if( ! mech->Turn( turn ) )
							break;
					}
					else
					{
						// FIXME: Consider reverse movement, as it may improve chances to hit with minimum ranges.
						if( ! mech->Step( BattleTech::Move::WALK ) )
						{
							// Try to walk around the obstruction.
							if( ! mech->Turn( Rand::Bool() ? -1 : 1 ) )
								break;
							if( ! mech->Step( BattleTech::Move::WALK ) )
								break;
						}
					}
				}
			}
			
			mech->Steps = best_steps;
			
			uint8_t speed_needed = mech->SpeedNeeded(true);
			if( (speed_needed == BattleTech::Move::JUMP) && best_target )
			{
				// If we're jumping, select the best possible landing orientation.
				for( size_t i = 0; i < 6; i ++ )
				{
					mech->Turn( 1 );
					
					BotAIAim aim;
					aim.Initialize( mech, best_target, server->State, map );
					double value = aim.Value();
					
					if( (value > best_value) && (mech->SpeedNeeded(true) != BattleTech::Move::INVALID) )
					{
						best_steps = mech->Steps;
						best_value = value;
					}
				}
				
				mech->Steps = best_steps;
			}
			
			Packet p( BattleTech::Packet::MOVEMENT );
			p.AddUInt( mech->ID );
			p.AddUChar( speed_needed );
			p.AddUChar( mech->Steps.size() );
			for( std::vector<MechStep>::const_iterator step = mech->Steps.begin(); step != mech->Steps.end(); step ++ )
				p.AddUChar( step->Move );
			server->ProcessPacket( &p, NULL );
		}
	}
	else if( (server->State == BattleTech::State::TAG) || (server->State == BattleTech::State::WEAPON_ATTACK) )
	{
		Packet p( BattleTech::Packet::SHOTS );
		p.AddUInt( mech->ID );
		p.AddChar( mech->TorsoTwist );
		p.AddChar( mech->TurnedArm );
		p.AddChar( mech->ProneFire );
		p.AddUChar( mech->ArmsFlipped ? 0x80 : 0 ); // count
		server->ProcessPacket( &p, NULL );
	}
	else if( server->State == BattleTech::State::PHYSICAL_ATTACK )
	{
		Packet p( BattleTech::Packet::MELEE );
		p.AddUInt( mech->ID );
		p.AddUChar( 0 ); // count
		server->ProcessPacket( &p, NULL );
	}
}


// -----------------------------------------------------------------------------


void BotAIAim::Initialize( Mech *from, const Mech *target, int state, const HexMap *map )
{
	From = from;
	Target = target;
	State = state;
	
	TorsoTwist = 0;
	ValueBonus = 0.;
	
	uint8_t x, y;
	From->GetPosition( &x, &y );
	Path = map->Path( x, y, Target->X, Target->Y );
	double angle = From->RelativeAngle( Target->X, Target->Y );
	
	if( State == BattleTech::State::MOVEMENT )
	{
		const Hex *hex = &(map->Hexes[ x ][ y ]);
		uint8_t speed = From->SpeedNeeded();
		
		// Slightly prefer getting closer to target.
		ValueBonus += 0.1 * (map->HexDist( From->X, From->Y, Target->X, Target->Y ) - map->HexDist( x, y, Target->X, Target->Y ));
		
		// Slightly prefer higher elevation, like a cat.
		ValueBonus += hex->Height * 0.1;
		
		// Prefer stopping in trees.
		ValueBonus += hex->Forest;
		
		// Prefer partial cover from return fire.
		ShotPath incoming = map->Path( Target->X, Target->Y, x, y );
		if( incoming.LineOfSight && incoming.PartialCover )
			ValueBonus += target->TookTurn ? 1.25 : 0.5;
		
		// Very slightly prefer facing the target.  This is mostly used for jump facing.
		ValueBonus += (180. - fabs(angle)) * 0.0001;
		
		// Prefer moving far enough for a better defense bonus.
		int entered = 0;
		if( speed == BattleTech::Move::JUMP )
		{
			entered = map->HexDist( From->X, From->Y, x, y );
			ValueBonus += 1.;
		}
		else
		{
			for( std::vector<MechStep>::const_iterator step = From->Steps.begin(); step != From->Steps.end(); step ++ )
				entered += step->Step; // FIXME: This will not be correct if the AI uses reverse movement.
		}
		if( entered >= 25 )
			ValueBonus += 6.;
		else if( entered >= 18 )
			ValueBonus += 5.;
		else if( entered >= 10 )
			ValueBonus += 4.;
		else if( entered >= 7 )
			ValueBonus += 3.;
		else if( entered >= 5 )
			ValueBonus += 2.;
		else if( entered >= 3 )
			ValueBonus += 1.;
		
		// Avoid using MASC or Supercharger multiple times in a row.
		if( (speed == BattleTech::Move::MASC) || (speed == BattleTech::Move::MASC_SUPERCHARGE) )
			ValueBonus -= 10. * From->MASCTurns;
		if( (speed == BattleTech::Move::SUPERCHARGE) || (speed == BattleTech::Move::MASC_SUPERCHARGE) )
			ValueBonus -= 10. * From->SuperchargerTurns;
	}
	
	if( (State == BattleTech::State::WEAPON_ATTACK) || (State == BattleTech::State::MOVEMENT) )
	{
		// Determine best torso twist to hit this target.
		uint8_t firing_arc = From->FiringArc( Target->X, Target->Y );
		if( firing_arc == BattleTech::Arc::LEFT_SIDE )
			TorsoTwist = -1;
		else if( firing_arc == BattleTech::Arc::RIGHT_SIDE )
			TorsoTwist = 1;
		else if( firing_arc == BattleTech::Arc::REAR )
		{
			double value_c = ValueWithTwist( from, 0 );
			double value_l = ValueWithTwist( from, -1 );
			double value_r = ValueWithTwist( from, 1 );
			if( (value_l > value_c) || (value_r > value_c) )
			{
				if( value_r > value_l )
					TorsoTwist = 1;
				else if( value_l > value_r )
					TorsoTwist = -1;
				else
					TorsoTwist = (angle > 0.) ? 1 : -1;
			}
		}
		
		// Temporarily set the Mech's TorsoTwist before UpdateWeaponsInRange.
		from->TorsoTwist = TorsoTwist;
	}
	
	UpdateWeaponsInRange( From, Target, State );
	ChooseWeapons();
	
	if( State < BattleTech::State::PHYSICAL_ATTACK )
		from->TorsoTwist = 0;
}


void BotAIAim::ChooseWeapons( void )
{
	WeaponsToFire.clear();
	for( std::map<uint8_t,int8_t>::const_iterator weapon = WeaponsInRange.begin(); weapon != WeaponsInRange.end(); weapon ++ )
	{
		const MechEquipment *eq = &(From->Equipment[ weapon->first ]);
		WeaponsToFire[ weapon->first ] = std::max<uint8_t>( 1, eq->Weapon->RapidFire );
	}
	
	int16_t base_heat = From->Heat - From->HeatDissipation;
	int16_t shot_heat = ShotHeat();
	while( shot_heat && ((shot_heat + base_heat) >= 5) )
	{
		// Always fire at least 1 shot.
		if( (WeaponsToFire.size() == 1) && (WeaponsToFire.begin()->second == 1) )
			break;
		
		double worst_value = 7777777.;
		uint8_t worst_index = 0;
		for( std::map<uint8_t,uint8_t>::const_iterator weap = WeaponsToFire.begin(); weap != WeaponsToFire.end(); weap ++ )
		{
			const MechEquipment *eq = &(From->Equipment[ weap->first ]);
			
			// Start with damage multiplied by likely rapid fire hits.
			double value = std::max<double>( 0.9, eq->Weapon->Damage )
			             * std::max<double>( 1., weap->second * 0.6 );
			
			// Multiply by likeliness to hit.
			std::map<uint8_t,int8_t>::const_iterator difficulty = WeaponsInRange.find( weap->first );
			if( (difficulty == WeaponsInRange.end()) || (difficulty->second > 12) )
				value = 0.;
			else
				value *= (11. - std::max<double>( 0., difficulty->second - 2 )) / 11.;
			
			// Multiply by likely cluster hits.
			if( eq->Weapon->Type == HMWeapon::MISSILE_STREAK )
				value *= std::max<double>( 1., eq->Weapon->ClusterWeaponSize );
			else
				value *= std::max<double>( 1., eq->Weapon->ClusterWeaponSize * 0.6 );
			
			// Divide by heat.
			double weap_heat = eq->Weapon->Heat * weap->second;
			if( eq->Weapon->Type == HMWeapon::MISSILE_STREAK )
				weap_heat *= (11. - std::max<double>( 0., difficulty->second - 2 )) / 11.;
			value /= std::max<double>( 0.01, weap_heat );
			
			if( value < worst_value )
			{
				worst_value = value;
				worst_index = weap->first;
			}
		}
		
		if( WeaponsToFire[ worst_index ] )
			WeaponsToFire[ worst_index ] --;
		else
			break;
		
		shot_heat = ShotHeat();
	}
	
	SelectedMelee.clear();
	if( PossibleMelee.size() )
		SelectedMelee.insert( *(PossibleMelee.begin()) );
}


int16_t BotAIAim::ShotHeat( void ) const
{
	if( (State != BattleTech::State::TAG) && (State != BattleTech::State::WEAPON_ATTACK) )
		return 0.;
	
	int16_t heat = 0.;
	
	for( std::map<uint8_t,uint8_t>::const_iterator weap = WeaponsToFire.begin(); weap != WeaponsToFire.end(); weap ++ )
	{
		std::map<uint8_t,int8_t>::const_iterator difficulty = WeaponsInRange.find( weap->first );
		if( (difficulty == WeaponsInRange.end()) || (difficulty->second > 12) )
			continue;
		
		const MechEquipment *eq = &(From->Equipment[ weap->first ]);
		int16_t weap_heat = eq->Weapon->Heat * weap->second;
		
		// Scale heat for Streak by likeliness to hit.
		if( eq->Weapon->Type == HMWeapon::MISSILE_STREAK )
			weap_heat *= (11. - std::max<double>( 0., difficulty->second - 2 )) / 10.9;
		
		heat += weap_heat;
	}
	
	return heat;
}


double BotAIAim::Value( void ) const
{
	double value = 0.;
	
	if( (State == BattleTech::State::TAG) || (State == BattleTech::State::WEAPON_ATTACK) || (State == BattleTech::State::MOVEMENT) )
	{
		for( std::map<uint8_t,uint8_t>::const_iterator weap = WeaponsToFire.begin(); weap != WeaponsToFire.end(); weap ++ )
		{
			std::map<uint8_t,int8_t>::const_iterator difficulty = WeaponsInRange.find( weap->first );
			if( (difficulty == WeaponsInRange.end()) || (difficulty->second > 12) )
				continue;
			
			const MechEquipment *eq = &(From->Equipment[ weap->first ]);
			bool fcs = eq->WeaponFCS && ! eq->WeaponFCS->Damaged;
			double damage = std::max<double>( 0.9, eq->Weapon->Damage )
			              * std::max<double>( 1., eq->Weapon->ClusterWeaponSize * (fcs ? 0.8 : 0.6) )
			              * std::max<double>( 1., weap->second * 0.6 );
			
			value += damage * (11. - std::max<double>( 0., difficulty->second - 2 )) / 11.;
		}
	}
	
	if( (State == BattleTech::State::PHYSICAL_ATTACK) || (State == BattleTech::State::MOVEMENT) )
	{
		for( std::set<MechMelee>::const_iterator melee = SelectedMelee.begin(); melee != SelectedMelee.end(); melee ++ )
			value += melee->Damage * (11. - std::max<double>( 0., melee->Difficulty - 2 )) / 11.;
	}
	
	// Be wary of targets that could still move away from their current position.
	uint8_t target_can_move = 0;
	if( (State == BattleTech::State::MOVEMENT) && ! Target->TookTurn )
	{
		target_can_move = std::max<uint8_t>( std::max<uint8_t>( Target->WalkDist(), Target->RunDist() ), std::max<uint8_t>( Target->MASCDist(), Target->JumpDist() ) );
		value /= 1. + 0.25 * target_can_move;
	}
	
	if( ! target_can_move )
	{
		// Prefer getting behind enemies.
		uint8_t x, y;
		From->GetPosition( &x, &y );
		if( Target->DamageArc( x, y ) == BattleTech::Arc::REAR )
			value *= 1.5;
	}
	
	// Prefer better cover and getting closer (even when no shots will hit).
	value += (ValueBonus * 2.);
	
	return value;
}


double BotAIAim::ValueWithTwist( Mech *from, int8_t twist )
{
	// Keep the old data to restore.
	int8_t torso_twist = from->TorsoTwist;
	std::map<uint8_t,int8_t> weapons_in_range = WeaponsInRange;
	std::map<uint8_t,uint8_t> weapons_to_fire = WeaponsToFire;
	std::set<MechMelee> possible_melee = PossibleMelee;
	std::set<MechMelee> selected_melee = SelectedMelee;
	
	// Change the data and calculate the value.
	from->TorsoTwist = twist;
	UpdateWeaponsInRange( from, Target, State );
	ChooseWeapons();
	double value = Value();
	
	// Restore the old data.
	from->TorsoTwist = torso_twist;
	WeaponsInRange = weapons_in_range;
	WeaponsToFire = weapons_to_fire;
	PossibleMelee = possible_melee;
	SelectedMelee = selected_melee;
	
	return value;
}


bool BotAIAim::operator < ( const BotAIAim &other ) const
{
	double value = Value(), other_value = other.Value();
	if( value != other_value )
		return (value > other_value);
	
	int16_t heat = ShotHeat(), other_heat = other.ShotHeat();
	if( heat != other_heat )
		return (heat < other_heat);
	
	if( WeaponsToFire.size() != other.WeaponsToFire.size() )
		return (WeaponsToFire.size() > other.WeaponsToFire.size());
	if( WeaponsInRange.size() != other.WeaponsInRange.size() )
		return (WeaponsInRange.size() > other.WeaponsInRange.size());
	if( SelectedMelee.size() != other.SelectedMelee.size() )
		return (SelectedMelee.size() > other.SelectedMelee.size());
	if( PossibleMelee.size() != other.PossibleMelee.size() )
		return (PossibleMelee.size() > other.PossibleMelee.size());
	
	if( Path.Distance != other.Path.Distance )
		return (Path.Distance < other.Path.Distance );
	
	return (Target < other.Target);
}
