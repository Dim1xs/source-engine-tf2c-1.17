//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TDC_WEAPON_GRENADELAUNCHER_H
#define TDC_WEAPON_GRENADELAUNCHER_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CWeaponGrenadeLauncher C_WeaponGrenadeLauncher
#endif

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CWeaponGrenadeLauncher : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponGrenadeLauncher, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponGrenadeLauncher();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_GRENADELAUNCHER; }

	virtual void FireProjectile( void );

#ifdef CLIENT_DLL
	virtual void EjectBrass( C_BaseAnimating *pEntity, int iAttachment = -1 );
#endif

private:
	CWeaponGrenadeLauncher( const CWeaponGrenadeLauncher & ) {}
};

#endif // TDC_WEAPON_GRENADELAUNCHER_H
