//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_weaponbase_rocket.h"
#include "tdc_gamerules.h"

// Server specific.
#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tdc_fx.h"
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif


//=============================================================================
//
// TF Base Rocket tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TDCBaseRocket, DT_TDCBaseRocket )

BEGIN_NETWORK_TABLE( CTDCBaseRocket, DT_TDCBaseRocket )
	// Client specific.
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),

	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

	RecvPropInt( RECVINFO( m_iDeflected ) ),
	RecvPropEHandle( RECVINFO( m_hLauncher ) ),

	RecvPropVector( RECVINFO( m_vecVelocity ), 0, RecvProxy_LocalVelocity ),

	RecvPropBool( RECVINFO( m_bCritical ) ),

	// Server specific.
#else
	SendPropVector( SENDINFO( m_vInitialVelocity ), 12 /*nbits*/, 0 /*flags*/, -3500.0f /*low value*/, 3500.0f /*high value*/ ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_INTEGRAL | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles( SENDINFO( m_angRotation ), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),

	SendPropInt( SENDINFO( m_iDeflected ), 4, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hLauncher ) ),

	SendPropVector( SENDINFO( m_vecVelocity ), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),

	SendPropBool( SENDINFO( m_bCritical ) ),
#endif
END_NETWORK_TABLE()

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTDCBaseRocket )
	DEFINE_ENTITYFUNC( RocketTouch ),
	DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()
#endif

#ifdef GAME_DLL
ConVar tdc_rocket_show_radius( "tdc_rocket_show_radius", "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render rocket radius." );
ConVar tdc_homing_rockets( "tdc_homing_rockets", "0", FCVAR_CHEAT, "What is \"Rocket + x = Death\"?" );
ConVar tdc_homing_deflected_rockets( "tdc_homing_deflected_rockets", "0", FCVAR_CHEAT, "Homing Crit Rockets 2: Back with Vengeance" );
ConVar tdc_bouncing_rockets( "tdc_bouncing_rockets", "0", FCVAR_CHEAT, "ROCKET STORM!" );
#endif

//=============================================================================
//
// Shared (client/server) functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTDCBaseRocket::CTDCBaseRocket()
{
	m_vInitialVelocity.Init();
	m_iDeflected = 0;
	m_hLauncher = NULL;

	// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = 0.0f;

	// Server specific.
#else

	m_flDamage = 0.0f;
	m_flRadius = 0.0f;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTDCBaseRocket::~CTDCBaseRocket()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::Spawn( void )
{
	// Precache.
	Precache();

	BaseClass::Spawn();

	// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = gpGlobals->curtime;

	// Server specific.
#else

	//Derived classes must have set model.
	Assert( GetModel() );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );

	SetCollisionGroup( TDC_COLLISION_GROUP_ROCKETS );

	UTIL_SetSize( this, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetGravity( 0.0f );

	// Setup the touch and think functions.
	SetTouch( &CTDCBaseRocket::RocketTouch );
	SetThink( &CTDCBaseRocket::FlyThink );
	SetNextThink( gpGlobals->curtime );

#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_hOldOwner = GetOwnerEntity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::PostDataUpdate( DataUpdateType_t type )
{
	// Pass through to the base class.
	BaseClass::PostDataUpdate( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity and angles into the interpolation history.
		CInterpolatedVar<Vector> &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();

		CInterpolatedVar<QAngle> &rotInterpolator = GetRotationInterpolator();
		rotInterpolator.ClearHistory();

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( flChangeTime - 1.0f, &vCurOrigin, false );

		QAngle vCurAngles = GetLocalAngles();
		rotInterpolator.AddToHead( flChangeTime - 1.0f, &vCurAngles, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( flChangeTime, &vCurOrigin, false );

		rotInterpolator.AddToHead( flChangeTime - 1.0, &vCurAngles, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCBaseRocket::DrawModel( int flags )
{
	// During the first 0.2 seconds of our life, don't draw ourselves.
	if ( gpGlobals->curtime - m_flSpawnTime < 0.2f )
		return 0;

	return BaseClass::DrawModel( flags );
}

void CTDCBaseRocket::Simulate( void )
{
	// Make sure the rocket is facing movement direction.
	if ( GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		QAngle angForward;
		VectorAngles( GetAbsVelocity(), angForward );
		SetAbsAngles( angForward );
	}

	BaseClass::Simulate();
}

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCBaseRocket *CTDCBaseRocket::Create( const char *pszClassname,
	const Vector &vecOrigin, const Vector &vecVelocity,
	float flDamage, float flRadius, bool bCritical,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	CTDCBaseRocket *pRocket = static_cast<CTDCBaseRocket*>( CBaseEntity::CreateNoSpawn( pszClassname, vecOrigin, vec3_angle, pOwner ) );
	if ( !pRocket )
		return NULL;

	// Set firing weapon.
	pRocket->SetLauncher( pWeapon );

	// Spawn.
	DispatchSpawn( pRocket );

	// Setup the initial velocity.
	pRocket->SetAbsVelocity( vecVelocity );
	pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pRocket->SetAbsAngles( angles );

	// Set team.
	pRocket->ChangeTeam( pOwner->GetTeamNumber() );

	pRocket->m_flDamage = flDamage;
	pRocket->m_flRadius = flRadius;
	pRocket->m_bCritical = bCritical;

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

	if ( tdc_bouncing_rockets.GetBool() && !pOther->IsCombatCharacter() )
	{
		Vector vecDir = GetAbsVelocity();
		float flDot = DotProduct( vecDir, pTrace->plane.normal );
		Vector vecReflect = vecDir - 2.0f * flDot * pTrace->plane.normal;
		SetAbsVelocity( vecReflect );

		VectorNormalize( vecReflect );

		QAngle vecAngles;
		VectorAngles( vecReflect, vecAngles );
		SetAbsAngles( vecAngles );

		return;
	}

	// Handle hitting skybox (disappear).
	if ( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	trace_t trace;
	memcpy( &trace, pTrace, sizeof( trace_t ) );
	Explode( &trace, pOther );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTDCBaseRocket::PhysicsSolidMaskForEntity( void ) const
{
	int teamContents = 0;

	if ( !TDCGameRules()->IsTeamplay() )
	{
		teamContents = CONTENTS_REDTEAM;
	}
	else if ( !CanCollideWithTeammates() )
	{
		// Only collide with the other team

		switch ( GetTeamNumber() )
		{
		case TDC_TEAM_RED:
			teamContents = CONTENTS_BLUETEAM;
			break;

		case TDC_TEAM_BLUE:
			teamContents = CONTENTS_REDTEAM;
			break;
		}
	}
	else
	{
		// Collide with all teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CBaseEntity *pAttacker = GetOwnerEntity();

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	bool bCrit = ( GetDamageType() & DMG_CRITICAL ) != 0;
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pOther->entindex(), ToBasePlayer( pAttacker ), GetTeamNumber(), bCrit, GetCustomDamageType() );
	CSoundEnt::InsertSound( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	CTDCRadiusDamageInfo radiusInfo;
	radiusInfo.m_DmgInfo.Set( this, pAttacker, GetOriginalLauncher(), vec3_origin, vecOrigin, GetDamage(), GetDamageType(), GetCustomDamageType() );
	radiusInfo.m_vecSrc = vecOrigin;
	radiusInfo.m_flRadius = m_flRadius;
	radiusInfo.m_flSelfDamageRadius = GetSelfDamageRadius();
	radiusInfo.m_iWeaponID = GetWeaponID();

	TDCGameRules()->RadiusDamage( radiusInfo );

	// Debug!
	if ( tdc_rocket_show_radius.GetBool() )
	{
		DrawRadius( m_flRadius );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Remove the rocket.
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTDCBaseRocket::GetDamageType( void ) const
{
	int iDmgType = g_aWeaponDamageTypes[GetWeaponID()];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTDCBaseRocket::GetSelfDamageRadius( void )
{
	// Original rocket radius?
	return 121.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBaseRocket::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = 10.0f;
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for ( angle = 0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for ( angle = 0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for ( angle = 0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseRocket::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	// Get rocket's speed.
	float flSpeed = GetAbsVelocity().Length();

	QAngle angForward;
	VectorAngles( vecDir, angForward );

	// Now change rocket's direction.
	SetAbsAngles( angForward );
	SetAbsVelocity( vecDir * flSpeed );

	// And change owner.
	IncremenentDeflected();
	SetOwnerEntity( pDeflectedBy );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );

	CBaseCombatCharacter *pBCC = pDeflectedBy->MyCombatCharacterPointer();
	
	if ( pBCC )
	{
		SetLauncher( pBCC->GetActiveWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increment deflects counter
//-----------------------------------------------------------------------------
void CTDCBaseRocket::IncremenentDeflected( void )
{
	m_iDeflected++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBaseRocket::SetLauncher( CBaseEntity *pLauncher )
{
	m_hLauncher = pLauncher;

	BaseClass::SetLauncher( pLauncher );
}

void CTDCBaseRocket::FlyThink( void )
{
	if ( tdc_homing_rockets.GetBool() || ( tdc_homing_deflected_rockets.GetBool() && m_iDeflected ) )
	{
		// Find the closest visible enemy player.
		CUtlVector<CTDCPlayer *> vecPlayers;
		int count = CollectPlayers( &vecPlayers, TEAM_ANY, true );
		float flClosest = FLT_MAX;
		Vector vecClosestTarget = vec3_origin;

		for ( int i = 0; i < count; i++ )
		{
			CTDCPlayer *pPlayer = vecPlayers[i];
			if ( pPlayer == GetOwnerEntity() )
				continue;

			if ( !pPlayer->IsEnemy( this ) )
				continue;

			Vector vecTarget = pPlayer->EyePosition();

			if ( FVisible( vecTarget, MASK_VISIBLE ) )
			{
				float flDistSqr = ( vecTarget - GetAbsOrigin() ).LengthSqr();
				if ( flDistSqr < flClosest )
				{
					flClosest = flDistSqr;
					vecClosestTarget = vecTarget;
				}
			}
		}

		// Head towards him.
		if ( vecClosestTarget != vec3_origin )
		{
			Vector vecTarget = vecClosestTarget;
			Vector vecDir = vecTarget - GetAbsOrigin();
			VectorNormalize( vecDir );

			float flSpeed = GetAbsVelocity().Length();
			QAngle angForward;
			VectorAngles( vecDir, angForward );
			SetAbsAngles( angForward );
			SetAbsVelocity( vecDir * flSpeed );
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

#endif
