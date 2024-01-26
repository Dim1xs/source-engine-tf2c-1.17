//=============================================================================
//
// Purpose:
//
//=============================================================================
#ifndef TDC_WEAPON_HUNTINGSHOTGUN_H
#define TDC_WEAPON_HUNTINGSHOTGUN_H

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponHuntingShotgun C_WeaponHuntingShotgun
#endif

class CWeaponHuntingShotgun : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponHuntingShotgun, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponHuntingShotgun();

	virtual ETDCWeaponID GetWeaponID( void ) const { return WEAPON_HUNTINGSHOTGUN; }
	virtual const Vector2D *GetSpreadPattern( int &iNumBullets );
};

#endif // TDC_WEAPON_HUNTINGSHOTGUN_H
