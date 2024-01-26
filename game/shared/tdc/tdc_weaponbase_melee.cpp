//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tdc_weaponbase_melee.h"
#include "effect_dispatch_data.h"
#include "tdc_gamerules.h"
#include "tdc_lagcompensation.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tdc_player.h"
#include "tdc_gamestats.h"
// Client specific.
#else
#include "c_tdc_player.h"
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TDCWeaponBaseMelee, DT_TDCWeaponBaseMelee )

BEGIN_NETWORK_TABLE( CTDCWeaponBaseMelee, DT_TDCWeaponBaseMelee )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flSmackTime ) ),
	RecvPropTime( RECVINFO( m_flSwingCountReset ) ),
	RecvPropInt( RECVINFO( m_iSwingCount ) ),
#else
	SendPropTime( SENDINFO( m_flSmackTime ) ),
	SendPropTime( SENDINFO( m_flSwingCountReset ) ),
	SendPropInt( SENDINFO( m_iSwingCount ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTDCWeaponBaseMelee )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flSmackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSwingCountReset, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iSwingCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

ConVar tdc_melee_fatigue_enable( "tdc_melee_fatigue_enable", "1", FCVAR_REPLICATED, "Enable melee fatigue." );
ConVar tdc_melee_fatigue_free_swings( "tdc_melee_fatigue_free_swings", "2", FCVAR_REPLICATED, "Number of swings before cooldowns are applied." );
ConVar tdc_melee_fatigue_reset_time( "tdc_melee_fatigue_reset_time", "2", FCVAR_REPLICATED, "Time in seconds it takes for melee fatigue to reset." );
ConVar tdc_melee_fatigue_multiplier( "tdc_melee_fatigue_multiplier", "0.18", FCVAR_REPLICATED, "Multiplier for melee fatigue." );
ConVar tdc_melee_fatigue_max( "tdc_melee_fatigue_max", "0.4", FCVAR_REPLICATED, "Multiplier maximum clamp for melee fatigue." );

//=============================================================================
//
// TFWeaponBase Melee functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTDCWeaponBaseMelee::CTDCWeaponBaseMelee()
{
	WeaponReset();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	m_flSmackTime = -1.0f;
	m_iSwingCount = 0;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTDCWeaponBaseMelee::CanHolster( void ) const
{
	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTDCWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;	
	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::PrimaryAttack()
{
	// Get the current player.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	// Swing the weapon.
	Swing( pPlayer );

#if !defined( CLIENT_DLL ) 
	pPlayer->NoteWeaponFired( this );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::SecondaryAttack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::DoImpactEffect( trace_t &tr, int nDamageType )
{
	if ( tr.m_pEnt->IsPlayer() )
	{
		WeaponSound( MELEE_HIT );
	}
	else
	{
		WeaponSound( MELEE_HIT_WORLD );
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::Swing( CTDCPlayer *pPlayer )
{
	CalcIsAttackCritical();

	// Play the melee swing and miss (whoosh) always.
	SendPlayerAnimEvent( pPlayer );

	DoViewModelAnimation();

	UpdateFatigue();

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	//SetWeaponIdleTime( m_flNextPrimaryAttack + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );
	
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}

	m_flSmackTime = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::UpdateFatigue( void )
{
	// Update the swing decay time.
	if ( gpGlobals->curtime >= m_flSwingCountReset )
	{
		m_iSwingCount = 0;
	}
	m_iSwingCount++;

	m_flSwingCountReset = gpGlobals->curtime + tdc_melee_fatigue_reset_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCWeaponBaseMelee::GetFireRate( void )
{
	if ( tdc_melee_fatigue_enable.GetBool() )
	{
		float flBaseRate = BaseClass::GetFireRate();
		int iSwings = ( m_iSwingCount - tdc_melee_fatigue_free_swings.GetInt() );
		if ( iSwings <= 0 )
		{
			return flBaseRate;
		}
		float flDecay = ( tdc_melee_fatigue_multiplier.GetFloat() * iSwings );
		if ( flDecay > tdc_melee_fatigue_max.GetFloat() )
		{
			flDecay = tdc_melee_fatigue_max.GetFloat();
		}
		return flBaseRate * ( 1.0 + flDecay );
	}
	else
	{
		return BaseClass::GetFireRate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::DoViewModelAnimation( void )
{
	Activity act = ( m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;

	if ( IsCurrentAttackACrit() )
		act = ACT_VM_SWINGHARD;

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::SendPlayerAnimEvent( CTDCPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::ItemPostFrame()
{
	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		Smack();
		m_flSmackTime = -1.0f;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWeaponBaseMelee::DoSwingTrace( trace_t &trace )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Get the current player.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->EyePosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();

	// See if we hit anything.
	CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &filter, &trace );

	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &filter, &trace );

		if ( trace.fraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = trace.m_pEnt;
			if ( !pHit || pHit->IsBSPModel() )
			{
				// Why duck hull min/max?
				FindHullIntersection( vecSwingStart, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, &filter );
			}

			// This is the point on the actual surface (the hull could have hit space)
			vecSwingEnd = trace.endpos;	
		}
	}

	return ( trace.fraction < 1.0f );
}

// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished 
//       playing.
// -----------------------------------------------------------------------------
void CTDCWeaponBaseMelee::Smack( void )
{
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

	// We hit, setup the smack.
	trace_t trace;

	if ( DoSwingTrace( trace ) )
	{
		// Get the current player.
		CTDCPlayer *pPlayer = GetTDCPlayerOwner();
		if ( !pPlayer )
			return;

		Vector vecForward; 
		AngleVectors( pPlayer->EyeAngles(), &vecForward );
		Vector vecSwingStart = pPlayer->EyePosition();
		Vector vecSwingEnd = vecSwingStart + vecForward * 48;

#ifndef CLIENT_DLL
		// Do Damage.
		ETDCDmgCustom iCustomDamage = TDC_DMG_CUSTOM_NONE;
		float flDamage = GetMeleeDamage( trace.m_pEnt, iCustomDamage );
		int iDmgType = GetDamageType();
		if ( IsCurrentAttackACrit() )
		{
			iDmgType |= DMG_CRITICAL;
		}

		{
			CDisablePredictionFiltering disabler;

			CTakeDamageInfo info( pPlayer, pPlayer, this, flDamage, iDmgType, iCustomDamage );
			CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * 80.0f );
			trace.m_pEnt->DispatchTraceAttack( info, vecForward, &trace );
			ApplyMultiDamage();
		}
#endif

		OnEntityHit( trace.m_pEnt );

		// Don't impact trace friendly players or objects
		if ( pPlayer->IsEnemy( trace.m_pEnt ) )
		{
			DoImpactEffect( trace, DMG_CLUB );
		}
	}

	FINISH_LAG_COMPENSATION();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTDCWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, ETDCDmgCustom &iCustomDamage )
{
	return (float)GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTDCWeaponBaseMelee::GetSwingRange( void )
{
	return 48;
}

void CTDCWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity )
{
}
