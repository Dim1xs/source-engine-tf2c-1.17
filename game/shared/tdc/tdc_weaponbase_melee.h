//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Melee 
//
//=============================================================================

#ifndef TDC_WEAPONBASE_MELEE_H
#define TDC_WEAPONBASE_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"
#include "tdc_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTDCWeaponBaseMelee C_TDCWeaponBaseMelee
#endif

//=============================================================================
//
// Weapon Base Melee Class
//
class CTDCWeaponBaseMelee : public CTDCWeaponBase
{
public:
	DECLARE_CLASS( CTDCWeaponBaseMelee, CTDCWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTDCWeaponBaseMelee();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool	HasPrimaryAmmo() { return true; }
	virtual bool	CanBeSelected() { return true; }
	virtual void	ItemPostFrame();
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	DoImpactEffect( trace_t &tr, int nDamageType );
	virtual bool	ShouldDrawCrosshair( void ) { return true; }
	virtual void	WeaponReset( void );

	virtual void	DoViewModelAnimation( void );

	bool			DoSwingTrace( trace_t &tr );
	virtual void	Smack( void );

	virtual float	GetMeleeDamage( CBaseEntity *pTarget, ETDCDmgCustom &iCustomDamage );
	virtual float	GetSwingRange( void );
	virtual float	GetFireRate( void );

	// Call when we hit an entity. Use for special weapon effects on hit.
	virtual void	OnEntityHit( CBaseEntity *pEntity );

	virtual void	SendPlayerAnimEvent( CTDCPlayer *pPlayer );

protected:
	void			Swing( CTDCPlayer *pPlayer );
	virtual void	UpdateFatigue( void );

protected:
	CNetworkVar( float, m_flSmackTime );
	CNetworkVar( float, m_flSwingCountReset );
	CNetworkVar( int, m_iSwingCount );

private:
	CTDCWeaponBaseMelee( const CTDCWeaponBaseMelee & ) {}
};

#endif // TDC_WEAPONBASE_MELEE_H
