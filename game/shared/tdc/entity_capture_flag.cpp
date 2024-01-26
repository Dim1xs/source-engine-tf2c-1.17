//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTDC Flag.
//
//=============================================================================//
#include "cbase.h"
#include "entity_capture_flag.h"
#include "tdc_gamerules.h"
#include "tdc_shareddefs.h"

#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include "hudelement.h"
#include "iclientmode.h"
#include "hud_numericdisplay.h"
#include "tdc_imagepanel.h"
#include "c_tdc_player.h"
#include "c_tdc_team.h"
#include "tdc_hud_objectivestatus.h"
#include "view.h"
#include "glow_outline_effect.h"
#include "functionproxy.h"

ConVar cl_flag_return_size( "cl_flag_return_size", "20", FCVAR_CHEAT );

#else
#include "tdc_player.h"
#include "tdc_team.h"
#include "tdc_objective_resource.h"
#include "tdc_gamestats.h"
#include "func_respawnroom.h"
#include "datacache/imdlcache.h"
#include "func_respawnflag.h"
#include "func_flagdetectionzone.h"
#include "tdc_announcer.h"

ConVar cl_flag_return_height( "cl_flag_return_height", "82", FCVAR_CHEAT );
#endif

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Used to make flag glow with color of its carrier.
//-----------------------------------------------------------------------------
class CProxyFlagGlow : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		Vector vecColor( 1, 1, 1 );

		C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity->GetMoveParent() );

		if ( pPlayer )
		{
			switch ( pPlayer->GetTeamNumber() )
			{
			case TDC_TEAM_RED:
				vecColor = Vector( 1.7f, 0.5f, 0.5f );
				break;
			case TDC_TEAM_BLUE:
				vecColor = Vector( 0.7f, 0.7f, 1.9f );
				break;
			}
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};

EXPOSE_INTERFACE( CProxyFlagGlow, CResultProxy, "FlagGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif

//=============================================================================
//
// CTDC Flag tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlag, DT_CaptureFlag )

BEGIN_NETWORK_TABLE( CCaptureFlag, DT_CaptureFlag )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropInt( SENDINFO( m_nFlagStatus ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flResetTime ) ),
	SendPropTime( SENDINFO( m_flNeutralTime ) ),
	SendPropTime( SENDINFO( m_flResetDelay ) ),
	SendPropEHandle( SENDINFO( m_hPrevOwner ) ),
	SendPropInt( SENDINFO( m_nUseTrailEffect ), 2, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_szHudIcon ) ),
	SendPropString( SENDINFO( m_szPaperEffect ) ),
	SendPropBool( SENDINFO( m_bVisibleWhenDisabled ) ),
	SendPropBool( SENDINFO( m_bGlowEnabled ) ),
	SendPropBool( SENDINFO( m_bLocked ) ),
	SendPropTime( SENDINFO( m_flUnlockTime ) ),
	SendPropTime( SENDINFO( m_flUnlockDelay ) ),
	SendPropString( SENDINFO( m_szViewModel ) ),
#else
	RecvPropInt( RECVINFO( m_bDisabled ) ),
	RecvPropInt( RECVINFO( m_nFlagStatus ) ),
	RecvPropTime( RECVINFO( m_flResetTime ) ),
	RecvPropTime( RECVINFO( m_flNeutralTime ) ),
	RecvPropTime( RECVINFO( m_flResetDelay ) ),
	RecvPropEHandle( RECVINFO( m_hPrevOwner ) ),
	RecvPropInt( RECVINFO( m_nUseTrailEffect ) ),
	RecvPropString( RECVINFO( m_szHudIcon ) ),
	RecvPropString( RECVINFO( m_szPaperEffect ) ),
	RecvPropBool( RECVINFO( m_bVisibleWhenDisabled ) ),
	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
	RecvPropBool( RECVINFO( m_bLocked ) ),
	RecvPropTime( RECVINFO( m_flUnlockTime ) ),
	RecvPropTime( RECVINFO( m_flUnlockDelay ) ),
	RecvPropString( RECVINFO( m_szViewModel ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CCaptureFlag )

	// Keyfields.
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_flResetDelay, FIELD_FLOAT, "ReturnTime" ),
	DEFINE_KEYFIELD( m_szModel, FIELD_STRING, "flag_model" ),
	DEFINE_KEYFIELD( m_szTrailEffect, FIELD_STRING, "flag_trail" ),
	DEFINE_KEYFIELD( m_nUseTrailEffect, FIELD_INTEGER, "trail_effect" ),
	DEFINE_KEYFIELD( m_bVisibleWhenDisabled, FIELD_BOOLEAN, "VisibleWhenDisabled" ),
	DEFINE_KEYFIELD( m_bLocked, FIELD_BOOLEAN, "StartLocked" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDrop", InputForceDrop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceReset", InputForceReset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceResetSilent", InputForceResetSilent ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceResetAndDisableSilent", InputForceResetAndDisableSilent ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetReturnTime", InputSetReturnTime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ShowTimer", InputShowTimer ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "ForceGlowDisabled", InputForceGlowDisabled ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetLocked", InputSetLocked ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetUnlockTime", InputSetUnlockTime ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnReturn, "OnReturn" ),
	DEFINE_OUTPUT( m_outputOnPickUp, "OnPickUp" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam1, "OnPickupTeam1" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam2, "OnPickupTeam2" ),
	DEFINE_OUTPUT( m_outputOnDrop, "OnDrop" ),
	DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_outputOnCapTeam1, "OnCapTeam1" ),
	DEFINE_OUTPUT( m_outputOnCapTeam2, "OnCapTeam2" ),
	DEFINE_OUTPUT( m_outputOnTouchSameTeam, "OnTouchSameTeam" ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( item_teamflag, CCaptureFlag );

//=============================================================================
//
// CTDC Flag functions.
//

CCaptureFlag::CCaptureFlag()
{
#ifdef CLIENT_DLL
	m_pPaperTrailEffect = NULL;
	m_pGlowEffect = NULL;
	m_bWasGlowEnabled = false;

	ListenForGameEvent( "localplayer_changeteam" );
#else
	m_hReturnIcon = NULL;
	m_hGlowTrail = NULL;
	m_flResetDelay = TDC_CTF_RESET_TIME;
	m_bGlowEnabled = true;

	m_szModel = MAKE_STRING( "models/flag/briefcase.mdl" );
	m_szTrailEffect = MAKE_STRING( "flagtrail" );
	V_strncpy( m_szHudIcon.GetForModify(), "../hud/objectives_flagpanel_carried", MAX_PATH );
	V_strncpy( m_szPaperEffect.GetForModify(), "player_intel_papertrail", MAX_PATH );
#endif	

	UseClientSideAnimation();
	m_nUseTrailEffect = TDC_FLAGEFFECTS_ALL;
	m_flUnlockTime = -1.0f;
}

CCaptureFlag::~CCaptureFlag()
{
#ifdef CLIENT_DLL
	delete m_pGlowEffect;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::FireGameEvent( IGameEvent *event )
{
#ifdef CLIENT_DLL
	if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		UpdateGlowEffect();
	}
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "flag_icon" ) )
	{
		V_strncpy( m_szHudIcon.GetForModify(), szValue, MAX_PATH );
		return true;
	}
	if ( FStrEq( szKeyName, "flag_paper" ) )
	{
		V_strncpy( m_szPaperEffect.GetForModify(), szValue, MAX_PATH );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

static struct FlagModel_t
{
	const char *model;
	const char *viewmodel;
} g_aFlagModels[] =
{
	{ "models/weapons/w_models/w_briefcase.mdl", "models/weapons/v_models/v_briefcase_mercenary.mdl" },
	{ "models/weapons/w_models/w_bomb_flag.mdl", "models/weapons/v_models/v_bomb_flag_mercenary.mdl" },
};

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::Precache( void )
{
	PrecacheModel( STRING( m_szModel ) );

	const char *pszViewmodel = "models/weapons/v_models/v_briefcase_mercenary.mdl";
	for ( int i = 0; i < ARRAYSIZE( g_aFlagModels ); i++ )
	{
		if ( V_stricmp( STRING( m_szModel ), g_aFlagModels[i].model ) == 0 )
		{
			pszViewmodel = g_aFlagModels[i].viewmodel;
		}
	}
	V_strncpy( m_szViewModel.GetForModify(), pszViewmodel, sizeof( m_szViewModel ) );

	BaseClass::Precache();

	// Team colored trail
	for ( int i = FIRST_GAME_TEAM; i < TDC_TEAM_COUNT; i++ )
	{
		char szModel[MAX_PATH];
		GetTrailEffect( i, szModel, MAX_PATH );
		PrecacheModel( szModel );
	}

	PrecacheParticleSystem( m_szPaperEffect );

	PrecacheScriptSound( TDC_CTF_FLAGSPAWN );
	PrecacheScriptSound( TDC_AD_CAPTURED_SOUND );
	PrecacheScriptSound( TDC_SD_FLAGSPAWN );

	PrecacheMaterial( "vgui/flagtime_full" );
	PrecacheMaterial( "vgui/flagtime_empty" );

	if ( m_szViewModel[0] )
	{
		PrecacheModel( m_szViewModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Spawn( void )
{
	if ( m_szModel == NULL_STRING )
	{
		Warning( "item_teamflag at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	// Precache the model and sounds.  Set the flag model.
	Precache();

	SetRenderMode( kRenderTransColor );
	//AddEffects( EF_BONEMERGE_FASTCULL );

	// Set the flag solid and the size for touching.
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	m_takedamage = DAMAGE_NO;
	
	SetModel( STRING( m_szModel ) );

	// Base class spawn.
	BaseClass::Spawn();

	// Set collision bounds so that they're independent from the model.
	SetCollisionBounds( Vector( -19.5f, -22.5f, -6.5f ), Vector( 19.5f, 22.5f, 6.5f ) );

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds( true, 24 );

	// Save the starting position, so we can reset the flag later if need be.
	m_vecResetPos = GetAbsOrigin();
	m_vecResetAng = GetAbsAngles();

	SetFlagStatus( TDC_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	m_bCaptured = false;

	SetDisabled( m_bDisabled );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::Activate( void )
{
	BaseClass::Activate();

	m_iOriginalTeam = GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the flag position state.
//-----------------------------------------------------------------------------
void CCaptureFlag::Reset( void )
{
	SetParent( NULL );

	// Set the flag position.
	SetAbsOrigin( m_vecResetPos );
	SetAbsAngles( m_vecResetAng );

	// No longer dropped, if it was.
	SetFlagStatus( TDC_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	if ( TDCGameRules()->GetGameType() == TDC_GAMETYPE_INVADE )
	{
		ChangeTeam( m_iOriginalTeam );
	}

	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::ResetMessage( void )
{
	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_ATTACK_DEFEND:
	{
		for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_ENEMYRETURNED );

				TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_RETURNED );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_TEAMRETURNED );

				TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_RETURNED );
			}
		}

		// Returned sound
		EmitSound( TDC_CTF_FLAGSPAWN );
		break;
	}
	case TDC_GAMETYPE_INVADE:
	{
		for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			CTeamRecipientFilter filter( iTeam, true );
			g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_RETURNED );
			TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_FlagReturned", "ico_notify_flag_home", iTeam );
		}
		break;
	}
	}

	// Output.
	m_outputOnReturn.FireOutput( this, this );

	UpdateReturnIcon();
}

//-----------------------------------------------------------------------------
// Purpose: Centralize gamemode-specific "can that team touch this flag" logic.
//-----------------------------------------------------------------------------
bool CCaptureFlag::CanTouchThisFlagType( const CBaseEntity *pOther )
{
	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_ATTACK_DEFEND:
		// Cannot touch flags owned by my own team
		return ( GetTeamNumber() != pOther->GetTeamNumber() );

	case TDC_GAMETYPE_INVADE:
		// Neutral flags can be touched by anyone
		if ( GetTeamNumber() == TEAM_UNASSIGNED )
			return true;
		// Otherwise: Can only touch flags owned by my own team
		return ( GetTeamNumber() == pOther->GetTeamNumber() );

	default:
		// Any other flag type: sure, go ahead, touch all you want!
		return true;
	}
}

bool UTIL_ItemCanBeTouchedByPlayer( CBaseEntity *pItem, CBasePlayer *pPlayer );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::FlagTouch( CBaseEntity *pOther )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( m_bLocked )
		return;

	if ( !TDCGameRules()->FlagsMayBeCapped() )
		return;

	// Can only be touched by a live player.
	CTDCPlayer *pPlayer = ToTDCPlayer( pOther );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	// Don't let the person who threw this flag pick it up until it hits the ground.
	// This way we can throw the flag to people, but not touch it as soon as we throw it ourselves
	if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pOther && m_bAllowOwnerPickup == false )
		return;

	// Prevent players from picking up flags through walls.
	if ( !UTIL_ItemCanBeTouchedByPlayer( this, pPlayer ) )
		return;

	if ( pOther->GetTeamNumber() == GetTeamNumber() )
	{
		m_outputOnTouchSameTeam.FireOutput( this, this );
	}

	// Can that team touch this flag?
	if ( !CanTouchThisFlagType( pOther ) )
		return;

	// Is the touching player about to teleport?
	if ( pPlayer->m_Shared.InCond( TDC_COND_SELECTED_TO_TELEPORT ) )
		return;

	// Don't let invulnerable players pick up flags
	if ( pPlayer->m_Shared.IsInvulnerable() )
		return;

	// Can't pick up while using a power-up.
	if ( pPlayer->m_Shared.IsCarryingPowerup() )
		return;

	// Do not allow the player to pick up multiple flags
	if ( pPlayer->HasTheFlag() )
		return;

	if ( PointInRespawnRoom( pPlayer, pPlayer->WorldSpaceCenter() ) )
		return;

	// Pick up the flag.
	PickUp( pPlayer, true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::PickUp( CTDCPlayer *pPlayer, bool bInvisible )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TDCGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_bAllowOwnerPickup )
	{
		if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pPlayer )
		{
			return;
		}
	}

	// Check whether we have a weapon that's prohibiting us from picking the flag up
	if ( !pPlayer->IsAllowedToPickUpFlag() )
		return;

	// Call into the base class pickup.
	BaseClass::PickUp( pPlayer, false );

	pPlayer->TeamFortress_SetSpeed();

	// Update the parent to set the correct place on the model to attach the flag.
	FollowEntity( pPlayer, true );

	// Remove the touch function.
	SetTouch( NULL );

	m_hPrevOwner = pPlayer;
	m_bAllowOwnerPickup = true;

	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_ATTACK_DEFEND:
	{
		for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_ENEMYSTOLEN );

				TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_TAKEN );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_TEAMSTOLEN );

				// exclude the guy who just picked it up
				filter.RemoveRecipient( pPlayer );

				TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_TAKEN );
			}
		}
		break;
	}
	case TDC_GAMETYPE_INVADE:
	{
		// Handle messages to the screen.
		CSingleUserRecipientFilter playerFilter( pPlayer );
		TDCGameRules()->SendHudNotification( playerFilter, "#TDC_Invade_PlayerPickup", "ico_notify_flag_moving", pPlayer->GetTeamNumber() );

		for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_ENEMYSTOLEN );
				TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_OtherTeamPickup", "ico_notify_flag_moving", iTeam );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_TEAMSTOLEN );

				filter.RemoveRecipient( pPlayer );
				TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_PlayerTeamPickup", "ico_notify_flag_moving", iTeam );
			}
		}

		// set the flag's team to match the player's team
		ChangeTeam( pPlayer->GetTeamNumber() );
		break;
	}
	}

	SetFlagStatus( TDC_FLAGINFO_STOLEN );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TDC_FLAGEVENT_PICKUP );
		event->SetInt( "priority", 8 );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGPICKUP );

	// Output.
	m_outputOnPickUp.FireOutput( this, this );

	switch ( pPlayer->GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		m_outputOnPickUpTeam1.FireOutput( this, this );
		break;

	case TDC_TEAM_BLUE:
		m_outputOnPickUpTeam2.FireOutput( this, this );
		break;
	}

	UpdateReturnIcon();

	HandleFlagPickedUpInDetectionZone( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Capture( CTDCPlayer *pPlayer, int nCapturePoint )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_CTF:
	{
		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( TDCGameRules()->GetScoreLimit() > 0 )
		{
			int nCaps = TFTeamMgr()->GetRoundScore( pPlayer->GetTeamNumber() );

			if ( ( nCaps >= 0 ) && ( TDCGameRules()->GetScoreLimit() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_ENEMYCAPTURED );

					TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_CAPTURED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_TEAMCAPTURED );

					TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_CAPTURED );
				}
			}
		}

		// Give temp crit boost to capper's team.
		if ( TDCGameRules() )
		{
			TDCGameRules()->HandleCTDCCaptureBonus( pPlayer->GetTeamNumber() );
		}

		// Reward the player
		CTDC_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		TFTeamMgr()->AddRoundScore( pPlayer->GetTeamNumber() );

		break;
	}
	case TDC_GAMETYPE_INVADE:
	{
		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( TDCGameRules()->GetScoreLimit() > 0 )
		{
			int nCaps = TFTeamMgr()->GetRoundScore( pPlayer->GetTeamNumber() );

			if ( ( nCaps >= 0 ) && ( TDCGameRules()->GetScoreLimit() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			CSingleUserRecipientFilter playerFilter( pPlayer );
			TDCGameRules()->SendHudNotification( playerFilter, "#TDC_Invade_PlayerCapture", "ico_notify_flag_home", pPlayer->GetTeamNumber() );

			for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_ENEMYCAPTURED );
					TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_OtherTeamCapture", "ico_notify_flag_home", iTeam );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_TEAMCAPTURED );

					filter.RemoveRecipient( pPlayer );
					TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_PlayerTeamCapture", "ico_notify_flag_home", iTeam );
				}
			}
		}

		// Give temp crit boost to capper's team.
		TDCGameRules()->HandleCTDCCaptureBonus( pPlayer->GetTeamNumber() );

		// Reward the player
		CTDC_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		TFTeamMgr()->AddRoundScore( pPlayer->GetTeamNumber() );

		break;
	}
	case TDC_GAMETYPE_ATTACK_DEFEND:
	{
		// Reward the player
		CTDC_GameStats.Event_PlayerCapturedPoint( pPlayer );
		break;
	}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TDC_FLAGEVENT_CAPTURE );
		event->SetInt( "priority", 9 );
		gameeventmanager->FireEvent( event );
	}

	SetFlagStatus( TDC_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	HandleFlagCapturedInDetectionZone( pPlayer );
	HandleFlagDroppedInDetectionZone( pPlayer );

	// Reset the flag.
	BaseClass::Drop( pPlayer, true );

	Reset();

	pPlayer->TeamFortress_SetSpeed();
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGCAPTURED );

	// Output.
	m_outputOnCapture.FireOutput( this, this );

	switch ( pPlayer->GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		m_outputOnCapTeam1.FireOutput( this, this );
		break;

	case TDC_TEAM_BLUE:
		m_outputOnCapTeam2.FireOutput( this, this );
		break;
	}

	m_bCaptured = true;
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( TDCGameRules()->InStalemate() )
	{
		// whoever capped the flag is the winner, give them enough caps to win
		CTDCTeam *pTeam = pPlayer->GetTDCTeam();
		if ( !pTeam )
			return;

		// if we still need more caps to trigger a win, give them to us
		int iScoreLimit = TDCGameRules()->GetScoreLimit();
		if ( pTeam->GetRoundScore() < iScoreLimit )
		{
			pTeam->AddRoundScore( iScoreLimit - TDCGameRules()->GetScoreLimit() );
		}
	}

	ManageSpriteTrail();
}

//-----------------------------------------------------------------------------
// Purpose: A player drops the flag.
//-----------------------------------------------------------------------------
void CCaptureFlag::Drop( CTDCPlayer *pPlayer, bool bVisible, bool bThrown /*= false*/, bool bMessage /*= true*/ )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	// Call into the base class drop.
	BaseClass::Drop( pPlayer, bVisible );

	pPlayer->TeamFortress_SetSpeed();

	if ( bThrown )
	{
		m_bAllowOwnerPickup = false;
		m_flOwnerPickupTime = gpGlobals->curtime + 3.0f;
	}

	Vector vecStart = pPlayer->EyePosition();
	Vector vecEnd = vecStart;
	vecEnd.z -= 8000.0f;
	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trace );
	SetAbsOrigin( trace.endpos );

	// HACK: Parent the flag if it's dropped on a train.
	// Fixes autstralium getting stuck in mid-air if dropped on the elevator in sd_doomsday.
	if ( trace.m_pEnt && trace.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
	{
		SetParent( trace.m_pEnt );
	}

	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_ATTACK_DEFEND:
	{
		if ( bMessage )
		{
			for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_ENEMYDROPPED );

					TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_DROPPED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_CTF_TEAMDROPPED );

					TDCGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_DROPPED );
				}
			}
		}

		SetFlagReturnIn( m_flResetDelay );
		break;
	}
	case TDC_GAMETYPE_INVADE:
	{
		if ( bMessage )
		{
			// Handle messages to the screen.
			CSingleUserRecipientFilter playerFilter( pPlayer );
			TDCGameRules()->SendHudNotification( playerFilter, "#TDC_Invade_PlayerFlagDrop", "ico_notify_flag_dropped", pPlayer->GetTeamNumber() );

			for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_ENEMYDROPPED );
					TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_FlagDrop", "ico_notify_flag_dropped", iTeam );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_INVADE_TEAMDROPPED );

					filter.RemoveRecipient( pPlayer );
					TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_FlagDrop", "ico_notify_flag_dropped", iTeam );
				}
			}
		}

		SetFlagReturnIn( m_flResetDelay );
		SetFlagNeutralIn( m_flResetDelay * 0.5f );

		break;
	}
	}

	// Reset the flag's angles.
	SetAbsAngles( m_vecResetAng );

	// Reset the touch function.
	SetTouch( &CCaptureFlag::FlagTouch );

	SetFlagStatus( TDC_FLAGINFO_DROPPED );

	// Output.
	m_outputOnDrop.FireOutput( this, this );

	UpdateReturnIcon();

	HandleFlagDroppedInDetectionZone( pPlayer );

	if ( PointInRespawnFlagZone( GetAbsOrigin() ) )
	{
		Reset();
		ResetMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		SetTouch( NULL );
		SetThink( NULL );
	}
	else
	{
		SetTouch( &CCaptureFlag::FlagTouch );
		SetThink( &CCaptureFlag::Think );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance();
	DispatchAnimEvents( this );

	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TDCGameRules()->FlagsMayBeCapped() )
		return;

	if ( m_bCaptured )
	{
		m_bCaptured = false;
		SetTouch( &CCaptureFlag::FlagTouch );
	}

	ManageSpriteTrail();

	if ( IsDropped() )
	{
		if ( !m_bAllowOwnerPickup )
		{
			if ( m_flOwnerPickupTime && gpGlobals->curtime > m_flOwnerPickupTime )
			{
				m_bAllowOwnerPickup = true;
			}
		}

		if ( TDCGameRules()->GetGameType() == TDC_GAMETYPE_INVADE )
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
			else if ( m_flNeutralTime && gpGlobals->curtime > m_flNeutralTime )
			{
				for ( int iTeam = TDC_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
				{
					CTeamRecipientFilter filter( iTeam, true );
					TDCGameRules()->SendHudNotification( filter, "#TDC_Invade_FlagNeutral", "ico_notify_flag_dropped", iTeam );
				}

				// reset the team to the original team setting (when it spawned)
				ChangeTeam( m_iOriginalTeam );

				ResetFlagNeutralTime();
			}
		}
		else
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
		}
	}
	else if ( m_bLocked )
	{
		if ( m_flUnlockTime > 0.0f && gpGlobals->curtime > m_flUnlockTime )
		{
			SetLocked( false );
			UpdateReturnIcon();
		}
	}
	else if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
	{
		ResetFlagReturnTime();
		UpdateReturnIcon();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::UpdateReturnIcon( void )
{
	if ( m_flResetTime > 0.0f || m_flUnlockTime > 0.0f )
	{
		if ( !m_hReturnIcon.Get() )
		{
			m_hReturnIcon = CBaseEntity::Create( "item_teamflag_return_icon", GetAbsOrigin() + Vector( 0, 0, cl_flag_return_height.GetFloat() ), vec3_angle, this );

			if ( m_hReturnIcon )
			{
				m_hReturnIcon->SetParent( this );
			}
		}
	}
	else
	{
		if ( m_hReturnIcon.Get() )
		{
			UTIL_Remove( m_hReturnIcon );
			m_hReturnIcon = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::GetTrailEffect( int iTeamNum, char *pszBuf, int iBufSize )
{
	V_snprintf( pszBuf, iBufSize, "effects/%s_%s.vmt", STRING( m_szTrailEffect ), g_aTeamNamesShort[iTeamNum] );
}

//-----------------------------------------------------------------------------
// Purpose: Handles the team colored trail sprites
//-----------------------------------------------------------------------------
void CCaptureFlag::ManageSpriteTrail( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetPrevOwner() );

	if ( IsStolen() &&
		( m_nUseTrailEffect == TDC_FLAGEFFECTS_ALL || m_nUseTrailEffect == TDC_FLAGEFFECTS_COLORTRAIL_ONLY ) &&
		pPlayer && pPlayer->GetAbsVelocity().Length() >= pPlayer->MaxSpeed() * 0.5f )
	{
		if ( !m_hGlowTrail )
		{
			char szEffect[128];
			GetTrailEffect( pPlayer->GetTeamNumber(), szEffect, sizeof( szEffect ) );

			m_hGlowTrail = CSpriteTrail::SpriteTrailCreate( szEffect, GetLocalOrigin(), false );
			m_hGlowTrail->FollowEntity( this );
			m_hGlowTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 96, 0 );
			m_hGlowTrail->SetBrightness( 96 );
			m_hGlowTrail->SetStartWidth( 32.0f );
			m_hGlowTrail->SetTextureResolution( 0.01f );
			m_hGlowTrail->SetLifeTime( 0.7f );
			m_hGlowTrail->SetTransmit( false );
		}
	}
	else
	{
		if ( m_hGlowTrail )
		{
			m_hGlowTrail->Remove();
			m_hGlowTrail = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the flag status
//-----------------------------------------------------------------------------
void CCaptureFlag::SetFlagStatus( int iStatus )
{
	MDLCACHE_CRITICAL_SECTION();

	m_nFlagStatus = iStatus;

	switch ( m_nFlagStatus )
	{
	case TDC_FLAGINFO_NONE:
	case TDC_FLAGINFO_DROPPED:
		ResetSequence( LookupSequence( "spin" ) );	// set spin animation if it's not being held
		break;
	case TDC_FLAGINFO_STOLEN:
		ResetSequence( LookupSequence( "idle" ) );	// set idle animation if it is being held
		break;
	default:
		AssertOnce( false );	// invalid stats
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputRoundActivate( inputdata_t &inputdata )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceDrop( inputdata_t &inputdata )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		pPlayer->DropFlag();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceReset( inputdata_t &inputdata )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
	ResetMessage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceResetSilent( inputdata_t &inputdata )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceResetAndDisableSilent( inputdata_t &inputdata )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputSetReturnTime( inputdata_t &inputdata )
{
	m_flResetDelay = (float)inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputShowTimer( inputdata_t &inputdata )
{
	// Show return icon with the specified time.
	float flTime = (float)inputdata.value.Int();
	SetFlagReturnIn( flTime );
	m_flResetDelay = flTime;
	UpdateReturnIcon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceGlowDisabled( inputdata_t &inputdata )
{
	m_bGlowEnabled = !inputdata.value.Bool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputSetLocked( inputdata_t &inputdata )
{
	SetLocked( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputSetUnlockTime( inputdata_t &inputdata )
{
	int nTime = inputdata.value.Int();

	if ( nTime <= 0 )
	{
		SetLocked( false );
		UpdateReturnIcon();
		return;
	}

	m_flUnlockTime = gpGlobals->curtime + (float)nTime;
	m_flUnlockDelay = (float)nTime;
	UpdateReturnIcon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::SetLocked( bool bLocked )
{
	m_bLocked = bLocked;

	if ( !m_bLocked )
	{
		m_flUnlockTime = -1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureFlag::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_nOldFlagStatus = m_nFlagStatus;
	m_iOldTeam = GetTeamNumber();
	m_bWasGlowEnabled = m_bGlowEnabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		UpdateGlowEffect();
	}
	else
	{
		if ( m_nFlagStatus != m_nOldFlagStatus || GetTeamNumber() != m_iOldTeam || m_bGlowEnabled != m_bWasGlowEnabled )
		{
			UpdateGlowEffect();
		}
	}

	UpdateFlagVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCaptureFlag::GetSkin( void )
{
	switch ( GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		return IsStolen() ? 3 : 0;
		break;
	case TDC_TEAM_BLUE:
		return IsStolen() ? 4 : 1;
		break;
	default:
		return IsStolen() ? 5 : 2;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::Simulate( void )
{
	BaseClass::Simulate();
	ManageTrailEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::ManageTrailEffects( void )
{
	if ( m_nFlagStatus == TDC_FLAGINFO_STOLEN &&
		( m_nUseTrailEffect == TDC_FLAGEFFECTS_ALL || m_nUseTrailEffect == TDC_FLAGEFFECTS_PAPERTRAIL_ONLY ) )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( GetPrevOwner() );

		if ( pPlayer )
		{
			Vector vecVelocity; pPlayer->EstimateAbsVelocity( vecVelocity );
			if ( vecVelocity.Length() >= pPlayer->MaxSpeed() * 0.5f )
			{
				if ( m_pPaperTrailEffect == NULL )
				{
					m_pPaperTrailEffect = ParticleProp()->Create( m_szPaperEffect, PATTACH_ABSORIGIN_FOLLOW );
				}
			}
			else
			{
				if ( m_pPaperTrailEffect )
				{
					ParticleProp()->StopEmission( m_pPaperTrailEffect );
					m_pPaperTrailEffect = NULL;
				}
			}
		}

	}
	else
	{
		if ( m_pPaperTrailEffect )
		{
			ParticleProp()->StopEmission( m_pPaperTrailEffect );
			m_pPaperTrailEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::UpdateGlowEffect( void )
{
	C_TDCPlayer *pCarrier = ToTDCPlayer( m_hPrevOwner.Get() );

	// If the flag is stolen only show the glow to the teammates of the carrier.
	if ( !m_bGlowEnabled || ( IsStolen() && pCarrier && pCarrier->IsEnemyPlayer() ) )
	{
		if ( m_pGlowEffect )
		{
			delete m_pGlowEffect;
			m_pGlowEffect = NULL;
		}
	}
	else
	{
		if ( !m_pGlowEffect )
		{
			m_pGlowEffect = new CGlowObject( this, vec3_origin, 1.0f, true, true );
		}

		Vector vecColor;
		TDCGameRules()->GetTeamGlowColor( GetTeamNumber(), vecColor.x, vecColor.y, vecColor.z );
		m_pGlowEffect->SetColor( vecColor );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CCaptureFlag::GetReturnProgress()
{
	if ( m_bLocked && m_flUnlockTime != 0.0f )
	{
		return RemapValClamped(
			m_flUnlockTime - gpGlobals->curtime,
			0.0f, m_flUnlockDelay,
			1.0f, 0.0f );
	}

	// In Invade the flag becomes neutral halfway through reset time.
	if ( TDCGameRules() && TDCGameRules()->GetGameType() == TDC_GAMETYPE_INVADE )
	{
		float flEventTime = ( m_flNeutralTime.Get() != 0.0f ) ? m_flNeutralTime.Get() : m_flResetTime.Get();
		float flTotalTime = m_flResetDelay * 0.5f;

		return RemapValClamped(
			flEventTime - gpGlobals->curtime,
			0.0f, flTotalTime,
			1.0f, 0.0f );
	}

	return RemapValClamped(
		m_flResetTime - gpGlobals->curtime,
		0.0f, m_flResetDelay,
		1.0f, 0.0f );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::UpdateFlagVisibility( void )
{
	if ( m_bDisabled )
	{
		// Make it either transparent or invisible.
		if ( m_bVisibleWhenDisabled )
		{
			SetRenderColorA( 180 );
		}
		else if ( !IsEffectActive( EF_NODRAW ) )
		{
			AddEffects( EF_NODRAW );
		}
	}
	else
	{
		if ( IsEffectActive( EF_NODRAW ) )
		{
			RemoveEffects( EF_NODRAW );
		}

		// Show it as transparent when locked.
		if ( m_bLocked )
		{
			SetRenderColorA( 180 );
		}
		else
		{
			SetRenderColorA( 255 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::GetHudIcon( int iTeamNum, char *pszBuf, int iBufSize )
{
	V_snprintf( pszBuf, iBufSize, "%s_%s", m_szHudIcon.Get(), GetTeamSuffix( iTeamNum, false, g_aTeamLowerNames, "neutral" ) );
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
BEGIN_NETWORK_TABLE( CCaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( item_teamflag_return_icon, CCaptureFlagReturnIcon );

CCaptureFlagReturnIcon::CCaptureFlagReturnIcon()
{
#ifdef CLIENT_DLL
	m_pReturnProgressMaterial_Empty = NULL;
	m_pReturnProgressMaterial_Full = NULL;
#endif
}

#ifdef GAME_DLL

void CCaptureFlagReturnIcon::Spawn( void )
{
	BaseClass::Spawn();

	UTIL_SetSize( this, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
}

int CCaptureFlagReturnIcon::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

#ifdef CLIENT_DLL

// This defines the properties of the 8 circle segments
// in the circular progress bar.
progress_segment_t Segments[8] =
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25, 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50, 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75, 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0, 0.0, 0.0, 0.5, 0.0, 1, 0 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RenderGroup_t CCaptureFlagReturnIcon::GetRenderGroup( void )
{
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}

void CCaptureFlagReturnIcon::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins.Init( -20, -20, -20 );
	theMaxs.Init( 20, 20, 20 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCaptureFlagReturnIcon::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );

	DrawReturnProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCaptureFlagReturnIcon::DrawReturnProgressBar( void )
{
	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *> ( GetOwnerEntity() );

	if ( !pFlag )
		return;

	// Don't draw if this flag is not going to reset
	if ( pFlag->GetResetDelay() <= 0 )
		return;

	if ( !TDCGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_pReturnProgressMaterial_Full )
	{
		m_pReturnProgressMaterial_Full = materials->FindMaterial( "vgui/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Empty )
	{
		m_pReturnProgressMaterial_Empty = materials->FindMaterial( "vgui/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Full || !m_pReturnProgressMaterial_Empty )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_flag_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = 255;

	switch ( pFlag->GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		ubColor[0] = 255;
		ubColor[1] = 0;
		ubColor[2] = 0;
		break;
	case TDC_TEAM_BLUE:
		ubColor[0] = 0;
		ubColor[1] = 0;
		ubColor[2] = 255;
		break;
	default:
		ubColor[0] = 200;
		ubColor[1] = 200;
		ubColor[2] = 200;
		break;
	}

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pReturnProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * -flSize ) + ( vUp * flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * flSize ) + ( vUp * flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * flSize ) + ( vUp * -flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * -flSize ) + ( vUp * -flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	float flProgress = pFlag->GetReturnProgress();

	pRenderContext->Bind( m_pReturnProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i = 0; i < 8; i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0, 1.0 );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( ( vOrigin + ( vRight * ( swipe_x - 0.5 ) ) + ( vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( ( vOrigin + ( vRight * ( Segments[i].vert2x - 0.5 ) ) + ( vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif
