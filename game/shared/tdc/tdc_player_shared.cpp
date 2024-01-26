//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tdc_gamerules.h"
#include "tdc_player_shared.h"
#include "takedamageinfo.h"
#include "tdc_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tdc_item.h"
#include "entity_capture_flag.h"
#include "in_buttons.h"
#include "tdc_viewmodel.h"
#include "tdc_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tdc_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tdc_fx.h"
#include "soundenvelope.h"
#include "c_tdc_playerclass.h"
#include "iviewrender.h"
#include "c_tdc_playerresource.h"
#include "c_tdc_team.h"

#define CTDCPlayerClass C_TDCPlayerClass

// Server specific.
#else
#include "tdc_player.h"
#include "te_effect_dispatch.h"
#include "tdc_fx.h"
#include "util.h"
#include "tdc_team.h"
#include "tdc_gamestats.h"
#include "tdc_playerclass.h"
#endif

ConVar tdc_always_loser( "tdc_always_loser", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Force loserstate to true." );

// TDC ConVars.
ConVar tdc_infinite_ammo( "tdc_infinite_ammo", "0", FCVAR_REPLICATED, "Enabled infinite ammo for all players. Weapons still need to be reloaded." );
ConVar tdc_disable_player_shadows( "tdc_disable_player_shadows", "0", FCVAR_REPLICATED, "Disables rendering of player shadows regardless of client's graphical settings." );
ConVar tdc_disablefreezecam( "tdc_disablefreezecam", "0", FCVAR_REPLICATED );
ConVar tdc_zoom_fov( "tdc_zoom_fov", "65", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tdc_player_restrict_class( "tdc_player_restrict_class", "", FCVAR_NOTIFY | FCVAR_REPLICATED, "Restricts players to a specific class." );

ConVar tdc_headshoteffect_mindmg( "tdc_headshoteffect_mindmg", "40", FCVAR_REPLICATED );
ConVar tdc_headshoteffect_maxdmg( "tdc_headshoteffect_maxdmg", "150", FCVAR_REPLICATED );
ConVar tdc_headshoteffect_mintime( "tdc_headshoteffect_mintime", "0.2", FCVAR_REPLICATED );
ConVar tdc_headshoteffect_maxtime( "tdc_headshoteffect_maxtime", "1.0", FCVAR_REPLICATED );
ConVar tdc_headshoteffect_fadetime( "tdc_headshoteffect_fadetime", "0.5", FCVAR_REPLICATED );

ConVar tdc_player_burn_speedboostfactor( "tdc_player_burn_speedboostfactor", "1.25", FCVAR_REPLICATED );
ConVar tdc_player_burn_spread( "tdc_player_burn_spread", "1", FCVAR_REPLICATED );
ConVar tdc_player_burn_spread_radius( "tdc_player_burn_spread_radius", "64", FCVAR_REPLICATED );
ConVar tdc_player_burn_spread_addtime( "tdc_player_burn_spread_addtime", "2", FCVAR_REPLICATED );

ConVar tdc_player_sprint_gain( "tdc_player_sprint_gain", "200", FCVAR_REPLICATED, "Sets how much speed is gained" );
ConVar tdc_player_sprint_accel_time("tdc_player_sprint_accel_time", "1", FCVAR_REPLICATED, "Sets how many seconds until max speed");

extern ConVar tdc_allow_special_classes;

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTDCPlayerShared, DT_TDCPlayerSharedLocal )
	RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
	RecvPropInt( RECVINFO( m_iDesiredWeaponID ) ),
	RecvPropBool( RECVINFO( m_bWaitingToRespawn ) ),
	RecvPropTime( RECVINFO( m_flRespawnTime ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nStreaks ), RecvPropInt( RECVINFO( m_nStreaks[0] ) ) ),
	RecvPropVector( RECVINFO( m_vecAirblastPos ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTDCPlayerShared, DT_TDCPlayerShared )
	RecvPropInt( RECVINFO( m_nPlayerCond ) ),
	RecvPropInt( RECVINFO( m_bJumping ) ),
	RecvPropInt( RECVINFO( m_nAirDucked ) ),
	RecvPropInt( RECVINFO( m_nPlayerState ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCondExpireTimeLeft ), RecvPropFloat( RECVINFO( m_flCondExpireTimeLeft[0] ) ) ),
	RecvPropTime( RECVINFO( m_flFlameRemoveTime ) ),
	RecvPropTime( RECVINFO( m_flStunExpireTime ) ),
	RecvPropEHandle( RECVINFO( m_hStunner ) ),
	RecvPropFloat( RECVINFO( m_flInvisibility ) ),
	RecvPropInt( RECVINFO( m_iDamageSourceType ) ),
	RecvPropFloat( RECVINFO(m_flSprintStartTime) ),
	// Local Data.
	RecvPropDataTable( "tdcsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TDCPlayerSharedLocal) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTDCPlayerShared )
	DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAirDucked, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flInvisibility, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	//DEFINE_PRED_FIELD( m_flInvisChangeCompleteTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTDCPlayerShared, DT_TDCPlayerSharedLocal )
	SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
	SendPropInt( SENDINFO( m_iDesiredWeaponID ) ),
	SendPropBool( SENDINFO( m_bWaitingToRespawn ) ),
	SendPropTime( SENDINFO( m_flRespawnTime ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nStreaks ), SendPropInt( SENDINFO_ARRAY( m_nStreaks ) ) ),
	SendPropVector( SENDINFO( m_vecAirblastPos ), -1, SPROP_COORD ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTDCPlayerShared, DT_TDCPlayerShared )
	SendPropInt( SENDINFO( m_nPlayerCond ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nAirDucked ), 2, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TDC_STATE_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCondExpireTimeLeft ), SendPropFloat( SENDINFO_ARRAY( m_flCondExpireTimeLeft ) ) ),
	SendPropTime( SENDINFO( m_flFlameRemoveTime ) ),
	SendPropTime( SENDINFO( m_flStunExpireTime ) ),
	SendPropEHandle( SENDINFO( m_hStunner ) ),
	SendPropFloat( SENDINFO( m_flInvisibility ), 32, SPROP_CHANGES_OFTEN, 0.0f, 1.0f ),
	SendPropInt( SENDINFO( m_iDamageSourceType ) ),
	SendPropFloat( SENDINFO( m_flSprintStartTime ) ),
	// Local Data.
	SendPropDataTable( "tdcsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TDCPlayerSharedLocal ), SendProxy_SendLocalDataTable ),
END_SEND_TABLE()

#endif


// --------------------------------------------------------------------------------------------------- //
// Shared CTDCPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

// --------------------------------------------------------------------------------------------------- //
// CTDCPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTDCPlayerShared::CTDCPlayerShared()
{
	m_nPlayerState.Set( TDC_STATE_WELCOME );
	m_bJumping = false;
	m_nAirDucked = 0;
	m_flInvisibility = 0.0f;

	m_flRespawnTime = 0.0f;

	m_iDesiredWeaponID = -1;
	m_iDamageSourceType = -1;

	m_iStunPhase = 0;

#ifdef CLIENT_DLL
	m_pCritSound = NULL;
	m_pCritEffect = NULL;
	m_pBurningEffect = NULL;
	m_pBurningSound = NULL;
	m_pSpeedEffect = NULL;
#endif
}

void CTDCPlayerShared::Init( CTDCPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;

	SetJumping( false );
}

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
void CTDCPlayerShared::AddCond( ETDCCond nCond, float flDuration /* = PERMANENT_CONDITION */ )
{
	Assert( nCond >= 0 && nCond < TDC_COND_LAST );
	m_nPlayerCond |= ( 1 << nCond );
	m_flCondExpireTimeLeft.Set( nCond, flDuration );
	OnConditionAdded( nCond );
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
void CTDCPlayerShared::RemoveCond( ETDCCond nCond )
{
	Assert( nCond >= 0 && nCond < TDC_COND_LAST );
	m_nPlayerCond &= ~( 1 << nCond );
	m_flCondExpireTimeLeft.Set( nCond, 0 );
	OnConditionRemoved( nCond );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::InCond( ETDCCond nCond ) const
{
	Assert( nCond >= 0 && nCond < TDC_COND_LAST );
	return ( ( m_nPlayerCond & ( 1 << nCond ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::RemoveCondIfPresent( ETDCCond nCond )
{
	if ( InCond( nCond ) )
	{
		RemoveCond( nCond );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTDCPlayerShared::GetConditionDuration( ETDCCond nCond ) const
{
	Assert( nCond >= 0 && nCond < TDC_COND_LAST );

	if ( InCond( nCond ) )
	{
		return m_flCondExpireTimeLeft[nCond];
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsCritBoosted( void ) const
{
	return ( InCond( TDC_COND_POWERUP_CRITDAMAGE ) || 
		InCond( TDC_COND_CRITBOOSTED_BONUS_TIME ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsInvulnerable( void ) const
{
	return ( InCond( TDC_COND_INVULNERABLE_SPAWN_PROTECT ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsStealthed( void ) const
{
	return ( InCond( TDC_COND_POWERUP_CLOAK ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsHasting( void ) const
{
	return ( InCond( TDC_COND_POWERUP_SPEEDBOOST ) ||
		InCond( TDC_COND_LASTSTANDING ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsMovementLocked( void ) const
{
	return ( InCond( TDC_COND_TAUNTING ) ||
		InCond( TDC_COND_STUNNED ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsCarryingPowerup( void ) const
{
	for ( int i = 0; g_aPowerups[i].cond != TDC_COND_LAST; i++ )
	{
		if ( InCond( g_aPowerups[i].cond ) )
			return true;
	}

	return false;
}

void CTDCPlayerShared::DebugPrintConditions( void )
{
#ifndef CLIENT_DLL
	const char *szDll = "Server";
#else
	const char *szDll = "Client";
#endif

	Msg( "( %s ) Conditions for player ( %d )\n", szDll, m_pOuter->entindex() );

	int i;
	int iNumFound = 0;
	for ( i = 0; i < TDC_COND_LAST; i++ )
	{
		if ( InCond( (ETDCCond)i ) )
		{
			if ( m_flCondExpireTimeLeft[i] == PERMANENT_CONDITION )
			{
				Msg( "( %s ) Condition %d - ( permanent cond )\n", szDll, i );
			}
			else
			{
				Msg( "( %s ) Condition %d - ( %.1f left )\n", szDll, i, m_flCondExpireTimeLeft[i] );
			}

			iNumFound++;
		}
	}

	if ( iNumFound == 0 )
	{
		Msg( "( %s ) No active conditions\n", szDll );
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnPreDataChanged( void )
{
	m_nOldConditions = m_nPlayerCond;
	m_bWasCritBoosted = IsCritBoosted();
	m_flOldFlameRemoveTime = m_flFlameRemoveTime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnDataChanged( void )
{
	// Update conditions from last network change
	SyncConditions( m_nPlayerCond, m_nOldConditions, 0, 0 );

	m_nOldConditions = m_nPlayerCond;

	if ( m_flFlameRemoveTime != m_flOldFlameRemoveTime )
	{
		m_pOuter->m_flBurnRenewTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes
//-----------------------------------------------------------------------------
void CTDCPlayerShared::SyncConditions( int nCond, int nOldCond, int nUnused, int iOffset )
{
	if ( nCond == nOldCond )
		return;

	int nCondChanged = nCond ^ nOldCond;
	int nCondAdded = nCondChanged & nCond;
	int nCondRemoved = nCondChanged & nOldCond;

	int i;
	for ( i = 0; i < 32; i++ )
	{
		if ( nCondAdded & (1<<i) )
		{
			OnConditionAdded( (ETDCCond)( i + iOffset ) );
		}
		else if ( nCondRemoved & (1<<i) )
		{
			OnConditionRemoved( (ETDCCond)( i + iOffset ) );
		}
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTDCPlayerShared::RemoveAllCond( void )
{
	int i;
	for ( i = 0; i < TDC_COND_LAST; i++ )
	{
		if ( InCond( (ETDCCond)i ) )
		{
			RemoveCond( (ETDCCond)i );
		}
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it recieves the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnConditionAdded( ETDCCond nCond )
{
	switch ( nCond )
	{
	case TDC_COND_POWERUP_CLOAK:
		OnAddStealthed();
		break;

	case TDC_COND_INVULNERABLE_SPAWN_PROTECT:
		OnAddInvulnerable();
		break;

	case TDC_COND_TELEPORTED:
		OnAddTeleported();
		break;

	case TDC_COND_TAUNTING:
		OnAddTaunting();
		break;

	case TDC_COND_POWERUP_CRITDAMAGE:
#ifdef CLIENT_DLL
		UpdateCritBoostEffect();
#endif
		break;

	case TDC_COND_BURNING:
		OnAddBurning();
		break;

	case TDC_COND_HEALTH_OVERHEALED:
#ifdef CLIENT_DLL
		m_pOuter->UpdateOverhealEffect();
#endif
		break;

	case TDC_COND_STUNNED:
		OnAddStunned();
		break;

	case TDC_COND_POWERUP_RAGEMODE:
		OnAddRagemode();
		break;

	case TDC_COND_POWERUP_SPEEDBOOST:
		OnAddSpeedBoost();
		break;

	case TDC_COND_LASTSTANDING:
#ifdef GAME_DLL
		m_pOuter->TakeHealth( m_pOuter->GetMaxHealth() * 2, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
#endif
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TDC_COND_SOFTZOOM:
		m_pOuter->SetFOV( m_pOuter, tdc_zoom_fov.GetInt(), 0.2f );
		break;

	case TDC_COND_SPRINT:
		OnAddSprint();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it recieves the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnConditionRemoved( ETDCCond nCond )
{
	switch ( nCond )
	{
	case TDC_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TDC_COND_POWERUP_CLOAK:
		OnRemoveStealthed();
		FadeInvis( 2.0f, false );
		break;

	case TDC_COND_INVULNERABLE_SPAWN_PROTECT:
		OnRemoveInvulnerable();
		break;

	case TDC_COND_TELEPORTED:
		OnRemoveTeleported();
		break;

	case TDC_COND_TAUNTING:
		OnRemoveTaunting();
		break;

	case TDC_COND_POWERUP_CRITDAMAGE:
#ifdef CLIENT_DLL
		UpdateCritBoostEffect();
#endif
		break;

	case TDC_COND_BURNING:
		OnRemoveBurning();
		break;

	case TDC_COND_HEALTH_OVERHEALED:
#ifdef CLIENT_DLL
		m_pOuter->UpdateOverhealEffect();
#endif
		break;

	case TDC_COND_STUNNED:
		OnRemoveStunned();
		break;

	case TDC_COND_AIRBLASTED:
		m_vecAirblastPos = vec3_origin;
		break;

	case TDC_COND_POWERUP_RAGEMODE:
		OnRemoveRagemode();
		break;

	case TDC_COND_POWERUP_SPEEDBOOST:
		OnRemoveSpeedBoost();
		break;

	case TDC_COND_LASTSTANDING:
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TDC_COND_SOFTZOOM:
		m_pOuter->SetFOV( m_pOuter, 0, 0.2f );
		break;

	case TDC_COND_SPRINT:
		OnRemoveSprint();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCPlayerShared::GetMaxBuffedHealth( void ) const
{
	float flBoostMax = m_pOuter->GetMaxHealth() * 2.0f;
	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	return iRoundDown;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::HealNegativeConds( void )
{
	static ETDCCond aConds[] =
	{
		TDC_COND_BURNING,
		//TDC_COND_BLEEDING,
		//TDC_COND_TRANQUILIZED,
	};

	bool bSuccess = false;

	for ( int i = 0; i < ARRAYSIZE( aConds ); i++ )
	{
		if ( RemoveCondIfPresent( aConds[i] ) )
			bSuccess = true;
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCPlayerShared::GetPowerupFlags( void )
{
	int nFlags = 0;

	for ( int i = 0; g_aPowerups[i].cond != TDC_COND_LAST; i++ )
	{
		if ( InCond( g_aPowerups[i].cond ) )
			nFlags |= ( 1 << i );
	}

	return nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTDCPlayerShared::ConditionGameRulesThink( void )
{
#ifdef GAME_DLL
	for ( int i = 0; i < TDC_COND_LAST; i++ )
	{
		if ( InCond( (ETDCCond)i ) )
		{
			// Ignore permanent conditions
			if ( m_flCondExpireTimeLeft[i] != PERMANENT_CONDITION )
			{
				m_flCondExpireTimeLeft.Set( i, Max( m_flCondExpireTimeLeft[i] - gpGlobals->frametime, 0.0f ) );

				if ( m_flCondExpireTimeLeft[i] == 0 )
				{
					RemoveCond( (ETDCCond)i );
				}
			}
		}
	}

	if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
	{
		if ( !InCond( TDC_COND_HEALTH_OVERHEALED ) )
		{
			AddCond( TDC_COND_HEALTH_OVERHEALED );
		}
	}
	else
	{
		if ( InCond( TDC_COND_HEALTH_OVERHEALED ) )
		{
			RemoveCond( TDC_COND_HEALTH_OVERHEALED );
		}
	}

	// Taunt
	if ( InCond( TDC_COND_TAUNTING ) )
	{
		if ( m_flTauntRemoveTime >= 0.0f && gpGlobals->curtime > m_flTauntRemoveTime )
		{
			//m_pOuter->SnapEyeAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );

			RemoveCond( TDC_COND_TAUNTING );
		}
	}

	if ( InCond( TDC_COND_BURNING ) )
	{
		// If we're underwater, put the fire out
		if ( gpGlobals->curtime > m_flFlameRemoveTime || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			RemoveCond( TDC_COND_BURNING );
		}
		else if ( gpGlobals->curtime >= m_flFlameBurnTime )
		{
			// Burn the player
			CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, m_hBurnWeapon, 3.0f, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, TDC_DMG_CUSTOM_BURNING );
			m_pOuter->TakeDamage( info );
			m_flFlameBurnTime = gpGlobals->curtime + 0.5f;
		}
		else if ( tdc_player_burn_spread.GetBool() )
		{
			const Vector pOrigin =  m_pOuter->GetAbsOrigin();
			float flBurnSpreadRadius = tdc_player_burn_spread_radius.GetFloat();
			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );

				if ( !pPlayer )
					continue;

				if ( pPlayer == this->m_pOuter )
					continue;

				if ( pPlayer->m_Shared.InCond( TDC_COND_BURNING ) )
					continue;

				const Vector pTargetOrigin = pPlayer->GetAbsOrigin();
				float flDistance = pTargetOrigin.DistTo( pOrigin );

				if ( flDistance > flBurnSpreadRadius )
					continue;

				pPlayer->m_Shared.SpreadFire( m_pOuter );

				IGameEvent *event = gameeventmanager->CreateEvent( "player_fire_spread" );
				if ( event )
				{
					event->SetInt( "source", m_pOuter->GetUserID() );
					event->SetInt( "target", pPlayer->GetUserID() );
					event->SetFloat( "burntime", pPlayer->m_Shared.m_flFlameRemoveTime - gpGlobals->curtime );
					gameeventmanager->FireEvent( event );
				}
			}
		}

		if ( m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5;
		}
	}

	if ( InCond( TDC_COND_STEALTHED_BLINK ) )
	{
		if ( 0.3f < ( gpGlobals->curtime - m_flLastStealthExposeTime ) )
		{
			RemoveCond( TDC_COND_STEALTHED_BLINK );
		}
	}

	if ( InCond( TDC_COND_STUNNED ) )
	{
		if ( gpGlobals->curtime > m_flStunExpireTime && m_iStunPhase == STUN_PHASE_END )
		{
			RemoveCond( TDC_COND_STUNNED );
		}
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveZoomed( void )
{
#ifdef GAME_DLL
	m_pOuter->SetFOV( m_pOuter, 0, 0.1f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddInvulnerable( void )
{
#ifndef CLIENT_DLL
	// Stock uber removes negative conditions.
	RemoveCondIfPresent( TDC_COND_BURNING );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveInvulnerable( void )
{
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::ShouldShowRecentlyTeleported( void )
{
	return ( InCond( TDC_COND_TELEPORTED ) && !InCond( TDC_COND_TELEPORTED ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddTaunting( void )
{
	CTDCWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();
	if ( pWpn )
	{
		// cancel any reload in progress.
		pWpn->AbortReload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveTaunting( void )
{
#ifdef GAME_DLL
	m_pOuter->StopTaunt();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddStunned( void )
{
	CTDCWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();

	if ( pWeapon )
	{
		pWeapon->OnControlStunned();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveStunned( void )
{
	m_flStunExpireTime = 0.0f;
	m_hStunner = NULL;
	m_iStunPhase = 0;

	CTDCWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();

	if ( pWeapon )
	{
		pWeapon->SetWeaponVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddRagemode( void )
{
#ifdef GAME_DLL
	CTDCWeaponBase *pWeapon = (CTDCWeaponBase *)m_pOuter->GiveNamedItem( "weapon_hammerfists" );
	if ( pWeapon )
	{
		m_pOuter->Weapon_Switch( pWeapon );
	}

	m_pOuter->TeamFortress_SetSpeed();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveRagemode( void )
{
#ifdef GAME_DLL
	CTDCWeaponBase *pWeapon = m_pOuter->Weapon_OwnsThisID( WEAPON_HAMMERFISTS );

	if ( pWeapon )
	{
		m_pOuter->SwitchToNextBestWeapon( NULL );
		pWeapon->UnEquip();
	}

	m_pOuter->TeamFortress_SetSpeed();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddSpeedBoost( void )
{
#ifdef CLIENT_DLL
	if ( !m_pSpeedEffect )
	{
		m_pSpeedEffect = m_pOuter->ParticleProp()->Create( "speed_boost_trail", PATTACH_ABSORIGIN_FOLLOW );
	}
#endif

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveSpeedBoost( void )
{
#ifdef CLIENT_DLL
	if ( m_pSpeedEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pSpeedEffect );
		m_pSpeedEffect = NULL;
	}
#else
	CSingleUserRecipientFilter filter( m_pOuter );
	m_pOuter->EmitSound( filter, m_pOuter->entindex(), "PowerupSpeedBoost.WearOff" );
#endif

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddSprint( void )
{
	m_flSprintStartTime = gpGlobals->curtime;
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveSprint( void )
{
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::Burn( CTDCPlayer *pAttacker, CTDCWeaponBase *pWeapon /*= NULL*/, float flBurnTime /*= 10.0f*/ )
{
#ifdef CLIENT_DLL

#else
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	if ( !InCond( TDC_COND_BURNING ) )
	{
		// Start burning
		AddCond( TDC_COND_BURNING );
		m_flFlameBurnTime = gpGlobals->curtime;	//asap
		// let the attacker know he burned me
		if ( pAttacker )
		{
			pAttacker->OnBurnOther( m_pOuter );
		}
	}

	m_flFlameRemoveTime = gpGlobals->curtime + flBurnTime;
	m_hBurnAttacker = pAttacker;
	m_hBurnWeapon = pWeapon;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::SpreadFire( CTDCPlayer *pAttacker, float flBurnTime /*= NULL*/ )
{
#ifdef CLIENT_DLL

#else
	if ( !flBurnTime )
		flBurnTime = ( pAttacker->m_Shared.m_flFlameRemoveTime + tdc_player_burn_spread_addtime.GetFloat() ) - gpGlobals->curtime;

	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	if ( TDCGameRules()->IsTeamplay() && m_pOuter->GetTeamNumber() == pAttacker->GetTeamNumber() )
		return;

	Burn( pAttacker, NULL, flBurnTime );
	SetDamageSourceType( TDC_DMG_SOURCE_PLAYER_SPREAD );
	
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::StunPlayer( float flDuration, CTDCPlayer *pStunner )
{
#ifdef GAME_DLL
	m_flStunExpireTime = Max( m_flStunExpireTime.Get(), gpGlobals->curtime + flDuration );
	m_hStunner = pStunner;
	AddCond( TDC_COND_STUNNED );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::AirblastPlayer( CTDCPlayer *pAttacker, const Vector &vecDir, float flSpeed )
{
	m_pOuter->SetGroundEntity( NULL );
	m_pOuter->ApplyAbsVelocityImpulse( vecDir * flSpeed );
	m_vecAirblastPos = pAttacker->GetAbsOrigin();
	AddCond( TDC_COND_AIRBLASTED, 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::ShovePlayer( CTDCPlayer *pAttacker, const Vector &vecDir, float flSpeed )
{
	m_pOuter->SetGroundEntity( NULL );
	m_pOuter->ApplyAbsVelocityImpulse( vecDir * flSpeed );
	m_vecAirblastPos = pAttacker->GetAbsOrigin();
	AddCond( TDC_COND_AIRBLASTED, 0.5f );

#ifdef GAME_DLL
	m_pOuter->AddDamagerToHistory( pAttacker );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveBurning( void )
{
#ifdef CLIENT_DLL
	if ( m_pBurningSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBurningSound );
		m_pBurningSound = NULL;
	}

	if ( m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pBurningEffect );
		m_pBurningEffect = NULL;
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
#else
	if ( !InState( TDC_STATE_DYING ) )
		ResetDamageSourceType();
	m_hBurnAttacker = NULL;
	m_hBurnWeapon = NULL;
	if ( gpGlobals->curtime > m_flFlameRemoveTime )
	{
		m_pOuter->EmitSound( "BurnSpeedBoost.WearOff" );
	} else {
		m_pOuter->EmitSound( "BurnSpeedBoost.Extinguish" );
	}
#endif
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddStealthed( void )
{
#ifdef CLIENT_DLL
	m_pOuter->RemoveAllDecals();
	m_pOuter->UpdateSpyStateChange();
	m_flInvisChangeCompleteTime = gpGlobals->curtime + 1.0f;
#else
	m_pOuter->EmitSound( "Player.Spy_Cloak" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnRemoveStealthed( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateSpyStateChange();
#else
	m_pOuter->EmitSound( "Player.Spy_UnCloak" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnAddBurning( void )
{
#ifdef CLIENT_DLL
	// Start the burning effect
	if ( !m_pBurningEffect )
	{
		const char *pszEffectName = ConstructTeamParticle( "burningplayer_%s", m_pOuter->GetTeamNumber(), true );
		m_pBurningEffect = m_pOuter->ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
		SetParticleToMercColor( m_pBurningEffect );

		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
	}

	// Start the looping burn sound
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( !m_pBurningSound )
	{
		CLocalPlayerFilter filter;
		m_pBurningSound = controller.SoundCreate( filter, m_pOuter->entindex(), "Player.OnFire" );
	}

	controller.Play( m_pBurningSound, 0.0, 100 );
	controller.SoundChangeVolume( m_pBurningSound, 1.0, 0.1 );
#else
	// play a fire-starting sound
	m_pOuter->EmitSound( "Fire.Engulf" );
	m_pOuter->EmitSound( "BurnSpeedBoost.Start" );
#endif
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
void CTDCPlayerShared::SetPlayerDominated( CTDCPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominated.Set( iPlayerIndex, bDominated );
	pPlayer->m_Shared.SetPlayerDominatingMe( m_pOuter, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player
//-----------------------------------------------------------------------------
void CTDCPlayerShared::SetPlayerDominatingMe( CTDCPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominatingMe.Set( iPlayerIndex, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsPlayerDominated( int iPlayerIndex )
{
	return m_bPlayerDominated.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerShared::NoteLastDamageTime( int nDamage )
{
	if ( !IsStealthed() )
		return;

	// we took damage
	if ( nDamage > 5 )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TDC_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerShared::OnSpyTouchedByEnemy( void )
{
	m_flLastStealthExposeTime = gpGlobals->curtime;
	AddCond( TDC_COND_STEALTHED_BLINK );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerShared::FadeInvis( float flInvisFadeTime, bool bNoAttack /*= false*/ )
{
	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTDCPlayerShared::InvisibilityThink( void )
{
	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TDC_COND_STEALTHED_BLINK ) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = 0.5f;
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f - ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) );
		}
		else
		{
			flTargetInvis = ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) * 0.5f );
		}
	}
	else
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f;
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	}

	flTargetInvis *= flTargetInvisScale;

	m_flInvisibility = clamp( flTargetInvis, 0.0f, 0.95f );
}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player [0..1]
//-----------------------------------------------------------------------------
float CTDCPlayerShared::GetPercentInvisible( void )
{
	return m_flInvisibility;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerShared::UpdateLoopingSounds( bool bDormant )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( bDormant )
	{
		// Pause sounds.
		if ( m_pBurningSound )
		{
			controller.Shutdown( m_pBurningSound );
		}

		if ( m_pCritSound )
		{
			controller.Shutdown( m_pCritSound );
		}
	}
	else
	{
		// Resume any sounds that should still be playing.
		if ( m_pBurningSound )
		{
			controller.Play( m_pBurningSound, 1.0f, 100.0f );
		}

		if ( m_pCritSound )
		{
			controller.Play( m_pCritSound, 1.0f, 100.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Crit effects handling.
//-----------------------------------------------------------------------------
void CTDCPlayerShared::UpdateCritBoostEffect( void )
{
	bool bShouldShow = true;

	if ( m_pOuter->IsDormant() )
	{
		bShouldShow = false;
	}
	else if ( !IsCritBoosted() )
	{
		bShouldShow = false;
	}
	else if ( IsStealthed() )
	{
		bShouldShow = false;
	}

	if ( IsCritBoosted() && !m_pOuter->IsDormant() && !IsStealthed() )
	{
		// Update crit effect model.
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );

			m_pCritEffect = NULL;
		}

		if ( !m_pOuter->ShouldDrawThisPlayer() )
		{
			m_hCritEffectHost = m_pOuter->GetViewModel( 0, false );
		}
		else
		{
			C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();

			// Don't add crit effect to weapons without a model.
			if ( pWeapon && pWeapon->GetWorldModelIndex() != 0 )
			{
				m_hCritEffectHost = pWeapon;
			}
			else
			{
				m_hCritEffectHost = m_pOuter;
			}
		}

		if ( m_hCritEffectHost.Get() )
		{
			const char *pszEffect = ConstructTeamParticle( "critgun_weaponmodel_%s", m_pOuter->GetTeamNumber(), true, g_aTeamNamesShort );
			m_pCritEffect = m_hCritEffectHost->ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
			SetParticleToMercColor( m_pCritEffect );
		}

		if ( !m_pCritSound )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			CLocalPlayerFilter filter;
			m_pCritSound = controller.SoundCreate( filter, m_pOuter->entindex(), "Weapon_General.CritPower" );
			controller.Play( m_pCritSound, 1.0, 100 );
		}
	}
	else
	{
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );

			m_pCritEffect = NULL;
		}

		m_hCritEffectHost = NULL;

		if ( m_pCritSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pCritSound );
			m_pCritSound = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::SetParticleToMercColor( CNewParticleEffect *pParticle )
{
	if ( pParticle && TDCGameRules() && !TDCGameRules()->IsTeamplay() )
	{
		pParticle->SetControlPoint( CUSTOM_COLOR_CP1, m_pOuter->m_vecPlayerColor );
		return true;
	}

	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCWeaponBase *CTDCPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine if player should do loser animations.
//-----------------------------------------------------------------------------
bool CTDCPlayerShared::IsLoser( void )
{
	if ( !m_pOuter->IsAlive() )
		return false;

	if ( tdc_always_loser.GetBool() )
		return true;

	if ( m_pOuter->GetActiveWeapon() == NULL )
		return true;

	if ( TDCGameRules() && TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		return ( TDCGameRules()->GetWinningTeam() != m_pOuter->GetTeamNumber() );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::IncrementAirDucks( void )
{
	m_nAirDucked++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerShared::ResetAirDucks( void )
{
	m_nAirDucked = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayerShared::GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom )
{
	const char *pszSequence = NULL;

	switch( iDamageCustom )
	{
	case TDC_DMG_CUSTOM_BACKSTAB:
		pszSequence = "primary_death_backstab";
		break;
	case TDC_DMG_CUSTOM_HEADSHOT:
		pszSequence = "primary_death_headshot";
		break;
	case TDC_DMG_CUSTOM_BURNING:
		// Disabled until we get shorter burning death animations.
		//pszSequence = "primary_death_burning";
		break;
	}

	if ( pszSequence != NULL )
	{
		return pAnim->LookupSequence( pszSequence );
	}

	return -1;
}

//=============================================================================
//
// Shared player code that isn't CTDCPlayerShared
//
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ||
		collisionGroup == TDC_COLLISION_GROUP_ROCKETS ||
		collisionGroup == TDC_COLLISION_GROUP_PUMPKINBOMBS )
	{
		switch ( GetTeamNumber() )
		{
		case TDC_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TDC_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		}
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCWeaponBase *CTDCPlayer::GetActiveTFWeapon( void ) const
{
	return assert_cast<CTDCWeaponBase *>( GetActiveWeapon() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsActiveTFWeapon( ETDCWeaponID iWeaponID ) const
{
	CTDCWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetWeaponID() == iWeaponID;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::TeamFortress_SetSpeed()
{
	float maxfbspeed;

	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			SetMaxSpeed( 400.0f );
		else
			SetMaxSpeed( 0 );
		return;
	}

	// Check for any reason why they can't move at all
	if ( GameRules()->InRoundRestart() )
	{
		SetMaxSpeed( 1 );
		return;
	}

	// First, get their max class speed
	if ( !HasTheFlag() )
	{
		maxfbspeed = GetPlayerClass()->GetData()->m_flMaxSpeed;
	}
	else
	{
		maxfbspeed = GetPlayerClass()->GetData()->m_flFlagSpeed; 
	}

	// 50% speed boost from the power-up.
	if ( m_Shared.IsHasting() )
	{
		maxfbspeed *= TDC_POWERUP_SPEEDBOOST_FACTOR;
	}

	if ( m_Shared.InCond( TDC_COND_BURNING ) )
	{
		maxfbspeed *= tdc_player_burn_speedboostfactor.GetFloat();
	}

	if ( m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
	{
		maxfbspeed *= 1.3f;
	}

	if ( m_Shared.InCond( TDC_COND_SPRINT ) )
	{
		float scale = Min( ( gpGlobals->curtime - m_Shared.m_flSprintStartTime ) / tdc_player_sprint_accel_time.GetFloat(), 1.0f);
		// Square it to ease in
		scale *= scale;
		maxfbspeed += scale *tdc_player_sprint_gain.GetInt();
	}

	// if we're in bonus time because a team has won, give the winners 110% speed and the losers 90% speed
	if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TDCGameRules()->GetWinningTeam();

		if ( iWinner != TEAM_UNASSIGNED )
		{
			if ( iWinner == GetTeamNumber() )
			{
				maxfbspeed *= 1.1f;
			}
			else
			{
				maxfbspeed *= 0.9f;
			}
		}
	}

	// Set the speed
	SetMaxSpeed( maxfbspeed );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayer::HasItem( void ) const
{
	return ( m_hItem.Get() != NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::SetItem( CTDCItem *pItem )
{
#ifdef GAME_DLL
	CTDCItem *pOldItem = m_hItem.Get();
#endif

	m_hItem = pItem;

#ifndef CLIENT_DLL
	if ( pItem )
	{
		if ( pItem->GetItemID() == TDC_ITEM_CAPTURE_FLAG )
		{
			// Give us the flag weapon.
			CTDCWeaponBase *pWeapon = (CTDCWeaponBase *)GiveNamedItem( "weapon_flag" );
			if ( pWeapon )
			{
				Weapon_Switch( pWeapon );
			}
		}
	}
	else if ( pOldItem )
	{
		if ( pOldItem->GetItemID() == TDC_ITEM_CAPTURE_FLAG )
		{
			// Holster and remove the flag weapon.
			CTDCWeaponBase *pWeapon = Weapon_OwnsThisID( WEAPON_FLAG );
			if ( pWeapon )
			{
				if ( pWeapon == GetActiveWeapon() )
				{
					SwitchToNextBestWeapon( NULL );
				}

				pWeapon->UnEquip();
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCItem	*CTDCPlayer::GetItem( void ) const
{
	return m_hItem;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CCaptureFlag *CTDCPlayer::GetTheFlag( void ) const
{
	if ( HasTheFlag() )
	{
		return assert_cast<CCaptureFlag *>( m_hItem.Get() );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player allowed to use a teleporter ?
//-----------------------------------------------------------------------------
bool CTDCPlayer::HasTheFlag( void ) const
{
	if ( HasItem() && GetItem()->GetItemID() == TDC_ITEM_CAPTURE_FLAG )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Are we allowed to pick the flag up?
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsAllowedToPickUpFlag( void ) const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTDCPlayer::Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	// if the weapon doesn't want to be auto-switched to, don't!	
	CTDCWeaponBase *pTFWeapon = dynamic_cast<CTDCWeaponBase *>( pOldWeapon );

	if ( pTFWeapon->AllowsAutoSwitchTo() == false )
	{
		return false;
	}

	return BaseClass::Weapon_ShouldSetLast( pOldWeapon, pNewWeapon );
}


//-----------------------------------------------------------------------------
// Purpose: Return true if we should always switch to or from this weapon
//-----------------------------------------------------------------------------
bool ShouldAlwaysSwitch(CTDCWeaponBase *pWeapon)
{
	if (!pWeapon)
		return false;

	ETDCWeaponID weaponID  =pWeapon->GetWeaponID();
	return (weaponID == WEAPON_FLAG || weaponID == WEAPON_HAMMERFISTS);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	if ( m_Shared.InCond( TDC_COND_SPRINT ) )
	{
		CTDCWeaponBase *pTDCWeapon = dynamic_cast<CTDCWeaponBase *>( pWeapon );
		CTDCWeaponBase *pActiveWeapon = GetActiveTFWeapon();
		if (!ShouldAlwaysSwitch(pTDCWeapon) && !ShouldAlwaysSwitch(pActiveWeapon))
		{
			return false;
		}
	}

	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
	return BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
	// Don't play footstep sounds while crouched.
	if ( GetFlags() & FL_DUCKING )
		return;

#ifdef CLIENT_DLL
	// In third person, only make footstep sounds through animevents.
	if ( m_flStepSoundTime != -1.0f )
	{
		if ( ShouldDrawThisPlayer() )
			return;
	}
	else
	{
		m_flStepSoundTime = 0.0f;
	}
#endif

	BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::GetStepSoundVelocities( float *velwalk, float *velrun )
{
	float flMaxSpeed = MaxSpeed();

	if ( ( GetFlags() & FL_DUCKING ) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		*velwalk = flMaxSpeed * 0.25;
		*velrun = flMaxSpeed * 0.3;
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3;
		*velrun = flMaxSpeed * 0.8;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking )
{
	float flMaxSpeed = MaxSpeed();

	switch ( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 400, 200 );
		if ( bWalking )
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 600, 400 );
		break;

	default:
		Assert( 0 );
		break;
	}

	if ( ( GetFlags() & FL_DUCKING ) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		m_flStepSoundTime += 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::CanAttack( void )
{
	if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN && TDCGameRules()->GetWinningTeam() != GetTeamNumber() )
	{
		return false;
	}

	if ( m_Shared.InCond( TDC_COND_STUNNED ) )
	{
		return false;
	}

	if ( TDCGameRules()->State_Get() == GR_STATE_PREROUND )
	{
		return false;
	}

	if ( m_Shared.InCond( TDC_COND_SPRINT ) )
	{
		return false;
	}


	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CTDCPlayer::GetClassEyeHeight( void )
{
	int iClass = m_PlayerClass.GetClassIndex();

	if ( iClass < TDC_FIRST_NORMAL_CLASS || iClass >= TDC_CLASS_COUNT_ALL )
		return VEC_VIEW_SCALED( this );

	float flHeight = GetPlayerClassData( iClass )->m_flViewHeight * GetModelScale();
	return Vector( 0, 0, flHeight );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCWeaponBase *CTDCPlayer::Weapon_OwnsThisID( ETDCWeaponID iWeaponID ) const
{
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTDCWeaponBase *pWpn = GetTDCWeapon( i );

		if ( pWpn == NULL )
			continue;

		if ( pWpn->GetWeaponID() == iWeaponID )
		{
			return pWpn;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Use this when checking if we can damage this player or something similar.
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsEnemy( const CBaseEntity *pEntity ) const
{
#if 0
	// Spectators are nobody's enemy.
	if ( m_pOuter->GetTeamNumber() < FIRST_GAME_TEAM || pEntity->GetTeamNumber() < FIRST_GAME_TEAM )
		return false;
#endif

	if ( !TDCGameRules()->IsTeamplay() )
		return true;

	return ( pEntity->GetTeamNumber() != GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::RemoveAmmo( int iCount, int iAmmoIndex )
{
	// Last Standing player in Infection has infinite ammo.
	if ( HasInfiniteAmmo() )
		return;

	BaseClass::RemoveAmmo( iCount, iAmmoIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCPlayer::GetAmmoCount( int iAmmoIndex ) const
{
	// Last Standing player in Infection has infinite ammo.
	if ( HasInfiniteAmmo() )
		return 999;

	return BaseClass::GetAmmoCount( iAmmoIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::GetMaxAmmo( int iAmmoIndex, bool bAddMissingClip /*= false*/ ) const
{
	int iMaxAmmo = GetPlayerClass()->GetData()->m_aAmmoMax[iAmmoIndex];
	int iMissingClip = 0;

	// If we have a weapon that overrides max ammo, use its value.
	// BUG: If player has multiple weapons using same ammo type then only the first one's value is used.
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTDCWeaponBase *pWeapon = GetTDCWeapon( i );

		if ( pWeapon && pWeapon->GetPrimaryAmmoType() == iAmmoIndex )
		{
			int iCustomMaxAmmo = pWeapon->GetMaxAmmo();
			if ( iCustomMaxAmmo )
			{
				iMaxAmmo = iCustomMaxAmmo;
			}

			// Allow getting ammo above maximum if the clip is not full ala L4D.
			if ( bAddMissingClip && pWeapon->UsesClipsForAmmo1() )
			{
				iMissingClip = pWeapon->GetMaxClip1() - pWeapon->Clip1();
			}

			break;
		}
	}

	return iMaxAmmo + iMissingClip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::HasInfiniteAmmo( void ) const
{
	return ( tdc_infinite_ammo.GetBool() ||
		TDCGameRules()->IsInstagib() ||
		m_Shared.InCond( TDC_COND_LASTSTANDING ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::JumpSound( void )
{
	const char *pszJumpSound = GetPlayerClass()->GetData()->m_szJumpSound;

	CPASAttenuationFilter filter( this, pszJumpSound );
	filter.UsePredictionRules();
	EmitSound( filter, entindex(), pszJumpSound );
}

//-----------------------------------------------------------------------------
// Purpose: Get the player from whose perspective we're viewing the world from.
//-----------------------------------------------------------------------------
CTDCPlayer *CTDCPlayer::GetObservedPlayer( bool bFirstPerson )
{
	if ( GetObserverMode() == OBS_MODE_IN_EYE || ( !bFirstPerson && GetObserverMode() == OBS_MODE_CHASE ) )
	{
		CTDCPlayer *pTarget = ToTDCPlayer( GetObserverTarget() );

		if ( pTarget )
		{
			return pTarget;
		}
	}

	return this;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CTDCPlayer::Weapon_ShootPosition( void )
{
	// If we're in over the shoulder third person fire from the side so it matches up with camera.
	if ( m_nButtons & IN_THIRDPERSON )
	{
		Vector vecForward, vecRight, vecUp, vecOffset;
		AngleVectors( EyeAngles(), &vecForward, &vecRight, &vecUp );
		vecOffset.Init( 0.0f, TDC_CAMERA_DIST_RIGHT, TDC_CAMERA_DIST_UP );

		if ( ShouldFlipViewModel() )
		{
			vecOffset.y *= -1.0f;
		}

		return ( EyePosition() + vecOffset.y * vecRight + vecOffset.z * vecUp );
	}

	return BaseClass::Weapon_ShootPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Sets player's eye angles - not camera angles!
//-----------------------------------------------------------------------------
void CTDCPlayer::SetEyeAngles( const QAngle &angles )
{
#ifdef GAME_DLL
	if ( pl.fixangle != FIXANGLE_NONE )
		return;
#endif

	pl.v_angle = m_angEyeAngles = angles;
}

//-----------------------------------------------------------------------------
// Purpose: Like CBaseEntity::HealthFraction, but uses GetMaxBuffedHealth instead of GetMaxHealth
//-----------------------------------------------------------------------------
float CTDCPlayer::HealthFractionBuffed() const
{
	int iMaxBuffedHealth = m_Shared.GetMaxBuffedHealth();
	if ( iMaxBuffedHealth == 0 )
		return 1.0f;

	float flFraction = ( float )GetHealth() / ( float )iMaxBuffedHealth;
	return clamp( flFraction, 0.0f, 1.0f );
}
