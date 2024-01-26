//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Projectile
//
//=============================================================================
#ifndef TDC_PROJECTILE_ROCKET_H
#define TDC_PROJECTILE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_rocket.h"


//=============================================================================
//
// Generic rocket.
//
class CProjectile_Rocket : public CTDCBaseRocket
{
public:
	DECLARE_CLASS( CProjectile_Rocket, CTDCBaseRocket );
	DECLARE_NETWORKCLASS();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_ROCKETLAUNCHER; }

	// Creation.
	static CProjectile_Rocket *Create( const Vector &vecOrigin, const Vector &vecVelocity,
		float flDamage, float flRadius, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );
	virtual void Spawn();
	virtual void Precache();
};

#endif	//TDC_PROJECTILE_ROCKET_H
