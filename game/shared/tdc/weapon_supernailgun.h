//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TDC_WEAPON_SUPERNAILGUN_H
#define TDC_WEAPON_SUPERNAILGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponSuperNailGun C_WeaponSuperNailGun
#endif

class CWeaponSuperNailGun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponSuperNailGun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID GetWeaponID() const { return WEAPON_SUPERNAILGUN; }

	virtual void FireProjectile( void );
};

#endif // TDC_WEAPON_SUPERNAILGUN_H
