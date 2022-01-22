/*
 *  HeavyMetal.cpp
 */

#include "HeavyMetal.h"

#include "Endian.h"
#include "BattleTechDefs.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


// -----------------------------------------------------------------------------


static short i16( const unsigned char *src )
{
	short s;
	Endian::CopyLittle( src, &s, 2 );
	return s;
}


static int i32( const unsigned char *src )
{
	int i;
	Endian::CopyLittle( src, &i, 4 );
	return i;
}


static float f32( const unsigned char *src )
{
	float f;
	Endian::CopyLittle( src, &f, 4 );
	return f;
}


// -----------------------------------------------------------------------------


unsigned char *HeavyMetal::hmstrcpy( char *dst, unsigned char *src )
{
	size_t len = i16( src );
	unsigned char *str = src + 2;
	
	if( dst )
	{
		memcpy( dst, str, len );
		dst[ len ] = '\0';
		
		for( size_t i = 0; i < len; i ++ )
		{
			if( str[ i ] == 0xB3 )
				dst[ i ] = '3';
		}
	}
	
	return str + len;
}


std::map< short, HMStructure > HeavyMetal::Structures( const char *filename )
{
	std::map< short, HMStructure > structures;
	unsigned char buf[ 256 ] = {0};
	
	FILE *f = fopen( filename, "rb" );
	if( ! f )
		return structures;
	size_t size = fread( buf, 1, sizeof(buf), f );
	fclose( f );
	f = NULL;
	
	short weight = 10;
	unsigned char *ptr = buf;
	while( ptr < buf + size )
	{
		HMStructure s;
		
		s.Head        = 3;
		s.CenterTorso = ptr[ 0 ];
		s.SideTorso   = ptr[ 2 ];
		s.Arm         = ptr[ 4 ];
		s.Leg         = ptr[ 6 ];
		
		ptr += 8;
		
		structures[ weight ] = s;
		weight += 5;
	}
	
	return structures;
}


std::map< short, HMWeapon > HeavyMetal::Weapons( const char *filename )
{
	std::map< short, HMWeapon > weapons;
	unsigned char buf[ 32768 ] = {0};
	char tmp[ 128 ] = {0};
	size_t tmp_len = 0;
	
	FILE *f = fopen( filename, "rb" );
	if( ! f )
		return weapons;
	size_t size = fread( buf, 1, sizeof(buf), f );
	fclose( f );
	f = NULL;
	unsigned char magic[ 4 ] = {0xF4,0x01,0x36,0x01};
	if( memcmp( buf, magic, 4 ) != 0 )
		return weapons;
	
	short next_wid = 0x33;
	unsigned char *ptr = buf + 4;
	while( ptr < buf + size )
	{
		HMWeapon w;
		
		w.Type = ptr[ 6 ];
		
		w.ID = i16( ptr + 8 );
		if( ! w.ID )
			w.ID = next_wid;
		
		ptr = hmstrcpy( tmp, ptr + 10 );
		tmp_len = strlen(tmp);
		if( tmp_len && (tmp[ tmp_len - 1 ] == ' ') )
			tmp[ tmp_len - 1 ] = '\0';
		w.Name = tmp;
		
		ptr = hmstrcpy( tmp, ptr );
		tmp_len = strlen(tmp);
		if( tmp_len && (tmp[ tmp_len - 1 ] == ' ') )
			tmp[ tmp_len - 1 ] = '\0';
		w.AmmoName = tmp;
		
		ptr = hmstrcpy( tmp, ptr );
		w.HeatStr = tmp;
		
		w.DamageMin = ptr[ 2 ];
		w.DamageAvg = ptr[ 4 ];
		
		ptr = hmstrcpy( tmp, ptr + 6 );
		w.DamageStr = tmp;
		
		w.ExplodeDamage = ptr[ 0 ];
		
		w.AmmoExplodes            = ptr[ 2 ] & 0x01;
		w.CanUseArtemisSRM        = ptr[ 2 ] & 0x02;
		w.CanUseTargetingComputer = ptr[ 2 ] & 0x04;
		w.CanUseArtemisLRM        = ptr[ 2 ] & 0x08;
		
		w.SnakeEyesEffect = ptr[ 4 ];
		w.ToHit = i16( ptr + 6 );
		w.RangeMin = ptr[ 8 ] - 1;
		w.RangeShort = ptr[ 10 ];
		w.RangeMedium = ptr[ 12 ];
		w.RangeLong = ptr[ 14 ];
		w.Tons = f32( ptr + 16 );
		w.Crits = ptr[ 20 ];
		
		w.AmmoPerTon = ((w.AmmoName != "") && (w.AmmoName != " ")) ? ptr[ 22 ] : 0;
		
		w.Price = i32( ptr + 24 );
		w.AmmoPrice = i32( ptr + 28 );
		
		ptr = hmstrcpy( tmp, ptr + 34 );
		
		w.TechLevel = ptr[ 0 ];
		
		ptr = hmstrcpy( tmp, ptr + 2 );
		w.Era = tmp;
		
		w.Defensive = ptr[ 0 ];
		w.BV = i16( ptr + 2 );
		w.AmmoBV = i16( ptr + 4 );
		
		ptr += 6;
		
		const char *name_str = w.Name.c_str();
		
		if( strstr( name_str, "Flamer" ) )
		{
			w.Damage = 0;
			w.Flamer = atoi( w.DamageStr.c_str() );
		}
		else
		{
			w.Damage = atoi( w.DamageStr.c_str() );  // FIXME: Check for x/x/x (Heavy Gauss Rifle); currently handled in BattleTechServer.
			w.Flamer = 0;
		}
		
		w.Heat = atoi( w.HeatStr.c_str() );
		
		w.ClusterWeaponSize = w.Damage ? 1 : 0;
		w.ClusterDamageGroup = 0;
		w.ClusterRollBonus = 0;
		
		if( strstr( w.DamageStr.c_str(), "/hit" ) )
		{
			for( const char *c = name_str; *c; c++ )
			{
				if( (*c >= '0') && (*c <= '9') )
				{
					w.ClusterWeaponSize = atoi( c );
					w.ClusterDamageGroup = 1;
					break;
				}
			}
		}
		
		if( strstr( name_str, "Adv." ) )
		{
			w.ClusterDamageGroup = 5;
			w.ClusterRollBonus = 2;
		}
		else if( strstr( name_str, "LRM" ) || strstr( name_str, "MRM" ) || strstr( name_str, "Rocket Launcher" ) )
			w.ClusterDamageGroup = 5;
		else if( strstr( name_str, "SRM" ) || strstr( name_str, "MML" ) )
			w.ClusterDamageGroup = 2;
		/*
		else if( strstr( name_str, "-X AC" ) )
		{
			w.ClusterWeaponSize = w.Damage;
			w.Damage = 1;
			w.ClusterDamageGroup = 1;
		}
		*/
		
		if( w.Type == HMWeapon::ULTRA_AC )
		{
			w.RapidFire = strstr( name_str, "Rotary" ) ? 6 : 2;
			w.ClusterDamageGroup = w.Damage;
		}
		else
			w.RapidFire = 1;
		
		w.HitSound = "w_hit.wav";
		w.Effect = BattleTech::Effect::NONE;
		
		w.TAG = false;
		w.Narc = false;
		
		if( w.Defensive && (w.Type != HMWeapon::MISC) )
			w.Sound = (w.Type == HMWeapon::ENERGY) ? "w_slas.wav" : "w_mg.wav";
		else if( w.Flamer )
		{
			w.Sound = "w_flamer.wav";
			w.HitSound = "w_hit_f.wav";
			w.Effect = BattleTech::Effect::FLAMER;
		}
		else if( strstr( name_str, "PPC" ) )
		{
			w.Sound = "w_ppc.wav";
			w.Effect = BattleTech::Effect::PPC;
		}
		else if( strstr( name_str, "Gauss" ) )
		{
			w.Sound = "w_gauss.wav";
			w.Effect = BattleTech::Effect::GAUSS;
		}
		else if( strstr( name_str, "TAG" ) )
		{
			w.Sound = "";
			w.HitSound = "w_lock.wav";
			w.TAG = true;
			w.Effect = BattleTech::Effect::TAG;
		}
		/*
		else if( strstr( name_str, "C3 Master" ) )
		{
			w.Sound = "";
			w.HitSound = "w_lock.wav";
			w.TAG = true;
			w.Effect = BattleTech::Effect::TAG;
			w.RangeShort = 97;
			w.RangeMedium = 98;
			w.RangeLong = 99;
		}
		*/
		else if( strstr( name_str, "Narc" ) )
		{
			w.Sound = "w_srm.wav";
			w.HitSound = "w_hit_n.wav";
			w.Narc = true;
			w.Effect = BattleTech::Effect::NARC;
		}
		else if( strstr( name_str, "Arrow" ) )
		{
			w.Sound = "w_lrm.wav";
			w.HitSound = "w_hit_m.wav";
			w.Effect = BattleTech::Effect::MISSILE;
		}
		else if( (w.Type == HMWeapon::MISSILE) || (w.Type == HMWeapon::MISSILE_3D6) || (w.Type == HMWeapon::MISSILE_STREAK) || (w.Type == HMWeapon::ONE_SHOT) )
		{
			w.Sound = strstr( name_str, "SRM" ) ? "w_srm.wav" : "w_lrm.wav";
			w.HitSound = "w_hit_m.wav";
			w.Effect = BattleTech::Effect::MISSILE;
		}
		else if( (w.Type == HMWeapon::BALLISTIC) || (w.Type == HMWeapon::ULTRA_AC) )
		{
			bool machinegun = strstr( name_str, "Machine Gun" );
			w.Sound = machinegun ? "w_mg.wav" : "w_ac.wav";
			w.HitSound = "w_hit_b.wav";
			w.Effect = machinegun ? BattleTech::Effect::MACHINEGUN : BattleTech::Effect::AUTOCANNON;
		}
		else if( w.Type == HMWeapon::ENERGY )
		{
			if( strstr( name_str, "Pulse" ) )
			{
				if( strstr( name_str, "Large" ) )
				{
					w.Sound = "w_lplas.wav";
					w.HitSound = "w_hit_l.wav";
					w.Effect = BattleTech::Effect::LARGE_PULSE;
				}
				else if( strstr( name_str, "Small" ) || strstr( name_str, "Micro" ) )
				{
					w.Sound = "w_splas.wav";
					w.Effect = BattleTech::Effect::SMALL_PULSE;
				}
				else
				{
					w.Sound = "w_mplas.wav";
					w.Effect = BattleTech::Effect::MEDIUM_PULSE;
				}
			}
			else
			{
				if( strstr( name_str, "Large" ) )
				{
					w.Sound = "w_llas.wav";
					w.Effect = BattleTech::Effect::LARGE_LASER;
				}
				else if( strstr( name_str, "Small" ) || strstr( name_str, "Micro" ) )
				{
					w.Sound = "w_slas.wav";
					w.Effect = BattleTech::Effect::SMALL_LASER;
				}
				else
				{
					w.Sound = "w_mlas.wav";
					w.Effect = BattleTech::Effect::MEDIUM_LASER;
				}
			}
		}
		
		weapons[ w.ID ] = w;
		next_wid = w.ID + 1;
	}
	
	return weapons;
}


std::string HeavyMetal::CritName( short wid, const std::map< short, HMWeapon > *weapons )
{
	if( wid == 0x00 )
		return "(Roll Again)";
	
	if( wid == 0x1AF )
		return "C3 Slave Unit";
	
	if( weapons )
	{
		if( wid > 400 )
		{
			std::map< short, HMWeapon >::const_iterator witer = weapons->find( wid - 400 );
			std::string name = (witer != weapons->end()) ? witer->second.AmmoName : "Ammo";
			return name;
		}
		
		std::map< short, HMWeapon >::const_iterator witer = weapons->find( wid );
		if( witer != weapons->end() )
		{
			std::string name = witer->second.Name;
			return name;
		}
	}
	
	if( wid == 0x01 )
		return "Shoulder";
	if( wid == 0x02 )
		return "Upper Arm Actuator";
	if( wid == 0x03 )
		return "Lower Arm Actuator";
	if( wid == 0x04 )
		return "Hand Actuator";
	if( wid == 0x05 )
		return "Hip";
	if( wid == 0x06 )
		return "Upper Leg Actuator";
	if( wid == 0x07 )
		return "Lower Leg Actuator";
	if( wid == 0x08 )
		return "Foot Actuator";
	if( wid == 0x09 )
		return "Single Heat Sink";
	if( wid == 0x0A )
		return "Double Heat Sink";
	if( wid == 0x0B )
		return "Jump Jet";
	if( wid == 0x0C )
		return "Life Support";
	if( wid == 0x0D )
		return "Sensors";
	if( wid == 0x0E )
		return "Cockpit";
	if( wid == 0x0F )
		return "Engine";
	if( wid == 0x10 )
		return "Gyro";
	if( wid == 0x11 )
		return "Hatchet";
	if( wid == 0x12 )
		return "Targeting Computer";
	if( wid == 0x13 )
		return "Turret Rotation Equipment";
	if( wid == 0x14 )
		return "(Endo Steel)";
	if( wid == 0x15 )
		return "(Ferro-Fibrous)";
	if( wid == 0x16 )
		return "(Triple-Strength Myomer)";
	if( wid == 0x17 )
		return "MASC";
	if( wid == 0x18 )
		return "Artemis IV FCS";
	if( wid == 0x19 )
		return "(CASE)";
	if( wid == 0x1A )
		return "Variable Range System";
	if( wid == 0x1B )
		return "Multi-Trac II System";
	if( wid == 0x1C )
		return "(Reactive Armor)";
	if( wid == 0x1D )
		return "(Laser Reflective Armor)";
	if( wid == 0x1E )
		return "Mechanical Jump Booster";
	if( wid == 0x1F )
		return "Sword";
	if( wid == 0x20 )
		return "Supercharger";
	if( wid == 0x21 )
		return "(Light Ferro-Fibrous)";
	if( wid == 0x22 )
		return "(Heavy Ferro-Fibrous)";
	if( wid == 0x23 )
		return "(Stealth)";
	
	if( wid == 0x25 )
		return "Double Heat Sink"; // Laser or 2xCompact
	if( wid == 0x26 )
		return "(CASE II)";
	if( wid == 0x27 )
		return "Null Signature System";
	if( wid == 0x28 )
		return "Coolant Pod";
	
	if( wid == 0x2A )
		return "Command Console";
	if( wid == 0x2B )
		return "Claw";
	if( wid == 0x2C )
		return "Mace";
	if( wid == 0x2D )
		return "Armored Cowl";
	if( wid == 0x2E )
		return "Buzzsaw";
	
	char idstr[ 8 ] = "";
	snprintf( idstr, 8, "%X", wid );
	return std::string(idstr);
}


bool HeavyMetal::HittableCrit( short wid )
{
	if( wid == 0x00 ) // Roll Again
		return false;
	if( wid == 0x14 ) // Endo Steel
		return false;
	if( wid == 0x15 ) // Ferro-Fibrous
		return false;
	if( wid == 0x16 ) // Triple-Strength Myomer
		return false;
	if( wid == 0x19 ) // CASE
		return false;
	if( wid == 0x1C ) // Reactive Armor
		return false;
	if( wid == 0x1D ) // Laser Reflective Armor
		return false;
	if( wid == 0x21 ) // Light Ferro-Fibrous
		return false;
	if( wid == 0x22 ) // Heavy Ferro-Fibrous
		return false;
	if( wid == 0x23 ) // Stealth
		return false;
	if( wid == 0x26 ) // CASE II
		return false;
	
	return true;
}


uint8_t HeavyMetal::CritSlots( short wid, bool clan, const std::map< short, HMWeapon > *weapons )
{
	if( wid == 0x00 )
		return 0;
	if( wid > 400 ) // Ammo
		return 1;
	
	if( weapons )
	{
		std::map< short, HMWeapon >::const_iterator witer = weapons->find( wid );
		if( witer != weapons->end() )
			return witer->second.Crits;
	}
	
	if( wid == 0x09 ) // Single Heat Sink
		return 1;
	if( wid == 0x0A ) // Double Heat Sink
		return (clan ? 2 : 3);
	if( wid == 0x0B ) // Jump Jet
		return 1;
	if( wid == 0x18 ) // Artemis IV FCS
		return 1;
	if( wid == 0x1E ) // Mechanical Jump Booster
		return 1;
	if( wid == 0x25 ) // Laser/Compact Heat Sink
		return (clan ? 2 : 1);
	if( wid == 0x28 ) // Coolant Pod
		return 1;
	
	return 255;
}
