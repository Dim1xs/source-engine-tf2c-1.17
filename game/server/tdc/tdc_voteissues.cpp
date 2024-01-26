//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tdc_shareddefs.h"
#include "tdc_voteissues.h"
#include "tdc_gamerules.h"
#include "tdc_gamestats.h"

//-----------------------------------------------------------------------------
//
// Purpose: Kick vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_kick_allowed( "sv_vote_issue_kick_allowed", "0", FCVAR_NONE, "Can players call votes to kick players from the server?" );
ConVar sv_vote_kick_ban_duration( "sv_vote_kick_ban_duration", "20", FCVAR_NONE, "The number of minutes a vote ban should last. (0 = Disabled)" );
ConVar sv_vote_issue_kick_namelock_duration( "sv_vote_issue_kick_namelock_duration", "120", FCVAR_NONE, "How long to prevent kick targets from changing their name (in seconds)." );

CKickIssue::CKickIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::Init()
{
	m_szPlayerName[0] = '\0';
	m_szSteamID[0] = '\0';
	m_iReason = 0;
	m_SteamID.Clear();
	m_hPlayerTarget = NULL;
	SetYesNoVoteCount( 0, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CKickIssue::GetDisplayString( void )
{
	switch ( m_iReason )
	{
	case KICK_REASON_IDLE:
		return "#TDC_vote_kick_player_idle";
	case KICK_REASON_SCAMMING:
		return "#TDC_vote_kick_player_scamming";
	case KICK_REASON_CHEATING:
		return "#TDC_vote_kick_player_cheating";
	default:
		return "#TDC_vote_kick_player_other";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CKickIssue::GetVotePassedString( void )
{
	if ( sv_vote_kick_ban_duration.GetInt() > 0 && m_SteamID.IsValid() )
		return "#TDC_vote_passed_ban_player";

	return "#TDC_vote_passed_kick_player";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CKickIssue::GetDetailsString()
{
	CBasePlayer *pPlayer = m_hPlayerTarget.Get();

	if ( pPlayer )
		return pPlayer->GetPlayerName();

	if ( m_szPlayerName[0] != '\0' )
		return m_szPlayerName;

	return "Unnamed";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <userID>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CKickIssue::IsEnabled()
{
	return sv_vote_issue_kick_allowed.GetBool();
}

void CKickIssue::NotifyGC( bool a2 )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CKickIssue::PrintLogData()
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int GetKickBanPlayerReason( const char *pszReason )
{
	if ( V_strncmp( pszReason, "other", 5 ) == 0 )
		return CKickIssue::KICK_REASON_NONE;

	if ( V_strncmp( pszReason, "cheating", 8 ) == 0 )
		return CKickIssue::KICK_REASON_CHEATING;

	if ( V_strncmp( pszReason, "idle", 4 ) == 0 )
		return CKickIssue::KICK_REASON_IDLE;

	if ( V_strncmp( pszReason, "scamming", 8 ) == 0 )
		return CKickIssue::KICK_REASON_SCAMMING;

	return CKickIssue::KICK_REASON_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CKickIssue::CreateVoteDataFromDetails( const char *pszDetails )
{
	int iPlayerID = 0;

	const char *pch;
	pch = strrchr( pszDetails, ' ' );

	if ( pch )
	{
		m_iReason = GetKickBanPlayerReason( pch + 1 );

		CUtlString string( pszDetails, pch + 1 - pszDetails );
		iPlayerID = atoi( string );
	}
	else
	{
		iPlayerID = atoi( pszDetails );
	}

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( iPlayerID );

	if ( pPlayer )
	{
		m_hPlayerTarget = pPlayer;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CKickIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	Init();

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	// Get the player and kick reason.
	if ( !CreateVoteDataFromDetails( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_PLAYERNOTFOUND;
		return false;
	}

	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	Assert( pPlayer );

	// Can't votekick special bots.
	if ( pPlayer->IsHLTV() || pPlayer->IsReplay() )
		return false;

	// Can't votekick spectators.
	if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return false;

	// Can't kick a player with RCON access or listen server host.
	if ( ( !engine->IsDedicatedServer() && pPlayer->entindex() == 1 ) || pPlayer->IsAutoKickDisabled() )
	{
		nFailCode = VOTE_FAILED_CANNOT_KICK_ADMIN;
		return false;
	}

	if ( nEntIndex != DEDICATED_SERVER )
	{
		CBasePlayer *pCaller = UTIL_PlayerByIndex( nEntIndex );
		if ( !pCaller )
			return false;

		// Can only call votekick on teammates.
		if ( pCaller->GetTeamNumber() != pPlayer->GetTeamNumber() )
		{
			return false;
		}
	}


	if ( !pPlayer->IsFakeClient() )
	{
		if ( !pPlayer->GetSteamID( &m_SteamID ) || !m_SteamID.IsValid() )
		{
			return false;
		}
	}

#if 0
	if ( TDCGameRules()->IsMannVsMachineMode() )
	{
		// Live TF2 has MvM specific fail codes here...
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::OnVoteStarted()
{
	CBasePlayer *pPlayer = GetTargetPlayer();

	// Remember their name and Steam ID string.
	// Live TF2 uses CSteamID::Render but we can't do that because we're missing some required library.
	V_strcpy_safe( m_szPlayerName, pPlayer->GetPlayerName() );
	V_strcpy( m_szSteamID, engine->GetPlayerNetworkIDString( pPlayer->edict() ) );

	if ( sv_vote_issue_kick_namelock_duration.GetFloat() > 0.0f )
	{
		g_voteController->AddPlayerToNameLockedList(
			m_SteamID,
			sv_vote_issue_kick_namelock_duration.GetFloat(),
			pPlayer->GetUserID() );
	}

	// Have the target player automatically vote No.
	g_voteController->TryCastVote( pPlayer->entindex(), "Option2" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::OnVoteFailed( int iEntityHoldingVote )
{
	CBaseIssue::OnVoteFailed( iEntityHoldingVote );
	SetYesNoVoteCount( 0, 0, 0 );
	PrintLogData();
	NotifyGC( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKickIssue::ExecuteCommand()
{
	PrintLogData();

	CBasePlayer *pPlayer = GetTargetPlayer();

	if ( pPlayer )
	{
		// Kick them.
		engine->ServerCommand( UTIL_VarArgs( "kickid %d %s;", pPlayer->GetUserID(), "#TDC_Vote_kicked" ) );
	}

	// Also ban their Steam ID if enabled.
	if ( sv_vote_kick_ban_duration.GetInt() > 0 && m_SteamID.IsValid() )
	{
		engine->ServerCommand( UTIL_VarArgs( "banid %d %s;", sv_vote_kick_ban_duration.GetInt(), m_szSteamID ) );
		g_voteController->AddPlayerToKickWatchList( m_SteamID, 60.0f * sv_vote_kick_ban_duration.GetFloat() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CKickIssue::IsTeamRestrictedVote()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CKickIssue::GetTargetPlayer( void )
{
	CBasePlayer *pPlayer = m_hPlayerTarget.Get();
	if ( !pPlayer && m_SteamID.IsValid() )
	{
		pPlayer = UTIL_PlayerBySteamID( m_SteamID );
	}

	return pPlayer;
}


//-----------------------------------------------------------------------------
//
// Purpose: Restart map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_restart_game_allowed( "sv_vote_issue_restart_game_allowed", "0", FCVAR_NONE, "Can players call votes to restart the game?" );
ConVar sv_vote_issue_restart_game_cooldown( "sv_vote_issue_restart_game_cooldown", "300", FCVAR_NONE, "Minimum time before another restart vote can occur (in seconds)." );

CRestartGameIssue::CRestartGameIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRestartGameIssue::GetDisplayString( void )
{
	return "#TDC_vote_restart_game";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRestartGameIssue::GetVotePassedString( void )
{
	return "#TDC_vote_passed_restart_game";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRestartGameIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		ListStandardNoArgCommand( pForWhom, GetTypeString() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRestartGameIssue::IsEnabled( void )
{
	return sv_vote_issue_restart_game_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRestartGameIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

extern ConVar mp_restartgame;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRestartGameIssue::ExecuteCommand( void )
{
	if ( sv_vote_issue_restart_game_cooldown.GetFloat() > 0.0f )
	{
		m_flNextCallTime = gpGlobals->curtime + sv_vote_issue_restart_game_cooldown.GetFloat();
	}

	mp_restartgame.SetValue( 1 );
}


//-----------------------------------------------------------------------------
//
// Purpose: Change map vote.
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_changelevel_allowed( "sv_vote_issue_changelevel_allowed", "0", FCVAR_NONE, "Can players call votes to change levels?" );

CChangeLevelIssue::CChangeLevelIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetDisplayString( void )
{
	return "#TDC_vote_changelevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetVotePassedString( void )
{
	return "#TDC_vote_passed_changelevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetDetailsString( void )
{
	return CBaseIssue::GetDetailsString();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <mapname>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::IsEnabled( void )
{
	return sv_vote_issue_changelevel_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::IsYesNoVote( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::CanTeamCallVote( int iTeam )
{
	return true;
}

bool VotableMap( const char *pszMapName )
{
	char szBuf[MAX_MAP_NAME];
	V_strcpy_safe( szBuf, pszMapName );

	return ( engine->FindMap( szBuf, MAX_MAP_NAME ) == IVEngineServer::eFindMap_Found );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChangeLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( !pszDetails[0] )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}

	if ( !VotableMap( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeLevelIssue::ExecuteCommand( void )
{
	engine->ChangeLevel( GetDetailsString(), NULL );
}


//-----------------------------------------------------------------------------
//
// Purpose: Next map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed", "1", FCVAR_NONE, "Can players call votes to set the next level?" );
ConVar sv_vote_issue_nextlevel_choicesmode( "sv_vote_issue_nextlevel_choicesmode", "0", FCVAR_NONE, "Present players with a list of lowest playtime maps to choose from?" );
ConVar sv_vote_issue_nextlevel_allowextend( "sv_vote_issue_nextlevel_allowextend", "1", FCVAR_NONE, "Allow players to extend the current map?" );
ConVar sv_vote_issue_nextlevel_prevent_change( "sv_vote_issue_nextlevel_prevent_change", "1", FCVAR_NONE, "Not allowed to vote for a nextlevel if one has already been set." );

CNextLevelIssue::CNextLevelIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetDisplayString( void )
{
	if ( IsInMapChoicesMode() )
	{
		return "#TDC_vote_nextlevel_choices";
	}

	return "#TDC_vote_nextlevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetVotePassedString( void )
{
	if ( sv_vote_issue_nextlevel_allowextend.GetBool() && V_strcmp( GetDetailsString(), "Extend current Map" ) == 0 )
	{
		return "#TDC_vote_passed_nextlevel_extend";
	}

	return "#TDC_vote_passed_nextlevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetDetailsString()
{
	return CBaseIssue::GetDetailsString();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNextLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		char szBuf[64];
		V_sprintf_safe( szBuf, "callvote %s <mapname>\n", GetTypeString() );
		ClientPrint( pForWhom, HUD_PRINTCONSOLE, szBuf );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::IsEnabled( void )
{
	return sv_vote_issue_nextlevel_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::IsYesNoVote( void )
{
	return !IsInMapChoicesMode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::CanTeamCallVote( int iTeam )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( sv_vote_issue_nextlevel_choicesmode.GetBool() && nEntIndex == DEDICATED_SERVER )
	{
		return ( pszDetails[0] == '\0' );
	}

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( !pszDetails[0] )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		return false;
	}

	if ( !VotableMap( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
		return false;
	}

	if ( !MultiplayRules()->IsMapInMapCycle( pszDetails ) )
	{
		nFailCode = VOTE_FAILED_MAP_NOT_VALID;
		return false;
	}

	if ( sv_vote_issue_nextlevel_prevent_change.GetBool() && nextlevel.GetString()[0] )
	{
		nFailCode = VOTE_FAILED_NEXTLEVEL_SET;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNextLevelIssue::GetNumberVoteOptions( void )
{
	return IsInMapChoicesMode() ? 5 : 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNextLevelIssue::GetQuorumRatio( void )
{
	if ( IsInMapChoicesMode() )
		return 0.1f;

	return CBaseIssue::GetQuorumRatio();
}

extern ConVar mp_maxrounds;
extern ConVar mp_winlimit;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNextLevelIssue::ExecuteCommand( void )
{
	if ( V_strcmp( GetDetailsString(), "Extend current Map" ) == 0 )
	{
		TDCGameRules()->ExtendCurrentMap();
	}
	else
	{
		nextlevel.SetValue( GetDetailsString() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{
	if ( !IsInMapChoicesMode() )
	{
		return CBaseIssue::GetVoteOptions( vecNames );
	}

	int iMapsNeeded = GetNumberVoteOptions();

	if ( sv_vote_issue_nextlevel_allowextend.GetBool() )
	{
		iMapsNeeded--;
	}

	// Get 4/5 maps with the smallest playtime.
	m_MapList.PurgeAndDeleteElements();
	CTDC_GameStats.GetVoteData( "NextLevel", iMapsNeeded, m_MapList );
	Assert( m_MapList.Count() != 0 );

	for ( const char *pszName : m_MapList )
	{
		vecNames.AddToTail( pszName );
	}

	// We don't want to show a vote with less than 2 options.
	if ( sv_vote_issue_nextlevel_allowextend.GetBool() || vecNames.Count() < 2 )
	{
		vecNames.AddToTail( "Extend current Map" );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNextLevelIssue::IsInMapChoicesMode( void )
{
	return ( !GetDetailsString()[0] && sv_vote_issue_nextlevel_choicesmode.GetBool() );
}


//-----------------------------------------------------------------------------
//
// Purpose: Extend map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_extendlevel_allowed( "sv_vote_issue_extendlevel_allowed", "1", FCVAR_NONE,  "Can players call votes to set the next level?" );
ConVar sv_vote_issue_extendlevel_quorum( "sv_vote_issue_extendlevel_quorum", "0.6", FCVAR_NONE, "What is the ratio of voters needed to reach quorum?" );

CExtendLevelIssue::CExtendLevelIssue( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CExtendLevelIssue::GetDisplayString( void )
{
	return "#TDC_vote_extendlevel";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CExtendLevelIssue::GetVotePassedString( void )
{
	return "#TDC_vote_passed_nextlevel_extend";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtendLevelIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		ListStandardNoArgCommand( pForWhom, GetTypeString() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CExtendLevelIssue::IsEnabled( void )
{
	return sv_vote_issue_extendlevel_allowed.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CExtendLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CExtendLevelIssue::GetQuorumRatio( void )
{
	return sv_vote_issue_extendlevel_quorum.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtendLevelIssue::ExecuteCommand( void )
{
	TDCGameRules()->ExtendCurrentMap();
}


//-----------------------------------------------------------------------------
//
// Purpose: Restart map vote
//
//-----------------------------------------------------------------------------
ConVar sv_vote_issue_scramble_teams_allowed( "sv_vote_issue_scramble_teams_allowed", "1", FCVAR_NONE, "Can players call votes to scramble the teams?" );
ConVar sv_vote_issue_scramble_teams_cooldown( "sv_vote_issue_scramble_teams_cooldown", "1200", FCVAR_NONE, "Minimum time before another scramble vote can occur (in seconds)." );

CScrambleTeams::CScrambleTeams( const char *pszTypeString ) : CBaseIssue( pszTypeString )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CScrambleTeams::GetDisplayString( void )
{
	return "#TDC_vote_scramble_teams";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CScrambleTeams::GetVotePassedString( void )
{
	return "#TDC_vote_passed_scramble_teams";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrambleTeams::ListIssueDetails( CBasePlayer *pForWhom )
{
	if ( IsEnabled() )
	{
		ListStandardNoArgCommand( pForWhom, GetTypeString() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CScrambleTeams::IsEnabled( void )
{
	return ( sv_vote_issue_scramble_teams_allowed.GetBool() && TDCGameRules()->IsTeamplay() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CScrambleTeams::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( CBaseIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime ) == false )
		return false;

	if ( !IsEnabled() )
	{
		nFailCode = VOTE_FAILED_ISSUE_DISABLED;
		return false;
	}

	if ( TeamplayGameRules()->ShouldScrambleTeams() )
	{
		// Scramble already scheduled.
		nFailCode = VOTE_FAILED_SCRAMBLE_IN_PROGRESS;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScrambleTeams::ExecuteCommand( void )
{
	if ( sv_vote_issue_scramble_teams_cooldown.GetFloat() > 0.0f )
	{
		m_flNextCallTime = gpGlobals->curtime + sv_vote_issue_scramble_teams_cooldown.GetFloat();
	}

	engine->ServerCommand( "mp_scrambleteams 2;" );
}
