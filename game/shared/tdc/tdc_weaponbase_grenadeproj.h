//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF basic grenade projectile functionality.
//
//=============================================================================//
#ifndef TDC_WEAPONBASE_GRENADEPROJ_H
#define TDC_WEAPONBASE_GRENADEPROJ_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"
#include "tdc_baseprojectile.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTDCBaseGrenade C_TDCBaseGrenade
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTDCBaseGrenade : public CTDCBaseProjectile
{
public:
	DECLARE_CLASS( CTDCBaseGrenade, CTDCBaseProjectile );
	DECLARE_NETWORKCLASS();

							CTDCBaseGrenade();
	virtual					~CTDCBaseGrenade();
	virtual void			Spawn();
	virtual void			Precache();

	CNetworkVar( int, m_iDeflected );

protected:
	CNetworkVar( float, m_flCreationTime );
	CNetworkHandle( CBaseEntity, m_hLauncher );
	CNetworkVar( bool, m_bCritical );
	CNetworkHandle( CBaseEntity, m_hDeflectOwner );

private:
	CTDCBaseGrenade( const CTDCBaseGrenade & );

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vInitialVelocity );

	// Client specific.
#ifdef CLIENT_DLL

public:
	virtual void			OnPreDataChanged( DataUpdateType_t updateType );
	virtual void			OnDataChanged( DataUpdateType_t type );
	virtual int				DrawModel( int flags );
	virtual bool			HasTeamSkins( void ) { return true; }

protected:
	EHANDLE					m_hOldOwner;

	// Server specific.
#else

public:
	DECLARE_DATADESC();

	static CTDCBaseGrenade *Create( const char *pszClassname, const Vector &position, const QAngle &angles,
		const Vector &velocity, const AngularImpulse &angVelocity,
		float flDamage, float flRadius, bool bCritical, float flTimer,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );
	 
	virtual int				GetDamageType() const;
	int						OnTakeDamage( const CTakeDamageInfo &info );

	virtual void			DetonateThink( void );
	virtual void			Detonate( void );

	// Damage accessors.
	float					GetDamage( void ) { return m_flDamage; }
	float					GetDamageRadius( void ) { return m_flRadius; }

	void					SetDamage( float flDamage ) { m_flDamage = flDamage; }
	void					SetDamageRadius( float flDamageRadius ) { m_flRadius = flDamageRadius; }

	void					SetCritical( bool bCritical ) { m_bCritical = bCritical; }

	virtual Vector			GetBlastForce() { return vec3_origin; }

	virtual void			BounceSound( void ) {}

	virtual float			GetShakeAmplitude( void ) { return 10.0; }
	virtual float			GetShakeRadius( void ) { return 300.0; }

	void					SetupInitialTransmittedGrenadeVelocity( const Vector &velocity ) { m_vInitialVelocity = velocity; }

	bool					ShouldNotDetonate( void );
	void					RemoveGrenade( bool bBlinkOut = true );

	void					SetTimer( float flTime ) { m_flDetonateTime = flTime; }
	float					GetDetonateTime( void ){ return m_flDetonateTime; }
	void					SetDetonateTimerLength( float timer );

	void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual void			Explode( trace_t *pTrace, int bitsDamageType );

	bool					UseImpactNormal()							{ return m_bUseImpactNormal; }
	const Vector			&GetImpactNormal( void ) const				{ return m_vecImpactNormal; }

	bool					IsEnemy( CBaseEntity *pOther );

	virtual void			SetLauncher( CBaseEntity *pLauncher );

	virtual bool			IsDeflectable() { return true; }
	virtual void			Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual void			IncremenentDeflected( void );

	virtual void			BlipSound( void ) {}
	void					SetNextBlipTime( float flTime ) { m_flNextBlipTime = flTime; }

	virtual int				GetBaseProjectileType( void ) const { return TDC_PROJECTILE_BASE_GRENADE; }

protected:
	void					DrawRadius( float flRadius );

	float					m_flDamage;
	float					m_flRadius;

	bool					m_bUseImpactNormal;
	Vector					m_vecImpactNormal;

	float					m_flNextBlipTime;

private:
	// Custom collision to allow for constant elasticity on hit surfaces.
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	float					m_flDetonateTime;
	bool					m_bInSolid;

#endif
};

#endif // TDC_WEAPONBASE_GRENADEPROJ_H
