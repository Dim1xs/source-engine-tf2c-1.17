//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TDC_WEAPONBASE_ROCKET_H
#define TDC_WEAPONBASE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tdc_baseprojectile.h"
#include "tdc_shareddefs.h"

#ifdef CLIENT_DLL
#define CTDCBaseRocket C_TDCBaseRocket
#endif

//=============================================================================
//
// TF Base Rocket.
//
class CTDCBaseRocket : public CTDCBaseProjectile
{

//=============================================================================
//
// Shared (client/server).
//
public:
	DECLARE_CLASS( CTDCBaseRocket, CTDCBaseProjectile );
	DECLARE_NETWORKCLASS();

	CTDCBaseRocket();
	~CTDCBaseRocket();

	virtual void	Precache( void );
	virtual void	Spawn( void );

	CNetworkVar( int, m_iDeflected );
	CNetworkHandle( CBaseEntity, m_hLauncher );

protected:
	// Networked.
	CNetworkVector( m_vInitialVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecVelocity );
	CNetworkVar( bool, m_bCritical );

//=============================================================================
//
// Client specific.
//
#ifdef CLIENT_DLL

public:
	virtual int		DrawModel( int flags );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	PostDataUpdate( DataUpdateType_t type );
	virtual void	Simulate( void );

protected:
	EHANDLE	m_hOldOwner;

private:
	float	 m_flSpawnTime;

//=============================================================================
//
// Server specific.
//
#else

public:
	DECLARE_DATADESC();

	static CTDCBaseRocket *Create( const char *pszClassname,
		const Vector &vecOrigin, const Vector &vecVelocity,
		float flDamage, float flRadius, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );

	virtual void	RocketTouch( CBaseEntity *pOther );
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );

	void			SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual float	GetDamage( void ) { return m_flDamage; }
	virtual int		GetDamageType( void ) const;
	virtual ETDCDmgCustom GetCustomDamageType( void ) const { return TDC_DMG_CUSTOM_NONE; }
	virtual float	GetSelfDamageRadius( void );
	void			DrawRadius( float flRadius );

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	virtual ETDCWeaponID	GetWeaponID( void ) const			{ return WEAPON_ROCKETLAUNCHER; }

	virtual CBaseEntity		*GetEnemy( void )			{ return m_hEnemy; }

	virtual bool	IsDeflectable() { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual void	IncremenentDeflected( void );
	virtual void	SetLauncher( CBaseEntity *pLauncher );

	virtual int		GetBaseProjectileType( void ) const { return TDC_PROJECTILE_BASE_ROCKET; }

protected:

	void			FlyThink( void );

protected:
	// Not networked.
	float					m_flDamage;
	float					m_flRadius;

	CHandle<CBaseEntity>	m_hEnemy;

	EHANDLE					m_hScorer;
#endif
};

#endif // TDC_WEAPONBASE_ROCKET_H