//=============================================================================//
//
// Purpose: Arrow used by Huntsman.
//
//=============================================================================//
#ifndef TDC_PROJECTILE_ARROW_H
#define TDC_PROJECTILE_ARROW_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_rocket.h"

#ifdef GAME_DLL
#include "SpriteTrail.h"
#endif

#ifdef CLIENT_DLL
#define CTDCProjectile_Arrow C_TDCProjectile_Arrow
#endif

class CTDCProjectile_Arrow : public CTDCBaseRocket
{
public:
	DECLARE_CLASS( CTDCProjectile_Arrow, CTDCBaseRocket );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTDCProjectile_Arrow();
	~CTDCProjectile_Arrow();

#ifdef GAME_DLL

	static CTDCProjectile_Arrow *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, float flGravity, CBaseEntity *pOwner, ProjectileType_t iType );

	virtual ETDCWeaponID	GetWeaponID( void ) const;

	virtual void	Precache( void );
	virtual void	Spawn( void );

	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	bool			CanHeadshot( void ) const;
	void			ArrowTouch( CBaseEntity *pOther );
	const char		*GetTrailParticleName( void );
	void			CreateTrail( void );

	virtual void	UpdateOnRemove( void );

#else

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateCritTrail( void );
	virtual bool	HasTeamSkins( void );

#endif

private:
#ifdef GAME_DLL
	CHandle<CSpriteTrail>		m_hSpriteTrail;
#endif
};

#ifdef CLIENT_DLL
class C_TDCStickyArrow : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_TDCStickyArrow, C_BaseAnimating );

	static C_TDCStickyArrow *Create( C_BaseAnimating *pAttachTo, int nModelIndex, int iBone, const Vector &vecPos, const QAngle &vecAngles );
};
#endif

#endif // TDC_PROJECTILE_ARROW_H
