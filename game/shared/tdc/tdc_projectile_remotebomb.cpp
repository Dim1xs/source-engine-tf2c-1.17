//=============================================================================
//
// Purpose: RC Bomb
//
//=============================================================================
#include "cbase.h"
#include "tdc_projectile_remotebomb.h"

#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#include "props.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_RemoteBomb, DT_Projectile_RemoteBomb )
BEGIN_NETWORK_TABLE( CProjectile_RemoteBomb, DT_Projectile_RemoteBomb )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flLiveTime ) ),
#else
	SendPropTime( SENDINFO( m_flLiveTime ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( projectile_remotebomb, CProjectile_RemoteBomb );
PRECACHE_REGISTER( projectile_remotebomb );

#define TDC_REMOTEBOMB_MODEL "models/weapons/w_models/w_rc_bomb.mdl"

CProjectile_RemoteBomb::CProjectile_RemoteBomb()
{
#ifdef GAME_DLL
	m_bTouched = false;
	m_bFizzle = false;
#else
	m_bPulsed = false;
#endif
}

CProjectile_RemoteBomb::~CProjectile_RemoteBomb()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

ETDCWeaponID CProjectile_RemoteBomb::GetWeaponID( void ) const
{
	return WEAPON_REMOTEBOMB;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CProjectile_RemoteBomb *CProjectile_RemoteBomb::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	float flDamage, float flRadius, bool bCritical, float flLiveDelay,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	CProjectile_RemoteBomb *pBomb = static_cast<CProjectile_RemoteBomb *>( CTDCBaseGrenade::Create( "projectile_remotebomb",
		position, angles, velocity, angVelocity, flDamage, flRadius, bCritical, FLT_MAX, pOwner, pWeapon ) );

	if ( pBomb )
	{
		pBomb->m_flLiveTime = gpGlobals->curtime + flLiveDelay;
	}

	return pBomb;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::Spawn( void )
{
	SetModel( TDC_REMOTEBOMB_MODEL );
	BaseClass::Spawn();

	SetTouch( &CProjectile_RemoteBomb::Hit );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::Precache( void )
{
	BaseClass::Precache();

	int iModel = PrecacheModel( TDC_REMOTEBOMB_MODEL );
	PrecacheGibsForModel( iModel );
	PrecacheTeamParticles( "stickybombtrail_%s", true );
	PrecacheTeamParticles( "remotebomb_pulse_%s", true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CProjectile_RemoteBomb::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CProjectile_RemoteBomb::ShouldTransmit( CCheckTransmitInfo *pInfo )
{
	// Always transmit to the player who fired it.
	if ( GetOwnerEntity() && pInfo->m_pClientEnt == GetOwnerEntity()->edict() )
	{
		return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::Detonate()
{
	// If we're detonating stickies then we're currently inside prediction
	// so we gotta make sure all effects show up.
	CDisablePredictionFiltering disabler;

	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	if ( m_bFizzle )
	{
		CEffectData data;
		data.m_vOrigin = GetAbsOrigin();
		data.m_vAngles = GetAbsAngles();
		data.m_nMaterial = GetModelIndex();

		DispatchEffect( "BreakModel", data );

		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::Fizzle( void )
{
	m_bFizzle = true;
	Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::Hit( CBaseEntity *pOther )
{
	if ( pOther && pOther->IsPlayer() )
	{
		Vector impactVelocity;
		GetVelocity( &impactVelocity, NULL );

		// Hard-coded values are kinda bad
		if ( impactVelocity.Length() > 400.0f )
		{
			VectorNormalize( impactVelocity );
			Vector vecDamageForce = impactVelocity * GetImpactDamage();

			CTakeDamageInfo info( this, GetOwnerEntity(), GetOriginalLauncher(), vecDamageForce, GetAbsOrigin(), GetImpactDamage(), DMG_BULLET );
			info.SetDamageCustom( TDC_DMG_REMOTEBOMB_IMPACT );

			pOther->TakeDamage( info );
		}

		SetTouch( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	m_bTouched = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CProjectile_RemoteBomb::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !m_takedamage )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();
	Assert( pAttacker );
	if ( !pAttacker )
		return 0;

	if ( m_bTouched && IsEnemy( pAttacker ) )
	{
		// Blow up when shot.
		if ( info.GetDamageType() & ( DMG_BULLET | DMG_CLUB | DMG_SLASH ) )
		{
#if 0
			m_bFizzle = true;
#else
			SetOwnerEntity( pAttacker );
			ChangeTeam( pAttacker->GetTeamNumber() );
#endif
			Detonate();
			return 1;
		}

		// Explosions push it around.
		if ( info.GetDamageType() & DMG_BLAST )
		{
			VPhysicsTakeDamage( info );
			return 1;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::UpdateOnRemove( void )
{
	CBaseEntity *pLauncher = GetOriginalLauncher();
	if ( pLauncher )
	{
		pLauncher->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::CreateTrails( void )
{
	const char *pszEffect = ConstructTeamParticle( "stickybombtrail_%s", GetTeamNumber(), true );
	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
	SetParticleColor( pEffect );

	if ( m_bCritical )
	{
		const char *pszEffectName = ConstructTeamParticle( "critical_grenade_%s", GetTeamNumber(), true );
		pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
		SetParticleColor( pEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CProjectile_RemoteBomb::OnDataChanged( DataUpdateType_t updateType )
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
void CProjectile_RemoteBomb::Simulate( void )
{
	BaseClass::Simulate();

	if ( m_bPulsed == false )
	{
		if ( gpGlobals->curtime >= m_flLiveTime )
		{
			const char *pszEffectName = ConstructTeamParticle( "remotebomb_pulse_%s", GetTeamNumber(), true );
			CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
			SetParticleColor( pEffect );

			m_bPulsed = true;
		}
	}
}
#endif
