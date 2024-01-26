//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tdc_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tdc_player.h"
// Server specific.
#else
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tdc_player.h"
#include "func_break.h"
#include "func_nogrenades.h"
#include "Sprite.h"
#include "tdc_fx.h"
#include "tdc_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF Grenade projectile tables.
//

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTDCBaseGrenade )
	DEFINE_THINKFUNC( DetonateThink ),
END_DATADESC()

ConVar tdc_grenade_show_radius( "tdc_grenade_show_radius", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Render radius of grenades" );
ConVar tdc_grenade_show_radius_time( "tdc_grenade_show_radius_time", "5.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to show grenade radius" );
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TDCBaseGrenade, DT_TDCBaseGrenade )

BEGIN_NETWORK_TABLE( CTDCBaseGrenade, DT_TDCBaseGrenade )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flCreationTime ) ),
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropEHandle( RECVINFO( m_hLauncher ) ),

	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

	RecvPropInt( RECVINFO( m_iDeflected ) ),
	RecvPropEHandle( RECVINFO( m_hDeflectOwner ) ),
#else
	SendPropTime( SENDINFO( m_flCreationTime ) ),
	SendPropVector( SENDINFO( m_vInitialVelocity ), 20 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/ ),
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropEHandle( SENDINFO( m_hLauncher ) ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_INTEGRAL | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles( SENDINFO( m_angRotation ), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),

	SendPropInt( SENDINFO( m_iDeflected ), 4, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hDeflectOwner ) ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTDCBaseGrenade::CTDCBaseGrenade()
{
	m_iDeflected = 0;

#ifndef CLIENT_DLL
	m_flDamage = 0.0f;
	m_flRadius = 100.0f;

	m_flNextBlipTime = 0.0f;
	m_flDetonateTime = 0.0f;

	m_bUseImpactNormal = false;
	m_vecImpactNormal.Init();

	m_bInSolid = false;
#endif

	SetSimulatedEveryTick( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTDCBaseGrenade::~CTDCBaseGrenade()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::Spawn( void )
{
	// Base class spawn.
	BaseClass::Spawn();

#ifdef GAME_DLL
	m_flCreationTime = gpGlobals->curtime;

	// So it will collide with physics props!
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	SetSolid( SOLID_BBOX );

	AddEffects( EF_NOSHADOW );

	// Set the grenade size here.
	UTIL_SetSize( this, Vector( -2.0f, -2.0f, -2.0f ), Vector( 2.0f, 2.0f, 2.0f ) );

	// Set the movement type.
	SetCollisionGroup( TDC_COLLISIONGROUP_GRENADES );

	VPhysicsInitNormal( SOLID_BBOX, 0, false );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	if ( GetOwnerEntity() )
	{
		// Set the team.
		ChangeTeam( GetOwnerEntity()->GetTeamNumber() );
	}

	// Setup the think and touch functions (see CBaseEntity).
	SetThink( &CTDCBaseGrenade::DetonateThink );
	SetNextThink( gpGlobals->curtime + 0.1 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheModel( NOGRENADE_SPRITE );
	PrecacheTeamParticles( "critical_grenade_%s", true );
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
void CTDCBaseGrenade::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_hOldOwner = GetOwnerEntity();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();

		interpolator.ClearHistory();
		float changeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( changeTime - 1.0, &vCurOrigin, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( changeTime, &vCurOrigin, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTDCBaseGrenade::DrawModel( int flags )
{
	if ( gpGlobals->curtime < ( m_flCreationTime + 0.1 ) )
		return 0;

	return BaseClass::DrawModel( flags );
}

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCBaseGrenade *CTDCBaseGrenade::Create( const char *pszClassname, const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	float flDamage, float flRadius, bool bCritical, float flTimer,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	CTDCBaseGrenade *pGrenade = static_cast<CTDCBaseGrenade *>( CBaseEntity::CreateNoSpawn( pszClassname, position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->SetLauncher( pWeapon );

		DispatchSpawn( pGrenade );

		pGrenade->SetupInitialTransmittedGrenadeVelocity( velocity );

		pGrenade->SetGravity( 0.4f/*BaseClass::GetGrenadeGravity()*/ );
		pGrenade->SetFriction( 0.2f/*BaseClass::GetGrenadeFriction()*/ );
		pGrenade->SetElasticity( 0.45f/*BaseClass::GetGrenadeElasticity()*/ );

		pGrenade->ChangeTeam( pOwner->GetTeamNumber() );

		IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &velocity, &angVelocity );
		}

		pGrenade->m_flDamage = flDamage;
		pGrenade->m_flRadius = flRadius;
		pGrenade->m_bCritical = bCritical;

		pGrenade->SetDetonateTimerLength( flTimer );
	}

	return pGrenade;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::Explode( trace_t *pTrace, int bitsDamageType )
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	// Explosion effect on client
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	CTDCPlayer *pTFAttacker = ToTDCPlayer( GetOwnerEntity() );
	int iEntIndex = ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() ) ? pTrace->m_pEnt->entindex() : -1;
	const Vector &vecNormal = UseImpactNormal() ? GetImpactNormal() : pTrace->plane.normal;

	TE_TFExplosion( filter, 0.0f, vecOrigin, vecNormal, GetWeaponID(), iEntIndex, pTFAttacker, GetTeamNumber(), m_bCritical );

	// Use the thrower's position as the reported position
	Vector vecReported = GetOwnerEntity() ? GetOwnerEntity()->GetAbsOrigin() : vec3_origin;

	float flRadius = GetDamageRadius();

	if ( tdc_grenade_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	CTDCRadiusDamageInfo radiusInfo;
	radiusInfo.m_DmgInfo.Set( this, GetOwnerEntity(), GetOriginalLauncher(), GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );
	radiusInfo.m_vecSrc = vecOrigin;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_flSelfDamageRadius = 146.0f;
	radiusInfo.m_iWeaponID = GetWeaponID();

	TDCGameRules()->RadiusDamage( radiusInfo );

	// Don't decal players with scorch.
	if ( pTrace->m_pEnt && !pTrace->m_pEnt->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	RemoveGrenade( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTDCBaseGrenade::GetDamageType() const
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
int CTDCBaseGrenade::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo info2 = info;

	// Reduce explosion damage so that we don't get knocked too far
	if ( info.GetDamageType() & DMG_BLAST )
	{
		info2.ScaleDamageForce( 0.05 );
	}

	return BaseClass::OnTakeDamage( info2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::DetonateThink( void )
{
	if ( !IsInWorld() )
	{
		Remove( );
		return;
	}
	
	BlipSound();

	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		Detonate();
		return;
	}

	StudioFrameAdvance();
	DispatchAnimEvents( this );

	SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::Detonate( void )
{
	// Putting this here just in case we're inside prediction to make sure all effects show up.
	CDisablePredictionFiltering disabler;

	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode( &tr, GetDamageType() );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the time at which the grenade will explode.
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::SetDetonateTimerLength( float timer )
{
	m_flDetonateTime = gpGlobals->curtime + timer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	//Assume all surfaces have the same elasticity
	float flSurfaceElasticity = 1.0;

	//Don't bounce off of players with perfect elasticity
	if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
	{
		flSurfaceElasticity = 0.3;
	}

#if 0
	// if its breakable glass and we kill it, don't bounce.
	// give some damage to the glass, and if it breaks, pass 
	// through it.
	bool breakthrough = false;

	if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable" ) )
	{
		breakthrough = true;
	}

	if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable_surf" ) )
	{
		breakthrough = true;
	}

	if (breakthrough)
	{
		CTakeDamageInfo info( this, this, 10, DMG_CLUB );
		trace.m_pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

		ApplyMultiDamage();

		if( trace.m_pEnt->m_iHealth <= 0 )
		{
			// slow our flight a little bit
			Vector vel = GetAbsVelocity();

			vel *= 0.4;

			SetAbsVelocity( vel );
			return;
		}
	}
#endif

	float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
	flTotalElasticity = clamp( flTotalElasticity, 0.0f, 0.9f );

	// NOTE: A backoff of 2.0f is a reflection
	Vector vecAbsVelocity;
	PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
	vecAbsVelocity *= flTotalElasticity;

	// Get the total velocity (player + conveyors, etc.)
	VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
	float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

	// Stop if on ground.
	if ( trace.plane.normal.z > 0.7f )			// Floor
	{
		// Verify that we have an entity.
		CBaseEntity *pEntity = trace.m_pEnt;
		Assert( pEntity );

		SetAbsVelocity( vecAbsVelocity );

		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			if ( pEntity->IsStandable() )
			{
				SetGroundEntity( pEntity );
			}

			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );

			//align to the ground so we're not standing on end
			QAngle angle;
			VectorAngles( trace.plane.normal, angle );

			// rotate randomly in yaw
			angle[1] = random->RandomFloat( 0, 360 );

			// TFTODO: rotate around trace.plane.normal

			SetAbsAngles( angle );			
		}
		else
		{
			Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;	
			Vector vecBaseDir = GetBaseVelocity();
			VectorNormalize( vecBaseDir );
			float flScale = vecDelta.Dot( vecBaseDir );

			VectorScale( vecAbsVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, vecVelocity ); 
			VectorMA( vecVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
			PhysicsPushEntity( vecVelocity, &trace );
		}
	}
	else
	{
		// If we get *too* slow, we'll stick without ever coming to rest because
		// we'll get pushed down by gravity faster than we can escape from the wall.
		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );
		}
		else
		{
			SetAbsVelocity( vecAbsVelocity );
		}
	}

	BounceSound();
}

bool CTDCBaseGrenade::ShouldNotDetonate( void )
{
	return InNoGrenadeZone( this );
}

void CTDCBaseGrenade::RemoveGrenade( bool bBlinkOut )
{
	if ( bBlinkOut )
	{
		// Sprite flash
		CSprite *pGlowSprite = CSprite::SpriteCreate( NOGRENADE_SPRITE, GetAbsOrigin(), false );
		if ( pGlowSprite )
		{
			pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
			pGlowSprite->SetThink( &CSprite::SUB_Remove );
			pGlowSprite->SetNextThink( gpGlobals->curtime + 1.0 );
		}
	}

	// Kill it
	SetTouch( NULL );
	SetThink( NULL );
	AddEffects( EF_NODRAW );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCBaseGrenade::IsEnemy( CBaseEntity *pOther )
{
	if ( pOther == GetOwnerEntity() )
		return false;

	if ( !pOther->IsPlayer() )
		return false;

	if ( !TDCGameRules()->IsTeamplay() )
		return true;

	return ( pOther->GetTeamNumber() != GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::SetLauncher( CBaseEntity *pLauncher )
{
	m_hLauncher = pLauncher;
	BaseClass::SetLauncher( pLauncher );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		Vector vecOldVelocity, vecVelocity;

		pPhysicsObject->GetVelocity( &vecOldVelocity, NULL );

		float flSpeed = vecOldVelocity.Length();
		vecVelocity = vecDir * flSpeed;
		AngularImpulse angVelocity( ( 600, random->RandomInt( -1200, 1200 ), 0 ) );

		// Now change grenade's direction.
		pPhysicsObject->SetVelocityInstantaneous( &vecVelocity, &angVelocity );
	}

	CBaseCombatCharacter *pBCC = pDeflectedBy->MyCombatCharacterPointer();

	IncremenentDeflected();
	m_hDeflectOwner = pDeflectedBy;
	SetOwnerEntity( pBCC );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );

	if ( pBCC )
	{
		SetLauncher( pBCC->GetActiveWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increment deflects counter
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::IncremenentDeflected( void )
{
	m_iDeflected++;
}

//-----------------------------------------------------------------------------
// Purpose: This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
//			Always ignores other grenade projectiles.
//-----------------------------------------------------------------------------
class CTraceFilterCollisionGrenades : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGrenades );

	CTraceFilterCollisionGrenades( const IHandleEntity *passentity )
		: m_pPassEnt(passentity)
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

		if ( pEntity )
		{
			if ( pEntity->GetCollisionGroup() == TDC_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TDC_COLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_NONE )
				return false;
			if ( pEntity->GetCollisionGroup() == TDC_COLLISION_GROUP_RESPAWNROOMS )
				return false;
		}

		return true;
	}

protected:
	const IHandleEntity *m_pPassEnt;
};

//-----------------------------------------------------------------------------
// Purpose: Grenades aren't solid to players, so players don't get stuck on
//			them when they're lying on the ground. We still want thrown grenades
//			to bounce of players though, so manually trace ahead and see if we'd
//			hit something that we'd like the grenade to "collide" with.
//-----------------------------------------------------------------------------
void CTDCBaseGrenade::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );

	Vector start = GetAbsOrigin();

	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGrenades filter( this );
	trace_t tr;

	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );

	bool bHitEnemy = ( tr.m_pEnt && IsEnemy( tr.m_pEnt ) );

	if ( tr.startsolid )
	{
		if ( m_bInSolid == false && ( CanCollideWithTeammates() == true || bHitEnemy == true ) )
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -0.2f; // bounce backwards
			pPhysics->SetVelocity( &vel, NULL );
		}
		m_bInSolid = true;
		return;
	}

	m_bInSolid = false;

	if ( tr.DidHit() )
	{
		Touch( tr.m_pEnt );
		
		if ( CanCollideWithTeammates() == true || bHitEnemy == true )
		{
			// reflect velocity around normal
			vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;

			// absorb 80% in impact
			vel *= GetElasticity();

			if ( bHitEnemy == true )
			{
				vel *= 0.5f;
			}

			angVel *= -0.5f;
			pPhysics->SetVelocity( &vel, &angVel );
		}
	}
}


void CTDCBaseGrenade::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = tdc_grenade_show_radius_time.GetFloat();
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}


#endif
