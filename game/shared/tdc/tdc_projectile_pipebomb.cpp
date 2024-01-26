//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_projectile_pipebomb.h"

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_Pipebomb, DT_Projectile_Pipebomb )

BEGIN_NETWORK_TABLE( CProjectile_Pipebomb, DT_Projectile_Pipebomb )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( projectile_pipe, CProjectile_Pipebomb );
PRECACHE_REGISTER( projectile_pipe );

#ifdef GAME_DLL
static string_t s_iszTrainName;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CProjectile_Pipebomb::CProjectile_Pipebomb()
{
#ifdef GAME_DLL
	m_bTouched = false;
	s_iszTrainName = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CProjectile_Pipebomb::~CProjectile_Pipebomb()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Client specific).
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::CreateTrails( void )
{
	const char *pszEffect = ConstructTeamParticle( "pipebombtrail_%s", GetTeamNumber(), true );
	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	SetParticleColor( pEffect );

	if ( m_bCritical )
	{
		const char *pszEffectName = ConstructTeamParticle( "critical_pipe_%s", GetTeamNumber(), true );
		pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
		SetParticleColor( pEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
	}
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
}

#else

//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Server specific).
//
#define TDC_WEAPON_PIPEGRENADE_MODEL		"models/weapons/w_models/w_grenade_china_lake.mdl"
#define TDC_WEAPON_PIPEBOMB_BOUNCE_SOUND	"Weapon_Grenade_Pipebomb.Bounce"
#define TDC_WEAPON_GRENADE_DETONATE_TIME 2.0f

BEGIN_DATADESC( CProjectile_Pipebomb )
	DEFINE_ENTITYFUNC( PipebombTouch ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_Pipebomb *CProjectile_Pipebomb::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	float flDamage, float flRadius, bool bCritical, float flTimer,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CProjectile_Pipebomb *>( CTDCBaseGrenade::Create( "projectile_pipe",
		position, angles, velocity, angVelocity, flDamage, flRadius, bCritical, flTimer, pOwner, pWeapon ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ETDCWeaponID CProjectile_Pipebomb::GetWeaponID( void ) const
{
	return WEAPON_GRENADELAUNCHER;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::Spawn()
{
	SetModel( TDC_WEAPON_PIPEGRENADE_MODEL );
	SetTouch( &CProjectile_Pipebomb::PipebombTouch );

	BaseClass::Spawn();

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::Precache()
{
	PrecacheModel( TDC_WEAPON_PIPEGRENADE_MODEL );

	PrecacheTeamParticles( "pipebombtrail_%s", true );
	PrecacheTeamParticles( "critical_pipe_%s", true );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::BounceSound( void )
{
	EmitSound( TDC_WEAPON_PIPEBOMB_BOUNCE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::PipebombTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Blow up if we hit an enemy we can damage
	if ( ShouldExplodeOnEntity( pOther ) )
	{
		// Save this entity as enemy, they will take 100% damage.
		m_hEnemy = pOther;
		Explode( &pTrace, GetDamageType() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	// Remember that it collided with something so it no longer explodes on contact.
	m_bTouched = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CProjectile_Pipebomb::ShouldExplodeOnEntity( CBaseEntity *pOther )
{
	// Train hack!
	if ( pOther->GetModelName() == s_iszTrainName && ( pOther->GetAbsVelocity().LengthSqr() > 1.0f ) )
		return true;

	if ( pOther->m_takedamage == DAMAGE_NO )
		return false;

	if ( pOther == GetOwnerEntity() )
	{
		// Don't explode on owner until the first bounce.
		if ( !m_bTouched )
			return false;
	}
	else if ( !IsEnemy( pOther ) )
	{
		// Don't explode on teammates for a short time after being fired.
		if ( !CanCollideWithTeammates() )
			return false;
	}

	// Explode on players.
	return pOther->IsPlayer();
}

//-----------------------------------------------------------------------------
// Purpose: If we are shot after being stuck to the world, move a bit, unless we're a sticky, in which case, fizzle out and die.
//-----------------------------------------------------------------------------
int CProjectile_Pipebomb::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !m_takedamage )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();

	Assert( pAttacker );

	if ( !pAttacker )
		return 0;

	if ( m_bTouched &&
		( info.GetDamageType() & ( DMG_BULLET | DMG_BLAST | DMG_CLUB | DMG_SLASH ) ) &&
		IsEnemy( info.GetAttacker() ) )
	{
		Vector vecForce = info.GetDamageForce();

		if ( info.GetDamageType() & DMG_BUCKSHOT )
		{
			vecForce *= 0.5f;
		}
		else if ( info.GetDamageType() & DMG_BULLET )
		{
			vecForce *= 0.8f;
		}
		else if ( info.GetDamageType() & DMG_BLAST )
		{
			vecForce *= 0.08f;
		}

		// If the force is sufficient, detach & move the pipebomb
		if ( vecForce.IsLengthGreaterThan( 1500.0f ) )
		{
			if ( VPhysicsGetObject() )
			{
				VPhysicsGetObject()->EnableMotion( true );
			}

			CTakeDamageInfo newInfo = info;
			newInfo.SetDamageForce( vecForce );

			VPhysicsTakeDamage( newInfo );
			m_bTouched = false;

			// It has moved the data is no longer valid.
			m_bUseImpactNormal = false;
			m_vecImpactNormal.Init();

			return 1;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_Pipebomb::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BaseClass::Deflected( pDeflectedBy, vecDir );
}

#endif
