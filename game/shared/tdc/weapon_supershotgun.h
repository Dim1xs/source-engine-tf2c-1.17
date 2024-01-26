//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_SUPERSHOTGUN_H
#define TDC_WEAPON_SUPERSHOTGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponSuperShotgun C_WeaponSuperShotgun
#endif

class CWeaponSuperShotgun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponSuperShotgun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponSuperShotgun();

	virtual ETDCWeaponID GetWeaponID() const { return WEAPON_SUPERSHOTGUN; }
	virtual const Vector2D *GetSpreadPattern( int &iNumBullets );

	DECLARE_ACTTABLE();
};

#endif // TDC_WEAPON_SUPERSHOTGUN_H
