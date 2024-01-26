//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TDC_WEAPON_NAILGUN_H
#define TDC_WEAPON_NAILGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponNailGun C_WeaponNailGun
#endif

class CWeaponNailGun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponNailGun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID GetWeaponID() const { return WEAPON_NAILGUN; }

	virtual void FireProjectile( void );
};

#endif // TDC_WEAPON_NAILGUN_H
