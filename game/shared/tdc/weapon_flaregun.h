//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. =======
//
// Purpose: A remake of Pyro's Flare Gun from live TF2
//
//=============================================================================
#ifndef TDC_WEAPON_FLAREGUN_H
#define TDC_WEAPON_FLAREGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponFlareGun C_WeaponFlareGun
#endif

class CWeaponFlareGun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponFlareGun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponFlareGun() {}

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_FLAREGUN; }

	virtual void FireProjectile( void );

private:
	CWeaponFlareGun( CWeaponFlareGun & );
};

#endif // TDC_WEAPON_FLAREGUN_H
