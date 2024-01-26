//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_PARSE_H
#define TDC_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_parse.h"
#include "networkvar.h"
#include "tdc_shareddefs.h"

//=============================================================================
//
// TF Weapon Info
//
enum
{
	TDC_EXPLOSION_WALL,
	TDC_EXPLOSION_AIR,
	TDC_EXPLOSION_WATER,
	TDC_EXPLOSION_COUNT,
};

struct WeaponData_t
{
	int		m_nDamage;
	int		m_nBulletsPerShot;
	int		m_nBurstSize;
	float	m_flBurstDelay;
	float	m_flRange;
	float	m_flSpread;
	float	m_flPunchAngle;
	float	m_flTimeFireDelay;				// Time to delay between firing
	float	m_flTimeIdle;					// Time to idle after firing
	float	m_flTimeIdleEmpty;				// Time to idle after firing last bullet in clip
	float	m_flTimeReloadStart;			// Time to start into a reload (ie. shotgun)
	float	m_flTimeReloadRefill;			// Time to refill the clip during a reload.
	float	m_flTimeReload;					// Time to reload
	bool	m_bDrawCrosshair;				// Should the weapon draw a crosshair
	int		m_iAmmoPerShot;					// How much ammo each shot consumes
	float	m_flProjectileSpeed;			// Start speed for projectiles (nail, etc.); NOTE: union with something non-projectile
	float	m_flSmackDelay;					// how long after swing should damage happen for melee weapons
	float	m_flDeployTime;
	bool	m_bDropMagOnReload;				// Should we drop the remaining mag on reload.

	void Init( void )
	{
		m_nDamage = 0;
		m_nBulletsPerShot = 0;
		m_nBurstSize = 0;
		m_flBurstDelay = 0.0f;
		m_flRange = 0.0f;
		m_flSpread = 0.0f;
		m_flPunchAngle = 0.0f;
		m_flTimeFireDelay = 0.0f;
		m_flTimeIdle = 0.0f;
		m_flTimeIdleEmpty = 0.0f;
		m_flTimeReloadStart = 0.0f;
		m_flTimeReload = 0.0f;
		m_flTimeReloadRefill = 0.0f;
		m_iAmmoPerShot = 0;
		m_flProjectileSpeed = 0.0f;
		m_flSmackDelay = 0.0f;
		m_flDeployTime = 0.0f;
		m_bDropMagOnReload = false;
	};
};

class CTDCWeaponInfo : public FileWeaponInfo_t
{
public:

	DECLARE_CLASS_GAMEROOT( CTDCWeaponInfo, FileWeaponInfo_t );
	
	CTDCWeaponInfo();
	~CTDCWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	WeaponData_t const &GetWeaponData( int iWeapon ) const	{ return m_WeaponData[iWeapon]; }

public:

	WeaponData_t	m_WeaponData[2];

	float	m_flDamageRadius;
	float	m_flPrimerTime;

	// Muzzle flash
	char	m_szMuzzleFlashParticleEffect[128];

	// Tracer
	char	m_szTracerEffect[128];
	int		m_iTracerFreq;

	// Eject Brass
	bool	m_bDoInstantEjectBrass;
	char	m_szBrassModel[128];

	// Magazine
	char	m_szMagazineModel[128];

	// Weapon Icon
	char	m_szAmmoIcon[128];

	// Weapon spawner loading icon
	char	m_szTimerIconFull[128];
	char	m_szTimerIconEmpty[128];
	Color	m_cTimerIconFullColor;
	Color	m_cTimerIconEmptyColor;

	// Explosion Effect
	char	m_szExplosionSound[128];
	char	m_szExplosionEffects[TDC_EXPLOSION_COUNT][128];
	bool	m_bHasTeamColoredExplosions;
	bool	m_bHasCritExplosions;

	bool	m_bDontDrop;

	int		m_iMaxAmmo;
	int		m_iSpawnAmmo;
	bool	m_bAlwaysDrop;
	bool	m_bUseHands;
	bool	m_bCanHeadshot;
	bool	m_bUseNewActTable;
	float	m_flOptimalRange;

	char	m_szViewModelHeavy[MAX_WEAPON_STRING];
	char	m_szViewModelLight[MAX_WEAPON_STRING];
};

#endif // TDC_WEAPON_PARSE_H
