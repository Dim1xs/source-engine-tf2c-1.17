//=============================================================================
//
// Purpose: Burst Rifle
//
//=============================================================================
#ifndef TDC_WEAPON_ASSAULTRIFLE_H
#define TDC_WEAPON_ASSAULTRIFLE_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CWeaponAssaultRifle C_WeaponAssaultRifle
#endif

//=============================================================================
//
// TF Weapon Assault Rifle
//
class CWeaponAssaultRifle : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponAssaultRifle, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponAssaultRifle() {}

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_ASSAULTRIFLE; }

	virtual void	ItemPostFrame( void );
	virtual void	FireProjectile( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	WeaponReset( void );

private:
	CNetworkVar( int, m_iBurstSize );

	CWeaponAssaultRifle( const CWeaponAssaultRifle & );
};

#endif // TDC_WEAPON_ASSAULTRIFLE_H
