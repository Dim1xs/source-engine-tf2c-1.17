//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_weaponbase_nail.h"
#include "effect_dispatch_data.h"
#include "tdc_shareddefs.h"
#include "tdc_gamerules.h"

#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#else
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "c_tdc_player.h"
#include "prediction.h"
#include "tdc_weaponbase_gun.h"
#endif

static struct TDCNailProjectileData_t
{
	const char *model;
	const char *trail;
	const char *trail_crit;

	int modelIndex;
} g_aNailData[TDC_NAIL_COUNT] =
{
	{
		"models/weapons/w_models/w_nail.mdl",
		"nailtrails_scout_%s",
		"nailtrails_scout_%s_crit",
	},
	{
		"models/weapons/w_models/w_nail.mdl",
		"nailtrails_super_%s",
		"nailtrails_super_%s_crit",
	},
};

static void PrecacheNails( void *pUser )
{
	for ( int i = 0; i < TDC_NAIL_COUNT; i++ )
	{
		g_aNailData[i].modelIndex = CBaseEntity::PrecacheModel( g_aNailData[i].model );

#ifdef GAME_DLL
		PrecacheTeamParticles( g_aNailData[i].trail, true );
		PrecacheTeamParticles( g_aNailData[i].trail_crit, true );
#endif
	}
}

PRECACHE_REGISTER_FN( PrecacheNails );

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTDCBaseNail )
	DEFINE_ENTITYFUNC( ProjectileTouch ),
	DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTDCBaseNail::CTDCBaseNail()
{
	m_bCritical = false;
	m_flDamage = 0.0f;
	m_iType = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTDCBaseNail::~CTDCBaseNail()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseNail::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseNail::Spawn( void )
{
	// Precache.
	Precache();

	BaseClass::Spawn();

	SetModelIndex( g_aNailData[m_iType].modelIndex );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	UTIL_SetSize( this, -Vector( 1.0f, 1.0f, 1.0f ), Vector( 1.0f, 1.0f, 1.0f ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetDamage( 25.0f );

	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );

	// Setup the touch and think functions.
	SetTouch( &CTDCBaseNail::ProjectileTouch );
	SetThink( &CTDCBaseNail::FlyThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTDCBaseNail::PhysicsSolidMaskForEntity( void ) const
{
	return BaseClass::PhysicsSolidMaskForEntity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseNail::ProjectileTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// pass through ladders
	if ( pTrace->surface.flags & CONTENTS_LADDER )
		return;

	if ( pOther->IsWorld() )
	{
		SetAbsVelocity( vec3_origin );
		AddSolidFlags( FSOLID_NOT_SOLID );

		// Remove immediately. Clientside projectiles will stick in the wall for a bit.
		UTIL_Remove( this );
		return;
	}

	CTakeDamageInfo info( this, GetOwnerEntity(), GetOriginalLauncher(), GetDamageForce(), GetAbsOrigin(), GetDamage(), GetDamageType() );

	Vector dir;
	AngleVectors( GetAbsAngles(), &dir );

	pOther->DispatchTraceAttack( info, dir, pNewTrace );
	ApplyMultiDamage();

	UTIL_Remove( this );
}

Vector CTDCBaseNail::GetDamageForce( void )
{
	Vector vecVelocity = GetAbsVelocity();
	VectorNormalize( vecVelocity );
	return ( vecVelocity * GetDamage() );
}

void CTDCBaseNail::FlyThink( void )
{
	QAngle angles;
	VectorAngles( GetAbsVelocity(), angles );
	SetAbsAngles( angles );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCBaseNail::GetDamageType( void ) const
{
	Assert( GetWeaponID() != WEAPON_NONE );
	int iDmgType = g_aWeaponDamageTypes[GetWeaponID()];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}
	return iDmgType;
}

#else

extern ConVar sv_gravity;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileCallback( const CEffectData &data )
{
	C_TDCPlayer *pPlayer = ToTDCPlayer( ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity ) );

	Vector vecSrc = data.m_vOrigin;
	Vector vecDir = data.m_vStart;
	float flSpeed = VectorNormalize( vecDir );

	// If we're seeing another player fire a projectile move it to the weapon's muzzle.
	// If we're in prediction then it's already taken care of.
	if ( !prediction->InPrediction() && pPlayer && !pPlayer->IsDormant() )
	{
		C_TDCWeaponBaseGun *pWeapon = dynamic_cast<C_TDCWeaponBaseGun *>( pPlayer->GetActiveWeapon() );

		if ( pWeapon )
		{
			pWeapon->GetProjectileFireSetup( pPlayer, Vector( 16, 6, -8 ), vecSrc, vecDir );
		}
	}

	Vector vecVelocity = vecDir * flSpeed;
	float flGravity = ( data.m_flMagnitude * sv_gravity.GetFloat() );
	Vector vecGravity( 0, 0, -flGravity );

	C_LocalTempEntity *pNail = tempents->ClientProjectile( vecSrc, vecVelocity, vecGravity, data.m_nMaterial, 6, pPlayer, "NailImpact" );
	if ( !pNail )
		return;

	pNail->ChangeTeam( data.m_nDamageType );
	pNail->m_nSkin = GetTeamSkin( data.m_nDamageType );

	CNewParticleEffect *pEffect = pNail->AddParticleEffect( GetParticleSystemNameFromIndex( data.m_nHitBox ) );

	if ( pEffect && data.m_bCustomColors )
	{
		pEffect->SetControlPoint( CUSTOM_COLOR_CP1, data.m_CustomColors.m_vecColor1 );
	}

	pNail->AddEffects( EF_NOSHADOW );
	pNail->flags |= FTENT_USEFASTCOLLISIONS;
}

DECLARE_CLIENT_EFFECT( "ClientProjectile_Nail", ClientsideProjectileCallback );

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCBaseNail *CTDCBaseNail::Create( const char *pszClassname,
	const Vector &vecOrigin, const Vector &vecVelocity,
	int iType, float flDamage, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	CTDCBaseNail *pProjectile = NULL;

	// Setup the initial angles.
	QAngle vecAngles;
	VectorAngles( vecVelocity, vecAngles );

	CTDCPlayer *pPlayer = ToTDCPlayer( pOwner );

#ifdef GAME_DLL
	pProjectile = static_cast<CTDCBaseNail *>( CBaseEntity::CreateNoSpawn( pszClassname, vecOrigin, vecAngles, pOwner ) );
	if ( !pProjectile )
		return NULL;

	pProjectile->m_iType = iType;
	pProjectile->SetLauncher( pWeapon );

	// Spawn.
	pProjectile->Spawn();

	pProjectile->SetAbsVelocity( vecVelocity );
	//pProjectile->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	pProjectile->SetAbsAngles( vecAngles );

	pProjectile->SetDamage( flDamage );
	pProjectile->SetGravity( 0.3f );
	pProjectile->SetCritical( bCritical );

	// Set team.
	pProjectile->ChangeTeam( pPlayer->GetTeamNumber() );

	// Hide the projectile and create a fake one on the client
	pProjectile->AddEffects( EF_NODRAW );
#endif

	const char *pszFormat = bCritical ? g_aNailData[iType].trail_crit : g_aNailData[iType].trail;
	const char *pszParticleName = ConstructTeamParticle( pszFormat, pPlayer->GetTeamNumber(), true );

	CEffectData data;
	data.m_vOrigin = vecOrigin;
	data.m_vStart = vecVelocity;
	data.m_flMagnitude = 0.3f;
	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	data.m_nDamageType = pPlayer->GetTeamNumber();
	data.m_bCustomColors = !TDCGameRules()->IsTeamplay();
	data.m_CustomColors.m_vecColor1 = pPlayer->m_vecPlayerColor;
#ifdef GAME_DLL
	data.m_nMaterial = pProjectile->GetModelIndex();
	data.m_nEntIndex = pPlayer->entindex();
#else
	data.m_nMaterial = g_aNailData[iType].modelIndex;
	data.m_hEntity = pPlayer;
#endif
	DispatchEffect( "ClientProjectile_Nail", data );

	return pProjectile;
}
