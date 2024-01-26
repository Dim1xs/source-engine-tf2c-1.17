//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#ifndef TDC_PROJECTILE_FLARE_H
#define TDC_PROJECTILE_FLARE_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CProjectile_Flare C_Projectile_Flare
#endif

class CProjectile_Flare : public CTDCBaseRocket
{
public:
	DECLARE_CLASS( CProjectile_Flare, CTDCBaseRocket );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CProjectile_Flare();
	~CProjectile_Flare();

#ifdef GAME_DLL

	static CProjectile_Flare *Create( const Vector &vecOrigin, const Vector &vecVelocity,
		float flDamage, float flRadius, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );
	virtual void	Spawn();
	virtual void	Precache();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_FLAREGUN; }

	virtual ETDCDmgCustom GetCustomDamageType( void ) const { return TDC_DMG_CUSTOM_BURNING; }
	virtual float GetSelfDamageRadius( void ) { return 0.0f; }

#else

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	void			CreateTrails( void );
	virtual bool	HasTeamSkins( void ) { return true; }

#endif
};

#endif //TDC_PROJECTILE_FLARE_H
