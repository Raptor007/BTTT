/*
 *  Mech.h
 */

#pragma once
class Mech;
class MechLocation;
class MechEquipment;
class MechLocationRoll;
class MechStep;
class MechMelee;

#include "PlatformSpecific.h"

#include "GameObject.h"
#include "BattleTechDefs.h"
#include "Variant.h"
#include "HeavyMetal.h"
#include "Animation.h"
#include "HexMap.h"
#include "RaptorGame.h"


class MechStep
{
public:
	uint8_t X, Y, Facing;
	uint8_t Cost;
	int8_t Step;
	int8_t Turn;
	uint8_t Jump;
	uint8_t Move;
};


class MechMelee
{
public:
	uint8_t Attack;
	uint8_t Limb;
	int8_t Difficulty;
	uint8_t Damage;
	uint8_t HitTable;
	
	bool operator < ( const MechMelee &other ) const;
};


class MechEquipment
{
public:
	uint16_t ID;
	std::string Name;
	MechLocation *Location;
	const HMWeapon *Weapon;
	std::set<uint8_t> WeaponArcs;
	MechEquipment *WeaponTC;
	MechEquipment *WeaponFCS;
	uint16_t ExplosionDamage;
	uint16_t Ammo;
	uint8_t Damaged;
	bool Jammed;
	uint8_t Fired;
	
	MechEquipment( void );
	MechEquipment( uint16_t eq_id, bool clan, uint16_t ammo = 0 );
	MechEquipment( Packet *packet, Mech *mech );
	virtual ~MechEquipment();
	
	void Init( uint16_t eq_id, bool clan, uint16_t ammo = 0 );
	
	virtual void AddToInitPacket( Packet *packet ) const;
	virtual void ReadFromInitPacket( Packet *packet, Mech *mech );
	
	Mech *MyMech( void ) const;
	int8_t Index( void ) const;
	bool WithinFiringArc( uint8_t x, uint8_t y ) const;
	int8_t ShotModifier( uint8_t range = 0, bool stealth = false ) const;
	uint8_t ClusterHits( uint8_t roll = 0, bool fcs = false, bool narc = false, bool ecm = false, bool indirect = false, MechEquipment *ams = NULL ) const;
	uint8_t HitsToDestroy( void ) const;
	void HitWithEvent( MechLocation *location = NULL );
	void Hit( bool directly = true, MechLocation *location = NULL );
};


class MechLocation
{
public:
	uint8_t Loc;
	std::string Name, ShortName;
	Mech *MyMech;
	MechLocation *DamageTransfer, *CritTransfer, *AttachedLimb;
	Animation Tex;
	uint16_t Structure, MaxStructure;
	uint16_t Armor, MaxArmor, RearArmor, MaxRearArmor;
	bool IsTorso, IsLeg, IsHead;
	std::vector<MechEquipment*> CriticalSlots;
	std::multiset<MechEquipment*> DamagedCriticals;
	uint8_t CASE;
	bool Narced;
	Clock HitClock;
	uint8_t HitWhere;
	bool Destroyed;
	Clock DestroyedClock;
	
	MechLocation( void );
	virtual ~MechLocation();
	
	bool IsArm( void ) const;
	uint16_t *ArmorPtr( uint8_t arc = BattleTech::Arc::FRONT );
	uint16_t ArmorAt( uint8_t arc = BattleTech::Arc::FRONT );
	uint16_t MaxArmorAt( uint8_t arc = BattleTech::Arc::FRONT );
	size_t IntactCriticalCount( uint16_t crit_id ) const;
	size_t DamagedCriticalCount( uint16_t crit_id ) const;
	std::set<MechEquipment*> Equipment( void ) const;
	void Damage( uint16_t damage, uint8_t arc = BattleTech::Arc::FRONT, uint8_t crit = 0 );
	void CriticalHitCheck( uint8_t crit = 1 );
	void CriticalHits( uint8_t hits = 1 );
};


class MechLocationRoll
{
public:
	MechLocation *Loc;
	uint8_t Table, Arc, Roll;
	bool Crit;
	
	std::string String( const char *verb = "hits" ) const;
	void Damage( uint16_t damage ) const;
};


class Mech : public GameObject
{
public:
	std::string Name, Var;
	uint8_t Tons;
	bool Clan, Quad, TSM, Stealth;
	uint8_t Walk, Jump;
	int8_t HeatDissipation;
	MechLocation Locations[ BattleTech::Loc::COUNT ];
	std::vector<MechEquipment> Equipment;
	MechEquipment *LifeSupport, *Sensors, *Cockpit, *Engine, *Gyro, *MASC, *Supercharger, *ECM;
	int8_t PilotSkill, GunnerySkill;
	uint8_t Team;
	Clock HitClock;
	
	std::vector<MechStep> Steps;
	uint8_t MoveSpeed, StandAttempts;
	std::map<uint8_t,uint8_t> WeaponsToFire;
	std::set<MechMelee> SelectedMelee;
	bool TookTurn;
	uint32_t DeclaredTarget;
	std::map<uint8_t,uint8_t> DeclaredWeapons;
	std::set<uint8_t> DeclaredMelee;
	
	uint8_t X, Y, Facing;
	int8_t TorsoTwist, ProneFire, TurnedArm;
	bool ArmsFlipped, Prone;
	
	Clock MoveClock;
	uint8_t MoveEffect;
	uint8_t FromX, FromY, FromFacing;
	int8_t FromTorsoTwist, FromProneFire, FromTurnedArm;
	bool FromArmsFlipped, FromProne;
	
	uint8_t MASCTurns, SuperchargerTurns;
	uint8_t PilotDamage, Unconscious;
	int16_t Heat, OutsideHeat;
	bool Shutdown;
	uint8_t Attack, Defense;
	uint8_t HeatMove, HeatFire;
	bool ActiveTSM, ActiveStealth;
	int8_t Spotted;
	bool Tagged;
	uint16_t PhaseDamage;
	std::queue<std::string> PSRs;
	uint8_t PSRModifier;
	std::string AutoFall;
	
	Mech( uint32_t id = 0 );
	virtual ~Mech();
	
	void ClientInit( void );
	void InitTex( void );
	
	bool PlayerShouldUpdateServer( void ) const;
	bool ServerShouldUpdatePlayer( void ) const;
	bool ServerShouldUpdateOthers( void ) const;
	bool CanCollideWithOwnType( void ) const;
	bool CanCollideWithOtherTypes( void ) const;
	bool IsMoving( void ) const;
	
	void AddToInitPacket( Packet *packet, int8_t precision = 0 );
	void ReadFromInitPacket( Packet *packet, int8_t precision = 0 );
	void AddToUpdatePacketFromServer( Packet *packet, int8_t precision = 0 );
	void ReadFromUpdatePacketFromServer( Packet *packet, int8_t precision = 0 );
	
	void SetVariant( const Variant &variant );
	
	std::string FullName( void ) const;
	std::string ShortName( void ) const;
	std::string ShortFullName( void ) const;
	
	const Player *Owner( void ) const;
	
	uint8_t WalkDist( void ) const;
	uint8_t RunDist( void ) const;
	uint8_t MASCDist( uint8_t speed = BattleTech::Speed::MASC_SUPERCHARGE ) const;
	uint8_t JumpDist( void ) const;
	std::string MPString( uint8_t speed = 0 ) const;
	
	MechLocationRoll LocationRoll( uint8_t arc = BattleTech::Arc::FRONT, uint8_t table = BattleTech::HitTable::STANDARD, bool leg_cover = false );
	MechLocation *HitLocation( uint8_t arc = BattleTech::Arc::FRONT, uint8_t roll = 0 );
	MechLocation *KickLocation( uint8_t arc = BattleTech::Arc::FRONT, uint8_t roll = 0 );
	MechLocation *PunchLocation( uint8_t arc = BattleTech::Arc::FRONT, uint8_t roll = 0 );
	
	void EngineExplode( void );
	uint8_t PSRModifiers( void ) const;
	bool PilotSkillCheck( std::string reason = "to avoid fall", int8_t modifier = 0, bool fall = true );
	void Fall( int8_t psr_modifier = 0 );
	void HitPilot( std::string reason = "", uint8_t hits = 1 );
	bool ConsciousnessRoll( bool wake = false ) const;
	
	bool Destroyed( bool force_check = false ) const;
	bool Immobile( void ) const;
	size_t DamagedCriticalCount( uint16_t crit_id ) const;
	bool Narced( void ) const;
	bool CanStand( void ) const;
	bool Ready( void ) const;
	bool ReadyAndAble( int phase = 0 ) const;
	bool ReadyAndAbleNoCache( int phase ) const;
	
	int8_t WeaponRollNeeded( const Mech *target, const ShotPath *path = NULL, const MechEquipment *eq = NULL ) const;
	bool SpottingWithoutTAG( void ) const;
	
	std::set<MechMelee> PhysicalAttacks( const Mech *target, int8_t modifier = 0 ) const;
	uint8_t PhysicalHitTable( uint8_t attack, const Mech *attacker = NULL ) const;
	
	MechEquipment *FindAmmo( uint16_t eq_id = 0 );
	uint16_t TotalAmmo( uint16_t eq_id ) const;
	bool SpendAmmo( uint16_t eq_id );
	
	uint8_t IntactEquipmentCount( uint16_t eq_id ) const;
	uint8_t ActiveProbeRange( void ) const;
	MechEquipment *AvailableAMS( uint8_t arc );
	bool FiredTAG( void ) const;
	
	void GetPosition( uint8_t *x, uint8_t *y, uint8_t *facing = NULL ) const;
	double RelativeAngle( uint8_t x, uint8_t y, int8_t twist = 0 ) const;
	uint8_t FiringArc( uint8_t x, uint8_t y, bool twist = true ) const;
	uint8_t DamageArc( uint8_t x, uint8_t y ) const;
	
	void BeginPhase( int phase );
	
	bool Step( uint8_t move );
	bool Turn( int8_t rotate );
	//bool JumpTo( uint8_t next_x, uint8_t next_y, uint8_t next_facing );
	int StepCost( void ) const;
	bool Reversed( void ) const;
	uint8_t SpeedNeeded( bool final_position = false ) const;
	uint8_t EnteredHexes( uint8_t speed = BattleTech::Speed::INVALID ) const;
	uint8_t DefenseBonus( uint8_t speed = BattleTech::Speed::INVALID, uint8_t entered = 0xFF ) const;
	uint8_t AttackPenalty( uint8_t speed = BattleTech::Speed::INVALID ) const;
	uint8_t MovementHeat( uint8_t speed = BattleTech::Speed::INVALID, uint8_t entered = 0xFF ) const;
	
	void Animate( double duration, uint8_t effect = BattleTech::Effect::NONE );
	
	void Draw( void );
	void DrawHealth( double x1, double y1, double x2, double y2 ) const;
};
