//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#ifndef TDC_WEAPON_ROCKETLAUNCHER_H
#define TDC_WEAPON_ROCKETLAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CWeaponRocketLauncher C_WeaponRocketLauncher
#endif

//=============================================================================
//
// TF Weapon Rocket Launcher.
//
class CWeaponRocketLauncher : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponRocketLauncher, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponRocketLauncher();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_ROCKETLAUNCHER; }
	
	virtual void FireProjectile( void );

private:
	CWeaponRocketLauncher( const CWeaponRocketLauncher & );
};

#endif // TDC_WEAPON_ROCKETLAUNCHER_H