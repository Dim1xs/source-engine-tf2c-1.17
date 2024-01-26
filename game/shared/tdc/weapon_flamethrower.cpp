//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Flame Thrower
//
//=============================================================================
#include "cbase.h"
#include "weapon_flamethrower.h"
#include "tdc_fx_shared.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "tdc_gamerules.h"
#include "tdc_player_shared.h"
#include "tdc_lagcompensation.h"

#if defined( CLIENT_DLL )

	#include "c_tdc_player.h"
	#include "vstdlib/random.h"
	#include "engine/IEngineSound.h"
	#include "soundenvelope.h"
	#include "iefx.h"

#else

	#include "explode.h"
	#include "tdc_player.h"
	#include "tdc_gamerules.h"
	#include "tdc_gamestats.h"
	#include "collisionutils.h"
	#include "tdc_team.h"
	#include "particle_parse.h"

	ConVar	tdc_debug_flamethrower("tdc_debug_flamethrower", "0", FCVAR_CHEAT, "Visualize the flamethrower damage." );
	ConVar  tdc_flamethrower_velocity( "tdc_flamethrower_velocity", "2300.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Initial velocity of flame damage entities." );
	ConVar	tdc_flamethrower_drag("tdc_flamethrower_drag", "0.89", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Air drag of flame damage entities." );
	ConVar	tdc_flamethrower_float("tdc_flamethrower_float", "50.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Upward float velocity of flame damage entities." );
	ConVar  tdc_flamethrower_flametime("tdc_flamethrower_flametime", "0.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to live of flame damage entities." );
	ConVar  tdc_flamethrower_vecrand("tdc_flamethrower_vecrand", "0.05", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Random vector added to initial velocity of flame damage entities." );
	ConVar  tdc_flamethrower_boxsize("tdc_flamethrower_boxsize", "8.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Size of flame damage entities." );
	ConVar  tdc_flamethrower_maxdamagedist("tdc_flamethrower_maxdamagedist", "350.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Maximum damage distance for flamethrower." );
	ConVar  tdc_flamethrower_shortrangedamagemultiplier("tdc_flamethrower_shortrangedamagemultiplier", "1.2", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Damage multiplier for close-in flamethrower damage." );
	ConVar  tdc_flamethrower_velocityfadestart("tdc_flamethrower_velocityfadestart", ".3", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time at which attacker's velocity contribution starts to fade." );
	ConVar  tdc_flamethrower_velocityfadeend("tdc_flamethrower_velocityfadeend", ".5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time at which attacker's velocity contribution finishes fading." );
	//ConVar  tf_flame_force( "tf_flame_force", "30" );
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// position of end of muzzle relative to shoot position
#define TDC_FLAMETHROWER_MUZZLEPOS_FORWARD		70.0f
#define TDC_FLAMETHROWER_MUZZLEPOS_RIGHT			12.0f
#define TDC_FLAMETHROWER_MUZZLEPOS_UP			-12.0f

#ifdef CLIENT_DLL
	extern ConVar tdc_muzzlelight;
#endif

ConVar  tdc_airblast( "tdc_airblast", "1", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Enable/Disable the Airblast function of the Flamethrower." );
ConVar  tdc_airblast_players( "tdc_airblast_players", "1", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Enable/Disable the Airblast pushing players." );
#ifdef GAME_DLL
ConVar	tdc_debug_airblast( "tdc_debug_airblast", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Visualize airblast box." );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFlamethrower, DT_WeaponFlameThrower )

BEGIN_NETWORK_TABLE( CWeaponFlamethrower, DT_WeaponFlameThrower )
#if defined( CLIENT_DLL )
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropBool( RECVINFO( m_bCritFire ) ),
	RecvPropFloat( RECVINFO( m_flAmmoUseRemainder ) ),
	RecvPropBool( RECVINFO( m_bHitTarget ) )
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bCritFire ) ),
	SendPropFloat( SENDINFO( m_flAmmoUseRemainder ), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bHitTarget ) )
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CWeaponFlamethrower )
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flAmmoUseRemainder, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.001f ),
	DEFINE_PRED_FIELD( m_flStartFiringTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flNextPrimaryAttackAnim, FIELD_FLOAT, 0 ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_flamethrower, CWeaponFlamethrower );
PRECACHE_WEAPON_REGISTER( weapon_flamethrower );

acttable_t CWeaponFlamethrower::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_FLAMETHROWER,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_FLAMETHROWER,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_FLAMETHROWER,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_FLAMETHROWER,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_FLAMETHROWER,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_FLAMETHROWER,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_FLAMETHROWER,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_FLAMETHROWER,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_FLAMETHROWER,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_FLAMETHROWER,	false },

	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_MP_ATTACK_STAND_PREFIRE_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_MP_ATTACK_STAND_POSTFIRE_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_MP_ATTACK_CROUCH_PREFIRE_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_MP_ATTACK_CROUCH_POSTFIRE_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_MP_ATTACK_SWIM_PREFIRE_FLAMETHROWER,	false },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_MP_ATTACK_SWIM_POSTFIRE_FLAMETHROWER,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponFlamethrower );

// ------------------------------------------------------------------------------------------------ //
// CWeaponFlamethrower implementation.
// ------------------------------------------------------------------------------------------------ //
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponFlamethrower::CWeaponFlamethrower()
{
	WeaponReset();

#if defined( CLIENT_DLL )
	m_pFlameEffect = NULL;
	m_pFiringStartSound = NULL;
	m_pFiringLoop = NULL;
	m_bFiringLoopCritical = false;
	m_pPilotLightSound = NULL;
	m_pHitTargetSound = NULL;
	m_pDynamicLight = NULL;
#else
	m_bSimulatingFlames = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponFlamethrower::~CWeaponFlamethrower()
{
	DestroySounds();
}


void CWeaponFlamethrower::DestroySounds( void )
{
#if defined( CLIENT_DLL )
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}
	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}
	if ( m_pPilotLightSound )
	{
		controller.SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
	if ( m_pHitTargetSound )
	{
		controller.SoundDestroy( m_pHitTargetSound );
		m_pHitTargetSound = NULL;
	}

	m_bHitTarget = false;
	m_bOldHitTarget = false;
#endif

}
void CWeaponFlamethrower::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_bHitTarget = false;
	m_flStartFiringTime = 0;
	m_flAmmoUseRemainder = 0;

#ifdef GAME_DLL
	m_flStopHitSoundTime = 0.0f;
#else
	m_iParticleWaterLevel = -1;

	StopFlame();
	StopPilotLight();
#endif

	DestroySounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::Precache( void )
{
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheScriptSound( "Player.AirBlastImpact" );
	PrecacheScriptSound( "Player.FlameOut" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	PrecacheScriptSound( "Weapon_FlameThrower.FireHit" );

	PrecacheTeamParticles( "flamethrower_%s", true );
	PrecacheTeamParticles( "flamethrower_crit_%s", true );

	PrecacheParticleSystem( "pyro_blast" );
	PrecacheParticleSystem( "deflect_fx" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFlamethrower::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
#ifdef CLIENT_DLL
		StartPilotLight();
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFlamethrower::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_bHitTarget = false;

#ifdef CLIENT_DLL
	StopFlame();
	StopPilotLight();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::ItemPostFrame()
{
	// Get the player owning the weapon.
	CTDCPlayer *pOwner = ToTDCPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	int iAmmo = pOwner->GetAmmoCount( m_iPrimaryAmmoType );
	bool bFired = false;

	if ( ( pOwner->m_nButtons & IN_ATTACK2 ) && m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		if ( iAmmo >= GetTDCWpnData().GetWeaponData( TDC_WEAPON_SECONDARY_MODE ).m_iAmmoPerShot )
		{
			// Disabled for now until we think of something better for it.
#if 0
			SecondaryAttack();
			bFired = true;
#endif
		}
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && iAmmo > 0 && m_iWeaponState != FT_STATE_AIRBLASTING )
	{
		PrimaryAttack();
		bFired = true;
	}

	if ( !bFired )
	{
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
			m_iWeaponState = FT_STATE_IDLE;
			m_bCritFire = false;
			m_bHitTarget = false;

#ifdef CLIENT_DLL
			StopFlame();
#endif
		}

		if ( !ReloadOrSwitchWeapons() )
		{
			WeaponIdle();
		}
	}

#ifdef GAME_DLL
	SimulateFlames();
#endif

	//BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::ItemBusyFrame( void )
{
#ifdef GAME_DLL
	SimulateFlames();
#endif
	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::ItemHolsterFrame( void )
{
#ifdef GAME_DLL
	SimulateFlames();
#endif
	BaseClass::ItemHolsterFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::PrimaryAttack()
{
	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTDCPlayer *pOwner = ToTDCPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
#if defined ( CLIENT_DLL )
		StopFlame();
#endif
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	CalcIsAttackCritical();

	// Because the muzzle is so long, it can stick through a wall if the player is right up against it.
	// Make sure the weapon can't fire in this condition by tracing a line between the eye point and the end of the muzzle.
	trace_t trace;	
	Vector vecEye = pOwner->EyePosition();
	Vector vecMuzzlePos = GetVisualMuzzlePos();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecMuzzlePos, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction < 1.0 && trace.m_pEnt->m_takedamage == DAMAGE_NO )
	{
		// there is something between the eye and the end of the muzzle, most likely a wall, don't fire, and stop firing if we already are
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
#ifdef CLIENT_DLL
			StopFlame();
#endif
			m_iWeaponState = FT_STATE_IDLE;
		}
		return;
	}

	switch ( m_iWeaponState )
	{
	case FT_STATE_IDLE:
	case FT_STATE_AIRBLASTING:
		{
			// Just started, play PRE and start looping view model anim

			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );

			m_flStartFiringTime = gpGlobals->curtime + 0.16;	// 5 frames at 30 fps

			m_iWeaponState = FT_STATE_STARTFIRING;
		}
		break;
	case FT_STATE_STARTFIRING:
		{
			// if some time has elapsed, start playing the looping third person anim
			if ( gpGlobals->curtime > m_flStartFiringTime )
			{
				m_iWeaponState = FT_STATE_FIRING;
				m_flNextPrimaryAttackAnim = gpGlobals->curtime;
			}
		}
		break;
	case FT_STATE_FIRING:
		{
			if ( gpGlobals->curtime >= m_flNextPrimaryAttackAnim )
			{
				pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flNextPrimaryAttackAnim = gpGlobals->curtime + 1.4;		// fewer than 45 frames!
			}
		}
		break;

	default:
		break;
	}

#if !defined (CLIENT_DLL)
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired( this );
#endif

	m_bCritFire = IsCurrentAttackACrit();

#ifdef CLIENT_DLL
	StartFlame();
#endif

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		// Find eligible entities in a cone in front of us.
		Vector vOrigin = pOwner->Weapon_ShootPosition();
		Vector vForward, vRight, vUp;
		QAngle vAngles = pOwner->EyeAngles() + pOwner->GetPunchAngle();
		AngleVectors( vAngles, &vForward, &vRight, &vUp );

#ifdef GAME_DLL
		// Crate the flame.
		// NOTE: Flames are no longer entities since there's no need for them to be such
		// plus we want to simulate them from the weapon during player tick.
		// So I've changed them to be a simple child class of the flamethrower. (Nicknine)
		CFlameParticle &flame = m_Flames[m_Flames.AddToTail()];
		flame.Init( GetFlameOriginPos(), pOwner->EyeAngles(), pOwner, this );
#endif
	}

	// Figure how much ammo we're using per shot and add it to our remainder to subtract.  (We may be using less than 1.0 ammo units
	// per frame, depending on how constants are tuned, so keep an accumulator so we can expend fractional amounts of ammo per shot.)
	m_flAmmoUseRemainder += (float)GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot * GetFireRate();

	// take the integer portion of the ammo use accumulator and subtract it from player's ammo count; any fractional amount of ammo use
	// remains and will get used in the next shot
	int iAmmoToSubtract = (int)m_flAmmoUseRemainder;
	if ( iAmmoToSubtract > 0 )
	{
		pOwner->RemoveAmmo( iAmmoToSubtract, m_iPrimaryAmmoType );
		m_flAmmoUseRemainder -= (float)iAmmoToSubtract;
		// round to 2 digits of precision
		//m_flAmmoUseRemainder = (float)( (int)( m_flAmmoUseRemainder * 100 ) ) / 100.0f;
	}

	m_flNextPrimaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + GetFireRate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::SecondaryAttack()
{
	if ( !tdc_airblast.GetBool() )
		return;

	// Get the player owning the weapon.
	CTDCPlayer *pOwner = ToTDCPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	m_iWeaponMode = TDC_WEAPON_SECONDARY_MODE;

	m_bCurrentAttackIsCrit = false;

#ifdef CLIENT_DLL
	StopFlame();
#endif

	m_iWeaponState = FT_STATE_AIRBLASTING;
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	WeaponSound( WPN_DOUBLE );

#ifdef CLIENT_DLL
	StartFlame();
#else
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired( this );

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION( pOwner, pOwner->GetCurrentCommand() );

	Vector vecDir;
	QAngle angDir = pOwner->EyeAngles();
	AngleVectors( angDir, &vecDir );

	Vector vecBlastSize = GetDeflectionSize();
	Vector vecOrigin = pOwner->Weapon_ShootPosition() + ( vecDir * Max( vecBlastSize.x, vecBlastSize.y ) );

	CBaseEntity *pList[64];
	int count = UTIL_EntitiesInBox( pList, 64, vecOrigin - vecBlastSize, vecOrigin + vecBlastSize, 0 );

	if ( tdc_debug_airblast.GetBool() )
	{
		NDebugOverlay::Box( vecOrigin, -vecBlastSize, vecBlastSize, 0, 0, 255, 100, 2.0 );
	}

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[i];

		if ( !pEntity || pEntity == pOwner || !pEntity->IsDeflectable() )
			continue;

		// Make sure we can actually see this entity so we don't hit anything through walls.
		if ( !pOwner->FVisible( pEntity, MASK_SOLID ) )
			continue;

		CDisablePredictionFiltering disabler;

		if ( pEntity->IsPlayer() )
		{
			if ( !pEntity->IsAlive() )
				continue;

			CTDCPlayer *pTFPlayer = ToTDCPlayer( pEntity );

			Vector vecPushDir;
			QAngle angPushDir = angDir;
			float flPitch = AngleNormalize( angPushDir[PITCH] );

			if ( pTFPlayer->GetGroundEntity() != NULL )
			{
				// If they're on the ground, always push them at least 45 degrees up.
				angPushDir[PITCH] = Min( -45.0f, flPitch );
			}
			else if ( flPitch > -45.0f )
			{
				// Proportionally raise the pitch.
				float flScale = RemapValClamped( flPitch, 0.0f, 90.0f, 1.0f, 0.0f );
				angPushDir[PITCH] = Max( -45.0f, flPitch - 45.0f * flScale );
			}

			AngleVectors( angPushDir, &vecPushDir );
			DeflectPlayer( pTFPlayer, pOwner, vecPushDir );
		}
		else if ( pOwner->IsEnemy( pEntity ) )
		{
			// Deflect projectile to the point that we're aiming at, similar to rockets.
			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecDeflect;
			GetProjectileReflectSetup( pOwner, vecPos, vecDeflect, false );

			DeflectEntity( pEntity, pOwner, vecDeflect );
		}
	}

	FINISH_LAG_COMPENSATION();
#endif

	pOwner->RemoveAmmo( GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iPrimaryAmmoType );

	// Don't allow firing immediately after airblasting.
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
}

#ifdef GAME_DLL
void CWeaponFlamethrower::DeflectEntity( CBaseEntity *pEntity, CTDCPlayer *pAttacker, Vector &vecDir )
{
	pEntity->Deflected( pAttacker, vecDir );
	pEntity->EmitSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	DispatchParticleEffect( "deflect_fx", PATTACH_ABSORIGIN_FOLLOW, pEntity );
}

void CWeaponFlamethrower::DeflectPlayer( CTDCPlayer *pVictim, CTDCPlayer *pAttacker, Vector &vecDir )
{
	if ( !pAttacker->IsEnemy( pVictim ) )
	{
		if ( pVictim->m_Shared.InCond( TDC_COND_BURNING ) )
		{
			// Extinguish teammates.
			pVictim->m_Shared.RemoveCond( TDC_COND_BURNING );
			pVictim->EmitSound( "Player.FlameOut" );

			CTDC_GameStats.Event_PlayerAwardBonusPoints( pAttacker, pVictim, 1 );
		}
	}
	else if ( tdc_airblast_players.GetBool() )
	{
		// Don't push players if they're too far off to the side. Ignore Z.
		Vector2D vecVictimDir = pVictim->WorldSpaceCenter().AsVector2D() - pAttacker->WorldSpaceCenter().AsVector2D();
		Vector2DNormalize( vecVictimDir );

		Vector2D vecDir2D = vecDir.AsVector2D();
		Vector2DNormalize( vecDir2D );

		float flDot = DotProduct2D( vecDir2D, vecVictimDir );
		if ( flDot >= 0.8 )
		{
			// Push enemy players.
			pVictim->m_Shared.AirblastPlayer( pAttacker, vecDir, 500 );
			pVictim->EmitSound( "Player.AirBlastImpact" );

			// Add pusher as recent damager so he can get a kill credit for pushing a player to his death.
			pVictim->AddDamagerToHistory( pAttacker, this );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFlamethrower::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		// If we were firing, stop
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
			m_iWeaponState = FT_STATE_IDLE;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle at it appears visually
//-----------------------------------------------------------------------------
Vector CWeaponFlamethrower::GetVisualMuzzlePos()
{
	return GetMuzzlePosHelper( true );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position at which to spawn flame damage entities
//-----------------------------------------------------------------------------
Vector CWeaponFlamethrower::GetFlameOriginPos()
{
	return GetMuzzlePosHelper( false );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle
//-----------------------------------------------------------------------------
Vector CWeaponFlamethrower::GetMuzzlePosHelper( bool bVisualPos )
{
	Vector vecMuzzlePos;
	CTDCPlayer *pOwner = ToTDCPlayer( GetPlayerOwner() );
	if ( pOwner ) 
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, &vecUp );
		vecMuzzlePos = pOwner->Weapon_ShootPosition();
		vecMuzzlePos +=  vecRight * TDC_FLAMETHROWER_MUZZLEPOS_RIGHT;
		// if asking for visual position of muzzle, include the forward component
		if ( bVisualPos )
		{
			vecMuzzlePos +=  vecForward * TDC_FLAMETHROWER_MUZZLEPOS_FORWARD;
		}
	}
	return vecMuzzlePos;
}

//-----------------------------------------------------------------------------
// Purpose: Return the size of the airblast detection box
//-----------------------------------------------------------------------------
Vector CWeaponFlamethrower::GetDeflectionSize()
{
	return Vector( 128.0f, 128.0f, 64.0f );
}

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if ( !GetPredictable() )
	{
		if ( IsCarrierAlive() && WeaponState() == WEAPON_IS_ACTIVE )
		{
			if ( m_iWeaponState > FT_STATE_IDLE )
			{
				StartFlame();
			}
			else
			{
				StartPilotLight();
			}
		}
		else
		{
			StopFlame();
			StopPilotLight();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::UpdateOnRemove( void )
{
	StopFlame();
	StopPilotLight();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		if ( !IsDormant() && bDormant )
		{
			StopFlame();
			StopPilotLight();
		}
	}

	// Deliberately skip base combat weapon to avoid being holstered
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::Simulate( void )
{
	BaseClass::Simulate();

	// Handle the flamethrower light
	if ( tdc_muzzlelight.GetBool() && m_iWeaponState > FT_STATE_IDLE && m_iWeaponState != FT_STATE_AIRBLASTING )
	{
		if ( !m_pDynamicLight || m_pDynamicLight->key != LIGHT_INDEX_MUZZLEFLASH + entindex() )
		{
			m_pDynamicLight = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + entindex() );
		}

		m_pDynamicLight->origin = GetVisualMuzzlePos();
		m_pDynamicLight->color.r = 255;
		m_pDynamicLight->color.g = 100;
		m_pDynamicLight->color.b = 10;
		m_pDynamicLight->die = gpGlobals->curtime + 1.0f;
		m_pDynamicLight->radius = 256.0f;
		m_pDynamicLight->style = 6;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	if ( m_pFlameEffect )
	{
		RestartParticleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::StartFlame()
{
	if ( m_iWeaponState == FT_STATE_AIRBLASTING )
	{
		C_BaseEntity *pModel = GetWeaponForEffect();
		DispatchParticleEffect( "pyro_blast", PATTACH_POINT_FOLLOW, pModel, m_iMuzzleAttachment );

		//CLocalPlayerFilter filter;
		//EmitSound( filter, entindex(), "Weapon_FlameThrower.AirBurstAttack" );
	}
	else
	{
		CTDCPlayer *pPlayer = GetTDCPlayerOwner();
		if ( !pPlayer )
			return;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		// normally, crossfade between start sound & firing loop in 3.5 sec
		float flCrossfadeTime = 3.5;

		if ( m_pFiringLoop && ( m_bCritFire != m_bFiringLoopCritical ) )
		{
			// If we're firing and changing between critical & noncritical, just need to change the firing loop.
			// Set crossfade time to zero so we skip the start sound and go to the loop immediately.

			flCrossfadeTime = 0;
			StopFlame( true );
		}

		StopPilotLight();

		if ( !m_pFiringStartSound && !m_pFiringLoop )
		{
			RestartParticleEffect();
			CLocalPlayerFilter filter;

			// Play the fire start sound
			const char *shootsound = GetShootSound( SINGLE );
			if ( flCrossfadeTime > 0.0 )
			{
				// play the firing start sound and fade it out
				m_pFiringStartSound = controller.SoundCreate( filter, entindex(), shootsound );
				controller.Play( m_pFiringStartSound, 1.0, 100 );
				controller.SoundChangeVolume( m_pFiringStartSound, 0.0, flCrossfadeTime );
			}

			// Start the fire sound loop and fade it in
			if ( m_bCritFire )
			{
				shootsound = GetShootSound( BURST );
			}
			else
			{
				shootsound = GetShootSound( SPECIAL1 );
			}
			m_pFiringLoop = controller.SoundCreate( filter, entindex(), shootsound );
			m_bFiringLoopCritical = m_bCritFire;

			// play the firing loop sound and fade it in
			if ( flCrossfadeTime > 0.0 )
			{
				controller.Play( m_pFiringLoop, 0.0, 100 );
				controller.SoundChangeVolume( m_pFiringLoop, 1.0, flCrossfadeTime );
			}
			else
			{
				controller.Play( m_pFiringLoop, 1.0, 100 );
			}
		}

		// Restart our particle effect if we've transitioned across water boundaries
		if ( m_iParticleWaterLevel != -1 && pPlayer->GetWaterLevel() != m_iParticleWaterLevel )
		{
			if ( m_iParticleWaterLevel == WL_Eyes || pPlayer->GetWaterLevel() == WL_Eyes )
			{
				RestartParticleEffect();
			}
		}

		if ( m_bHitTarget != m_bOldHitTarget )
		{
			if ( m_bHitTarget )
			{
				CLocalPlayerFilter filter;
				m_pHitTargetSound = controller.SoundCreate( filter, entindex(), "Weapon_FlameThrower.FireHit" );
				controller.Play( m_pHitTargetSound, 1.0f, 100.0f );
			}
			else if ( m_pHitTargetSound )
			{
				controller.SoundDestroy( m_pHitTargetSound );
				m_pHitTargetSound = NULL;
			}

			m_bOldHitTarget = m_bHitTarget;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::StopFlame( bool bAbrupt /* = false */ )
{
	if ( ( m_pFiringLoop || m_pFiringStartSound ) && !bAbrupt )
	{
		// play a quick wind-down poof when the flame stops
		CLocalPlayerFilter filter;
		const char *shootsound = GetShootSound( SPECIAL3 );
		EmitSound( filter, entindex(), shootsound );
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}

	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}

	if ( m_pFlameEffect )
	{
		if ( m_hFlameEffectHost.Get() )
		{
			m_hFlameEffectHost->ParticleProp()->StopEmission( m_pFlameEffect );
			m_hFlameEffectHost = NULL;
		}

		m_pFlameEffect = NULL;
	}

	if ( !bAbrupt )
	{
		if ( m_pHitTargetSound )
		{
			controller.SoundDestroy( m_pHitTargetSound );
			m_pHitTargetSound = NULL;
		}

		m_bOldHitTarget = false;
		m_bHitTarget = false;
	}

	if ( m_pDynamicLight && m_pDynamicLight->key == LIGHT_INDEX_MUZZLEFLASH + entindex() )
	{
		m_pDynamicLight->die = gpGlobals->curtime;
		m_pDynamicLight = NULL;
	}

	m_iParticleWaterLevel = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::StartPilotLight()
{
	if ( !m_pPilotLightSound )
	{
		StopFlame();

		// Create the looping pilot light sound
		const char *pilotlightsound = GetShootSound( SPECIAL2 );
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::StopPilotLight()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::RestartParticleEffect( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_pFlameEffect && m_hFlameEffectHost.Get() )
	{
		m_hFlameEffectHost->ParticleProp()->StopEmission( m_pFlameEffect );
	}

	m_iParticleWaterLevel = pOwner->GetWaterLevel();
	int iTeam = pOwner->GetTeamNumber();

	// Start the appropriate particle effect
	const char *pszParticleEffect;
	if ( pOwner->GetWaterLevel() == WL_Eyes )
	{
		pszParticleEffect = "flamethrower_underwater";
	}
	else if ( m_bCritFire )
	{
		pszParticleEffect = ConstructTeamParticle( "flamethrower_crit_%s", iTeam, true );
	}
	else
	{
		pszParticleEffect = ConstructTeamParticle( "flamethrower_%s", iTeam, true );
	}

	// Start the effect on the viewmodel if our owner is the local player
	C_BaseEntity *pModel = GetWeaponForEffect();
	m_pFlameEffect = pModel->ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, m_iMuzzleAttachment );
	pModel->ParticleProp()->AddControlPoint( m_pFlameEffect, 2, pOwner, PATTACH_ABSORIGIN_FOLLOW );
	m_hFlameEffectHost = pModel;

	pOwner->m_Shared.SetParticleToMercColor( m_pFlameEffect );
}

#else
//-----------------------------------------------------------------------------
// Purpose: Notify client that we're hitting an enemy.
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::SetHitTarget( void )
{
	if ( m_iWeaponState > FT_STATE_IDLE )
	{
		if ( !m_bHitTarget )
			m_bHitTarget = true;

		m_flStopHitSoundTime = gpGlobals->curtime + 0.2f;
		SetContextThink( &CWeaponFlamethrower::HitTargetThink, gpGlobals->curtime + 0.1f, "FlameThrowerHitTargetThink" );
	}
}

void CWeaponFlamethrower::HitTargetThink( void )
{
	if ( m_flStopHitSoundTime != 0.0f && m_flStopHitSoundTime > gpGlobals->curtime )
	{
		m_bHitTarget = false;
		m_flStopHitSoundTime = 0.0f;
		SetContextThink( NULL, 0, "FlameThrowerHitTargetThink" );
	}
	else
	{
		SetContextThink( &CWeaponFlamethrower::HitTargetThink, gpGlobals->curtime + 0.1f, "FlameThrowerHitTargetThink" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponFlamethrower::SimulateFlames( void )
{
	if ( m_Flames.Count() == 0 )
		return;

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	// Simulate all flames.
	m_bSimulatingFlames = true;
	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );
	for ( int i = m_Flames.Count() - 1; i >= 0; i-- )
	{
		if ( m_Flames[i].FlameThink() == false )
		{
			m_Flames.FastRemove( i );
		}
	}
	FINISH_LAG_COMPENSATION();
	m_bSimulatingFlames = false;
}
#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFlameParticle::Init( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CWeaponFlamethrower *pWeapon )
{
	m_vecOrigin = m_vecPrevPos = m_vecInitialPos = vecOrigin;
	m_hOwner = pOwner;
	m_pOuter = pWeapon;

	float flBoxSize = tdc_flamethrower_boxsize.GetFloat();
	m_vecMins.Init( -flBoxSize, -flBoxSize, -flBoxSize );
	m_vecMaxs.Init( flBoxSize, flBoxSize, flBoxSize );

	// Set team.
	m_iTeamNum = pOwner->GetTeamNumber();
	m_iDmgType = pWeapon->GetDamageType();
	if ( pWeapon->IsCurrentAttackACrit() )
	{
		m_iDmgType |= DMG_CRITICAL;
	}
	m_flDmgAmount = pWeapon->GetProjectileDamage() * pWeapon->GetFireRate();

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float velocity = tdc_flamethrower_velocity.GetFloat();
	m_vecBaseVelocity = vecForward * velocity;
	m_vecBaseVelocity += RandomVector( -velocity * tdc_flamethrower_vecrand.GetFloat(), velocity * tdc_flamethrower_vecrand.GetFloat() );
	m_vecAttackerVelocity = pOwner->GetAbsVelocity();
	m_vecVelocity = m_vecBaseVelocity;

	m_flTimeRemove = gpGlobals->curtime + ( tdc_flamethrower_flametime.GetFloat() * random->RandomFloat( 0.9, 1.1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
bool CFlameParticle::FlameThink( void )
{
	// Remove it if it's expired.
	if ( gpGlobals->curtime >= m_flTimeRemove )
		return false;

	// If the player changes team, remove all flames immediately to prevent griefing.
	if ( !m_hOwner || m_hOwner->GetTeamNumber() != m_iTeamNum )
		return false;

	// Do collision detection.  We do custom collision detection because we can do it more cheaply than the
	// standard collision detection (don't need to check against world unless we might have hit an enemy) and
	// flame entity collision detection w/o this was a bottleneck on the X360 server
	if ( m_vecOrigin != m_vecPrevPos )
	{
		//CTDCTeam *pTeam = pAttacker->GetOpposingTFTeam();
		//if ( !pTeam )
		//	return;

		bool bHitWorld = false;

		ForEachEnemyTFTeam( m_iTeamNum, [&]( int iTeam )
		{
			CTDCTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				return true;

			// check collision against all enemy players
			for ( int iPlayer = 0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
			{
				CBasePlayer *pPlayer = pTeam->GetPlayer( iPlayer );
				// Is this player connected, alive, and an enemy?
				if ( pPlayer && pPlayer != m_hOwner && pPlayer->IsConnected() && pPlayer->IsAlive() )
				{
					CheckCollision( pPlayer, &bHitWorld );
					if ( bHitWorld )
						return false;
				}
			}

			return true;
		} );

		if ( bHitWorld )
			return false;
	}

	// Calculate how long the flame has been alive for
	float flFlameElapsedTime = tdc_flamethrower_flametime.GetFloat() - ( m_flTimeRemove - gpGlobals->curtime );
	// Calculate how much of the attacker's velocity to blend in to the flame's velocity.  The flame gets the attacker's velocity
	// added right when the flame is fired, but that velocity addition fades quickly to zero.
	float flAttackerVelocityBlend = RemapValClamped( flFlameElapsedTime, tdc_flamethrower_velocityfadestart.GetFloat(),
		tdc_flamethrower_velocityfadeend.GetFloat(), 1.0, 0 );

	// Reduce our base velocity by the air drag constant
	m_vecBaseVelocity *= tdc_flamethrower_drag.GetFloat();

	// Add our float upward velocity
	m_vecVelocity = m_vecBaseVelocity + Vector( 0, 0, tdc_flamethrower_float.GetFloat() ) + ( flAttackerVelocityBlend * m_vecAttackerVelocity );

	// Render debug visualization if convar on
	if ( tdc_debug_flamethrower.GetInt() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = ( (int)( gpGlobals->curtime * 10 ) ) % 255;
			NDebugOverlay::Box( m_vecOrigin, m_vecMins, m_vecMaxs, val, 255, val, 0, 0 );
			//NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 );
		}
		else
		{
			NDebugOverlay::Box( m_vecOrigin, m_vecMins, m_vecMaxs, 0, 100, 255, 0, 0 );
			//NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
		}
	}

	//SetNextThink( gpGlobals->curtime );

	// Move!
	m_vecPrevPos = m_vecOrigin;
	m_vecOrigin += m_vecVelocity * gpGlobals->frametime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks collisions against other entities
//-----------------------------------------------------------------------------
void CFlameParticle::CheckCollision( CBaseEntity *pOther, bool *pbHitWorld )
{
	*pbHitWorld = false;

	// if we've already burnt this entity, don't do more damage, so skip even checking for collision with the entity
	int iIndex = m_hEntitiesBurnt.Find( pOther );
	if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
		return;

	// Do a bounding box check against the entity
	Vector vecMins, vecMaxs;
	pOther->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
	CBaseTrace trace;
	Ray_t ray;
	float flFractionLeftSolid;
	ray.Init( m_vecPrevPos, m_vecOrigin, m_vecMins, m_vecMaxs );
	if ( IntersectRayWithBox( ray, vecMins, vecMaxs, 0.0, &trace, &flFractionLeftSolid ) )
	{
		// if bounding box check passes, check player hitboxes
		trace_t trHitbox;
		trace_t trWorld;
		bool bTested = pOther->GetCollideable()->TestHitboxes( ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox );
		if ( !bTested || !trHitbox.DidHit() )
			return;

		// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything.  Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		Vector vDir = ray.m_Delta;
		vDir.NormalizeInPlace();
		UTIL_TraceLine( m_vecOrigin + vDir * m_vecMins.x, m_vecInitialPos, MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &trWorld );

		if ( tdc_debug_flamethrower.GetInt() )
		{
			NDebugOverlay::Line( trWorld.startpos, trWorld.endpos, 0, 255, 0, true, 3.0f );
		}

		if ( trWorld.fraction == 1.0 )
		{
			// if there is nothing solid in the way, damage the entity
			OnCollide( pOther );
		}
		else
		{
			// we hit the world, remove ourselves
			*pbHitWorld = true;
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CFlameParticle::OnCollide( CBaseEntity *pOther )
{
	// remember that we've burnt this player
	m_hEntitiesBurnt.AddToTail( pOther );

	float flDistance = m_vecOrigin.DistTo( m_vecInitialPos );
	float flMultiplier;
	if ( flDistance <= 125 )
	{
		// at very short range, apply short range damage multiplier
		flMultiplier = tdc_flamethrower_shortrangedamagemultiplier.GetFloat();
	}
	else
	{
		// make damage ramp down from 100% to 60% from half the max dist to the max dist
		flMultiplier = RemapValClamped( flDistance, tdc_flamethrower_maxdamagedist.GetFloat() / 2, tdc_flamethrower_maxdamagedist.GetFloat(), 1.0, 0.6 );
	}
	float flDamage = m_flDmgAmount * flMultiplier;
	flDamage = Max( flDamage, 1.0f );
	if ( tdc_debug_flamethrower.GetInt() )
	{
		Msg( "Flame touch dmg: %.1f\n", flDamage );
	}

	m_pOuter->SetHitTarget();

	CTakeDamageInfo info( m_hOwner, m_hOwner, m_pOuter, flDamage, m_iDmgType, TDC_DMG_CUSTOM_BURNING );
	info.SetReportedPosition( m_hOwner->GetAbsOrigin() );

	// We collided with pOther, so try to find a place on their surface to show blood
	trace_t pTrace;
	UTIL_TraceLine( m_vecOrigin, pOther->WorldSpaceCenter(), MASK_SOLID | CONTENTS_HITBOX, NULL, COLLISION_GROUP_NONE, &pTrace );

	pOther->DispatchTraceAttack( info, m_vecVelocity, &pTrace );
	ApplyMultiDamage();
}

#endif // GAME_DLL
