//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_WEAPON_HAMMERFISTS_H
#define TDC_WEAPON_HAMMERFISTS_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CWeaponHammerFists C_WeaponHammerFists
#endif

//=============================================================================
//
// Fists weapon class.
//
class CWeaponHammerFists : public CTDCWeaponBaseMelee
{
public:
	DECLARE_CLASS( CWeaponHammerFists, CTDCWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID GetWeaponID( void ) const { return WEAPON_HAMMERFISTS; }
	
	virtual bool CanHolster( void ) const;
	virtual bool ForceWeaponSwitch( void ) const;
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();

	virtual void SendPlayerAnimEvent( CTDCPlayer *pPlayer );
	virtual void DoViewModelAnimation( void );

	void Punch( void );
};

#endif // TDC_WEAPON_HAMMERFISTS_H
