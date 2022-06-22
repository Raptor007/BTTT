/*
 *  HexMap.h
 */

#pragma once
class Hex;
class HexTouch;
class ShotPath;
class HexMap;

#include "PlatformSpecific.h"

#include "GameObject.h"
#include "Mech.h"


class Hex
{
public:
	int X, Y;
	int8_t Height;
	uint8_t Forest;
	
	Pos3D Center( void ) const;
	virtual ~Hex();
	
	int CostToEnter( const Hex *from = NULL, bool reverse = false ) const;
};


class HexTouch
{
public:
	const Hex *HexPtr;
	uint8_t Forest;
	uint8_t Cover;
	
	HexTouch( const Hex *hex, uint8_t forest, uint8_t cover );
	virtual ~HexTouch();
	bool operator < ( const HexTouch &other ) const;
};


class ShotPath : public std::vector<const Hex*>
{
public:
	int Distance;
	int Modifier;
	bool LineOfSight;
	bool PartialCover;
	bool LegWeaponsBlocked;
	std::map<uint32_t,uint8_t> ECM;
	
	ShotPath( void );
	virtual ~ShotPath();
	void Clear( void );
	bool ECMvsTeam( uint8_t team ) const;
};


class HexMap : public GameObject
{
public:
	uint32_t Seed;
	std::vector<Vec3D> DirVec;
	std::vector< std::vector<Hex> > Hexes;
	
	HexMap( uint32_t id = 0 );
	virtual ~HexMap();
	
	void SetSize( size_t w, size_t h );
	void Randomize( uint32_t seed = time(NULL) % 9999 + 1 );
	
	void ClientInit( void );
	
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
	
	bool Valid( int x, int y ) const;
	Pos3D Center( int x, int y ) const;
	Vec3D Direction( int dir ) const;
	Vec3D Direction( double dir ) const;
	Hex *Nearest( float xf, float yf );
	std::map<uint8_t,Hex*> Adjacent( int x, int y );
	std::map<uint8_t,const Hex*> Adjacent_const( int x, int y ) const;
	int CostToEnter( int x, int y, const Hex *from = NULL, bool reverse = false ) const;
	int HexDist( int x1, int y1, int x2, int y2 ) const;
	double FloatDist( int x1, int y1, int x2, int y2 ) const;
	bool LineCrosses( int ax, int ay, int bx, int by, int hx, int hy ) const;
	ShotPath Path( int x1, int y1, int x2, int y2, int8_t h1 = 0, int8_t h2 = 0 ) const;
	int JumpCost( int x1, int y1, int x2, int y2 ) const;
	
	Mech *MechAt( int x, int y );
	const Mech *MechAt_const( int x, int y ) const;
	size_t MechsAt( int x, int y ) const;
	size_t TeamsAt( int x, int y ) const;
	std::set<const Mech*> MechECMsAt( int x, int y ) const;
	
	void Draw( void );
	void DrawHexOutline( int x, int y, float thickness, float r, float g, float b, float a = 0.9f ) const;
};
