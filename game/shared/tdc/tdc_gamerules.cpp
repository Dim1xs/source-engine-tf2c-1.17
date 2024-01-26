//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tdc_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tdc_weaponbase.h"
#include "time.h"
#include "viewport_panel_names.h"
#include "tdc_merc_customizations.h"
#include "tdc_announcer.h"
#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tdc_player.h"
	#include "c_tdc_objective_resource.h"
	#include "voice_status.h"
	#include "c_tdc_team.h"

	#define CTeam C_Team
#else
	#include "mapentities.h"
	#include "gameinterface.h"
	#include "serverbenchmark_base.h"
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "tdc_bot_temp.h"
	#include "tdc_player.h"
	#include "tdc_team.h"
	#include "player_resource.h"
	#include "entity_tdcstart.h"
	#include "filesystem.h"
	#include "tdc_objective_resource.h"
	#include "tdc_player_resource.h"
	#include "playerclass_info_parse.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "tdc_gamestats.h"
	#include "entity_capture_flag.h"
	#include "tdc_player_resource.h"
	#include "tier0/icommandline.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "vote_controller.h"
	#include "tdc_voteissues.h"
	#include "tdc_weaponbase_rocket.h"
	#include "tdc_weaponbase_grenadeproj.h"
	#include "tdc_weaponbase_nail.h"
	#include "eventqueue.h"
	#include "nav_mesh.h"
	#include "game.h"
	#include "pathtrack.h"
	#include "entity_ammopack.h"
	#include "entity_weaponspawn.h"
	#include "tdc_powerupbase.h"
	#include "triggers.h"
	#include <tier1/netadr.h>
	#include "tdc_viewmodel.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
static CTDCGameRulesProxy *g_pGameRulesProxy;

extern bool IsInCommentaryMode( void );

#if defined( REPLAY_ENABLED )
extern IReplaySystem *g_pReplay;
#endif // REPLAY_ENABLED
#endif

enum
{
	BIRTHDAY_RECALCULATE,
	BIRTHDAY_OFF,
	BIRTHDAY_ON,
};

static int g_TauntCamAchievements[TDC_CLASS_COUNT_ALL] =
{
	0,		// TDC_CLASS_UNDEFINED
	0,		// TDC_CLASS_GRUNT_NORMAL,
	0,		// TDC_CLASS_ZOMBIE
};

extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar sv_turbophysics;
extern ConVar mp_chattime;
extern ConVar sv_alltalk;
extern ConVar tv_delaymapchange;
extern ConVar sv_vote_issue_nextlevel_allowed;
extern ConVar sv_vote_issue_nextlevel_choicesmode;

ConVar mp_tournament( "mp_tournament", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );

ConVar mp_teams_unbalance_limit( "mp_teams_unbalance_limit", "1", FCVAR_REPLICATED,
	"Teams are unbalanced when one team has this many more players than the other team. (0 disables check)",
	true, 0,	// min value
	true, 30	// max value
	);

ConVar mp_maxrounds( "mp_maxrounds", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "max number of rounds to play before server changes maps", true, 0, false, 0 );
ConVar mp_winlimit( "mp_winlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Max score one team can reach before server changes maps", true, 0, false, 0 );
ConVar mp_bonusroundtime( "mp_bonusroundtime", "15", FCVAR_REPLICATED, "Time after round win until round restarts", true, 5, true, 15 );
ConVar mp_bonusroundtime_final( "mp_bonusroundtime_final", "15", FCVAR_REPLICATED, "Time after final round ends until round restarts", true, 5, true, 300 );
ConVar mp_forceautoteam( "mp_forceautoteam", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Automatically assign players to teams when joining." );

#if defined( _DEBUG ) || defined( STAGING_ONLY )
ConVar mp_developer( "mp_developer", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED | FCVAR_NOTIFY, "1: basic conveniences (instant respawn and class change, etc).  2: add combat conveniences (infinite ammo, buddha, etc)" );
#endif // _DEBUG || STAGING_ONLY

#ifdef GAME_DLL
ConVar mp_showroundtransitions( "mp_showroundtransitions", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show gamestate round transitions." );
ConVar mp_enableroundwaittime( "mp_enableroundwaittime", "1", FCVAR_REPLICATED, "Enable timers to wait between rounds." );
ConVar mp_showcleanedupents( "mp_showcleanedupents", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show entities that are removed on round respawn." );
ConVar mp_restartround( "mp_restartround", "0", FCVAR_GAMEDLL, "If non-zero, the current round will restart in the specified number of seconds" );

ConVar mp_stalemate_timelimit( "mp_stalemate_timelimit", "240", FCVAR_REPLICATED, "Timelimit (in seconds) of the stalemate round." );
ConVar mp_autoteambalance( "mp_autoteambalance", "1", FCVAR_NOTIFY );

ConVar mp_stalemate_enable( "mp_stalemate_enable", "0", FCVAR_NOTIFY, "Enable/Disable stalemate mode." );
ConVar mp_match_end_at_timelimit( "mp_match_end_at_timelimit", "0", FCVAR_NOTIFY, "Allow the match to end when mp_timelimit hits instead of waiting for the end of the current round." );

ConVar mp_holiday_nogifts( "mp_holiday_nogifts", "0", FCVAR_NOTIFY, "Set to 1 to prevent holiday gifts from spawning when players are killed." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_SwitchTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		if ( TDCGameRules() && TDCGameRules()->IsTeamplay() )
		{
			TDCGameRules()->SetSwitchTeams( true );
			mp_restartgame.SetValue( 5 );
			TDCGameRules()->ShouldResetScores( false, false );
			TDCGameRules()->ShouldResetRoundsPlayed( false );
		}
	}
}

static ConCommand mp_switchteams( "mp_switchteams", cc_SwitchTeams, "Switch teams and restart the game" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_ScrambleTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		if ( TDCGameRules() && TDCGameRules()->IsTeamplay() )
		{
			TDCGameRules()->SetScrambleTeams( true );
			mp_restartgame.SetValue( 5 );
			TDCGameRules()->ShouldResetScores( true, !TDCGameRules()->IsRoundBasedMode() );

			if ( args.ArgC() == 2 )
			{
				// Don't reset the roundsplayed when mp_scrambleteams 2 is passed
				if ( atoi( args[1] ) == 2 )
				{
					TDCGameRules()->ShouldResetRoundsPlayed( false );
				}
			}
		}
	}
}

static ConCommand mp_scrambleteams( "mp_scrambleteams", cc_ScrambleTeams, "Scramble the teams and restart the game" );
ConVar mp_scrambleteams_auto( "mp_scrambleteams_auto", "1", FCVAR_NOTIFY, "Server will automatically scramble the teams if criteria met.  Only works on dedicated servers." );
ConVar mp_scrambleteams_auto_windifference( "mp_scrambleteams_auto_windifference", "2", FCVAR_NOTIFY, "Number of round wins a team must lead by in order to trigger an auto scramble." );

CON_COMMAND_F( mp_forcewin, "Forces team to win", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( TDCGameRules() && TDCGameRules()->IsTeamplay() )
	{
		int iTeam = TEAM_UNASSIGNED;
		if ( args.ArgC() == 1 )
		{
			if ( engine->IsDedicatedServer() )
				return;

			// if no team specified, use player 1's team
			CBasePlayer *pPlayer = UTIL_GetListenServerHost();
			if ( pPlayer )
			{
				iTeam = pPlayer->GetTeamNumber();
			}
		}
		else if ( args.ArgC() == 2 )
		{
			// if team # specified, use that
			iTeam = atoi( args[1] );
		}
		else
		{
			Msg( "Usage: mp_forcewin <opt: team#>" );
			return;
		}

		int iWinReason = ( TEAM_UNASSIGNED == iTeam ? WINREASON_STALEMATE : WINREASON_ALL_POINTS_CAPTURED );
		TDCGameRules()->SetWinningTeam( iTeam, iWinReason );
	}
}

#endif // GAME_DLL

#ifdef GAME_DLL
// TF overrides the default value of this convar
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", (IsX360()?"15":"30"), FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY, "WaitingForPlayers time length in seconds" );

ConVar mp_scrambleteams_mode( "mp_scrambleteams_mode", "0", FCVAR_NOTIFY, "Sets team scramble mode:\n0 - score/connection time ratio\n1 - kill/death ratio\n2 - score\n3 - class\n4 - random" );

ConVar hide_server( "hide_server", "0", FCVAR_GAMEDLL, "Whether the server should be hidden from the master server" );

ConVar tdc_bot_count( "tdc_bot_count", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

ConVar tdc_gravetalk( "tdc_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat." );
ConVar tdc_spectalk( "tdc_spectalk", "0", FCVAR_NOTIFY, "Allows living players to hear spectators using text chat." );

ConVar mp_humans_must_join_team( "mp_humans_must_join_team", "any", FCVAR_NOTIFY, "Restricts human players to a single team {any, red, blue, green, yellow, spectator}" );

// TDC cvars
ConVar tdc_gametype( "tdc_gametype", "ffa", FCVAR_NOTIFY, "Sets the game mode. Possible values: ffa tdm duel ctf ad invade bm tbm infection vip" );
#else
ConVar tdc_particles_disable_weather( "tdc_particles_disable_weather", "0", FCVAR_ARCHIVE, "Disable particles related to weather effects." );
#endif

// TDC specific replicated cvars.
ConVar tdc_allow_thirdperson( "tdc_allow_thirdperson", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow players to switch to third person mode." );
ConVar tdc_ffa_fraglimit( "tdc_ffa_fraglimit", "25", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_tdm_fraglimit( "tdc_tdm_fraglimit", "50", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_duel_fraglimit( "tdc_duel_fraglimit", "10", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_ctf_scorelimit( "tdc_ctf_scorelimit", "3", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_invade_scorelimit( "tdc_invade_scorelimit", "3", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_bloodmoney_scorelimit( "tdc_bloodmoney_scorelimit", "25", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_teambloodmoney_scorelimit( "tdc_teambloodmoney_scorelimit", "50", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tdc_duel_bonusroundtime( "tdc_duel_bonusroundtime", "8", FCVAR_REPLICATED, "", true, 5, true, 15 );
ConVar tdc_infection_roundtime( "tdc_infection_roundtime", "210", FCVAR_NOTIFY | FCVAR_REPLICATED, "Round time in Infection mode in seconds. Humans win if they survive for this long.", true, 10, false, 0 );
ConVar tdc_instagib( "tdc_instagib", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enables Instagib mutator. Takes effect after map restart." );


#define SETUP_RESPAWN_CVARS(GAMETYPE, RESPAWNTIME)\
	ConVar tdc_##GAMETYPE##_respawntime( "tdc_" #GAMETYPE "_respawntime", #RESPAWNTIME, FCVAR_NOTIFY | FCVAR_REPLICATED, "Respawn time for the " #GAMETYPE " gametype" );

SETUP_RESPAWN_CVARS( ffa, 5 );
SETUP_RESPAWN_CVARS( duel, 5 );
SETUP_RESPAWN_CVARS( bm, 5 );
SETUP_RESPAWN_CVARS( tdm, 10 );
SETUP_RESPAWN_CVARS( tbm, 10 );
SETUP_RESPAWN_CVARS( ctf, 10 );
SETUP_RESPAWN_CVARS( ad, 10 );
SETUP_RESPAWN_CVARS( invade, 10 );
SETUP_RESPAWN_CVARS( infection, 10 );
SETUP_RESPAWN_CVARS( vip, 10 );

#ifdef GAME_DLL
static void NemesisSystemChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	// Clear all and any domination relationships when changing cvar.
	if ( TDCGameRules() )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer )
			{
				pPlayer->RemoveNemesisRelationships( false );
			}
		}
	}
}
ConVar tdc_nemesis_relationships( "tdc_nemesis_relationships", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable domination/revenge system.", NemesisSystemChangedCallback );
#else
ConVar tdc_nemesis_relationships( "tdc_nemesis_relationships", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable domination/revenge system." );
#endif

// Utility function
bool FindInList( const char **pStrings, const char *pToFind )
{
	int i = 0;
	while ( pStrings[i][0] != 0 )
	{
		if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
			return true;
		i++;
	}

	return false;
}


/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position
							
	Vector(-24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24,  24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24,  24, 55 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);							

const CViewVectors *CTDCGameRules::GetViewVectors() const
{
	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTDCGameRules );

#ifdef CLIENT_DLL
void RecvProxy_TeamplayRoundState( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TDCGameRules *pGamerules = ( C_TDCGameRules *)pStruct;
	int iRoundState = pData->m_Value.m_Int;
	pGamerules->SetRoundState( iRoundState );
}
#endif 

BEGIN_NETWORK_TABLE_NOBASE( CTDCGameRules, DT_TDCGameRules )
#ifdef CLIENT_DLL

	RecvPropInt( RECVINFO( m_iRoundState ), 0, RecvProxy_TeamplayRoundState ),
	RecvPropBool( RECVINFO( m_bInWaitingForPlayers ) ),
	RecvPropInt( RECVINFO( m_iWinningTeam ) ),
	RecvPropInt( RECVINFO( m_bInOvertime ) ),
	RecvPropInt( RECVINFO( m_bInSetup ) ),
	RecvPropInt( RECVINFO( m_bSwitchedTeamsThisRound ) ),
	RecvPropBool( RECVINFO( m_bAwaitingReadyRestart ) ),
	RecvPropTime( RECVINFO( m_flRestartRoundTime ) ),
	RecvPropTime( RECVINFO( m_flMapResetTime ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bTeamReady), RecvPropBool( RECVINFO(m_bTeamReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bStopWatch ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bPlayerReady), RecvPropBool( RECVINFO(m_bPlayerReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bCheatsEnabledDuringLevel ) ),

	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropBool( RECVINFO( m_bTeamPlay ) ),
	RecvPropBool( RECVINFO( m_bInstagib ) ),
	RecvPropEHandle( RECVINFO( m_hWaitingForPlayersTimer ) ),
	RecvPropEHandle( RECVINFO( m_hStalemateTimer ) ),
	RecvPropEHandle( RECVINFO( m_hTimeLimitTimer ) ),
	RecvPropEHandle( RECVINFO( m_hRoundTimer ) ),
	RecvPropInt( RECVINFO( m_iMapTimeBonus ) ),
	RecvPropInt( RECVINFO( m_iFragLimitBonus ) ),
	RecvPropEHandle( RECVINFO( m_hVIPPlayer ) ),

#else

	SendPropInt( SENDINFO( m_iRoundState ), 5 ),
	SendPropBool( SENDINFO( m_bInWaitingForPlayers ) ),
	SendPropInt( SENDINFO( m_iWinningTeam ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bInOvertime ) ),
	SendPropBool( SENDINFO( m_bInSetup ) ),
	SendPropBool( SENDINFO( m_bSwitchedTeamsThisRound ) ),
	SendPropBool( SENDINFO( m_bAwaitingReadyRestart ) ),
	SendPropTime( SENDINFO( m_flRestartRoundTime ) ),
	SendPropTime( SENDINFO( m_flMapResetTime ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bTeamReady ), SendPropBool( SENDINFO_ARRAY( m_bTeamReady ) ) ),
	SendPropBool( SENDINFO( m_bStopWatch ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerReady ), SendPropBool( SENDINFO_ARRAY( m_bPlayerReady ) ) ),
	SendPropBool( SENDINFO( m_bCheatsEnabledDuringLevel ) ),

	SendPropInt( SENDINFO( m_nGameType ), 4, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropBool( SENDINFO( m_bTeamPlay ) ),
	SendPropBool( SENDINFO( m_bInstagib ) ),
	SendPropEHandle( SENDINFO( m_hWaitingForPlayersTimer ) ),
	SendPropEHandle( SENDINFO( m_hStalemateTimer ) ),
	SendPropEHandle( SENDINFO( m_hTimeLimitTimer ) ),
	SendPropEHandle( SENDINFO( m_hRoundTimer ) ),
	SendPropInt( SENDINFO( m_iMapTimeBonus ) ),
	SendPropInt( SENDINFO( m_iFragLimitBonus ) ),
	SendPropEHandle( SENDINFO( m_hVIPPlayer ) ),

#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tdc_gamerules, CTDCGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TDCGameRulesProxy, DT_TDCGameRulesProxy );

#ifdef CLIENT_DLL
void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CTDCGameRules *pRules = TDCGameRules();
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CTDCGameRulesProxy, DT_TDCGameRulesProxy )
	RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TDCGameRules ), RecvProxy_TFGameRules )
END_RECV_TABLE()

void CTDCGameRulesProxy::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	TDCGameRules()->OnPreDataChanged( updateType );
}
void CTDCGameRulesProxy::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	TDCGameRules()->OnDataChanged( updateType );
}

#else
void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTDCGameRules *pRules = TDCGameRules();
	Assert( pRules );
	pRecipients->SetAllRecipients();
	return pRules;
}

BEGIN_SEND_TABLE( CTDCGameRulesProxy, DT_TDCGameRulesProxy )
	SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TDCGameRules ), SendProxy_TFGameRules ),
END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTDCGameRulesProxy )
	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetStalemateOnTimelimit", InputSetStalemateOnTimelimit ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRequiredObserverTarget", InputSetRequiredObserverTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTeamScore", InputAddRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTeamScore", InputAddBlueTeamScore ),

	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),

	DEFINE_INPUTFUNC( FIELD_STRING, "HandleMapEvent", InputHandleMapEvent ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetRoundRespawnFreezeEnabled", InputSetRoundRespawnFreezeEnabled ),

	// Outputs.
	DEFINE_OUTPUT( m_OnWonByTeam1, "OnWonByTeam1" ),
	DEFINE_OUTPUT( m_OnWonByTeam2, "OnWonByTeam2" ),
	DEFINE_OUTPUT( m_Team1PlayersChanged, "Team1PlayersChanged" ),
	DEFINE_OUTPUT( m_Team2PlayersChanged, "Team2PlayersChanged" ),
	DEFINE_OUTPUT( m_OnStateEnterBetweenRounds, "OnStateEnterBetweenRounds" ),
	DEFINE_OUTPUT( m_OnStateEnterPreRound, "OnStateEnterPreRound" ),
	DEFINE_OUTPUT( m_OnStateExitPreRound, "OnStateExitPreRound" ),
	DEFINE_OUTPUT( m_OnStateEnterRoundRunning, "OnStateEnterRoundRunning" ),

END_DATADESC()

CTDCGameRulesProxy::CTDCGameRulesProxy()
{
	g_pGameRulesProxy = this;

	memset( m_bEnabledModes, 0, sizeof( m_bEnabledModes ) );
	m_bEnabledModes[TDC_GAMETYPE_FFA] = true;
	m_iDefaultType = TDC_GAMETYPE_FFA;

	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "player_team" );
}

CTDCGameRulesProxy::~CTDCGameRulesProxy()
{
	if ( g_pGameRulesProxy == this )
	{
		g_pGameRulesProxy = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRulesProxy::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "DefaultType" ) )
	{
		m_iDefaultType = GetGameTypeFromString( szValue );
		return true;
	}

	for ( int i = 1; i < TDC_GAMETYPE_COUNT; i++ )
	{
		if ( FStrEq( szKeyName, UTIL_VarArgs( "%sEnabled", g_aGameTypeInfo[i].name ) ) )
		{
			m_bEnabledModes[i] = !!atoi( szValue );
			return true;
		}

		if ( FStrEq( szKeyName, UTIL_VarArgs( "OnStart%s", g_aGameTypeInfo[i].name ) ) )
		{
			m_OnStartMode[i].ParseEventAction( szValue );
			return true;
		}
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "teamplay_round_win" ) )
	{
		switch ( event->GetInt( "team" ) )
		{
		case TDC_TEAM_RED:
			m_OnWonByTeam1.FireOutput( this, this );
			break;
		case TDC_TEAM_BLUE:
			m_OnWonByTeam2.FireOutput( this, this );
			break;
		}
	}
	else if ( FStrEq( event->GetName(), "player_team" ) )
	{
		switch ( event->GetInt( "team" ) )
		{
		case TDC_TEAM_RED:
			m_Team1PlayersChanged.FireOutput( this, this );
			break;
		case TDC_TEAM_BLUE:
			m_Team2PlayersChanged.FireOutput( this, this );
			break;
		}

		switch ( event->GetInt( "oldteam" ) )
		{
		case TDC_TEAM_RED:
			m_Team1PlayersChanged.FireOutput( this, this );
			break;
		case TDC_TEAM_BLUE:
			m_Team2PlayersChanged.FireOutput( this, this );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputRoundActivate( inputdata_t &data )
{
	OnGameTypeStart( (ETDCGameType)TDCGameRules()->GetGameType() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputSetStalemateOnTimelimit( inputdata_t &inputdata )
{
	TDCGameRules()->SetStalemateOnTimelimit( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TDCGameRules()->SetTeamGoalString( TDC_TEAM_RED, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TDCGameRules()->SetTeamGoalString( TDC_TEAM_BLUE, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputSetRequiredObserverTarget( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String() );
	TDCGameRules()->SetRequiredObserverTarget( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputAddRedTeamScore( inputdata_t &inputdata )
{
	CTDCTeam *pTeam = GetGlobalTFTeam( TDC_TEAM_RED );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputAddBlueTeamScore( inputdata_t &inputdata )
{
	CTDCTeam *pTeam = GetGlobalTFTeam( TDC_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->AddScore( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	if ( TDCGameRules() )
	{
		TDCGameRules()->BroadcastSound( 255, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	if ( TDCGameRules() )
	{
		TDCGameRules()->BroadcastSound( TDC_TEAM_RED, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	if ( TDCGameRules() )
	{
		TDCGameRules()->BroadcastSound( TDC_TEAM_BLUE, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputHandleMapEvent( inputdata_t &inputdata )
{
	// Do nothing.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata )
{
	TDCGameRules()->SetPreRoundFreezeTimeEnabled( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::Activate()
{
	// Determine the gamemode.
	ETDCGameType iType = GetGameTypeFromString( tdc_gametype.GetString() );

	// If invalid or unsupported gamemode's selected fall back to default.
	if ( !IsGameTypeValid( iType ) )
		iType = m_iDefaultType;

	TDCGameRules()->m_nGameType = iType;
	TDCGameRules()->m_bTeamPlay = g_aGameTypeInfo[iType].teamplay;
	TDCGameRules()->m_bAllowStalemateAtTimelimit = g_aGameTypeInfo[iType].end_at_timelimit;
	TDCGameRules()->m_bInstagib = tdc_instagib.GetBool();

	OnGameTypeStart( iType );
	CTDCPickupItem::UpdatePowerupsForGameType( iType );

	// Now that we know how many teams there should be remove any that we don't need.
	TFTeamMgr()->RemoveExtraTeams();
	TDCGameRules()->Activate();

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ETDCGameType CTDCGameRulesProxy::GetGameTypeFromString( const char *pszName )
{
	if ( pszName && pszName[0] )
	{
		for ( int i = 0; i < TDC_GAMETYPE_COUNT; i++ )
		{
			if ( FStrEq( pszName, g_aGameTypeInfo[i].name ) )
				return (ETDCGameType)i;
		}
	}

	Assert( 0 );
	return TDC_GAMETYPE_FFA;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRulesProxy::IsGameTypeValid( ETDCGameType iType )
{
	if ( iType <= 0 )
		return false;

	return m_bEnabledModes[iType];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRulesProxy::OnGameTypeStart( ETDCGameType iType )
{
	if ( iType > 0 )
	{
		m_OnStartMode[iType].FireOutput( this, this );
	}
}

#endif

#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		// Very lame that the base code needs this defined
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTDCGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

#ifdef GAME_DLL
unsigned char g_aAuthDataKey[8] = { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 };
unsigned char g_aAuthDataXOR[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCGameRules::CTDCGameRules()
{
#ifdef GAME_DLL
	m_bInOvertime = false;
	m_bInSetup = false;
	m_bSwitchedTeamsThisRound = false;
	m_flStopWatchTotalTime = -1.0f;
	m_bAllowBetweenRounds = true;

	ListenForGameEvent( "server_changelevel_failed" );

	m_pCurStateInfo = NULL;
	State_Transition( GR_STATE_PREGAME );

	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	InitTeams();
	ResetMapTime();
	ResetScores();
	m_bInWaitingForPlayers = false;
	m_bAwaitingReadyRestart = false;
	m_flRestartRoundTime = -1.0f;
	m_flMapResetTime = 0.0f;
	m_iWinningTeam = TEAM_UNASSIGNED;

	m_bAllowStalemateAtTimelimit = false;
	m_bChangelevelAfterStalemate = false;
	m_flRoundStartTime = 0.0f;
	m_flNewThrottledAlertTime = 0.0f;
	m_flStartBalancingTeamsAt = 0.0f;
	m_bPrintedUnbalanceWarning = false;
	m_flFoundUnbalancedTeamsTime = -1.0f;
	m_flWaitingForPlayersTimeEnds = -1.0f;
	m_flLastTeamWin = -1.0f;

	m_nRoundsPlayed = 0;

	m_bStopWatch = false;
	m_bAwaitingReadyRestart = false;

	if ( IsInTournamentMode() )
	{
		m_bAwaitingReadyRestart = true;
	}

	m_flAutoBalanceQueueTimeEnd = -1.0f;
	m_nAutoBalanceQueuePlayerIndex = -1;
	m_nAutoBalanceQueuePlayerScore = -1;

	m_bCheatsEnabledDuringLevel = false;

	ResetPlayerAndTeamReadyState();

	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "player_escort_score" );

	m_bVoteMapOnNextRound = false;
	m_bVotedForNextMap = false;
	m_iMapTimeBonus = 0;
	m_iMaxRoundsBonus = 0;
	m_iWinLimitBonus = 0;
	m_iFragLimitBonus = 0;

	m_flNextVoteThink = 0.0f;

	m_bPreRoundFreezeTime = true;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[32];
	V_sprintf_safe( szCommand, "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

	// Load 'authenticated' data
	unsigned char szPassword[8];
	V_memcpy(szPassword, g_aAuthDataKey, sizeof(szPassword));
	for (unsigned int i = 0; i < sizeof(szPassword); ++i)
		szPassword[i] ^= g_aAuthDataXOR[i] ^ 0x00;

	m_pAuthData = ReadEncryptedKVFile(filesystem, "scripts/authdata", szPassword, true);
	V_memset(szPassword, 0x00, sizeof(szPassword));

#else // GAME_DLL

	ListenForGameEvent( "game_newmap" );

	// Using our own voice bubble.
	GetClientVoiceMgr()->SetHeadLabelsDisabled( true );
	
#endif

	// Initialize the game type
	m_nGameType.Set( TDC_GAMETYPE_UNDEFINED );

	// Initialize the classes here.
	InitPlayerClasses();

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 1 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	Assert( g_aWeaponDamageTypes[WEAPON_COUNT] == TDC_DMG_SENTINEL_VALUE );
	Assert( FStrEq( g_aWeaponNames[WEAPON_COUNT], "WEAPON_COUNT" ) );	

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
}

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"point_commentary_node",
	"point_viewcontrol",
	"func_precipitation",
	"func_team_wall",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"commentary_auto",
	"point_commentary_node",
	"point_commentary_viewpoint",
	"bot_roster",
	"info_populator",

	"tdc_gamerules",
	"tdc_player_manager",
	"tdc_team",
	"tdc_objective_resource",
	//"keyframe_rope",
	//"move_rope",
	"tdc_viewmodel",
	"vote_controller",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::PointsMayBeCaptured( void )
{
	if ( IsInWaitingForPlayers() )
		return false;

	return ( State_Get() == GR_STATE_RND_RUNNING || State_Get() == GR_STATE_STALEMATE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::LevelInitPostEntity( void )
{
	BaseClass::LevelInitPostEntity();

#ifdef GAME_DLL
	m_bCheatsEnabledDuringLevel = sv_cheats && sv_cheats->GetBool();
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are running tournament mode
//-----------------------------------------------------------------------------
bool CTDCGameRules::IsInTournamentMode( void )
{
	return mp_tournament.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we should even bother to do balancing stuff
//-----------------------------------------------------------------------------
bool CTDCGameRules::ShouldBalanceTeams( void )
{
	if ( IsInTournamentMode() == true )
		return false;

	if ( IsInTraining() == true || IsInItemTestingMode() )
		return false;

	if ( !IsTeamplay() )
		return false;

	if ( IsInfectionMode() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the passed team change would cause unbalanced teams
//-----------------------------------------------------------------------------
bool CTDCGameRules::WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam )
{
	// players are allowed to change to their own team
	if ( iNewTeam == iCurrentTeam )
		return false;

	// if mp_teams_unbalance_limit is 0, don't check
	if ( ShouldBalanceTeams() == false )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	// if they are joining a non-playing team, allow
	if ( iNewTeam < FIRST_GAME_TEAM )
		return false;

	CTeam *pNewTeam = GetGlobalTeam( iNewTeam );

	if ( !pNewTeam )
	{
		Assert( 0 );
		return true;
	}

	// add one because we're joining this team
	int iNewTeamPlayers = pNewTeam->GetNumPlayers() + 1;

	// for each game team
	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
	{
		if ( pTeam == pNewTeam )
			continue;

		int iNumPlayers = pTeam->GetNumPlayers();

		if ( i == iCurrentTeam )
		{
			iNumPlayers = Max( 0, iNumPlayers - 1 );
		}

		if ( ( iNewTeamPlayers - iNumPlayers ) > mp_teams_unbalance_limit.GetInt() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam )
{
	if ( ShouldBalanceTeams() == false )
	{
		return false;
	}

#ifndef CLIENT_DLL
	if ( IsInCommentaryMode() )
		return false;
#endif

	int iMostPlayers = 0;
	int iLeastPlayers = MAX_PLAYERS + 1;

	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
	{
		int iNumPlayers = pTeam->GetNumPlayers();

		if ( iNumPlayers < iLeastPlayers )
		{
			iLeastPlayers = iNumPlayers;
			iLightestTeam = i;
		}

		if ( iNumPlayers > iMostPlayers )
		{
			iMostPlayers = iNumPlayers;
			iHeaviestTeam = i;
		}
	}

	if ( ( iMostPlayers - iLeastPlayers ) > mp_teams_unbalance_limit.GetInt() )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::GetBonusRoundTime( bool bFinal /*= false*/ )
{
	if ( IsInDuelMode() )
		return tdc_duel_bonusroundtime.GetFloat();

	return bFinal ? mp_bonusroundtime_final.GetInt() : Max( 5, mp_bonusroundtime.GetInt() );
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTDCGameRules::CalcPlayerScore( RoundStats_t *pRoundStats )
{
	switch ( TDCGameRules()->GetGameType() )
	{
		// FFA uses a different scoring system.
	case TDC_GAMETYPE_FFA:
	case TDC_GAMETYPE_DUEL:
		return ( pRoundStats->m_iStat[TFSTAT_KILLS] ) -
			( pRoundStats->m_iStat[TFSTAT_SUICIDES] ) -
			( pRoundStats->m_iStat[TFSTAT_ENV_DEATHS] );

	case TDC_GAMETYPE_BM:
		return pRoundStats->m_iStat[TFSTAT_CAPTURES];

	default:
	{
		int iScore = ( pRoundStats->m_iStat[TFSTAT_KILLS] * TDC_SCORE_KILL ) +
			( pRoundStats->m_iStat[TFSTAT_CAPTURES] * TDC_SCORE_CAPTURE ) +
			( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TDC_SCORE_DEFEND ) +
			( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TDC_SCORE_DESTROY_BUILDING ) +
			( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] * TDC_SCORE_HEADSHOT ) +
			( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TDC_SCORE_BACKSTAB ) +
			( pRoundStats->m_iStat[TFSTAT_HEALING] / TDC_SCORE_HEAL_HEALTHUNITS_PER_POINT ) +
			( pRoundStats->m_iStat[TFSTAT_DAMAGE] / TDC_SCORE_DAMAGE_PER_POINT ) +
			( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TDC_SCORE_KILL_ASSISTS_PER_POINT ) +
			( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TDC_SCORE_TELEPORTS_PER_POINT ) +
			( pRoundStats->m_iStat[TFSTAT_INVULNS] / TDC_SCORE_INVULN ) +
			( pRoundStats->m_iStat[TFSTAT_REVENGE] / TDC_SCORE_REVENGE ) +
			( pRoundStats->m_iStat[TFSTAT_BONUS] / TDC_SCORE_BONUS_PER_POINT );
		return Max( iScore, 0 );
	}
	}
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::TimerMayExpire( void )
{
	for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
	{
		if ( !pFlag->IsHome() )
			return false;
	}

	return BaseClass::TimerMayExpire();
}

// sort functor for the list of players that we're going to use to scramble the teams
static int ScramblePlayersSort( CTDCPlayer* const *p1, CTDCPlayer* const *p2 )
{
	return ( ( *p2 )->GetTeamScrambleScore() - ( *p1 )->GetTeamScrambleScore() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::HandleScrambleTeams( void )
{
	int i = 0;
	CTDCPlayer *pTFPlayer = NULL;
	CUtlVector<CTDCPlayer *> pListPlayers;

	// add all the players (that are on blue or red) to our temp list
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pTFPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && ( pTFPlayer->GetTeamNumber() >= FIRST_GAME_TEAM ) )
		{
			pListPlayers.AddToHead( pTFPlayer );
		}
	}

	// sort the list
	if ( mp_scrambleteams_mode.GetInt() == TDC_SCRAMBLEMODE_RANDOM )
	{
		pListPlayers.Shuffle();
	}
	else
	{
		FOR_EACH_VEC( pListPlayers, i )
			pListPlayers[i]->CalculateTeamScrambleScore();

		pListPlayers.Sort( ScramblePlayersSort );
	}

	// loop through and put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	for ( i = 0; i < pListPlayers.Count(); i++ )
	{
		pListPlayers[i]->ForceChangeTeam( TEAM_SPECTATOR );
		pListPlayers[i]->RemoveNemesisRelationships( false );
	}

	// loop through and auto team everyone
	for ( i = 0; i < pListPlayers.Count(); i++ )
	{
		pListPlayers[i]->ForceChangeTeam( TDC_TEAM_AUTOASSIGN );
	}

	ResetTeamsRoundWinTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::HandleSwitchTeams( void )
{
	// Don't do this if we're about to scramble.
	if ( ShouldScrambleTeams() )
		return;

	// respawn the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() == TDC_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TDC_TEAM_BLUE );
			}
			else if ( pPlayer->GetTeamNumber() == TDC_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TDC_TEAM_RED );
			}
		}
	}

	// switch the team scores
	CTDCTeam *pRedTeam = GetGlobalTFTeam( TDC_TEAM_RED );
	CTDCTeam *pBlueTeam = GetGlobalTFTeam( TDC_TEAM_BLUE );

	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );

		nRed = pRedTeam->GetWinCount();
		nBlue = pBlueTeam->GetWinCount();

		pRedTeam->SetWinCount( nBlue );
		pBlueTeam->SetWinCount( nRed );
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#TDC_TeamsSwitched" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "weapon_" ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::ShouldCreateEntity( const char *pszClassName )
{
	return !FindInList( s_PreserveEnts, pszClassName );
}

//Runs think for all player's conditions
//Need to do this here instead of the player so players that crash still run their important thinks
void CTDCGameRules::RunPlayerConditionThink( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->m_Shared.ConditionGameRulesThink();
		}
	}
}

void CTDCGameRules::FrameUpdatePostEntityThink()
{
	BaseClass::FrameUpdatePostEntityThink();

	RunPlayerConditionThink();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTDCGameRules::SetupOnRoundStart( void )
{
	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	// All entities have been spawned, now activate them
	pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	m_szMostRecentCappers[0] = 0;

	if ( g_pGameRulesProxy )
	{
		g_pGameRulesProxy->m_OnStateEnterPreRound.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
	}

	if ( IsInDuelMode() )
	{
		StartDuelRound();
	}
	else if ( IsInfectionMode() )
	{
		StartInfectedRound();

		// Must do this after assigning players since Last Standing of the previous round is taken into account..
		m_hLastStandingPlayer = NULL;
	}
	else if ( IsVIPMode() )
	{
		StartVIPRound();
	}

	if ( !IsInWaitingForPlayers() && m_bPreRoundFreezeTime )
	{
		if ( !m_hStalemateTimer )
		{
			m_hStalemateTimer = (CTeamRoundTimer*)CBaseEntity::Create( "game_round_timer", vec3_origin, vec3_angle );
			m_hStalemateTimer->SetName( AllocPooledString( "zz_deathmatch_preround_timer" ) );
			m_hStalemateTimer->SetShowInHud( true );
		}

		variant_t sVariant;
		sVariant.SetInt( 5 * mp_enableroundwaittime.GetInt() );

		m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTDCGameRules::SetupOnRoundRunning( void )
{
	// Reset player speeds after preround lock
	CTDCPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
	}

	if ( g_pGameRulesProxy )
	{
		g_pGameRulesProxy->m_OnStateEnterRoundRunning.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
	}

	if ( IsInfectionMode() )
	{
		if ( !m_hRoundTimer )
		{
			m_hRoundTimer = (CTeamRoundTimer*)CBaseEntity::Create( "game_round_timer", vec3_origin, vec3_angle );
			m_hRoundTimer->SetName( AllocPooledString( "zz_infection_timer" ) );
			m_hRoundTimer->SetShowInHud( true );
		}

		variant_t sVariant;
		sVariant.SetInt( tdc_infection_roundtime.GetInt() );

		m_hRoundTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
		m_hRoundTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCGameRules::PreRound_End( void )
{
	if ( g_pGameRulesProxy )
	{
		g_pGameRulesProxy->m_OnStateExitPreRound.FireOutput( g_pGameRulesProxy, g_pGameRulesProxy );
	}

	if ( m_hStalemateTimer.Get() )
	{
		UTIL_Remove( m_hStalemateTimer );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTDCGameRules::PreviousRoundEnd( void )
{
	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCGameRules::SendWinPanelInfo( void )
{
	// TODO: Special win panel for Duel.
	if ( IsInDuelMode() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "duel_end" );
		if ( event )
		{
			event->SetInt( "winner", m_hWinningPlayer ? m_hWinningPlayer->entindex() : 0 );
			event->SetInt( "win_reason", m_iWinReason );
			
			if ( AreNextDuelingPlayersSet() )
			{
				event->SetInt( "next_player_1", m_NextDuelingPlayers[0]->entindex() );
				event->SetInt( "next_player_2", m_NextDuelingPlayers[1]->entindex() );
			}

			gameeventmanager->FireEvent( event );
		}
		return;
	}

	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",
			( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED ||
			m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ?
		m_szMostRecentCappers : "" );
		winEvent->SetInt( "flagcaplimit", GetScoreLimit() );

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			CTeam *pTeam = GetGlobalTeam( i );	
			if ( pTeam )
			{
				winEvent->SetInt( UTIL_VarArgs( "%s_score", g_aTeamLowerNames[i] ), pTeam->GetScore() );
			}
		}

		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		bool bCollectAllPlayers = ( m_iWinningTeam == TEAM_UNASSIGNED || TDCGameRules()->IsInfectionMode() );

		for ( int iPlayerIndex = 1; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
		{
			CTDCPlayer *pTFPlayer = ToTDCPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;

			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( iPlayerTeam < FIRST_GAME_TEAM )
				continue;

			if ( !bCollectAllPlayers && m_iWinningTeam != iPlayerTeam )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( pTFPlayer );

			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
			}
			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iTotalScore = iTotalScore;
		}
		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 3 players by round score in the event data
		int numPlayers = Min( 3, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "";
			V_sprintf_safe( szPlayerIndexVal, "player_%d", i + 1 );
			V_sprintf_safe( szPlayerScoreVal, "player_%d_points", i + 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );
		}

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTDCGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTDCGameRules::ArenaPlayerRoundScoreSortFunc( const ArenaPlayerRoundScore_t *pRoundScore1, const ArenaPlayerRoundScore_t *pRoundScore2 )
{
	// Sort priority:
	// 1) Score
	// 2) Healing
	// 3) Damage
	// 4) Lifetime
	// 5) Kills
	// 6) Index

	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	if ( pRoundScore1->iHealing != pRoundScore2->iHealing )
		return pRoundScore2->iHealing - pRoundScore1->iHealing;

	if ( pRoundScore1->iDamage != pRoundScore2->iDamage )
		return pRoundScore2->iDamage - pRoundScore1->iDamage;

	if ( pRoundScore1->iLifeTime != pRoundScore2->iLifeTime )
		return pRoundScore2->iLifeTime - pRoundScore1->iLifeTime;

	if ( pRoundScore1->iKills != pRoundScore2->iKills )
		return pRoundScore2->iKills - pRoundScore1->iKills;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTDCGameRules::SetupOnStalemateStart( void )
{
	// Respawn all the players
	RespawnPlayers( true );

	// Remove everyone's objects
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
		}
	}

	// Disable all the active health packs in the world
	m_hDisabledHealthKits.Purge();

	for ( CTDCPickupItem *pPowerup : CTDCPickupItem::AutoList() )
	{
		CHealthKit *pHealthPack = dynamic_cast<CHealthKit *>( pPowerup );

		if ( pHealthPack && !pHealthPack->IsDisabled() )
		{
			pHealthPack->SetDisabled( true );
			m_hDisabledHealthKits.AddToTail( pHealthPack );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled
	for ( int i = 0; i < m_hDisabledHealthKits.Count(); i++ )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::CheckNextLevelCvar( bool bAllowEnd /*= true*/ )
{
	if ( nextlevel.GetString() && *nextlevel.GetString() )
	{
		if ( bAllowEnd )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
			if ( event )
			{
				event->SetString( "reason", "NextLevel CVAR" );
				gameeventmanager->FireEvent( event );
			}

			GoToIntermission();
		}
		return true;
	}

	return false;
}

void CC_CH_ForceRespawn( void )
{
	if ( TDCGameRules() )
	{
		TDCGameRules()->RespawnPlayers( true );
	}
}
static ConCommand mp_forcerespawnplayers( "mp_forcerespawnplayers", CC_CH_ForceRespawn, "Force all players to respawn.", FCVAR_CHEAT );

static ConVar mp_tournament_allow_non_admin_restart( "mp_tournament_allow_non_admin_restart", "1", FCVAR_NONE, "Allow mp_tournament_restart command to be issued by players other than admin." );
void CC_CH_TournamentRestart( void )
{
	if ( mp_tournament_allow_non_admin_restart.GetBool() == false )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;
	}

	TDCGameRules()->RestartTournament();
}
static ConCommand mp_tournament_restart( "mp_tournament_restart", CC_CH_TournamentRestart, "Restart Tournament Mode on the current level." );

void CTDCGameRules::RestartTournament( void )
{
	if ( IsInTournamentMode() == false )
		return;

	SetInWaitingForPlayers( true );
	m_bAwaitingReadyRestart = true;
	m_flStopWatchTotalTime = -1.0f;
	m_bStopWatch = false;

	// we might have had a stalemate during the last round
	// so reset this bool each time we restart the tournament
	m_bChangelevelAfterStalemate = false;

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady.Set( i, false );
	}

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bForceRespawn - respawn player even if dead or dying
//			bTeam - if true, only respawn the passed team
//			iTeam  - team to respawn
//-----------------------------------------------------------------------------
void CTDCGameRules::RespawnPlayers( bool bForceRespawn, bool bTeam /* = false */, int iTeam/* = TEAM_UNASSIGNED */ )
{
	if ( bTeam )
	{
		Assert( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() );
	}

	int iPlayersSpawned = 0;

	CBasePlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		// Check for team specific spawn
		if ( bTeam && pPlayer->GetTeamNumber() != iTeam )
			continue;

		// players that haven't chosen a team/class can never spawn
		if ( !pPlayer->IsReadyToPlay() )
		{
			// Let the player spawn immediately when they do pick a class
			if ( pPlayer->ShouldGainInstantSpawn() )
			{
				pPlayer->AllowInstantSpawn();
			}

			continue;
		}

		// If we aren't force respawning, don't respawn players that:
		// - are alive
		// - are still in the death anim stage of dying
		if ( !bForceRespawn )
		{
			if ( pPlayer->IsAlive() )
				continue;

			if ( GetGameType() == TDC_GAMETYPE_INVADE && InOvertime() )
			{
				// Don't respawn in invade
				return;
			}


			if ( m_iRoundState != GR_STATE_PREROUND )
			{
				// If the player hasn't been dead the minimum respawn time, he
				// waits until the next wave.
				//if ( bTeam && !HasPassedMinRespawnTime( pPlayer ) )
				//	continue;

				if ( !pPlayer->IsReadyToSpawn() )
				{
					// Let the player spawn immediately when they do pick a class
					if ( pPlayer->ShouldGainInstantSpawn() )
					{
						pPlayer->AllowInstantSpawn();
					}

					continue;
				}
			}

		}

		// Respawn this player
		pPlayer->ForceRespawn();
		iPlayersSpawned++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a round winner.
//-----------------------------------------------------------------------------
void CTDCGameRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false*/, bool bDontAddScore /* = false*/, bool bFinal /*= false*/ )
{
	// Commentary doesn't let anyone win
	if ( IsInCommentaryMode() )
		return;

	if ( ( team != TEAM_UNASSIGNED ) && ( team <= LAST_SHARED_TEAM || team >= GetNumberOfTeams() ) )
	{
		Assert( !"SetWinningTeam() called with invalid team." );
		return;
	}

	// are we already in this state?
	if ( State_Get() == GR_STATE_TEAM_WIN )
		return;

	SetSwitchTeams( bSwitchTeams );

	m_iWinningTeam = team;
	m_iWinReason = iWinReason;

	PlayWinSong( team );

	if ( !bDontAddScore && team != TEAM_UNASSIGNED )
	{
		GetGlobalTeam( team )->AddScore( TEAMPLAY_ROUND_WIN_SCORE );
	}

	// this was a sudden death win if we were in stalemate then a team won it
	bool bWasSuddenDeath = ( InStalemate() && m_iWinningTeam >= FIRST_GAME_TEAM );

	State_Transition( GR_STATE_TEAM_WIN );

	m_flLastTeamWin = gpGlobals->curtime;

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_win" );
	if ( event )
	{
		event->SetInt( "team", team );
		event->SetInt( "winreason", iWinReason );
		event->SetBool( "full_round", bForceMapReset );
		event->SetFloat( "round_time", gpGlobals->curtime - m_flRoundStartTime );
		event->SetBool( "was_sudden_death", bWasSuddenDeath );

		gameeventmanager->FireEvent( event );
	}

	// send team scores
	SendTeamScoresEvent();

	if ( team == TEAM_UNASSIGNED )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_STALEMATE );
		}
	}

	// Auto scramble teams?
	if ( bForceMapReset && mp_scrambleteams_auto.GetBool() )
	{
		if ( IsInTournamentMode() || ShouldSkipAutoScramble() )
			return;

#ifndef DEBUG
		// Don't bother on a listen server - usually not desirable
		if ( !engine->IsDedicatedServer() )
			return;
#endif // DEBUG

		// Skip if we have a nextlevel set
		if ( !FStrEq( nextlevel.GetString(), "" ) )
			return;

		// Look for impending level change
		if ( ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate ) && GetTimeLeft() <= 300 )
			return;

		if ( GetRoundsRemaining() == 1 )
			return;

		if ( GetWinsRemaining() == 1 )
			return;

		// Increment win counters
		if ( m_iWinningTeam >= FIRST_GAME_TEAM )
		{
			GetGlobalTFTeam( m_iWinningTeam )->IncrementWins();
		}

		// Did we hit our win delta?
		int nWinDelta = -1;
		int nHighestWins = 0;

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			int nWins = GetGlobalTFTeam( i )->GetWinCount();

			if ( nWins > nHighestWins )
			{
				if ( nWinDelta == -1 )
				{
					nWinDelta = 0;
				}
				else
				{
					nWinDelta = nWins - nHighestWins;
				}

				nHighestWins = nWins;
			}
		}

		if ( nWinDelta >= mp_scrambleteams_auto_windifference.GetInt() )
		{
			// Let the server know we're going to scramble on round restart
#if defined( TDC_DLL )
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
			if ( event )
			{
				event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
				gameeventmanager->FireEvent( event );
			}
#else
			const char *pszMessage = "#game_scramble_onrestart";
			if ( pszMessage )
			{
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pszMessage );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pszMessage );
			}
#endif
			UTIL_LogPrintf( "World triggered \"ScrambleTeams_Auto\"\n" );

			SetScrambleTeams( true );
			ShouldResetScores( true, false );
			ShouldResetRoundsPlayed( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a stalemate
//			Most often a team_control_point_master saying that the
//			round timer expired 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetStalemate( int iReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false */ )
{
	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
		return;

	if ( !mp_stalemate_enable.GetBool() )
	{
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bForceMapReset, bSwitchTeams );
		return;
	}

	if ( InStalemate() )
		return;

	m_iWinningTeam = TEAM_UNASSIGNED;

	PlaySuddenDeathSong();

	State_Transition( GR_STATE_STALEMATE );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
	if ( event )
	{
		event->SetInt( "reason", iReason );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::IsGameUnderTimeLimit( void )
{
	return ( mp_timelimit.GetInt() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamRoundTimer *CTDCGameRules::GetActiveRoundTimer( void )
{
	int iTimerEntIndex = ObjectiveResource()->GetTimerInHUD();
	return ( dynamic_cast<CTeamRoundTimer *>( UTIL_EntityByIndex( iTimerEntIndex ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::HandleTimeLimitChange( void )
{
	// check that we have an active timer in the HUD and use mp_timelimit if we don't
	if ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0 )
	{
		CreateTimeLimitTimer();
	}
	else
	{
		if ( m_hTimeLimitTimer )
		{
			UTIL_Remove( m_hTimeLimitTimer );
			m_hTimeLimitTimer = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ResetPlayerAndTeamReadyState( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_bTeamReady.Set( i, false );
	}

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}

#ifdef GAME_DLL
	// Note <= MAX_PLAYERS vs < MAX_PLAYERS above
	for ( int i = 0; i <= MAX_PLAYERS; i++ )
	{
		m_bPlayerReadyBefore[i] = false;
	}
#endif // GAME_DLL
}

bool CTDCGameRules::PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext )
{
	if ( m_flNewThrottledAlertTime <= gpGlobals->curtime )
	{
		BroadcastSound( iTeam, sound );
		m_flNewThrottledAlertTime = gpGlobals->curtime + fDelayBeforeNext;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags )
{
	//send it to everyone
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_broadcast_audio" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		event->SetString( "sound", sound );
		event->SetInt( "additional_flags", iAdditionalSoundFlags );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::ShouldSkipAutoScramble( void )
{
	return ( IsInfectionMode() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetOvertime( bool bOvertime )
{
	if ( m_bInOvertime == bOvertime )
		return;

	if ( bOvertime )
	{
		UTIL_LogPrintf( "World triggered \"Round_Overtime\"\n" );
	}

	m_bInOvertime = bOvertime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::Activate()
{
	tdc_bot_count.SetValue( 0 );

	if ( IsInTraining() || IsInItemTestingMode() )
	{
		hide_server.SetValue( 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CheckChatText( CBasePlayer *pPlayer, char *pText )
{
	CheckChatForReadySignal( pPlayer, pText );

	BaseClass::CheckChatText( pPlayer, pText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg )
{
	if ( IsInTournamentMode() == false )
	{
		if ( m_bAwaitingReadyRestart && FStrEq( chatmsg, mp_clan_ready_signal.GetString() ) )
		{
			int iTeam = pPlayer->GetTeamNumber();
			if ( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() )
			{
				m_bTeamReady.Set( iTeam, true );

				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_team_ready" );
				if ( event )
				{
					event->SetInt( "team", iTeam );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetInWaitingForPlayers( bool bWaitingForPlayers )
{
	// never waiting for players when loading a bug report
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	if ( m_bInWaitingForPlayers == bWaitingForPlayers )
		return;

	if ( ShouldWaitForPlayersInPregame() && m_flWaitingForPlayersTimeEnds == -1 && IsInTournamentMode() == false )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	m_bInWaitingForPlayers = bWaitingForPlayers;

	if ( m_bInWaitingForPlayers )
	{
		m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();
	}
	else
	{
		m_flWaitingForPlayersTimeEnds = -1;

		if ( m_hWaitingForPlayersTimer )
		{
			UTIL_Remove( m_hWaitingForPlayersTimer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetSetup( bool bSetup )
{
	if ( m_bInSetup == bSetup )
		return;

	m_bInSetup = bSetup;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CheckWaitingForPlayers( void )
{
	// never waiting for players when loading a bug report, or training
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background || !AllowWaitingForPlayers() )
		return;

	if ( mp_waitingforplayers_restart.GetBool() )
	{
		if ( m_bInWaitingForPlayers )
		{
			m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();

			if ( m_hWaitingForPlayersTimer )
			{
				variant_t sVariant;
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			}
		}
		else
		{
			SetInWaitingForPlayers( true );
		}

		mp_waitingforplayers_restart.SetValue( 0 );
	}

	bool bCancelWait = ( mp_waitingforplayers_cancel.GetBool() || IsInItemTestingMode() ) && !IsInTournamentMode();

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		bCancelWait = true;
#endif // _DEBUG || STAGING_ONLY

	if ( bCancelWait )
	{
		mp_waitingforplayers_cancel.SetValue( 0 );

		if ( m_bInWaitingForPlayers )
		{
			if ( ShouldWaitForPlayersInPregame() )
			{
				m_flRestartRoundTime = gpGlobals->curtime;	// reset asap
				return;
			}

			// Cancel the wait period and manually Resume() the timer if 
			// it's not supposed to start paused at the beginning of a round.
			// We must do this before SetInWaitingForPlayers() is called because it will
			// restore the timer in the HUD and set the handle to NULL
			CTeamRoundTimer *pActiveTimer = dynamic_cast<CTeamRoundTimer *>( UTIL_EntityByIndex( ObjectiveResource()->GetTimerInHUD() ) );

			if ( pActiveTimer && !pActiveTimer->StartPaused() )
			{
				pActiveTimer->ResumeTimer();
			}

			SetInWaitingForPlayers( false );
			return;
		}
	}

	if ( m_bInWaitingForPlayers )
	{
		if ( IsInTournamentMode() == true )
			return;

		// only exit the waitingforplayers if the time is up, and we are not in a round
		// restart countdown already, and we are not waiting for a ready restart
		if ( gpGlobals->curtime > m_flWaitingForPlayersTimeEnds && m_flRestartRoundTime < 0 && !m_bAwaitingReadyRestart )
		{
			m_flRestartRoundTime = gpGlobals->curtime;	// reset asap
		}
		else
		{
			if ( !m_hWaitingForPlayersTimer )
			{
				// Stop any timers, and bring up a new one
				m_hWaitingForPlayersTimer = (CTeamRoundTimer*)CBaseEntity::Create( "game_round_timer", vec3_origin, vec3_angle );
				m_hWaitingForPlayersTimer->SetName( AllocPooledString( "zz_teamplay_waiting_timer" ) );
				m_hWaitingForPlayersTimer->SetShowInHud( true );

				variant_t sVariant;
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				m_hWaitingForPlayersTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CheckRestartRound( void )
{
	if ( mp_clan_readyrestart.GetBool() && IsInTournamentMode() == false )
	{
		m_bAwaitingReadyRestart = true;

		for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
		{
			m_bTeamReady.Set( i, false );
		}

		const char *pszReadyString = mp_clan_ready_signal.GetString();

		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#clan_ready_rules", pszReadyString );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#clan_ready_rules", pszReadyString );

		// Don't let them put anything malicious in there
		if ( pszReadyString == NULL || Q_strlen( pszReadyString ) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_ready_restart" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		mp_clan_readyrestart.SetValue( 0 );

		// cancel any restart round in progress
		m_flRestartRoundTime = -1;
	}

	// Restart the game if specified by the server
	int iRestartDelay = mp_restartround.GetInt();
	bool bRestartGameNow = mp_restartgame_immediate.GetBool();
	if ( iRestartDelay == 0 && !bRestartGameNow )
	{
		iRestartDelay = mp_restartgame.GetInt();
	}

	if ( iRestartDelay > 0 || bRestartGameNow )
	{
		int iDelayMax = 60;

		if ( iRestartDelay > iDelayMax )
		{
			iRestartDelay = iDelayMax;
		}

		SetInStopWatch( false );

		if ( bRestartGameNow )
		{
			iRestartDelay = 0;
		}

		m_flRestartRoundTime = gpGlobals->curtime + iRestartDelay;

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_restart_seconds" );
		if ( event )
		{
			event->SetInt( "seconds", iRestartDelay );
			gameeventmanager->FireEvent( event );
		}

		if ( IsInTournamentMode() == false )
		{
			// let the players know
			const char *pFormat = NULL;

			if ( mp_restartgame.GetInt() > 0 )
			{
				if ( ShouldSwitchTeams() )
				{
					pFormat = ( iRestartDelay > 1 ) ? "#game_switch_in_secs" : "#game_switch_in_sec";
				}
				else if ( ShouldScrambleTeams() )
				{
					pFormat = ( iRestartDelay > 1 ) ? "#game_scramble_in_secs" : "#game_scramble_in_sec";

#if defined ( TDC_DLL )
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
					if ( event )
					{
						event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
						gameeventmanager->FireEvent( event );
					}

					pFormat = NULL;
#endif
				}
			}
			else if ( mp_restartround.GetInt() > 0 )
			{
				pFormat = ( iRestartDelay > 1 ) ? "#round_restart_in_secs" : "#round_restart_in_sec";
			}

			if ( pFormat )
			{
				char strRestartDelay[64];
				Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pFormat, strRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pFormat, strRestartDelay );
			}
		}

		mp_restartround.SetValue( 0 );
		mp_restartgame.SetValue( 0 );
		mp_restartgame_immediate.SetValue( 0 );

		// cancel any ready restart in progress
		m_bAwaitingReadyRestart = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::CheckTimeLimit( bool bAllowEnd /*= true*/ )
{
	if ( IsInPreMatch() == true )
		return false;

	if ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate )
	{
		// If there's less than 5 minutes to go, just switch now. This avoids the problem
		// of sudden death modes starting shortly after a new round starts.
		const int iMinTime = 5;
		bool bSwitchDueToTime = ( mp_timelimit.GetInt() > iMinTime && GetTimeLeft() < ( iMinTime * 60 ) );

		if ( IsInTournamentMode() == true )
		{
			if ( TournamentModeCanEndWithTimelimit() == false )
			{
				return false;
			}

			bSwitchDueToTime = false;
		}
		else if ( IsInDuelMode() )
		{
			bSwitchDueToTime = false;
		}

		if ( GetTimeLeft() <= 0 || m_bChangelevelAfterStalemate || bSwitchDueToTime )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Time Limit" );
					gameeventmanager->FireEvent( event );
				}

				SendTeamScoresEvent();

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::CheckWinLimit( bool bAllowEnd /*= true*/ )
{
	// has one team won the specified number of rounds?
	if ( mp_winlimit.GetInt() <= 0 )
		return false;

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );

		if ( pTeam->GetScore() >= mp_winlimit.GetInt() + m_iWinLimitBonus )
		{
			if ( bAllowEnd )
			{
				UTIL_LogPrintf( "Team \"%s\" triggered \"Intermission_Win_Limit\"\n", pTeam->GetName() );

				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Win Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::CheckMaxRounds( bool bAllowEnd /*= true*/ )
{
	if ( mp_maxrounds.GetInt() > 0 && IsInPreMatch() == false )
	{
		if ( GetRoundsRemaining() == 0 )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Round Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::GetWinsRemaining( void )
{
	if ( mp_winlimit.GetInt() <= 0 )
		return -1;

	int iMaxWins = 0;

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );

		if ( pTeam->GetScore() > iMaxWins )
		{
			iMaxWins = pTeam->GetScore();
		}
	}

	return Max( mp_winlimit.GetInt() + m_iWinLimitBonus - iMaxWins, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::GetRoundsRemaining( void )
{
	if ( mp_maxrounds.GetInt() <= 0 )
		return -1;

	return Max( mp_maxrounds.GetInt() + m_iMaxRoundsBonus - m_nRoundsPlayed, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CheckReadyRestart( void )
{
	// check round restart
	if ( m_flRestartRoundTime > 0 && m_flRestartRoundTime <= gpGlobals->curtime && !g_pServerBenchmark->IsBenchmarkRunning() )
	{
		m_flRestartRoundTime = -1;

		// time to restart!
		State_Transition( GR_STATE_RESTART );
	}

	// check ready restart
	if ( m_bAwaitingReadyRestart )
	{
		bool bTeamNotReady = false;
		for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
		{
			if ( !m_bTeamReady[i] )
			{
				bTeamNotReady = true;
				break;
			}
		}

		if ( !bTeamNotReady )
		{
			mp_restartgame.SetValue( 5 );
			m_bAwaitingReadyRestart = false;

			ShouldResetScores( true, true );
			ShouldResetRoundsPlayed( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTDCGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::CanGoToStalemate( void )
{
	// In CTDC, don't go to stalemate if one of the flags isn't at home
	for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
	{
		if ( !pFlag->IsHome() )
			return false;
	}

	// check that one team hasn't won by capping
	bool bTied;
	if ( CheckScoreLimit( true, bTied ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Transition( gamerules_roundstate_t newState )
{
	m_prevState = State_Get();

	State_Leave();
	State_Enter( newState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter( gamerules_roundstate_t newState )
{
	m_iRoundState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	m_flLastRoundStateChangeTime = gpGlobals->curtime;

	if ( mp_showroundtransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			Msg( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "Gamerules: entering state #%d\n", newState );
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		( this->*m_pCurStateInfo->pfnEnterState )( );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		( this->*m_pCurStateInfo->pfnLeaveState )( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		( this->*m_pCurStateInfo->pfnThink )( );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGameRulesRoundStateInfo* CTDCGameRules::State_LookupInfo( gamerules_roundstate_t state )
{
	static CGameRulesRoundStateInfo playerStateInfos[] =
	{
		{ GR_STATE_INIT, "GR_STATE_INIT", &CTDCGameRules::State_Enter_INIT, NULL, &CTDCGameRules::State_Think_INIT },
		{ GR_STATE_PREGAME, "GR_STATE_PREGAME", &CTDCGameRules::State_Enter_PREGAME, NULL, &CTDCGameRules::State_Think_PREGAME },
		{ GR_STATE_STARTGAME, "GR_STATE_STARTGAME", &CTDCGameRules::State_Enter_STARTGAME, NULL, &CTDCGameRules::State_Think_STARTGAME },
		{ GR_STATE_PREROUND, "GR_STATE_PREROUND", &CTDCGameRules::State_Enter_PREROUND, &CTDCGameRules::State_Leave_PREROUND, &CTDCGameRules::State_Think_PREROUND },
		{ GR_STATE_RND_RUNNING, "GR_STATE_RND_RUNNING", &CTDCGameRules::State_Enter_RND_RUNNING, NULL, &CTDCGameRules::State_Think_RND_RUNNING },
		{ GR_STATE_TEAM_WIN, "GR_STATE_TEAM_WIN", &CTDCGameRules::State_Enter_TEAM_WIN, NULL, &CTDCGameRules::State_Think_TEAM_WIN },
		{ GR_STATE_RESTART, "GR_STATE_RESTART", &CTDCGameRules::State_Enter_RESTART, NULL, &CTDCGameRules::State_Think_RESTART },
		{ GR_STATE_STALEMATE, "GR_STATE_STALEMATE", &CTDCGameRules::State_Enter_STALEMATE, &CTDCGameRules::State_Leave_STALEMATE, &CTDCGameRules::State_Think_STALEMATE },
		{ GR_STATE_GAME_OVER, "GR_STATE_GAME_OVER", NULL, NULL, NULL },
		{ GR_STATE_BONUS, "GR_STATE_BONUS", &CTDCGameRules::State_Enter_BONUS, &CTDCGameRules::State_Leave_BONUS, &CTDCGameRules::State_Think_BONUS },
		{ GR_STATE_BETWEEN_RNDS, "GR_STATE_BETWEEN_RNDS", &CTDCGameRules::State_Enter_BETWEEN_RNDS, &CTDCGameRules::State_Leave_BETWEEN_RNDS, &CTDCGameRules::State_Think_BETWEEN_RNDS },
	};

	for ( int i = 0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iRoundState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_INIT( void )
{
	InitTeams();
	ResetMapTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_INIT( void )
{
	State_Transition( GR_STATE_PREGAME );
}

//-----------------------------------------------------------------------------
// Purpose: The server is idle and waiting for enough players to start up again. 
//			When we find an active player go to GR_STATE_STARTGAME.
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_PREGAME( void )
{
	m_flNextPeriodicThink = gpGlobals->curtime + 0.1;
	SetInWaitingForPlayers( false );
	m_flRestartRoundTime = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_PREGAME( void )
{
	CheckRespawns();

	// we'll just stay in pregame for the bugbait reports
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
		return;

	// Commentary stays in this mode too
	if ( IsInCommentaryMode() )
		return;

	if ( ShouldWaitForPlayersInPregame() )
	{
		// In Arena mode, start waiting for players period once we have enough players ready to play.
		if ( CountActivePlayers() > 0 )
		{
			if ( !IsInWaitingForPlayers() )
			{
				m_flWaitingForPlayersTimeEnds = 0.0f;
				SetInWaitingForPlayers( true );
			}

			CheckReadyRestart();
		}
		else
		{
			if ( IsInWaitingForPlayers() )
			{
				SetInWaitingForPlayers( false );
				m_flRestartRoundTime = -1.0f;
			}
		}
	}
	else if ( CountActivePlayers() > 0 )
	{
		State_Transition( GR_STATE_STARTGAME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Wait a bit and then spawn everyone into the preround
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_STARTGAME( void )
{
	m_flStateTransitionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_STARTGAME()
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		if ( !IsInTraining() && !IsInItemTestingMode() )
		{
			ConVarRef tf_bot_offline_practice( "tf_bot_offline_practice" );
			if ( mp_waitingforplayers_time.GetFloat() > 0 && tf_bot_offline_practice.GetInt() == 0 )
			{
				// go into waitingforplayers, reset at end of it
				SetInWaitingForPlayers( true );
			}
		}

		State_Transition( GR_STATE_PREROUND );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_PREROUND( void )
{
	BalanceTeams( false );

	m_flStartBalancingTeamsAt = gpGlobals->curtime + 60.0;

	RoundRespawn();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_start" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	if ( m_bPreRoundFreezeTime )
	{
		m_flStateTransitionTime = gpGlobals->curtime + 5 * mp_enableroundwaittime.GetFloat();
	}
	else
	{
		m_flStateTransitionTime = gpGlobals->curtime;
	}

	StopWatchModeThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Leave_PREROUND( void )
{
	PreRound_End();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_PREROUND( void )
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		State_Transition( GR_STATE_RND_RUNNING );
	}

	CheckRespawns();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_RND_RUNNING( void )
{
	SetupOnRoundRunning();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_active" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	if ( !IsInWaitingForPlayers() )
	{
		PlayStartRoundVoice();
	}

	m_bChangeLevelOnRoundEnd = false;

	m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_RND_RUNNING( void )
{
	//if we don't find any active players, return to GR_STATE_PREGAME
	if ( CountActivePlayers() <= 0 )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate
			g_pReplay->SV_EndRecordingSession();
		}
#endif

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( m_flNextBalanceTeamsTime < gpGlobals->curtime )
	{
		BalanceTeams( true );
		m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
	}

	CheckRespawns();

	// check round restart
	CheckReadyRestart();

	// See if we're coming up to the server timelimit, in which case force a stalemate immediately.
	if ( mp_timelimit.GetInt() > 0 && IsInPreMatch() == false && GetTimeLeft() <= 0 )
	{
		if ( m_bAllowStalemateAtTimelimit || ( mp_match_end_at_timelimit.GetBool() && !IsValveMap() ) )
		{
			if ( !IsRoundBasedMode() )
			{
				bool bTied;
				CheckScoreLimit( false, bTied );

				// If two or more players are tied for the lead the game continues until the tie is broken.
				if ( bTied )
				{
					SetOvertime( true );
				}
				else
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
					if ( event )
					{
						event->SetString( "reason", "Reached Time Limit" );
						gameeventmanager->FireEvent( event );
					}

					GoToIntermission();
				}
			}
			else
			{
				int iDrawScoreCheck = -1;
				int iWinningTeam = 0;
				bool bTeamsAreDrawn = true;
				for ( int i = FIRST_GAME_TEAM; ( i < GetNumberOfTeams() ) && bTeamsAreDrawn; i++ )
				{
					int iTeamScore = GetGlobalTeam( i )->GetScore();

					if ( iTeamScore > iDrawScoreCheck )
					{
						iWinningTeam = i;
					}

					if ( iTeamScore != iDrawScoreCheck )
					{
						if ( iDrawScoreCheck == -1 )
						{
							iDrawScoreCheck = iTeamScore;
						}
						else
						{
							bTeamsAreDrawn = false;
						}
					}
				}

				if ( bTeamsAreDrawn )
				{
					if ( CanGoToStalemate() )
					{
						m_bChangelevelAfterStalemate = true;
						SetStalemate( STALEMATE_SERVER_TIMELIMIT );
					}
					else
					{
						SetOvertime( true );
					}
				}
				else
				{
					SetWinningTeam( iWinningTeam, WINREASON_TIMELIMIT, true, false, true );
				}
			}
		}
	}

	StopWatchModeThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_TEAM_WIN( void )
{
	m_flStateTransitionTime = gpGlobals->curtime + GetBonusRoundTime();
	m_nRoundsPlayed++;

	InternalHandleTeamWin( m_iWinningTeam );

	SendWinPanelInfo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_TEAM_WIN( void )
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		bool bDone = ( CheckTimeLimit() || CheckWinLimit() || CheckMaxRounds() || CheckNextLevelCvar() );

		// check the win limit, max rounds, time limit and nextlevel cvar before starting the next round
		if ( !bDone )
		{
			PreviousRoundEnd();

			if ( ShouldGoToBonusRound() )
			{
				State_Transition( GR_STATE_BONUS );
			}
			else
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate
					g_pReplay->SV_EndRecordingSession();
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
		}
		else if ( IsInTournamentMode() )
		{
			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( !pPlayer )
					continue;

				pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
			}

			RestartTournament();

			State_Transition( GR_STATE_RND_RUNNING );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_STALEMATE( void )
{
	m_flStalemateStartTime = gpGlobals->curtime;
	SetupOnStalemateStart();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
		m_hStalemateTimer = NULL;
	}

	int iTimeLimit = mp_stalemate_timelimit.GetInt();

	if ( iTimeLimit > 0 )
	{
		variant_t sVariant;
		if ( !m_hStalemateTimer )
		{
			m_hStalemateTimer = (CTeamRoundTimer*)CBaseEntity::Create( "game_round_timer", vec3_origin, vec3_angle );
			m_hStalemateTimer->SetName( AllocPooledString( "zz_stalemate_timer" ) );
			m_hStalemateTimer->SetShowInHud( true );
		}

		sVariant.SetInt( iTimeLimit );
		m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Leave_STALEMATE( void )
{
	SetupOnStalemateEnd();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_BONUS( void )
{
	SetupOnBonusStart();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Leave_BONUS( void )
{
	SetupOnBonusEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_BONUS( void )
{
	BonusStateThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_BETWEEN_RNDS( void )
{
	BetweenRounds_Start();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Leave_BETWEEN_RNDS( void )
{
	BetweenRounds_End();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_BETWEEN_RNDS( void )
{
	BetweenRounds_Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_STALEMATE( void )
{
	//if we don't find any active players, return to GR_STATE_PREGAME
	if ( CountActivePlayers() <= 0 )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate
			g_pReplay->SV_EndRecordingSession();
		}
#endif

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( IsInTournamentMode() == true && IsInWaitingForPlayers() == true )
	{
		CheckReadyRestart();
		CheckRespawns();
		return;
	}

	// If a game has more than 2 active teams, the old function won't work.
	// Which is why we had to replace it with this one.
	CUtlVector<CTeam *> pAliveTeams;

	// Last team standing wins.
	for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		Assert( pTeam );

		int iPlayers = pTeam->GetNumPlayers();
		if ( iPlayers )
		{
			bool bFoundLiveOne = false;
			for ( int player = 0; player < iPlayers; player++ )
			{
				if ( pTeam->GetPlayer( player ) && pTeam->GetPlayer( player )->IsAlive() )
				{
					bFoundLiveOne = true;
					break;
				}
			}

			if ( bFoundLiveOne )
			{
				pAliveTeams.AddToTail( pTeam );
			}
		}
	}

	if ( pAliveTeams.Count() == 1 )
	{
		// The live team has won. 
		int iAliveTeam = pAliveTeams[0]->GetTeamNumber();
		SetWinningTeam( iAliveTeam, WINREASON_OPPONENTS_DEAD );
	}
	else if ( pAliveTeams.Count() == 0 ||
		( m_hStalemateTimer && TimerMayExpire() && m_hStalemateTimer->GetTimeRemaining() <= 0 ) )
	{
		// Both teams are dead. Pure stalemate.
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: manual restart
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Enter_RESTART( void )
{
	// send scores
	SendTeamScoresEvent();

	// send restart event
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_restart_round" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	SetInWaitingForPlayers( false );

	ResetScores();

	// reset the round time
	ResetMapTime();

	State_Transition( GR_STATE_PREROUND );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::State_Think_RESTART( void )
{
	// should never get here, State_Enter_RESTART sets us into a different state
	Assert( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ResetTeamsRoundWinTracking( void )
{
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		GetGlobalTFTeam( i )->ResetWins();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::InitTeams( void )
{
	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::CountActivePlayers( void )
{
	int count = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer == nullptr )
			continue;

		if ( pPlayer->IsReadyToPlay() )
			++count;
	}

	// In Duel and Infected modes game cannot start unless there are at least two players.
	if ( IsInDuelMode() || IsInfectionMode() )
	{
		if ( State_Get() <= GR_STATE_PREGAME && count < 2 )
			return 0;
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::RoundRespawn( void )
{
	// remove any buildings, grenades, rockets, etc. the player put into the world
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
		}
	}

	// reset round score
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < GetNumberOfTeams(); ++iTeam )
	{
		CTDCTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->ResetRoundScore();
	}

	CTDC_GameStats.ResetRoundStats();
	m_flRoundStartTime = gpGlobals->curtime;
	CleanUpMap();

	if ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0 )
	{
		CreateTimeLimitTimer();
	}

	if ( !IsInWaitingForPlayers() )
	{
		UTIL_LogPrintf( "World triggered \"Round_Start\"\n" );
	}

	// Setup before respawning players, so we can mess with spawnpoints
	SetupOnRoundStart();

	// Do we need to switch the teams?
	m_bSwitchedTeamsThisRound = false;
	if ( ShouldSwitchTeams() )
	{
		m_bSwitchedTeamsThisRound = true;
		HandleSwitchTeams();
		SetSwitchTeams( false );
	}

	// Do we need to switch the teams?
	if ( ShouldScrambleTeams() )
	{
		HandleScrambleTeams();
		SetScrambleTeams( false );
	}

#if defined( REPLAY_ENABLED )
	bool bShouldWaitToStartRecording = ShouldWaitToStartRecording();
	if ( g_pReplay && g_pReplay->SV_ShouldBeginRecording( bShouldWaitToStartRecording ) )
	{
		// Tell the replay manager that it should begin recording the new round as soon as possible
		g_pReplay->SV_GetContext()->GetSessionRecorder()->StartRecording();
	}
#endif

	// Free any edicts that were marked deleted. This should hopefully clear some out
	//  so the below function can use the now freed ones.
	engine->AllowImmediateEdictReuse();

	RespawnPlayers( true );

	// reset per-round scores for each player
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->ResetPerRoundStats();
		}
	}

	if ( m_bVoteMapOnNextRound )
	{
		// Vote has been scheduled.
		StartNextMapVote();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CleanUpMap( void )
{
	if ( mp_showcleanedupents.GetInt() )
	{
		Msg( "CleanUpMap\n===============\n" );
		Msg( "  Entities: %d (%d edicts)\n", gEntList.NumberOfEntities(), gEntList.NumberOfEdicts() );
	}

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		if ( !RoundCleanupShouldIgnore( pCur ) )
		{
			if ( mp_showcleanedupents.GetInt() & 1 )
			{
				Msg( "Removed Entity: %s\n", pCur->GetClassname() );
			}
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Clear out the event queue
	g_EventQueue.Clear();

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	engine->AllowImmediateEdictReuse();

	if ( mp_showcleanedupents.GetInt() & 2 )
	{
		Msg( "  Entities Left:\n" );
		pCur = gEntList.FirstEnt();
		while ( pCur )
		{
			Msg( "  %s (%d)\n", pCur->GetClassname(), pCur->entindex() );
			pCur = gEntList.NextEnt( pCur );
		}
	}

	// Now reload the map entities.
	class CTeamplayMapEntityFilter : public IMapEntityFilter
	{
	public:
		CTeamplayMapEntityFilter()
		{
			m_pRules = TDCGameRules();
		}

		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( m_pRules->ShouldCreateEntity( pClassname ) )
				return true;

			// Increment our iterator since it's not going to call CreateNextEntity for this ent.
			if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );
			}

			return false;
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CTeamplayMapEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
		CTDCGameRules *m_pRules;
	};
	CTeamplayMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}

	// Process event queue now, we want all inputs to be received before players spawn.
	g_EventQueue.ServiceEvents();
}

//-----------------------------------------------------------------------------
// Purpose: Called by the gamemode to prompt a respawn check on all players.
//-----------------------------------------------------------------------------
void CTDCGameRules::CheckRespawns( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer* pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( pPlayer->IsAlive() )
			continue;

		if ( HasRespawnWaitPeriodPassed( pPlayer ) && pPlayer->IsReadyToSpawn() )
		{
			pPlayer->ForceRespawn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for batching players in teamplay modes.
//-----------------------------------------------------------------------------
float CTDCGameRules::GetNextRespawnWave( int iTeam, CBasePlayer* pBasePlayer )
{
	CTDCPlayer* pPlayer = ToTDCPlayer( pBasePlayer );
	if ( !pPlayer )
		return 0.0f;

	float flRespawnTime = GetRespawnTime( pPlayer );
	if ( !flRespawnTime )
		return 0.0f;

	if ( IsTeamplay() )
	{
		//TODO: Bundle
	}

	return flRespawnTime + gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Get the respawn time for the current gamemode.
//-----------------------------------------------------------------------------
float CTDCGameRules::GetRespawnTime( CTDCPlayer* pPlayer )
{
	float flRespawnTime = 0.0f;

	switch ( GetGameType() )
	{
	case TDC_GAMETYPE_FFA:
		flRespawnTime = tdc_ffa_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_TDM:
		flRespawnTime = tdc_tdm_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_DUEL:
		flRespawnTime = tdc_duel_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_BM:
		flRespawnTime = tdc_bm_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_TBM:
		flRespawnTime = tdc_tbm_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_CTF:
		flRespawnTime = tdc_ctf_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_ATTACK_DEFEND:
		flRespawnTime = tdc_ad_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_INVADE:
		flRespawnTime = tdc_invade_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_INFECTION:
		if ( !( pPlayer->IsZombie() && m_hLastStandingPlayer ) )
			flRespawnTime = tdc_infection_respawntime.GetFloat();
		break;
	case TDC_GAMETYPE_VIP:
		flRespawnTime = tdc_vip_respawntime.GetFloat();
		break;
	default:
		break;
	}

	return flRespawnTime;
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for sorting players by time spent connected ( user ID )
//-----------------------------------------------------------------------------
bool CTDCGameRules::HasRespawnWaitPeriodPassed( CTDCPlayer* pPlayer )
{
	if ( !pPlayer )
		return false;

	return ( pPlayer->m_Shared.IsWaitingToRespawn() || pPlayer->IsBot() ) && ( pPlayer->m_Shared.GetTimeUntilRespawn() < 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for sorting players by time spent connected ( user ID )
//-----------------------------------------------------------------------------
static int SwitchPlayersSort( CBaseMultiplayerPlayer * const *p1, CBaseMultiplayerPlayer * const *p2 )
{
	// sort by score
	return ( ( *p2 )->GetTeamBalanceScore() - ( *p1 )->GetTeamBalanceScore() );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the teams are balanced after this function
//-----------------------------------------------------------------------------
void CTDCGameRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	if ( mp_autoteambalance.GetBool() == false )
	{
		return;
	}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return;
#endif // _DEBUG || STAGING_ONLY

	if ( IsInTraining() || IsInItemTestingMode() )
	{
		return;
	}

	// we don't balance for a period of time at the start of the game
	if ( gpGlobals->curtime < m_flStartBalancingTeamsAt )
	{
		return;
	}

	// wrap with this bool, indicates it's a round running switch and not a between rounds insta-switch
	if ( bRequireSwitcheesToBeDead )
	{
		// we don't balance if there is less than 60 seconds on the active timer
		CTeamRoundTimer *pActiveTimer = GetActiveRoundTimer();
		if ( pActiveTimer && pActiveTimer->GetTimeRemaining() < 60 )
		{
			return;
		}
	}

	int iHeaviestTeam = TEAM_UNASSIGNED, iLightestTeam = TEAM_UNASSIGNED;

	// Figure out if we're unbalanced
	if ( !AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
	{
		m_flFoundUnbalancedTeamsTime = -1;
		m_bPrintedUnbalanceWarning = false;
		return;
	}

	if ( m_flFoundUnbalancedTeamsTime < 0 )
	{
		m_flFoundUnbalancedTeamsTime = gpGlobals->curtime;
	}

	// if teams have been unbalanced for X seconds, play a warning 
	if ( !m_bPrintedUnbalanceWarning && ( ( gpGlobals->curtime - m_flFoundUnbalancedTeamsTime ) > 1.0 ) )
	{
		// print unbalance warning
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#TDC_AutoBalance_Warning" );
		m_bPrintedUnbalanceWarning = true;
	}

	// teams are unblanced, figure out some players that need to be switched

	DevMsg( "Auto-balanced started.\n" );
	bool bTeamsChanged;

	do
	{
		CTeam *pHeavyTeam = GetGlobalTeam( iHeaviestTeam );
		CTeam *pLightTeam = GetGlobalTeam( iLightestTeam );

		Assert( pHeavyTeam && pLightTeam );

		bTeamsChanged = false;
		int iNumSwitchesRequired = ( pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers() ) / 2;

		// sort the eligible players and switch the n best candidates
		CUtlVector<CBaseMultiplayerPlayer *> vecPlayers;

		CBaseMultiplayerPlayer *pPlayer;

		int iScore;

		int i;
		for ( i = 0; i < pHeavyTeam->GetNumPlayers(); i++ )
		{
			pPlayer = ToBaseMultiplayerPlayer( pHeavyTeam->GetPlayer( i ) );

			if ( !pPlayer )
				continue;

			if ( !pPlayer->CanBeAutobalanced() )
				continue;

			// calculate a score for this player. higher is more likely to be switched
			iScore = pPlayer->CalculateTeamBalanceScore();

			pPlayer->SetTeamBalanceScore( iScore );

			vecPlayers.AddToTail( pPlayer );
		}

		// sort the vector
		vecPlayers.Sort( SwitchPlayersSort );

		DevMsg( "Taking players from %s to %s. %d switches required.\n", pHeavyTeam->GetName(), pLightTeam->GetName(), iNumSwitchesRequired );

		int iNumCandiates = iNumSwitchesRequired + 2;

		for ( int i = 0; i < vecPlayers.Count() && iNumSwitchesRequired > 0 && i < iNumCandiates; i++ )
		{
			pPlayer = vecPlayers.Element( i );

			Assert( pPlayer );

			if ( !pPlayer )
				continue;

			if ( bRequireSwitcheesToBeDead == false || !pPlayer->IsAlive() )
			{
				// We're trying to avoid picking a player that's recently
				// been auto-balanced by delaying their selection in the hope
				// that a better candidate comes along.
				if ( bRequireSwitcheesToBeDead )
				{
					int nPlayerTeamBalanceScore = pPlayer->CalculateTeamBalanceScore();

					// Do we already have someone in the queue?
					if ( m_nAutoBalanceQueuePlayerIndex > 0 )
					{
						// Is this player's score worse?
						if ( nPlayerTeamBalanceScore < m_nAutoBalanceQueuePlayerScore )
						{
							m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
							m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
						}
					}
					// Has this person been switched recently?
					else if ( nPlayerTeamBalanceScore < -10000 )
					{
						// Put them in the queue
						m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
						m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
						m_flAutoBalanceQueueTimeEnd = gpGlobals->curtime + 3.0f;

						continue;
					}

					// If this is the player in the queue...
					if ( m_nAutoBalanceQueuePlayerIndex == pPlayer->entindex() )
					{
						// Pass until their timer is up
						if ( m_flAutoBalanceQueueTimeEnd > gpGlobals->curtime )
							continue;
					}
				}

				pPlayer->ChangeTeam( iLightestTeam, false, true );
				pPlayer->SetLastForcedChangeTeamTimeToNow();

				m_nAutoBalanceQueuePlayerScore = -1;
				m_nAutoBalanceQueuePlayerIndex = -1;

				// tell people that we've switched this player
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
				if ( event )
				{
					event->SetInt( "player", pPlayer->entindex() );
					event->SetInt( "team", iLightestTeam );
					gameeventmanager->FireEvent( event );
				}

				iNumSwitchesRequired--;

				DevMsg( "Moved %s(%d) to team %s. %d switches remaining.\n", pPlayer->GetPlayerName(), i, pLightTeam->GetName(), iNumSwitchesRequired );

				int iOldLightest = iLightestTeam;
				int iOldHeaviest = iHeaviestTeam;

				// Re-calculate teams unbalanced state after each swap.
				if ( AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
				{
					if ( iHeaviestTeam != iOldHeaviest || iLightestTeam != iOldLightest )
					{
						// Recalculate players to be swapped.
						bTeamsChanged = true;
						break;
					}
				}
			}
		}
	} while ( bTeamsChanged );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ResetScores( void )
{
	if ( m_bResetTeamScores )
	{
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			GetGlobalTeam( i )->ResetScores();
		}

		m_iWinLimitBonus = 0;
	}

	if ( m_bResetPlayerScores )
	{
		CBasePlayer *pPlayer;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer == NULL )
				continue;

			if ( FNullEnt( pPlayer->edict() ) )
				continue;

			pPlayer->ResetScores();
		}

		m_iFragLimitBonus = 0;
	}

	if ( m_bResetRoundsPlayed )
	{
		m_nRoundsPlayed = 0;
		m_iMaxRoundsBonus = 0;
	}

	// assume we always want to reset the scores 
	// unless someone tells us not to for the next reset 
	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	//m_flStopWatchTime = -1.0f;

	IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_reset" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ResetMapTime( void )
{
	m_flMapResetTime = gpGlobals->curtime;
	m_iMapTimeBonus = 0;

	m_bVoteMapOnNextRound = false;
	m_bVotedForNextMap = false;

	// send an event with the time remaining until map change
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_map_time_remaining" );
	if ( event )
	{
		event->SetInt( "seconds", GetTimeLeft() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::PlayStartRoundVoice( void )
{
	g_TFAnnouncer.Speak( TDC_ANNOUNCER_ARENA_ROUNDSTART );

	/*
	for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, UTIL_VarArgs( "Game.TeamRoundStart%d", i ) );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::PlayWinSong( int team )
{
	if ( IsInfectionMode() )
	{
		if ( team == TDC_TEAM_HUMANS )
		{
			g_TFAnnouncer.Speak( TDC_ANNOUNCER_INFECTION_HUMANSWIN );
		}
		else if ( team == TDC_TEAM_ZOMBIES )
		{
			g_TFAnnouncer.Speak( TDC_ANNOUNCER_INFECTION_ZOMBIESWIN );
		}

		return;
	}

	if ( team == TEAM_UNASSIGNED )
	{
		PlayStalemateSong();
	}
	else
	{
		//BroadcastSound( TEAM_UNASSIGNED, UTIL_VarArgs( "Game.TeamWin%d", team ) );

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			g_TFAnnouncer.Speak( i, ( i == team ) ? TDC_ANNOUNCER_VICTORY : TDC_ANNOUNCER_DEFEAT );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::PlaySuddenDeathSong( void )
{
	g_TFAnnouncer.Speak( TDC_ANNOUNCER_SUDDENDEATH );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::PlayStalemateSong( void )
{
	g_TFAnnouncer.Speak( TDC_ANNOUNCER_STALEMATE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
			continue;

		if ( pPlayer->GetTeamNumber() != iWinningTeam )
		{
			pPlayer->ClearExpression();

			pPlayer->DropFlag();

			// Hide their weapon.
			CTDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
			if ( pWeapon )
			{
				pWeapon->SetWeaponVisible( false );
			}
		}
		else
		{
			pPlayer->m_Shared.AddCond( TDC_COND_CRITBOOSTED_BONUS_TIME );
		}

		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::MapHasActiveTimer( void )
{
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "game_round_timer" ) ) != NULL )
	{
		CTeamRoundTimer *pTimer = assert_cast<CTeamRoundTimer*>( pEntity );
		if ( pTimer && pTimer->ShowInHud() && ( Q_stricmp( STRING( pTimer->GetEntityName() ), "zz_teamplay_timelimit_timer" ) != 0 ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::CreateTimeLimitTimer( void )
{
	// this is the same check we use in State_Think_RND_RUNNING()
	// don't show the timelimit timer if we're not going to end the map when it runs out
	bool bAllowStalemate = ( m_bAllowStalemateAtTimelimit || ( mp_match_end_at_timelimit.GetBool() && !IsValveMap() ) );
	if ( !bAllowStalemate )
		return;

	if ( !m_hTimeLimitTimer )
	{
		m_hTimeLimitTimer = (CTeamRoundTimer*)CBaseEntity::Create( "game_round_timer", vec3_origin, vec3_angle );
		m_hTimeLimitTimer->SetName( AllocPooledString( "zz_teamplay_timelimit_timer" ) );
		m_hTimeLimitTimer->SetShowInHud( true );
	}

	variant_t sVariant;
	sVariant.SetInt( GetTimeLeft() );
	m_hTimeLimitTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
	m_hTimeLimitTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
}

// Skips players except for the specified one.
class CTraceFilterHitPlayer : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterHitPlayer, CTraceFilterSimple );

	CTraceFilterHitPlayer( const IHandleEntity *passentity, IHandleEntity *pHitEntity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
		m_pHitEntity = pHitEntity;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( !pEntity )
			return false;

		if ( pEntity->IsPlayer() && pEntity != m_pHitEntity )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

private:
	const IHandleEntity *m_pHitEntity;
};

CTDCRadiusDamageInfo::CTDCRadiusDamageInfo()
{
	m_iClassIgnore = CLASS_NONE;
	m_pEntityIgnore = NULL;
	m_flSelfDamageRadius = 0.0f;
}

bool CTDCRadiusDamageInfo::ApplyToEntity( CBaseEntity *pEntity ) const
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&( ~CONTENTS_HITBOX );
	trace_t		tr;
	float		falloff;
	Vector		vecSpot;

	if ( m_DmgInfo.GetDamageType() & DMG_HALF_FALLOFF || ( pEntity == m_DmgInfo.GetAttacker() && m_DmgInfo.GetWeapon() ) )
	{
		// Always use 0.5 for self-damage so that rocket jumping is not screwed up.
		falloff = 0.5;
	}
	else
	{
		falloff = 0.0;
	}

	CBaseEntity *pInflictor = m_DmgInfo.GetInflictor();
	CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );

	//	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// This value is used to scale damage when the explosion is blocked by some other object.
	float flBlockedDamagePercent = 0.0f;

	// Check that the explosion can 'see' this entity, trace through players.
	vecSpot = pEntity->BodyTarget( m_vecSrc, false );
	CTraceFilterHitPlayer filter( m_DmgInfo.GetInflictor(), pEntity, COLLISION_GROUP_PROJECTILE );
	UTIL_TraceLine( m_vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );

	if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
		return false;

	// Adjust the damage - apply falloff.
	float flAdjustedDamage = 0.0f;
	float flDistanceToEntity;

	// Rockets store the ent they hit as the enemy and have already
	// dealt full damage to them by this time
	if ( pInflictor && pEntity == pInflictor->GetEnemy() )
	{
		// Full damage, we hit this entity directly
		flDistanceToEntity = 0;
	}
	else if ( pEntity->IsPlayer() )
	{
		// Use whichever is closer, absorigin or worldspacecenter
		float flToWorldSpaceCenter = ( m_vecSrc - pEntity->WorldSpaceCenter() ).Length();
		float flToOrigin = ( m_vecSrc - pEntity->GetAbsOrigin() ).Length();

		flDistanceToEntity = Min( flToWorldSpaceCenter, flToOrigin );
	}
	else
	{
		flDistanceToEntity = ( m_vecSrc - tr.endpos ).Length();
	}

	flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, m_flRadius, m_DmgInfo.GetDamage(), m_DmgInfo.GetDamage() * falloff );
	if ( flAdjustedDamage <= 0 )
		return false;

	// the explosion can 'see' this entity, so hurt them!
	if ( tr.startsolid )
	{
		// if we're stuck inside them, fixup the position and distance
		tr.endpos = m_vecSrc;
		tr.fraction = 0.0;
	}

	CTakeDamageInfo adjustedInfo = m_DmgInfo;
	//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
	adjustedInfo.SetDamage( flAdjustedDamage - ( flAdjustedDamage * flBlockedDamagePercent ) );

	if ( m_iWeaponID == WEAPON_FLAREGUN && pPlayer )
	{
		if ( pInflictor && pPlayer == pInflictor->GetEnemy() )
		{
			if ( pPlayer->m_Shared.InCond( TDC_COND_BURNING ) )
			{
				// Do critical damage to burning players on direct hit.
				adjustedInfo.AddDamageType( DMG_CRITICAL );
			}
		}
	}

	Vector dir = vecSpot - m_vecSrc;
	VectorNormalize( dir );

	// If we don't have a damage force, manufacture one
	if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
	{
		CalculateExplosiveDamageForce( &adjustedInfo, dir, m_vecSrc );
	}
	else
	{
		// Assume the force passed in is the maximum force. Decay it based on falloff.
		float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
		adjustedInfo.SetDamageForce( dir * flForce );
		adjustedInfo.SetDamagePosition( m_vecSrc );
	}

	if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
	{
		adjustedInfo.SetDamagePosition( tr.endpos );

		ClearMultiDamage();
		pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
		ApplyMultiDamage();
	}
	else
	{
		pEntity->TakeDamage( adjustedInfo );
	}

	// Now hit all triggers along the way that respond to damage... 
	pEntity->TraceAttackToTriggers( adjustedInfo, m_vecSrc, tr.endpos, dir );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::RadiusDamage( CTDCRadiusDamageInfo &radiusInfo )
{
	CTakeDamageInfo &info = radiusInfo.m_DmgInfo;
	CBaseEntity *pAttacker = info.GetAttacker();
	int iPlayersDamaged = 0;

	CBaseEntity *pEntity = NULL;
	for ( CEntitySphereQuery sphere( radiusInfo.m_vecSrc, radiusInfo.m_flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( pEntity == radiusInfo.m_pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( radiusInfo.m_iClassIgnore != CLASS_NONE && pEntity->Classify() == radiusInfo.m_iClassIgnore )
		{
			continue;
		}

		// Skip the attacker as we'll handle him separately.
		if ( pEntity == pAttacker )
			continue;

		// Checking distance from source because Valve were apparently too lazy to fix the engine function.
		Vector vecHitPoint;
		pEntity->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );

		if ( vecHitPoint.DistToSqr( radiusInfo.m_vecSrc ) > Square( radiusInfo.m_flRadius ) )
			continue;

		if ( radiusInfo.ApplyToEntity( pEntity ) )
		{
			CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );

			if ( pPlayer && pPlayer->IsEnemy( pAttacker ) )
			{
				iPlayersDamaged++;
			}
		}
	}

	info.SetDamagedOtherPlayers( iPlayersDamaged );

	// For attacker, radius and damage need to be consistent so custom weapons don't screw up rocket jumping.
	if ( pAttacker )
	{
		if ( radiusInfo.m_flSelfDamageRadius )
		{
			// Use stock radius.
			radiusInfo.m_flRadius = radiusInfo.m_flSelfDamageRadius;
		}

		Vector vecHitPoint;
		pAttacker->CollisionProp()->CalcNearestPoint( radiusInfo.m_vecSrc, &vecHitPoint );

		if ( vecHitPoint.DistToSqr( radiusInfo.m_vecSrc ) <= Square( radiusInfo.m_flRadius ) )
		{
			radiusInfo.ApplyToEntity( pAttacker );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTDCGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	CTDCRadiusDamageInfo radiusInfo;
	radiusInfo.m_DmgInfo = info;
	radiusInfo.m_vecSrc = vecSrcIn;
	radiusInfo.m_flRadius = flRadius;
	radiusInfo.m_iClassIgnore = iClassIgnore;
	radiusInfo.m_pEntityIgnore = pEntityIgnore;

	RadiusDamage( radiusInfo );
}

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		if ( sv_alltalk.GetBool() )
			return true;

		// Dead players can only be heard by other dead team mates but only if a match is in progress
		if ( TDCGameRules()->State_Get() != GR_STATE_TEAM_WIN && TDCGameRules()->State_Get() != GR_STATE_GAME_OVER ) 
		{
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false || tdc_gravetalk.GetBool() )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
/*
// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TDC_TEAM_RED, TDC_TEAM_BLUE, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};
*/
// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //
	
// World.cpp calls this but we don't use it in TF.
void InitBodyQue()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCGameRules::~CTDCGameRules()
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	TFTeamMgr()->Shutdown();
	ShutdownCustomResponseRulesDicts();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::LevelShutdown()
{
	hide_server.Revert();
}

//-----------------------------------------------------------------------------
// Purpose: TDC Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CTDCGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( pEdict );

	const char *pcmd = args[0];

	// Handle some player commands here as they relate more directly to gamerules state
	if ( FStrEq( pcmd, "nextmap" ) )
	{
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			char szNextMap[32];

			if ( nextlevel.GetString() && *nextlevel.GetString() && engine->IsMapValid( nextlevel.GetString() ) )
			{
				V_strcpy_safe( szNextMap, nextlevel.GetString() );
			}
			else
			{
				GetNextLevelName( szNextMap, sizeof( szNextMap ) );
			}

			ClientPrint( pPlayer, HUD_PRINTTALK, "#TDC_nextmap", szNextMap);

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}

		return true;
	}
	else if ( FStrEq( pcmd, "timeleft" ) )
	{	
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			if ( mp_timelimit.GetInt() > 0 )
			{
				int iTimeLeft = GetTimeLeft();

				char szMinutes[5];
				char szSeconds[3];

				if ( iTimeLeft <= 0 )
				{
					V_sprintf_safe( szMinutes, "0" );
					V_sprintf_safe( szSeconds, "00" );
				}
				else
				{
					V_sprintf_safe( szMinutes, "%d", iTimeLeft / 60 );
					V_sprintf_safe( szSeconds, "%02d", iTimeLeft % 60 );
				}				

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TDC_timeleft", szMinutes, szSeconds );
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTTALK, "#TDC_timeleft_nolimit" );
			}

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}
		return true;
	}
	else if( pPlayer->ClientCommand( args ) )
	{
        return true;
	}

	return BaseClass::ClientCommand( pEdict, args );
}

// Add the ability to ignore the world trace
void CTDCGameRules::Think()
{
	if ( m_ctBotCountUpdate.IsElapsed() )
	{
		m_ctBotCountUpdate.Start( 5.0f );

		int nBots = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer != nullptr && pPlayer->IsFakeClient() )
				++nBots;
		}

		tdc_bot_count.SetValue( nBots );
	}

	if ( !g_fGameOver )
	{
		if ( gpGlobals->curtime > m_flNextPeriodicThink )
		{
			bool bTied;
			if ( CheckScoreLimit( true, bTied ) )
				return;
		}

		if ( IsInfectionMode() )
		{
			if ( Infected_RunLogic() )
				return;
		}
		else if ( IsVIPMode() )
		{
			if ( VIP_RunLogic() )
				return;
		}

		ManageServerSideVoteCreation();
	}
	else
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime && ( m_flIntermissionEndTime < gpGlobals->curtime ) )
		{
			if ( !IsX360() )
			{
				ChangeLevel(); // intermission is over
			}
			else
			{
				IGameEvent * event = gameeventmanager->CreateEvent( "player_stats_updated" );
				if ( event )
				{
					event->SetBool( "forceupload", true );
					gameeventmanager->FireEvent( event );
				}
				engine->MultiplayerEndGame();
			}

			// Don't run this code again
			m_flIntermissionEndTime = 0.f;
		}

		return;
	}

	State_Think();

	if ( m_hWaitingForPlayersTimer )
	{
		Assert( m_bInWaitingForPlayers );
	}

	if ( gpGlobals->curtime > m_flNextPeriodicThink )
	{
		// Don't end the game during win or stalemate states
		if ( IsRoundBasedMode() && State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_STALEMATE && State_Get() != GR_STATE_GAME_OVER )
		{
			if ( CheckWinLimit() )
				return;

			if ( CheckMaxRounds() )
				return;
		}

		CheckRestartRound();
		CheckWaitingForPlayers();

		m_flNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	// Watch dog for cheats ever being enabled during a level
	if ( !m_bCheatsEnabledDuringLevel && sv_cheats && sv_cheats->GetBool() )
	{
		m_bCheatsEnabledDuringLevel = true;
	}

	// Bypass teamplay think.
	CGameRules::Think();
}

void CTDCGameRules::GoToIntermission( void )
{
	if ( IsInTournamentMode() == true )
		return;

	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	if ( GetGameType() == TDC_GAMETYPE_FFA )
	{
		// Deathmatch results panel needs this.
		SendDeathmatchResults();
	}

	CTDC_GameStats.Event_GameEnd();

	float flWaitTime = mp_chattime.GetInt();

	if ( tv_delaymapchange.GetBool() )
	{
		if ( HLTVDirector()->IsActive() )
			flWaitTime = MAX( flWaitTime, HLTVDirector()->GetDelay() );
	}

	m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;

	// set all players to FL_FROZEN
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			pPlayer->AddFlag( FL_FROZEN );
			pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		}
	}

	// Print out map stats to a text file
	//WriteStatsFile( "stats.xml" );

	State_Enter( GR_STATE_GAME_OVER );
}

bool CTDCGameRules::CheckScoreLimit( bool bAllowEnd, bool &bTied )
{
	bTied = false;

	if ( !PointsMayBeCaptured() )
		return false;

	int iFragLimit = GetScoreLimit();

	if ( iFragLimit <= 0 )
		return false;

	switch ( GetGameType() )
	{
	case TDC_GAMETYPE_FFA:
	case TDC_GAMETYPE_BM:
	{
		CTeam *pTeam = GetGlobalTeam( FIRST_GAME_TEAM );
		CTDCPlayer *pWinner = NULL;
		int iDrawScoreCheck = INT_MIN;

		for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
		{
			CTDCPlayer *pPlayer = ToTDCPlayer( pTeam->GetPlayer( i ) );
			if ( !pPlayer )
				continue;

			PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( pPlayer );
			int iScore = CalcPlayerScore( &pStats->statsAccumulated );

			if ( iScore > iDrawScoreCheck )
			{
				iDrawScoreCheck = iScore;
				bTied = false;
			}
			else if ( iScore == iDrawScoreCheck )
			{
				bTied = true;
			}

			if ( iScore >= iFragLimit )
			{
				pWinner = pPlayer;
			}
		}

		if ( pWinner && !bTied )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Frag Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}

		break;
	}
	case TDC_GAMETYPE_TDM:
	case TDC_GAMETYPE_TBM:
	{
		CTeam *pWinner = NULL;
		int iDrawScoreCheck = -1;

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			CTDCTeam *pTeam = GetGlobalTFTeam( i );
			int iScore = pTeam->GetRoundScore();

			if ( iScore > iDrawScoreCheck )
			{
				iDrawScoreCheck = iScore;
				bTied = false;
			}
			else if ( iScore == iDrawScoreCheck )
			{
				bTied = true;
			}

			if ( iScore >= iFragLimit )
			{
				pWinner = pTeam;
			}
		}

		if ( pWinner && !bTied )
		{
			if ( bAllowEnd )
			{
				UTIL_LogPrintf( "Team \"%s\" triggered \"Frag_Limit\"\n", pWinner->GetName() );

				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Frag Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
		
		break;
	}
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_INVADE:
	{
		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			CTDCTeam *pTeam = GetGlobalTFTeam( i );
			int iScore = pTeam->GetRoundScore();

			if ( iScore >= iFragLimit )
			{
				if ( bAllowEnd )
				{
					SetWinningTeam( i, WINREASON_FLAG_CAPTURE_LIMIT );
				}
				return true;
			}
		}

		break;
	}
	case TDC_GAMETYPE_DUEL:
	{
		CTDCTeam *pTeam = GetGlobalTFTeam( FIRST_GAME_TEAM );

		if ( pTeam->GetNumPlayers() == 1 )
		{
			if ( bAllowEnd )
			{
				// The other player left! Declare the remaining one the winner.
				SetWinningPlayer( pTeam->GetTDCPlayer( 0 ), WINREASON_DUEL_OPPONENTLEFT );
				return true;
			}
		}
		else if ( pTeam->GetNumPlayers() == 0 )
		{
			if ( bAllowEnd )
			{
				// Wat. Both players must have left at the same time.
				SetWinningPlayer( NULL, WINREASON_DUEL_BOTHPLAYERSLEFT );
			}
			return true;
		}
		else
		{
			int iDrawScoreCheck = INT_MIN;
			CTDCPlayer *pWinner = NULL;

			for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
			{
				CTDCPlayer *pPlayer = ToTDCPlayer( pTeam->GetPlayer( i ) );
				if ( !pPlayer )
					continue;

				PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( pPlayer );
				int iScore = CalcPlayerScore( &pStats->statsCurrentRound );

				if ( iScore > iDrawScoreCheck )
				{
					iDrawScoreCheck = iScore;
					bTied = false;
				}
				else if ( iScore == iDrawScoreCheck )
				{
					bTied = true;
				}

				if ( iScore >= iFragLimit )
				{
					pWinner = pPlayer;
				}
			}

			if ( pWinner && !bTied )
			{
				if ( bAllowEnd )
				{
					SetWinningPlayer( pWinner, WINREASON_DUEL_FRAGLIMIT );
				}
				return true;
			}
		}

		break;
	}
	}

	return false;
}

int CTDCGameRules::GetFragsRemaining( void )
{
	if ( !PointsMayBeCaptured() )
		return -1;

	int iFragLimit = GetScoreLimit();
	if ( iFragLimit <= 0 )
		return -1;

	switch ( GetGameType() )
	{
	case TDC_GAMETYPE_FFA:
	case TDC_GAMETYPE_DUEL:
	case TDC_GAMETYPE_BM:
	{
		CTeam *pTeam = GetGlobalTeam( FIRST_GAME_TEAM );
		int iMaxScore = 0;

		for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
		{
			CTDCPlayer *pPlayer = ToTDCPlayer( pTeam->GetPlayer( i ) );
			if ( !pPlayer )
				continue;

			PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( pPlayer );
			int iScore = CalcPlayerScore( &pStats->statsCurrentRound );

			if ( iScore > iMaxScore )
			{
				iMaxScore = iScore;
			}
		}

		return ( iFragLimit - iMaxScore );
	}
	case TDC_GAMETYPE_TDM:
	case TDC_GAMETYPE_TBM:
	{
		int iMaxScore = 0;

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			CTeam *pTeam = GetGlobalTeam( i );
			int iScore = pTeam->GetScore();

			if ( iScore > iMaxScore )
			{
				iMaxScore = iScore;
			}
		}

		return ( iFragLimit - iMaxScore );
	}
	}

	return -1;
}

bool CTDCGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	// guard against NULL pointers if players disconnect
	if ( !pPlayer || !pAttacker )
		return false;

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

int CTDCGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pAttacker )
{
	if ( !pPlayer || !pAttacker || !pAttacker->IsPlayer() )
		return GR_NOTTEAMMATE;

	// No teams in FFA.
	if ( !IsTeamplay() )
		return GR_NOTTEAMMATE;

	// Either you are on the other player's team or you're not.
	if ( pPlayer->InSameTeam( pAttacker ) )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}


Vector DropToGround( 
	CBaseEntity *pMainEnt, 
	const Vector &vPos, 
	const Vector &vMins, 
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //

CBaseEntity *CTDCGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	// drop down to ground
	Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

	// Move the player to the place it said.
	pPlayer->SetLocalOrigin( GroundPos + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTDCGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
	if ( IsTeamplay() )
	{
		// Check the team.
		if ( pSpot->GetTeamNumber() != TEAM_UNASSIGNED && pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
			return false;
	}

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTDCPlayerSpawn *pCTDCSpawn = dynamic_cast<CTDCPlayerSpawn*>( pSpot );
	if ( pCTDCSpawn )
	{
		if ( pCTDCSpawn->IsDisabled() )
			return false;

		if ( !pCTDCSpawn->IsEnabledForMode( m_nGameType ) )
			return false;
	}

	Vector mins = GetViewVectors()->m_vHullMin;
	Vector maxs = GetViewVectors()->m_vHullMax;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return !trace.DidHit();
}

Vector CTDCGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTDCGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

int CTDCGameRules::ItemShouldRespawn( CItem *pItem )
{
	// Weapons should not respawn in Infection.
	if ( IsInfectionMode() && pItem->ClassMatches( "item_weaponspawner" ) )
		return GR_ITEM_RESPAWN_NO;

	return BaseClass::ItemShouldRespawn( pItem );
}

float CTDCGameRules::FlItemRespawnTime( CItem *pItem )
{
	return 10.0f;
}

bool CTDCGameRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	// Zombies cannot pickup any items.
	if ( pTFPlayer->IsZombie() )
		return false;

	return true;
}

void CTDCGameRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
	CTDCPickupItem *pTFItem = dynamic_cast<CTDCPickupItem *>( pItem );
	if ( pTFItem )
	{
		pTFItem->FireOutputsOnPickup( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	CTDCPlayer *pTFPlayer = ToTDCPlayer(pPlayer);

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == true )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TDC_Chat_Spec";
		}
		else
		{
			if ( pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TDC_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TDC_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TDC_Chat_Team";
				}
			}
		}
	}
	else if ( pTFPlayer->IsPlayerDev() && pTFPlayer->GetClientConVarBoolValue( "tdc_dev_mark" ) )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TDC_Chat_DevSpec";
		}
		else
		{
			if (pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN)
			{
				pszFormat = "TDC_Chat_DevDead";
			}
			else
			{
				pszFormat = "TDC_Chat_Dev";
			}
		}
	}
	else
	{	
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TDC_Chat_AllSpec";	
		}
		else
		{
			if ( pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TDC_Chat_AllDead";
			}
			else
			{
				pszFormat = "TDC_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTDCGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		if ( iActivity != ACT_INVALID )
		{
			CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

			if ( pTFPlayer )
			{
				pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );
			}
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTDCGameRules::ChangePlayerName( CTDCPlayer *pPlayer, const char *pszNewName )
{
	const char *pszOldName = pPlayer->GetPlayerName();

	IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		event->SetString( "oldname", pszOldName );
		event->SetString( "newname", pszNewName );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SetPlayerName( pszNewName );

	pPlayer->m_flNextNameChangeTime = gpGlobals->curtime + 10.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::OnNavMeshLoad()
{
	TheNavMesh->SetPlayerSpawnName( "info_player_teamspawn" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );
	int iPlayerIndex = pPlayer->entindex();

	const char *pszName = engine->GetClientConVarValue( iPlayerIndex, "name" );
	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && V_strcmp( pszOldName, pszName ) != 0 )		
	{
		if ( pTFPlayer->m_flNextNameChangeTime < gpGlobals->curtime )
		{
			ChangePlayerName( pTFPlayer, pszName );
		}
		else
		{
			// no change allowed, force engine to use old name again
			engine->ClientCommand( pPlayer->edict(), "name \"%s\"", pszOldName );

			// tell client that he hit the name change time limit
			ClientPrint( pTFPlayer, HUD_PRINTTALK, "#Name_change_limit_exceeded" );
		}
	}

	// keep track of their cl_autorezoom value
	pTFPlayer->SetAutoRezoom( pTFPlayer->GetClientConVarBoolValue( "cl_autorezoom" ) );

	pTFPlayer->SetFlipViewModel( pTFPlayer->GetClientConVarBoolValue( "cl_flipviewmodels" ) );

	pTFPlayer->SetViewModelFOV( clamp( pTFPlayer->GetClientConVarFloatValue( "viewmodel_fov" ), 0.1f, 179.0f ) );

	Vector vecVMOffset;
	vecVMOffset.x = clamp( pTFPlayer->GetClientConVarFloatValue( "viewmodel_offset_x" ), TDC_VM_MIN_OFFSET, TDC_VM_MAX_OFFSET );
	vecVMOffset.y = clamp( pTFPlayer->GetClientConVarFloatValue( "viewmodel_offset_y" ), TDC_VM_MIN_OFFSET, TDC_VM_MAX_OFFSET );
	vecVMOffset.z = clamp( pTFPlayer->GetClientConVarFloatValue( "viewmodel_offset_z" ), TDC_VM_MIN_OFFSET, TDC_VM_MAX_OFFSET );
	pTFPlayer->SetViewModelOffset( vecVMOffset );

	pTFPlayer->SetHoldToZoom( pTFPlayer->GetClientConVarBoolValue( "tdc_zoom_hold" ) );

	// Force player's FOV to 90 in background maps as they were designed with that FOV in mind.
	if ( gpGlobals->eLoadType != MapLoad_Background )
	{
		int iDesiredFov = pTFPlayer->GetClientConVarIntValue( "fov_desired" );
		int iFov = clamp( iDesiredFov, 90, MAX_FOV );
		pTFPlayer->SetDefaultFOV( iFov );
	}
	else
	{
		pTFPlayer->SetDefaultFOV( 90 );
	}
}

static const char *g_aTaggedConVars[] =
{
	"mp_fadetoblack",
	"fadetoblack",

	"mp_friendlyfire",
	"friendlyfire",

	"tdc_allow_thirdperson",
	"thirdperson",

	"mp_disable_respawn_times",
	"norespawntime",

	"mp_stalemate_enable",
	"suddendeath",

	"tdc_bot_count",
	"bots",
};

//-----------------------------------------------------------------------------
// Purpose: Tags
//-----------------------------------------------------------------------------
void CTDCGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	COMPILE_TIME_ASSERT( ARRAYSIZE( g_aTaggedConVars ) % 2 == 0 );

	BaseClass::GetTaggedConVarList( pCvarTagList );

	for ( int i = 0; i < ARRAYSIZE( g_aTaggedConVars ); i += 2 )
	{
		KeyValues *pKeyValue = new KeyValues( g_aTaggedConVars[i] );
		pKeyValue->SetString( "convar", g_aTaggedConVars[i] );
		pKeyValue->SetString( "tag", g_aTaggedConVars[i+1] );

		pCvarTagList->AddSubKey( pKeyValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );

	CTDCPlayer *pPlayer = ToTDCPlayer( CBaseEntity::Instance( pEntity ) );
	if ( !pPlayer )
		return;

	if ( FStrEq( pKeyValues->GetName(), "FreezeCamTaunt" ) )
	{
		int iCmdPlayerID = pPlayer->GetUserID();
		int iAchieverIndex = pKeyValues->GetInt( "achiever" );

		CTDCPlayer *pAchiever = ToTDCPlayer( UTIL_PlayerByIndex( iAchieverIndex ) );
		if ( pAchiever && ( pAchiever->GetUserID() != iCmdPlayerID ) )
		{
			int iClass = pAchiever->GetPlayerClass()->GetClassIndex();

			if ( g_TauntCamAchievements[iClass] != 0 )
			{
				pAchiever->AwardAchievement( g_TauntCamAchievements[iClass] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTDCGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTDCPlayer *pTFPlayer = (CTDCPlayer*)pPlayer;

		if ( pTFPlayer )
		{
			// Get the max carrying capacity for this ammo
			int iMaxCarry = pTFPlayer->GetMaxAmmo( iAmmoIndex, true );

			// Does the player have room for more of this type of ammo?
			if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CTDCPlayer *pTDCPlayerVictim = ToTDCPlayer( pVictim );
	CTDCPlayer *pScorer = ToTDCPlayer( info.GetAttacker() );
	CTDCPlayer *pAssister = GetAssister( pTDCPlayerVictim, pScorer, info );

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;

	if ( pScorer )
	{	
		CalcDominationAndRevenge( pScorer, pTDCPlayerVictim, false, &iDeathFlags );

		if ( pAssister )
		{
			CalcDominationAndRevenge( pAssister, pTDCPlayerVictim, true, &iDeathFlags );
		}

		// In TDM, add to player's team score (unless it's a teamkill).
		if ( m_nGameType == TDC_GAMETYPE_TDM && pScorer->IsEnemy( pTDCPlayerVictim ) )
		{
			TFTeamMgr()->AddRoundScore( pScorer->GetTeamNumber() );
		}
	}

	pTDCPlayerVictim->SetDeathFlags( iDeathFlags );

	if ( pAssister )
	{
		CTDC_GameStats.Event_AssistKill( pAssister, pVictim );
	}

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTDCGameRules::CalcDominationAndRevenge( CTDCPlayer *pAttacker, CTDCPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	if ( TDCGameRules()->IsInfectionMode() || !tdc_nemesis_relationships.GetBool() )
		return;

	PlayerStats_t *pStatsVictim = CTDC_GameStats.FindPlayerStats( pVictim );

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[pAttacker->entindex()] + 1;		
	if ( iKillsUnanswered == TDC_KILLS_DOMINATION )
	{			
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TDC_DEATH_ASSISTER_DOMINATION : TDC_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );
		pAttacker->UpdateDominationsCount();
		// record stats
		CTDC_GameStats.Event_PlayerDominatedOther( pAttacker );
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TDC_DEATH_ASSISTER_REVENGE : TDC_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );
		pVictim->UpdateDominationsCount();
		// record stats
		CTDC_GameStats.Event_PlayerRevenge( pAttacker );
	}
}

//-----------------------------------------------------------------------------
// Purpose: trace line rules
//-----------------------------------------------------------------------------
float CTDCGameRules::WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd,
	unsigned int mask, trace_t *ptr )
{
	if ( mask & CONTENTS_HITBOX )
	{
		// Special case for arrows.
		UTIL_TraceLine( vecStart, vecEnd, mask, pEntity, pEntity->GetCollisionGroup(), ptr );
		return 1.0f;
	}

	return BaseClass::WeaponTraceEntity( pEntity, vecStart, vecEnd, mask, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTDCGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tdc_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTDCObjectiveResource *)CBaseEntity::Create( "tdc_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the entity that will send our data to the client.
	CBaseEntity *pEnt = CBaseEntity::Create( "tdc_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
	pEnt->SetName( AllocPooledString( "tdc_gamerules" ) );

	CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle );

	// Add vote issues. They automatically add themselves to vote controller which deletes them on level shutdown.
	new CKickIssue( "Kick" );
	new CRestartGameIssue( "RestartGame" );
	new CChangeLevelIssue( "ChangeLevel" );
	new CNextLevelIssue( "NextLevel" );
	new CExtendLevelIssue( "ExtendLevel" );
	new CScrambleTeams( "ScrambleTeams" );
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
void CTDCGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTDCPlayer *pVictim, KillingWeaponData_t &weaponData )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CTDCPlayer *pScorer = ToTDCPlayer( info.GetAttacker() );
	CTDCWeaponBase *pWeapon = dynamic_cast<CTDCWeaponBase *>( info.GetWeapon() );

	const char *killer_weapon_name = "world";
	const char *killer_weapon_log_name = "";
	ETDCWeaponID iWeaponID = WEAPON_NONE;

	if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_BURNING )
	{
		if ( pWeapon )
		{
			// Player stores last weapon that burned him so if he burns to death we know what killed him.
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();

			if ( pInflictor && pInflictor != pScorer )
			{
				CTDCBaseRocket *pRocket = dynamic_cast<CTDCBaseRocket *>( pInflictor );

				if ( pRocket && pRocket->m_iDeflected )
				{
					// Fire weapon deflects go here.
					switch ( pRocket->GetWeaponID() )
					{
					case WEAPON_FLAREGUN:
						killer_weapon_name = "deflect_flare";
						break;
					}
				}
			}
		}
		else if ( pVictim )
		{
			switch ( pVictim->m_Shared.GetDamageSourceType() )
			{
			case TDC_DMG_SOURCE_PLAYER_SPREAD:
				killer_weapon_name = "fire_spread";
				iWeaponID = WEAPON_NONE;
				break;
			default:
				killer_weapon_name = "weapon_flamethrower";
				iWeaponID = WEAPON_FLAMETHROWER;
				break;
			}
		} else {
			// Default to flamethrower if no burn weapon is specified.
			killer_weapon_name = "weapon_flamethrower";
			iWeaponID = WEAPON_FLAMETHROWER;
		}

		if ( pScorer == pVictim || !pScorer )
		{
			killer_weapon_name = "fire_suicide";
		}
	}
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If the inflictor is the killer, then it must be their current weapon doing the damage
		if ( pWeapon )
		{
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = pInflictor->GetClassname();

		// Ideally, we need a derived method here but 95% of the time this is going to be CBaseProjectile
		// and this is still better than doing 3 dynamic casts.
		CTDCBaseProjectile *pProjectile = dynamic_cast<CTDCBaseProjectile *>( pInflictor );

		if ( pProjectile )
		{
			switch ( pProjectile->GetBaseProjectileType() )
			{
			case TDC_PROJECTILE_BASE_NAIL:
			{
				iWeaponID = pProjectile->GetWeaponID();

				CTDCWeaponBase *pLauncher = assert_cast<CTDCWeaponBase *>( pProjectile->GetOriginalLauncher() );

				if ( pLauncher )
				{
					killer_weapon_name = pLauncher->GetClassname();
				}

				break;
			}
			case TDC_PROJECTILE_BASE_ROCKET:
			{
				CTDCBaseRocket *pRocket = static_cast<CTDCBaseRocket *>( pInflictor );

				iWeaponID = pRocket->GetWeaponID();

				if ( pRocket->m_iDeflected )
				{
					switch ( pRocket->GetWeaponID() )
					{
					case WEAPON_ROCKETLAUNCHER:
						killer_weapon_name = "deflect_rocket";
						break;
					}
				}

				break;
			}
			case TDC_PROJECTILE_BASE_GRENADE:
			{
				CTDCBaseGrenade *pGrenade = static_cast<CTDCBaseGrenade *>( pInflictor );

				iWeaponID = pGrenade->GetWeaponID();

				if ( pGrenade->m_iDeflected )
				{
					switch ( pGrenade->GetWeaponID() )
					{
					case WEAPON_GRENADELAUNCHER:
						killer_weapon_name = "deflect_promode";
						break;
					}
				}

				break;
			}
			default:
				Assert( false );
				break;
			}
		}
	}

	// Handle custom kill types after we've figured out weapon ID.
	const char *pszCustomKill = NULL;

	switch ( info.GetDamageCustom() )
	{
	case TDC_DMG_CUSTOM_SUICIDE:
		pszCustomKill = "world";
		break;
	case TDC_DMG_TELEFRAG:
		pszCustomKill = "telefrag";
		break;
	case TDC_DMG_DISPLACER_TELEFRAG:
		pszCustomKill = "displacer_telefrag";
		break;
	case TDC_DMG_REMOTEBOMB_IMPACT:
		pszCustomKill = "projectile_remotebomb_impact";
		break;
	case TDC_DMG_STOMP:
		pszCustomKill = "stomp";
	}

	if ( pszCustomKill != NULL )
	{
		V_strcpy_safe( weaponData.szWeaponName, pszCustomKill );
		weaponData.iWeaponID = iWeaponID;
		return;
	}

	// strip certain prefixes from inflictor's classname
	const char *prefix[] = { "weapon_", "npc_", "func_" };
	for ( int i = 0; i < ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = V_strlen( prefix[i] );
		if ( V_strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			break;
		}
	}

	V_strcpy_safe( weaponData.szWeaponName, killer_weapon_name );
	V_strcpy_safe( weaponData.szWeaponLogName, killer_weapon_log_name );
	weaponData.iWeaponID = iWeaponID;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CTDCPlayer *CTDCGameRules::GetAssister( CTDCPlayer *pVictim, CTDCPlayer *pScorer, const CTakeDamageInfo &info )
{
	// No assists in FFA.
	if ( !TDCGameRules()->IsTeamplay() )
		return NULL;

	if ( pScorer && pVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pScorer == pVictim )
			return NULL;

		if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_SUICIDE )
			return NULL;

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTDCPlayer *pRecentDamager = GetRecentDamager( pVictim, 1, 3.0f );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specifed recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTDCPlayer *CTDCGameRules::GetRecentDamager( CTDCPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	Assert( iDamager < MAX_DAMAGER_HISTORY );

	DamagerHistory_t &damagerHistory = pVictim->GetDamagerHistory( iDamager );
	if ( ( NULL != damagerHistory.hDamager ) && ( gpGlobals->curtime - damagerHistory.flTimeDamage <= flMaxElapsed ) )
	{
		CTDCPlayer *pRecentDamager = ToTDCPlayer( damagerHistory.hDamager );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTDCGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTDCGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	int killer_ID = 0;

	// Find the killer & the scorer
	CTDCPlayer *pTFPlayerVictim = ToTDCPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTDCPlayer *pScorer = ToTDCPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTDCPlayer *pAssister = GetAssister( pTFPlayerVictim, pScorer, info );

	// Work out what killed the player, and send a message to all clients about it
	KillingWeaponData_t weaponData;
	GetKillingWeaponName( info, pTFPlayerVictim, weaponData );

	if ( pScorer )	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );

	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", weaponData.szWeaponName );
		event->SetString( "weapon_logclassname", weaponData.szWeaponLogName );
		event->SetInt( "weaponid", weaponData.iWeaponID );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted
		event->SetInt( "death_flags", pTFPlayerVictim->GetDeathFlags() );
		//event->SetInt( "playerpenetratecount", info.GetPlayerPenetrationCount() );
		event->SetInt( "powerup_flags", pScorer ? pScorer->m_Shared.GetPowerupFlags() : 0 );

		gameeventmanager->FireEvent( event );
	}
}

bool CTDCGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	const CSteamID *pPlayerID = engine->GetClientSteamID( pEntity );

	KeyValues *pKV = m_pAuthData->FindKey( "bans" );
	if ( pKV )
	{
		for ( KeyValues *pSub = pKV->GetFirstTrueSubKey(); pSub; pSub = pSub->GetNextTrueSubKey() )
		{
			KeyValues *pIDSub = pSub->FindKey( "id" );
			if ( pIDSub && pPlayerID && pIDSub->GetUint64() == pPlayerID->ConvertToUint64() )
			{
				// SteamID is banned
				KeyValues *pMsgSub = pSub->FindKey( "message" );
				if ( pMsgSub )
				{
					V_strncpy( reject, pMsgSub->GetString(), maxrejectlen );
				}
				return false;
			}

			KeyValues *pIPSub = pSub->FindKey( "ip" );
			if ( pIPSub && pszAddress && !V_strcmp( pIPSub->GetString(), pszAddress ) )
			{
				// IP is banned
				KeyValues *pMsgSub = pSub->FindKey( "message" );
				if ( pMsgSub )
				{
					V_strncpy( reject, pMsgSub->GetString(), maxrejectlen );
				}
				return false;
			}
		}
	}

	return BaseClass::ClientConnected( pEntity, pszName, pszAddress, reject, maxrejectlen );
}

void CTDCGameRules::ClientDisconnected( edict_t *pClient )
{
	// clean up anything they left behind
	CTDCPlayer *pPlayer = ToTDCPlayer( GetContainingEntity( pClient ) );
	if ( pPlayer )
	{
		pPlayer->TeamFortress_ClientDisconnected();
		if ( IsVIPMode() )
		{
			ResetVIP();
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

// Falling damage stuff.
#define TDC_PLAYER_MAX_SAFE_FALL_SPEED	650	

float CTDCGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_Local.m_flFallVelocity > TDC_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		float flFallDamage = 5.0f * ( pPlayer->m_Local.m_flFallVelocity / 300.0f );

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		pPlayer->EmitSound( "Player.FallDamage" );
		CTakeDamageInfo info( GetContainingEntity( INDEXENT( 0 ) ), GetContainingEntity( INDEXENT( 0 ) ), flFallDamage, DMG_FALL );
		pPlayer->TakeDamage( info );

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TDC_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TDC_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TDC_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TDC_FIRST_NORMAL_CLASS; iClass < TDC_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_sprintf_safe( szName, "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType )
{
	UserMessageBegin( filter, "HudNotify" );
	WRITE_BYTE( iType );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotifyCustom" );
	WRITE_STRING( pszText );
	WRITE_STRING( pszIcon );
	WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the game description in the server browser
//-----------------------------------------------------------------------------
const char *CTDCGameRules::GetGameDescription( void )
{
	return "Team Deathmatch Classic";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::HandleCTDCCaptureBonus( int iTeam )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ManageServerSideVoteCreation( void )
{
	if ( State_Get() == GR_STATE_PREGAME )
		return;

	if ( IsInTournamentMode() )
		return;

	if ( IsInTraining() || IsInItemTestingMode() )
		return;

	if ( gpGlobals->curtime < m_flNextVoteThink )
		return;

	if ( sv_vote_issue_nextlevel_allowed.GetBool() &&
		sv_vote_issue_nextlevel_choicesmode.GetBool() &&
		!nextlevel.GetString()[0] &&
		!m_bVotedForNextMap && !m_bVoteMapOnNextRound )
	{
		
		if ( IsRoundBasedMode() )
		{
			int iWinsLeft = GetWinsRemaining();
			if ( iWinsLeft == 1 )
			{
				StartNextMapVote();
			}
			else if ( iWinsLeft == 0 )
			{
				// We're about to change map and we didn't choose the next map yet.
				// Bring up the vote now!
				StartNextMapVote();
			}

			int iRoundsLeft = GetRoundsRemaining();
			if ( iRoundsLeft == 1 )
			{
				// The next round will be the last one, schedule the vote.
				ScheduleNextMapVote();
			}
			else if ( iRoundsLeft == 0 )
			{
				// We're about to change map and we didn't choose the next map yet.
				// Bring up the vote now!
				StartNextMapVote();
			}
		}
		else
		{
			// In non-round based modes, check the frag limit.
			int iFragsLeft = GetFragsRemaining();
			if ( iFragsLeft != -1 && iFragsLeft <= 5 )
			{
				StartNextMapVote();
			}
		}
		
		if ( IsGameUnderTimeLimit() && GetTimeLeft() <= 180 )
		{
			// 3 minutes left, call the map vote now.
			StartNextMapVote();
		}
	}

	m_flNextVoteThink = gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::StartNextMapVote( void )
{
	if ( m_bVotedForNextMap )
		return;

	if ( g_voteController )
	{
		g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", "" );
	}

	m_bVotedForNextMap = true;
	m_bVoteMapOnNextRound = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ScheduleNextMapVote( void )
{
	if ( !m_bVotedForNextMap )
	{
		m_bVoteMapOnNextRound = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::ExtendCurrentMap( void )
{
	if ( mp_timelimit.GetInt() > 0 )
	{
		m_iMapTimeBonus += 15 * 60;
		HandleTimeLimitChange();
	}

	if ( mp_maxrounds.GetInt() > 0 )
	{
		m_iMaxRoundsBonus += 5;
	}

	if ( mp_winlimit.GetInt() > 0 )
	{
		m_iWinLimitBonus += 3;
	}

	if ( !IsRoundBasedMode() && GetScoreLimit() > 0 )
	{
		m_iFragLimitBonus += 10;
	}

	// Bring up the vote again when the time is right.
	m_bVotedForNextMap = false;
	m_bVoteMapOnNextRound = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SendDeathmatchResults( void )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "deathmatch_results" );
	if ( !event )
		return;

	// Determine top 3 players of this game.

	// build a vector of players & round scores.
	CUtlVector<PlayerRoundScore_t> vecPlayerScore;
	for ( int iPlayerIndex = 1; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
	{
		CTDCPlayer *pTFPlayer = ToTDCPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( !pTFPlayer || !pTFPlayer->IsConnected() )
			continue;

		// Filter out spectators.
		if ( pTFPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			continue;

		int iRoundScore = 0, iTotalScore = 0, iKills = 0, iDeaths = 0;
		PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( pTFPlayer );

		if ( pStats )
		{
			iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
			iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
			iKills = pStats->statsCurrentRound.m_iStat[TFSTAT_KILLS];
			iDeaths = pStats->statsCurrentRound.m_iStat[TFSTAT_DEATHS];
		}

		PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
		playerRoundScore.iPlayerIndex = iPlayerIndex;
		playerRoundScore.iRoundScore = iRoundScore;
		playerRoundScore.iTotalScore = iTotalScore;
		playerRoundScore.iKills = iKills;
		playerRoundScore.iDeaths = iDeaths;
	}

	// Sort players by score.
	vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

	// Set top 3 players in event data.
	int numPlayers = Min( 3, vecPlayerScore.Count() );
	for ( int i = 0; i < numPlayers; i++ )
	{
		// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
		if ( 0 == vecPlayerScore[i].iRoundScore )
			break;

		char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "";
		char szPlayerKillsVal[64] = "", szPlayerDeathsVal[64] = "";
		V_sprintf_safe( szPlayerIndexVal, "player_%d", i + 1 );
		V_sprintf_safe( szPlayerScoreVal, "player_%d_points", i + 1 );
		V_sprintf_safe( szPlayerKillsVal, "player_%d_kills", i + 1 );
		V_sprintf_safe( szPlayerDeathsVal, "player_%d_deaths", i + 1 );
		event->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
		event->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );
		event->SetInt( szPlayerKillsVal, vecPlayerScore[i].iKills );
		event->SetInt( szPlayerDeathsVal, vecPlayerScore[i].iDeaths );
	}

	gameeventmanager->FireEvent( event );
}

// sort function for the list of players that we're going to use to scramble the teams
static int DuelQueueSort( CTDCPlayer* const *p1, CTDCPlayer* const *p2 )
{
	CTDCPlayer *pPlayer1 = ( *p1 );
	CTDCPlayer *pPlayer2 = ( *p2 );

	// Winnner gets to stay so put him at the head of the queue.
	if ( pPlayer1 == TDCGameRules()->GetWinnngPlayer() )
		return -1;

	if ( pPlayer2 == TDCGameRules()->GetWinnngPlayer() )
		return 1;

	int iTeam1 = pPlayer1->GetTeamNumber();
	int iTeam2 = pPlayer2->GetTeamNumber();

	// Move the loser to the end of the queue, that way he will not get selected unless there are no other candidates.
	if ( iTeam1 == FIRST_GAME_TEAM && iTeam2 != FIRST_GAME_TEAM )
		return 1;

	if ( iTeam1 != FIRST_GAME_TEAM && iTeam2 == FIRST_GAME_TEAM )
		return -1;

	float flSpecTime1 = gpGlobals->curtime - pPlayer1->GetSpectatorSwitchTime();
	float flSpecTime2 = gpGlobals->curtime - pPlayer2->GetSpectatorSwitchTime();

	// Sort players by how much time they've been waiting to play.
	if ( flSpecTime1 < flSpecTime2 )
		return 1;

	if ( flSpecTime1 > flSpecTime2 )
		return -1;

	return 0;
}

bool CTDCGameRules::AssignNextDuelingPlayers( void )
{
	CUtlVector<CTDCPlayer *> aCandidates;
	m_NextDuelingPlayers.RemoveAll();

	// Collect players who are waiting to play.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsReadyToPlay() )
		{
			aCandidates.AddToTail( pPlayer );
		}
	}

	if ( aCandidates.Count() < 2 )
		return false;

	// Shuffle the list before sorting it so players with equal waiting time will be in random order.
	aCandidates.Shuffle();

	//TODO: respawn wave fixups.
	if ( !m_bPrevRoundWasWaitingForPlayers )
	{
		aCandidates.Sort( DuelQueueSort );
	}

	m_NextDuelingPlayers.AddToTail( aCandidates[0] );
	m_NextDuelingPlayers.AddToTail( aCandidates[1] );

	return true;
}

void CTDCGameRules::StartDuelRound( void )
{
	// Have we assigned dueling players yet?
	if ( !AreNextDuelingPlayersSet() )
	{
		if ( !AssignNextDuelingPlayers() )
		{
			State_Transition( GR_STATE_PREGAME );
			return;
		}
	}

	// Move everyone back to spectators except for the assigned players.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		if ( m_NextDuelingPlayers.Find( pPlayer ) != m_NextDuelingPlayers.InvalidIndex() )
		{
			pPlayer->ForceChangeTeam( FIRST_GAME_TEAM );
		}
		else if ( pPlayer->GetTeamNumber() == FIRST_GAME_TEAM )
		{
			pPlayer->ForceChangeTeam( TEAM_SPECTATOR );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "duel_start" );
	if ( event )
	{
		event->SetInt( "player_1", m_NextDuelingPlayers[0]->entindex() );
		event->SetInt( "player_2", m_NextDuelingPlayers[1]->entindex() );

		gameeventmanager->FireEvent( event );
	}
}

bool CTDCGameRules::AreNextDuelingPlayersSet( void )
{
	if ( m_NextDuelingPlayers.Count() < 2 )
		return false;

	for ( CTDCPlayer *pPlayer : m_NextDuelingPlayers )
	{
		if ( !pPlayer || !pPlayer->IsReadyToPlay() )
			return false;
	}

	return true;
}

CTDCPlayer *CTDCGameRules::GetDuelOpponent( CTDCPlayer *pPlayer )
{
	CTDCTeam *pTeam = GetGlobalTFTeam( FIRST_GAME_TEAM );

	for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
	{
		CTDCPlayer *pOther = pTeam->GetTDCPlayer( i );
		if ( pOther && pOther != pPlayer )
			return pOther;
	}

	return NULL;
}

void CTDCGameRules::SetWinningPlayer( CTDCPlayer *pWinner, int iWinReason /*= WINREASON_NONE*/ )
{
	// Commentary doesn't let anyone win
	if ( IsInCommentaryMode() )
		return;

	// are we already in this state?
	if ( State_Get() == GR_STATE_TEAM_WIN )
		return;

	m_hWinningPlayer = pWinner;
	m_iWinningTeam = FIRST_GAME_TEAM;
	m_iWinReason = iWinReason;

	if ( pWinner )
	{
		// TEMP: Re-using normal win/lose sounds, will need to get custom ones later.
		CTeam *pTeam = GetGlobalTeam( FIRST_GAME_TEAM );

		for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
		{
			CBasePlayer *pPlayer = pTeam->GetPlayer( i );
			if ( !pPlayer )
				continue;

			g_TFAnnouncer.Speak( pPlayer, ( pPlayer == pWinner ) ? TDC_ANNOUNCER_VICTORY : TDC_ANNOUNCER_DEFEAT );
		}
	}
	else
	{
		g_TFAnnouncer.Speak( TDC_ANNOUNCER_STALEMATE );
	}

	if ( IsInDuelMode() )
	{
		// Don't assign the next match if the time is up.
		if ( CheckTimeLimit( false ) || CheckWinLimit( false ) || CheckMaxRounds( false ) || CheckNextLevelCvar( false ) )
		{
			m_NextDuelingPlayers.RemoveAll();
		}
		else
		{
			AssignNextDuelingPlayers();
		}
	}

	State_Transition( GR_STATE_TEAM_WIN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::StartInfectedRound( void )
{
	CUtlVector<CTDCPlayer *> activePlayers;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsReadyToPlay() )
			continue;

		activePlayers.AddToTail( pPlayer );
	}

	if ( activePlayers.Count() < 2 )
	{
		State_Transition( GR_STATE_PREGAME );
		return;
	}

	activePlayers.Shuffle();

	// Split players up - 75% humans, 25% infected.
	int iNumInfected = Max( activePlayers.Count() / 4, 1 );

	FOR_EACH_VEC( activePlayers, i )
	{
		activePlayers[i]->ForceChangeTeam( i < iNumInfected ? TDC_TEAM_ZOMBIES : TDC_TEAM_HUMANS, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::Infected_RunLogic( void )
{
	if ( !PointsMayBeCaptured() )
		return false;

	CUtlVector<CTDCPlayer *> aliveHumans;
	CUtlVector<CTDCPlayer *> aliveZombies;
	CUtlVector<CTDCPlayer *> deadZombies;
	CTDCTeam *pTeamHumans = GetGlobalTFTeam( TDC_TEAM_HUMANS );
	CTDCTeam *pTeamZombies = GetGlobalTFTeam( TDC_TEAM_ZOMBIES );

	// Check if there any surviving humans.
	for ( int i = pTeamHumans->GetNumPlayers() - 1; i >= 0; i-- )
	{
		CTDCPlayer *pPlayer = pTeamHumans->GetTDCPlayer( i );
		if ( !pPlayer )
			continue;

		if ( pPlayer->IsAlive() )
		{
			aliveHumans.AddToTail( pPlayer );
		}
		else if ( gpGlobals->curtime - pPlayer->GetDeathTime() > TDC_DEATH_ANIMATION_TIME )
		{
			// Move dead humans to zombies team.
			pPlayer->ChangeTeam( TDC_TEAM_ZOMBIES, false, true );
		}
	}

	// Check if there any zombies.
	for ( int i = pTeamZombies->GetNumPlayers() - 1; i >= 0; i-- )
	{
		CTDCPlayer *pPlayer = pTeamZombies->GetTDCPlayer( i );
		if ( !pPlayer )
			continue;

		if ( pPlayer->IsAlive() )
		{
			aliveZombies.AddToTail( pPlayer );
		}
		else
		{
			deadZombies.AddToTail( pPlayer );
		}
	}

	// Last alive human becomes Last Standing.
	if ( aliveHumans.Count() == 1 && m_hLastStandingPlayer.Get() == NULL )
	{
		m_hLastStandingPlayer = aliveHumans[0];
		m_hLastStandingPlayer->m_Shared.AddCond( TDC_COND_LASTSTANDING );
		
		// Play "Last Standing" announcement.
		CSingleUserRecipientFilter filter( m_hLastStandingPlayer );
		g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_ARENA_LASTSTANDING );

		// Respawn all zombies now.
		for ( CTDCPlayer *pPlayer : deadZombies )
		{
			pPlayer->ForceRespawn();
			aliveZombies.AddToTail( pPlayer );
		}

		deadZombies.RemoveAll();
	}

	if ( aliveHumans.Count() != 0 )
	{
		if ( pTeamZombies->GetNumPlayers() <= 0 )
		{
			SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE );
			return true;
		}
		else if ( aliveZombies.Count() == 0 && aliveHumans.Count() == 1 )
		{
			// RED has killed all the zombies!
			SetWinningTeam( TDC_TEAM_HUMANS, WINREASON_OPPONENTS_DEAD );
			return true;
		}
		else
		{
			// RED wins if they survive until time runs out.
			if ( m_hRoundTimer && m_hRoundTimer->GetTimeRemaining() == 0.0f )
			{
				SetWinningTeam( TDC_TEAM_HUMANS, WINREASON_DEFEND_UNTIL_TIME_LIMIT );
				return true;
			}
		}
	}
	else
	{
		// BLU wins if they kill all humans.
		SetWinningTeam( TDC_TEAM_ZOMBIES, WINREASON_OPPONENTS_DEAD );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::StartVIPRound( void )
{
	ResetVIP();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::VIP_RunLogic( void )
{
	if ( !m_hVIPPlayer )
	{
		m_hVIPPlayer = SelectVIP( TDC_TEAM_RED );
		if ( m_hVIPPlayer != nullptr)
		{
			m_hVIPPlayer->ForceRespawn();
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCPlayer *CTDCGameRules::SelectVIP(int iTeamNum)
{
	CUtlVector<CTDCPlayer *> activePlayers;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer || pPlayer->GetTeamNumber() != iTeamNum || !pPlayer->IsReadyToPlay() )
			continue;

		activePlayers.AddToTail(pPlayer);
	}

	if ( activePlayers.Count() < 2 )
	{
		State_Transition( GR_STATE_PREGAME );
		return nullptr;
	}

	CTDCPlayer *pNewVIP = nullptr;

	// So we don't select the same player
	do
	{
		int iRandIdx = RandomInt(0, activePlayers.Count() - 1);
		pNewVIP = activePlayers[iRandIdx];
	} while ( pNewVIP == m_hVIPPlayer );

	return pNewVIP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::GetAssignedHumanTeam()
{
	const char *pszHumanTeamName = mp_humans_must_join_team.GetString();
	
	for ( int iTeamNum = TDC_TEAM_COUNT - 1; iTeamNum != TEAM_UNASSIGNED; --iTeamNum )
	{
		if ( V_stricmp( pszHumanTeamName, g_aTeamNames[iTeamNum] ) == 0 )
			return iTeamNum;
	}
	
	/* the default value of mp_humans_must_join_team is assumed to be "any" */
	return TEAM_ANY;
}

#endif  // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap( collisionGroup0, collisionGroup1 );
	}
	
	//Don't stand on COLLISION_GROUP_WEAPONs
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets fired by tf_point_weapon_mimic don't collide with other rockets.
	if ( collisionGroup1 == TDC_COLLISION_GROUP_ROCKETS_NOTSOLID )
	{
		if ( collisionGroup0 == TDC_COLLISION_GROUP_ROCKETS || collisionGroup0 == TDC_COLLISION_GROUP_ROCKETS_NOTSOLID )
			return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( collisionGroup1 == TDC_COLLISION_GROUP_ROCKETS || collisionGroup1 == TDC_COLLISION_GROUP_ROCKETS_NOTSOLID )
	{
		if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
			return true;

		if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT )
			return false;

		if ( collisionGroup0 == COLLISION_GROUP_WEAPON )
			return false;

		if ( collisionGroup0 == TDC_COLLISIONGROUP_GRENADES )
			return false;
	}

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TDC_COLLISIONGROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TDC_COLLISIONGROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TDC_COLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );

#if 0
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
#endif

	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TDC_COLLISIONGROUP_GRENADES ) && 
		 ( collisionGroup1 == TDC_COLLISIONGROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TDC_COLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TDC_COLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60 + (float)m_iMapTimeBonus;
	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	// If the round timer is longer, let the round complete
	// TFTODO: Do we need to worry about the timelimit running our during a round?

	int iTime = (int)( flMapChangeTime - gpGlobals->curtime );
	if ( iTime < 0 )
	{
		iTime = 0;
	}

	return iTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();
	
	if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
#ifdef GAME_DLL
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTDC_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
#ifdef GAME_DLL
		// if this is a capture event, remember the player who made the capture		
		auto iEventType = (ETDCFlagEventTypes)event->GetInt( "eventtype" );
		if ( TDC_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
#endif
	}
#ifdef GAME_DLL
	else if ( !Q_strcmp( eventName, "player_escort_score" ) )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( event->GetInt( "player" ) ) );

		if ( pPlayer )
		{
			CTDC_GameStats.Event_PlayerScoresEscortPoints( pPlayer, event->GetInt( "points" ) );
		}
	}
	else if ( g_fGameOver && !Q_strcmp( eventName, "server_changelevel_failed" ) )
	{
		Warning( "In gameover, but failed to load the next map. Trying next map in cycle.\n" );
		nextlevel.SetValue( "" );
		ChangeLevel();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;

		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i = 1; i < TDC_AMMO_COUNT; i++ )
		{
			def.AddAmmoType( g_aAmmoNames[i], DMG_BULLET, TRACER_LINE, 0, 0, 5000, 2400, 0, 10, 14 );
			Assert( def.Index( g_aAmmoNames[i] ) == i );
		}
	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::FlagsMayBeCapped( void )
{
	return PointsMayBeCaptured();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCGameRules::GetTeamGoalString( int iTeam )
{
	switch ( GetGameType() )
	{
	case TDC_GAMETYPE_FFA:
		return "FREE FOR ALL";
		break;
	case TDC_GAMETYPE_TDM:
		return "TEAM DEATHMATCH";
		break;
	case TDC_GAMETYPE_DUEL:
		return "DUEL GAMEMODE";
		break;
	case TDC_GAMETYPE_CTF: //leave CTF behavior as is
		if ( iTeam == TDC_TEAM_RED )
			return m_pszTeamGoalStringRed.Get();
		if ( iTeam == TDC_TEAM_BLUE) 
			return m_pszTeamGoalStringBlue.Get();
		break;
	case TDC_GAMETYPE_INVADE:
		return "INVADE";
		break;
	case TDC_GAMETYPE_BM:
		return "BM";
		break;
	case TDC_GAMETYPE_TBM:
		return "TEAM BM";
		break;
	case TDC_GAMETYPE_INFECTION:
		return "INFECTION";
		break;
	case TDC_GAMETYPE_VIP:
		return "VIP ESCORT";
	default:
		return "UNKNOWN GAMETYPE";
		break;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGameRules::GetScoreLimit( void )
{
	int iFragLimit;

	switch ( GetGameType() )
	{
	case TDC_GAMETYPE_FFA:
		iFragLimit = tdc_ffa_fraglimit.GetInt();
		break;
	case TDC_GAMETYPE_TDM:
		iFragLimit = tdc_tdm_fraglimit.GetInt();
		break;
	case TDC_GAMETYPE_DUEL:
		iFragLimit = tdc_duel_fraglimit.GetInt();
		break;
	case TDC_GAMETYPE_CTF:
		iFragLimit = tdc_ctf_scorelimit.GetInt();
		break;
	case TDC_GAMETYPE_INVADE:
		iFragLimit = tdc_invade_scorelimit.GetInt();
		break;
	case TDC_GAMETYPE_BM:
		iFragLimit = tdc_bloodmoney_scorelimit.GetInt();
		break;
	case TDC_GAMETYPE_TBM:
		iFragLimit = tdc_teambloodmoney_scorelimit.GetInt();
		break;
	default:
		iFragLimit = 0;
		break;
	}

	if ( !IsRoundBasedMode() && iFragLimit > 0 )
	{
		iFragLimit += m_iFragLimitBonus;
	}

	return iFragLimit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
#ifdef CLIENT_DLL
	pPlayer = C_BasePlayer::GetLocalPlayer();
#endif

	if ( pPlayer && !pPlayer->IsAlive() && pPlayer->GetTeamNumber() <= LAST_SHARED_TEAM )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::AllowThirdPersonCamera( void )
{
	return tdc_allow_thirdperson.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon in the player's inventory that would be better than
//			the given weapon.
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CTDCGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	CBaseCombatWeapon *pCheck;
	CBaseCombatWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.

	int iCurrentWeight = -1;
	int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	// If I have a weapon, make sure I'm allowed to holster it
	if ( pCurrentWeapon )
	{
		if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
		{
			// Either this weapon doesn't allow autoswitching away from it or I
			// can't put this weapon away right now, so I can't switch.
			return NULL;
		}

		iCurrentWeight = pCurrentWeapon->GetWeight();
	}

	for ( int i = 0; i < pPlayer->WeaponCount(); ++i )
	{
		pCheck = pPlayer->GetWeapon( i );
		if ( !pCheck )
			continue;

		// If we have an active weapon and this weapon doesn't allow autoswitching away
		// from another weapon, skip it.
		if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
			continue;

		if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
		{
			// this weapon is from the same category. 
			if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
				return pCheck;
		}
		else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
		{
			//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
			// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
			// that the player was using. This will end up leaving the player with his heaviest-weighted 
			// weapon. 
			if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
			{
				// if this weapon is useable, flag it as the best
				iBestWeight = pCheck->GetWeight();
				pBest = pCheck;
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	return pBest;
}

#ifdef GAME_DLL

	Vector MaybeDropToGround( 
							CBaseEntity *pMainEnt, 
							bool bDropToGround, 
							const Vector &vPos, 
							const Vector &vMins, 
							const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//

		Vector mins, maxs;
		pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;


		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;

		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;

					if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

		//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}

#else // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::SetRoundState( int iRoundState )
{
	m_iRoundState = iRoundState;
	m_flLastRoundStateChangeTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_bOldInWaitingForPlayers = m_bInWaitingForPlayers;
	m_bOldInOvertime = m_bInOvertime;
	m_bOldInSetup = m_bInSetup;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED || 
		m_bOldInWaitingForPlayers != m_bInWaitingForPlayers ||
		m_bOldInOvertime != m_bInOvertime ||
		m_bOldInSetup != m_bInSetup )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( State_Get() == GR_STATE_STALEMATE )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
			if ( event )
			{
				event->SetInt( "reason", STALEMATE_JOIN_MID );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	}

	if ( m_bInOvertime && ( m_bOldInOvertime != m_bInOvertime ) )
	{
		HandleOvertimeBegin();
	}
}

void CTDCGameRules::HandleOvertimeBegin()
{
	g_TFAnnouncer.Speak( TDC_ANNOUNCER_OVERTIME );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() || State_Get() == GR_STATE_PREGAME || State_Get() == GR_STATE_STARTGAME )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameRules::GetTeamGlowColor( int nTeam, float &r, float &g, float &b )
{
	switch ( nTeam )
	{
		case TDC_TEAM_BLUE:
			r = 0.49f; g = 0.66f; b = 0.77f;
			break;

		case TDC_TEAM_RED:
			r = 0.74f; g = 0.23f; b = 0.23f;
			break;

		default:
			r = 0.76f; g = 0.76f; b = 0.76f;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	V_strcpy_safe( strFullpath, "media/" );	// Assume we must play out of the media directory
	V_strcat_safe( strFullpath, mapname );

	if ( bWithExtension )
	{
		V_strcat_safe( strFullpath, ".bik" );		// Assume we're a .bik extension type
	}

	return strFullpath;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameRules::AllowWeatherParticles( void )
{
	return !tdc_particles_disable_weather.GetBool();
}

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName )
{
	KeyValues *pKeyvalToAdd = new KeyValues( pszName );

	if ( pKeyvalToAdd )
		pKeys->AddSubKey( pKeyvalToAdd );
}
#endif
