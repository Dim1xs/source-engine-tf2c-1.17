//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_projectile_flare.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tdc_player.h"
#include "particles_new.h"
#else
#include "tdc_player.h"
#include "tdc_fx.h"
#endif

#define TDC_WEAPON_FLARE_MODEL "models/weapons/w_models/w_flaregun_shell.mdl"

BEGIN_DATADESC( CProjectile_Flare )
END_DATADESC()

LINK_ENTITY_TO_CLASS( projectile_flare, CProjectile_Flare );
PRECACHE_REGISTER( projectile_flare );

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_Flare, DT_Projectile_Flare )
BEGIN_NETWORK_TABLE( CProjectile_Flare, DT_Projectile_Flare )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CProjectile_Flare::CProjectile_Flare()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CProjectile_Flare::~CProjectile_Flare()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Flare::Precache()
{
	PrecacheModel( TDC_WEAPON_FLARE_MODEL );

	PrecacheTeamParticles( "flaregun_trail_%s", true );
	PrecacheTeamParticles( "flaregun_trail_crit_%s", true );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CProjectile_Flare::Spawn()
{
	SetModel( TDC_WEAPON_FLARE_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( 0.3f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_Flare *CProjectile_Flare::Create( const Vector &vecOrigin, const Vector &vecVelocity,
	float flDamage, float flRadius, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CProjectile_Flare *>( CTDCBaseRocket::Create( "projectile_flare", vecOrigin, vecVelocity, flDamage, flRadius, bCritical, pOwner, pWeapon ) );
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Flare::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();		
	}
	// Watch owner changes and change trail accordingly.
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Flare::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	const char *pszFormat = m_bCritical ? "flaregun_trail_crit_%s" : "flaregun_trail_%s";
	const char *pszEffectName = ConstructTeamParticle( pszFormat, GetTeamNumber(), true );

	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	SetParticleColor( pEffect );
}

#endif