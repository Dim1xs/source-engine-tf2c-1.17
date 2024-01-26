//=============================================================================//
//
// Purpose: Plasma blast used by TELEMAX DISPLACER CANNON.
//
//=============================================================================//
#ifndef TDC_PROJECTILE_PLASMA_H
#define TDC_PROJECTILE_PLASMA_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
#define CProjectile_Plasma C_Projectile_Plasma
#define CProjectile_PlasmaBomblet C_Projectile_PlasmaBomblet
#endif

class CProjectile_Plasma : public CTDCBaseRocket
{
public:
	DECLARE_CLASS( CProjectile_Plasma, CTDCBaseRocket );
	DECLARE_NETWORKCLASS();

	CProjectile_Plasma();
	~CProjectile_Plasma();

#ifdef GAME_DLL

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_DISPLACER; }

	static CProjectile_Plasma *Create( const Vector &vecOrigin, const Vector &vecVelocity,
		float flDamage, float flRadius, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );
	virtual void	Spawn();
	virtual void	Precache();

	// Overrides.
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	virtual bool	IsDeflectable( void ) { return false; }

	virtual void	StopLoopingSounds( void );

#else

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );

#endif
};


#ifdef CLIENT_DLL
#define CProjectile_PlasmaBomb C_Projectile_PlasmaBomb
#endif

class CProjectile_PlasmaBomb : public CTDCBaseRocket
{
public:
	DECLARE_CLASS( CProjectile_PlasmaBomb, CTDCBaseRocket );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	static CProjectile_PlasmaBomb *Create( const Vector &vecOrigin, const Vector &vecVelocity,
		float flDamage, float flRadius, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_DISPLACER; }

	virtual void	Spawn( void );
	virtual void	Precache();

	virtual bool	IsDeflectable( void ) { return false; }
	virtual float	GetSelfDamageRadius( void ) { return 0.0f; }

	virtual int		GetDamageType( void ) const;
	virtual ETDCDmgCustom GetCustomDamageType( void ) const { return TDC_DMG_DISPLACER_BOMB; }

#else

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );

#endif
};

#endif // TDC_PROJECTILE_PLASMA_H