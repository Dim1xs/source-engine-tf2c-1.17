//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#ifndef TDC_WEAPON_GRENADE_PIPEBOMB_H
#define TDC_WEAPON_GRENADE_PIPEBOMB_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CProjectile_Pipebomb C_Projectile_Pipebomb
#endif

//=============================================================================
//
// TF Pipebomb Grenade
//
class CProjectile_Pipebomb : public CTDCBaseGrenade
{
public:
	DECLARE_CLASS( CProjectile_Pipebomb, CTDCBaseGrenade );
	DECLARE_NETWORKCLASS();

	CProjectile_Pipebomb();
	~CProjectile_Pipebomb();

#ifdef CLIENT_DLL

public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );

#else

public:
	DECLARE_DATADESC();

	// Creation.
	static CProjectile_Pipebomb *Create( const Vector &position, const QAngle &angles,
		const Vector &velocity, const AngularImpulse &angVelocity,
		float flDamage, float flRadius, bool bCritical, float flTimer,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );

	// Unique identifier.
	virtual ETDCWeaponID	GetWeaponID( void ) const;

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();

	virtual void	BounceSound( void );
	virtual void	Detonate();

	virtual void	PipebombTouch( CBaseEntity *pOther );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	bool			ShouldExplodeOnEntity( CBaseEntity *pOther );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	virtual CBaseEntity *GetEnemy( void ) { return m_hEnemy; }

	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

private:
	EHANDLE			m_hEnemy;
	bool			m_bTouched;

#endif
};
#endif // TDC_WEAPON_GRENADE_PIPEBOMB_H
