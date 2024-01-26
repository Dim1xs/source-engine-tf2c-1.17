//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Base Projectile
//
//=============================================================================
#ifndef TDC_WEAPONBASE_NAIL_H
#define TDC_WEAPONBASE_NAIL_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_baseprojectile.h"
#include "tdc_shareddefs.h"

enum
{
	TDC_NAIL_NORMAL,
	TDC_NAIL_SUPER,
	TDC_NAIL_COUNT,
};

#ifdef CLIENT_DLL
#define CTDCBaseNail C_TDCBaseNail
#endif

//=============================================================================
//
// Nail projectile
//
class CTDCBaseNail : public CTDCBaseProjectile
{
public:
	DECLARE_CLASS( CTDCBaseNail, CTDCBaseProjectile );

protected:
	static CTDCBaseNail *Create( const char *pszClassname,
		const Vector &vecOrigin, const Vector &vecVelocity,
		int iType, float flDamage, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );

#ifdef GAME_DLL
public:
	DECLARE_DATADESC();

	CTDCBaseNail();
	~CTDCBaseNail();

	void			Precache( void );
	void			Spawn( void );
	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual int		GetBaseProjectileType( void ) const { return TDC_PROJECTILE_BASE_NAIL; }

	bool			IsCritical( void )				{ return m_bCritical; }
	void			SetCritical( bool bCritical )	{ m_bCritical = bCritical; }

	void			ProjectileTouch( CBaseEntity *pOther );

	float			GetDamage() { return m_flDamage; }
	void			SetDamage( float flDamage ) { m_flDamage = flDamage; }

	Vector			GetDamageForce( void );
	virtual int		GetDamageType( void ) const;

private:
	void			FlyThink( void );

protected:
	bool			m_bCritical;
	float			m_flDamage;
	int				m_iType;
#endif
};

#endif // TDC_WEAPONBASE_NAIL_H
