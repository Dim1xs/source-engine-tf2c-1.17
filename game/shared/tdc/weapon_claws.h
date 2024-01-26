//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_CLAWS_H
#define TDC_WEAPON_CLAWS_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CWeaponClaws C_WeaponClaws
#endif

class CWeaponClaws : public CTDCWeaponBaseMelee
{
public:
	DECLARE_CLASS( CWeaponClaws, CTDCWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_CLAWS; }

	virtual void		WeaponReset( void );
	virtual void		ItemPostFrame( void );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void		PrimaryAttack( void );
	virtual void		SecondaryAttack( void );
	
	void				Pounce( void );
	void				StopCharging( void );

	virtual bool		HasEffectMeter( void ) { return true; }
	virtual float		GetEffectBarProgress( void );
	virtual const char	*GetEffectLabelText( void ) { return "#TDC_Pounce"; }

private:
	CNetworkVar( float, m_flChargeBeginTime );

	EHANDLE m_hBackstabVictim;
};

#endif // TDC_WEAPON_CLAWS_H
