
//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket
//
//=============================================================================
#include "cbase.h"
#include "tdc_projectile_rocket.h"
#include "tdc_player.h"

//=============================================================================
//
// TF Rocket functions (Server specific).
//
#define ROCKET_MODEL "models/weapons/w_models/w_rocket.mdl"

LINK_ENTITY_TO_CLASS( projectile_rocket, CProjectile_Rocket );
PRECACHE_REGISTER( projectile_rocket );

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_Rocket, DT_Projectile_Rocket )
BEGIN_NETWORK_TABLE( CProjectile_Rocket, DT_Projectile_Rocket )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_Rocket *CProjectile_Rocket::Create( const Vector &vecOrigin, const Vector &vecVelocity,
	float flDamage, float flRadius, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CProjectile_Rocket *>( CTDCBaseRocket::Create( "projectile_rocket", vecOrigin, vecVelocity, flDamage, flRadius, bCritical, pOwner, pWeapon ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Rocket::Spawn()
{
	SetModel( ROCKET_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Rocket::Precache()
{
	PrecacheModel( ROCKET_MODEL );

	PrecacheTeamParticles( "critical_rocket_%s", true );
	PrecacheParticleSystem( "rockettrail" );
	PrecacheParticleSystem( "rockettrail_dm" );
	PrecacheParticleSystem( "rockettrail_underwater" );

	BaseClass::Precache();
}
