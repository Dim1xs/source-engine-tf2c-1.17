//=============================================================================//
//
// Purpose: Plasma blast used by TELEMAX DISPLACER CANNON.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_projectile_plasma.h"

#define TDC_WEAPON_PLASMA_MODEL "models/weapons/w_models/w_displacer_projectile1.mdl"

#ifdef GAME_DLL
ConVar tdc_debug_plasma( "tdc_debug_plasma", "0", FCVAR_CHEAT );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_Plasma, DT_Projectile_Plasma );
BEGIN_NETWORK_TABLE( CProjectile_Plasma, DT_Projectile_Plasma )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( projectile_plasma, CProjectile_Plasma );
PRECACHE_REGISTER( projectile_plasma );

CProjectile_Plasma::CProjectile_Plasma()
{
}

CProjectile_Plasma::~CProjectile_Plasma()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_Plasma *CProjectile_Plasma::Create( const Vector &vecOrigin, const Vector &vecVelocity,
	float flDamage, float flRadius, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CProjectile_Plasma *>( CTDCBaseRocket::Create( "projectile_plasma", vecOrigin, vecVelocity, flDamage, flRadius, bCritical, pOwner, pWeapon ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Plasma::Spawn()
{
	// Setting a model here because baseclass wants it.
	SetModel( TDC_WEAPON_PLASMA_MODEL );
	BaseClass::Spawn();

	EmitSound( "Weapon_Displacer.Energy" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Plasma::Precache()
{
	PrecacheModel( TDC_WEAPON_PLASMA_MODEL );
	PrecacheScriptSound( "Weapon_Displacer.Energy" );

	PrecacheTeamParticles( "displacer_trail_primary_%s", true );
	PrecacheTeamParticles( "displacer_primary_%s_crit", true );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Plasma::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	BaseClass::Explode( pTrace, pOther );

	// Create the bomblets.
	const int nBombs = 20;
	const float flStep = 360.0f / (float)nBombs;

	for ( int i = 0; i < nBombs; i++ )
	{
		// Get direction and apply deviation
		Vector vecDir;
		QAngle localAngles( RandomFloat( -20.f, -70.f ), (float)i * flStep + RandomFloat( -flStep / 2, flStep / 2 ), 0.0f );
		AngleVectors( localAngles, &vecDir );

		// Correct the rotation
		VMatrix matAdjust;
		MatrixBuildRotation( matAdjust, Vector( 0, 0, 1 ), pTrace->plane.normal );
		vecDir = matAdjust * vecDir;

		// Pull it out a bit so it doesn't collide with other bomblets.
		Vector vecOrigin = GetAbsOrigin() + vecDir * 1.0f;
		Vector vecVelocity = vecDir * RandomFloat( 250, 750 );

		CProjectile_PlasmaBomb *pBomb = CProjectile_PlasmaBomb::Create( vecOrigin, vecVelocity, m_flDamage / 3, m_flRadius, m_bCritical, GetOwnerEntity(), GetOriginalLauncher() );

		if ( tdc_debug_plasma.GetBool() )
		{
			NDebugOverlay::Line( pBomb->GetAbsOrigin(), pBomb->GetAbsOrigin() + vecDir * 50.0f, 0, 255, 0, true, 10.0f );
		}

		pBomb->SetDamage( GetDamage() / 3 );
		pBomb->SetCritical( m_bCritical );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Plasma::StopLoopingSounds( void )
{
	StopSound( "Weapon_Displacer.Energy" );
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Plasma::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Plasma::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	const char *pszEffect = ConstructTeamParticle( m_bCritical ? "displacer_trail_primary_%s_crit" : "displacer_trail_primary_%s", GetTeamNumber(), true );
	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	SetParticleColor( pEffect );
}

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_PlasmaBomb, DT_Projectile_PlasmaBomb );
BEGIN_NETWORK_TABLE( CProjectile_PlasmaBomb, DT_Projectile_PlasmaBomb )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( projectile_plasma_bomb, CProjectile_PlasmaBomb );
PRECACHE_REGISTER( projectile_plasma_bomb );

#define TDC_WEAPON_PLASMA_BOMB_MODEL "models/weapons/w_models/w_displacer_projectile2.mdl"

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_PlasmaBomb *CProjectile_PlasmaBomb::Create( const Vector &vecOrigin, const Vector &vecVelocity,
	float flDamage, float flRadius, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CProjectile_PlasmaBomb *>( CTDCBaseRocket::Create( "projectile_plasma_bomb", vecOrigin, vecVelocity, flDamage, flRadius, bCritical, pOwner, pWeapon ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_PlasmaBomb::Precache()
{
	PrecacheModel( TDC_WEAPON_PLASMA_BOMB_MODEL );

	PrecacheTeamParticles( "displacer_trail_secondary_%s", true );
	PrecacheTeamParticles( "displacer_trail_secondary_%s_crit", true );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_PlasmaBomb::Spawn( void )
{
	SetModel( TDC_WEAPON_PLASMA_BOMB_MODEL );
	BaseClass::Spawn();
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CProjectile_PlasmaBomb::GetDamageType( void ) const
{
	// No range damage modifier.
	return BaseClass::GetDamageType() & ~DMG_USEDISTANCEMOD;
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_PlasmaBomb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_PlasmaBomb::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	const char *pszEffect = ConstructTeamParticle( m_bCritical ? "displacer_trail_secondary_%s_crit" : "displacer_trail_secondary_%s", GetTeamNumber(), true );
	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	SetParticleColor( pEffect );
}

#endif
