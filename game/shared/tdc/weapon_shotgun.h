//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_SHOTGUN_H
#define TDC_WEAPON_SHOTGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CWeaponShotgun C_WeaponShotgun
#endif

//=============================================================================
//
// Shotgun class.
//
class CWeaponShotgun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponShotgun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponShotgun();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_SHOTGUN; }

private:
	CWeaponShotgun( const CWeaponShotgun & ) {}
};

#endif // TDC_WEAPON_SHOTGUN_H
