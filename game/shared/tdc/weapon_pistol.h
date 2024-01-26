//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TDC_WEAPON_PISTOL_H
#define TDC_WEAPON_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CWeaponPistol C_WeaponPistol
#endif

//=============================================================================
//
// TF Weapon Pistol.
//
class CWeaponPistol : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponPistol, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponPistol() {}

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_PISTOL; }

private:
	CWeaponPistol( const CWeaponPistol & );
};

#endif // TDC_WEAPON_PISTOL_H