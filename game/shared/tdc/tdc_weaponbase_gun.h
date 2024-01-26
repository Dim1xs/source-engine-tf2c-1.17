//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#ifndef TDC_WEAPONBASE_GUN_H
#define TDC_WEAPONBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"
#include "tdc_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTDCWeaponBaseGun C_TDCWeaponBaseGun
#endif

#define ZOOM_CONTEXT		"ZoomContext"
#define ZOOM_REZOOM_TIME	1.4f

//=============================================================================
//
// Weapon Base Melee Gun
//
class CTDCWeaponBaseGun : public CTDCWeaponBase
{
public:

	DECLARE_CLASS( CTDCWeaponBaseGun, CTDCWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTDCWeaponBaseGun();

	virtual void ItemPostFrame( void );
	virtual void ItemBusyFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );

	virtual void DoMuzzleFlash();

	void ToggleZoom( void );

	virtual void FireProjectile( void );
	virtual void GetProjectileFireSetup( CTDCPlayer *pPlayer, const Vector &vecOffset, Vector &vecSrc, Vector &vecOutDir, bool bHitTeammates = true );
	void GetProjectileReflectSetup( CTDCPlayer *pPlayer, const Vector &vecPos, Vector &vecDeflect, bool bHitTeammates = true, bool bUseHitboxes = false );

	void ImpactWaterTrace( CTDCPlayer *pPlayer, trace_t &trace, const Vector &vecStart );

	int GetAmmoPerShot( void );
	virtual float GetWeaponSpread( void );

	virtual void AddViewKick( void );
	virtual float GetProjectileDamage( void );
	virtual float GetProjectileSpeed( void );

	void HandleSoftZoom( bool bZoomIn );

	virtual void PlayWeaponShootSound( void );

	virtual Activity	GetPrimaryAttackActivity( void );

	bool AtLastShot( int iAmmoType );

private:
	CTDCWeaponBaseGun( const CTDCWeaponBaseGun & );

	bool m_bInZoom;
};

#endif // TDC_WEAPONBASE_GUN_H