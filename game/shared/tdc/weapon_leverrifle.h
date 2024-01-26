//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TDC_WEAPON_SNIPERRIFLE_H
#define TDC_WEAPON_SNIPERRIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CWeaponLeverRifle C_WeaponLeverRifle
#endif

//=============================================================================
//
// Sniper Rifle class.
//
class CWeaponLeverRifle : public CTDCWeaponBaseGun
{
public:

	DECLARE_CLASS( CWeaponLeverRifle, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponLeverRifle();
	~CWeaponLeverRifle();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_LEVERRIFLE; }

	virtual void Precache();

	virtual bool Deploy( void );
	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual float GetWeaponSpread( void );
	virtual bool  CanFireAccurateShot( int nBulletsPerShot );

	virtual float GetProjectileDamage( void );

	virtual void ItemPostFrame( void );
	virtual void FireProjectile( void );
	virtual bool Lower( void );
	virtual void WeaponIdle( void );
	virtual Activity GetPrimaryAttackActivity( void );

	virtual void WeaponReset( void );

	virtual bool CanHeadshot( void );
	bool CanCritHeadshot( void );

	virtual void GetTracerOrigin( Vector &vecPos );

	virtual void UpdateOnRemove( void );

#ifdef GAME_DLL
	virtual int	 UpdateTransmitState( void );
#else
	virtual CStudioHdr *OnNewModel( void );
	virtual void Simulate( void );
	virtual bool ShouldDrawCrosshair( void );
	virtual float ViewModelOffsetScale( void );

	void UpdateLaserBeam( bool bEnabled );
#endif

	bool IsZoomed( void );

private:
	void HandleZooms( void );
	void ToggleZoom( void );
	void ZoomIn( void );
	void ZoomOut( void );

protected:
	CNetworkVar( float, m_flZoomTime );

#ifdef CLIENT_DLL
	CNewParticleEffect *m_pLaserBeamEffect;
	float m_flZoomTransitionTime;
	int m_iBeamAttachment;
#endif

	CWeaponLeverRifle( const CWeaponLeverRifle & );
};

#endif // TDC_WEAPON_SNIPERRIFLE_H
