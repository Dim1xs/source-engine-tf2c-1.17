//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tdc_projectile_nail.h"


//=============================================================================
//
// TF Syringe Projectile functions (Server specific).
//
LINK_ENTITY_TO_CLASS( projectile_nail, CProjectile_Syringe );
PRECACHE_REGISTER( projectile_nail );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_Syringe *CProjectile_Syringe::Create( const Vector &vecOrigin, const Vector &vecVelocity,
	int iType, float flDamage, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CProjectile_Syringe *>( CTDCBaseNail::Create( "projectile_nail", vecOrigin, vecVelocity, iType, flDamage, bCritical, pOwner, pWeapon ) );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ETDCWeaponID CProjectile_Syringe::GetWeaponID( void ) const
{
	if ( m_iType == TDC_NAIL_SUPER )
		return WEAPON_SUPERNAILGUN;

	return WEAPON_NAILGUN;
}
#endif
