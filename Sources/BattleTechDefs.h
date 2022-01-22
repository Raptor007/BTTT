/*
 *  BattleTechDefs.h
 */

#pragma once

#include "PlatformSpecific.h"
#include "RaptorDefs.h"


namespace BattleTech
{
	namespace State
	{
		enum
		{
			SETUP = Raptor::State::GAME_SPECIFIC,
			INITIATIVE,
			MOVEMENT,
			TAG,
			WEAPON_ATTACK,
			PHYSICAL_ATTACK,
			HEAT,
			END,
			GAME_OVER
		};
	}
	
	namespace Packet
	{
		enum
		{
			EVENTS      = 'Evnt',
			TEAM_TURN   = 'Turn',
			CHANGE_MAP  = 'Map ',
			SPAWN_MECH  = 'Spwn',
			REMOVE_MECH = 'Remv',
			READY       = 'RedE',
			MOVEMENT    = 'Move',
			STAND       = 'Stnd',
			SPOT        = 'Spot',
			SHOTS       = 'Shot',
			MELEE       = 'Mele',
			ATTENTION   = 'Attn'
		};
	}
	
	namespace Object
	{
		enum
		{
			MAP  = 'Map ',
			MECH = 'Mech',
			LIMB = 'Limb'
		};
	}
	
	namespace Effect
	{
		enum
		{
			NONE = 0,
			ATTENTION,
			BLINK,
			STEP,
			TURN,
			JUMP,
			FALL,
			STAND,
			TORSO_TWIST,
			EXPLOSION,
			SMOKE,
			FIRST_WEAPON,
			LARGE_LASER = FIRST_WEAPON,
			LARGE_PULSE,
			MEDIUM_LASER,
			MEDIUM_PULSE,
			SMALL_LASER,
			SMALL_PULSE,
			FLAMER,
			PPC,
			GAUSS,
			AUTOCANNON,
			MACHINEGUN,
			MISSILE,
			TAG,
			NARC,
			LAST_WEAPON = NARC,
			FIRST_MELEE,
			LEFT_PUNCH = FIRST_MELEE,
			RIGHT_PUNCH,
			LEFT_KICK,
			RIGHT_KICK,
			LAST_MELEE = RIGHT_KICK
		};
	}
	
	namespace RGB332
	{
		enum
		{
			BLACK      = 0x00,
			WHITE      = 0xFF,
			GREY       = 0x6E,
			DARK_GREY  = 0x49,
			RED        = 0xE0,
			GREEN      = 0x1C,
			BLUE       = 0x03,
			DARK_RED   = 0x60,
			DARK_GREEN = 0x0C,
			DARK_BLUE  = 0x02,
			CYAN       = 0x1F,
			YELLOW     = 0xFC,
			ORANGE     = 0xEC,
			VIOLET     = 0xE3,
			PURPLE     = 0x62
		};
	}

	namespace Stat
	{
		enum
		{
			NONE = 0,
			EQUIPMENT,
			AMMO,
			SHUTDOWN,
			PILOT,
			HEAT,
			HEAT_MASC_SC,
			HEAT_ADD,
			HEATSINKS,
			ACTIVE_SPECIAL,
			NARC
		};
	}
	
	namespace Dir
	{
		enum
		{
			UP = 0,
			UP_RIGHT,
			DOWN_RIGHT,
			DOWN,
			DOWN_LEFT,
			UP_LEFT
		};
	}
	
	namespace Move
	{
		enum
		{
			INVALID = 0,
			STOP,
			WALK,
			REVERSE,
			RUN,
			MASC,
			SUPERCHARGE,
			MASC_SUPERCHARGE,
			JUMP,
			LEFT,
			RIGHT
		};
	}
	
	namespace Loc
	{
		enum
		{
			UNKNOWN = -1,
			LEFT_ARM = 0,
			LEFT_TORSO,
			LEFT_LEG,
			RIGHT_ARM,
			RIGHT_TORSO,
			RIGHT_LEG,
			HEAD,
			CENTER_TORSO,
			COUNT,
			LEFT_TORSO_REAR = COUNT,
			RIGHT_TORSO_REAR,
			CENTER_TORSO_REAR
		};
	}
	
	namespace Arc
	{
		enum
		{
			FRONT = 0,
			REAR,
			LEFT_SIDE,
			RIGHT_SIDE,
			STRUCTURE
		};
	}
	
	namespace HitTable
	{
		enum
		{
			NONE = 0,
			KICK,
			PUNCH,
			STANDARD
		};
	}
	
	namespace Equipment
	{
		enum
		{
			SHOULDER           = 0x01,
			UPPER_ARM_ACTUATOR = 0x02,
			LOWER_ARM_ACTUATOR = 0x03,
			HAND_ACTUATOR      = 0x04,
			HIP                = 0x05,
			UPPER_LEG_ACTUATOR = 0x06,
			LOWER_LEG_ACTUATOR = 0x07,
			FOOT_ACTUATOR      = 0x08,
			SINGLE_HEAT_SINK   = 0x09,
			DOUBLE_HEAT_SINK   = 0x0A,
			JUMP_JET           = 0x0B,
			LIFE_SUPPORT       = 0x0C,
			SENSORS            = 0x0D,
			COCKPIT            = 0x0E,
			ENGINE             = 0x0F,
			GYRO               = 0x10,
			HATCHET            = 0x11,
			TARGETING_COMPUTER = 0x12,
			MASC               = 0x17,
			ARTEMIS_IV_FCS     = 0x18,
			CASE               = 0x19,
			JUMP_BOOSTER       = 0x1E,
			SWORD              = 0x1F,
			SUPERCHARGER       = 0x20,
			LASER_HEAT_SINK    = 0x25,
			CASE_II            = 0x26,
			COOLANT_POD        = 0x28
		};
	}
	
	namespace Melee
	{
		enum
		{
			NONE = 0,
			KICK,
			PUNCH,
			CLUB,
			PUSH,
			CHARGE,
			DFA,
			HATCHET = BattleTech::Equipment::HATCHET,
			SWORD   = BattleTech::Equipment::SWORD
		};
	}
}
