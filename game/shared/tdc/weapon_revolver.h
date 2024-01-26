//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_SIXSHOOTER_H
#define TDC_WEAPON_SIXSHOOTER_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponRevolver C_WeaponRevolver
#endif

class CWeaponRevolver : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponRevolver, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID GetWeaponID() const { return WEAPON_REVOLVER; }
};

#endif // TDC_WEAPON_SIXSHOOTER_H
