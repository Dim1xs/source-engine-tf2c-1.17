//=============================================================================
//
// Purpose: "Weapon" for testing cubemaps
//
//=============================================================================
#ifndef TDC_WEAPON_CUBEMAP_H
#define TDC_WEAPON_CUBEMAP_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase.h"

#ifdef CLIENT_DLL
#define CWeaponCubemap C_WeaponCubemap
#endif

class CWeaponCubemap : public CTDCWeaponBase
{
public:
	DECLARE_CLASS( CWeaponCubemap, CTDCWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID GetWeaponID( void ) const { return WEAPON_CUBEMAP; }

	virtual void PrimaryAttack( void ) { /* Do nothing */ }
	virtual void SecondaryAttack( void ) { /* Do nothing */ }
};

#endif // TDC_WEAPON_CUBEMAP_H
