//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_STENGUN_H
#define TDC_WEAPON_STENGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponStenGun C_WeaponStenGun
#endif

class CWeaponStenGun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponStenGun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID GetWeaponID() const { return WEAPON_STENGUN; }
};

#endif // TDC_WEAPON_STENGUN_H
