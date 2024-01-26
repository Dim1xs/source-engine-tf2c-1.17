//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#include "cbase.h"
#include "tdc_team.h"
#include "entitylist.h"
#include "util.h"
#include "tdc_gamerules.h"
#include "tdc_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// TF Team tables.
//
IMPLEMENT_SERVERCLASS_ST( CTDCTeam, DT_TDCTeam )
	SendPropInt( SENDINFO( m_iRoundScore ) ),
END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( tdc_team, CTDCTeam );

//=============================================================================
//
// TF Team Manager Functions.
//
CTDCTeamManager s_TFTeamManager;

CTDCTeamManager *TFTeamMgr()
{
	return &s_TFTeamManager;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCTeamManager::CTDCTeamManager()
{
	m_UndefinedTeamColor.r = 255;
	m_UndefinedTeamColor.g = 255;
	m_UndefinedTeamColor.b = 255;
	m_UndefinedTeamColor.a = 0;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCTeamManager::Init( void )
{
	// Clear the list.
	Shutdown();

	// Create the team list.
	for ( int iTeam = 0; iTeam < TDC_TEAM_COUNT; ++iTeam )
	{
		int index = Create( g_aTeamNames[iTeam] );
		Assert( index == iTeam );
		if ( index != iTeam )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCTeamManager::Shutdown( void )
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCTeamManager::Create( const char *pName )
{
	CTDCTeam *pTeam = static_cast<CTDCTeam*>( CreateEntityByName( "tdc_team" ) );
	if ( pTeam )
	{
		// Add the team to the global list of teams.
		int iTeam = g_Teams.AddToTail( pTeam );

		// Initialize the team.
		pTeam->Init( pName, iTeam );
		pTeam->NetworkProp()->SetUpdateInterval( 0.75f );

		return iTeam;
	}

	// Error.
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Remove any teams that are not active on the current map.
//-----------------------------------------------------------------------------
void CTDCTeamManager::RemoveExtraTeams( void )
{
	int nActiveTeams = TDC_TEAM_COUNT;

	if ( !TDCGameRules()->IsTeamplay() )
	{
		nActiveTeams = FIRST_GAME_TEAM + 1;
	}

	for ( int i = g_Teams.Count() - 1; i >= nActiveTeams; i-- )
	{
		UTIL_Remove( g_Teams[i] );
		g_Teams.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCTeamManager::GetRoundScore( int iTeam )
{
	if ( !IsValidTeam( iTeam ) )
		return -1;

	CTDCTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return -1;

	return pTeam->GetRoundScore();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCTeamManager::AddRoundScore( int iTeam, int iScore /*= 1*/ )
{
	if ( !IsValidTeam( iTeam ) )
		return;

	CTDCTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return;

	pTeam->AddRoundScore( iScore );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCTeamManager::AddTeamScore( int iTeam, int iScoreToAdd )
{
	if ( !IsValidTeam( iTeam ) )
		return;

	CTeam *pTeam = GetGlobalTeam( iTeam );
	if ( !pTeam )
		return;

	pTeam->AddScore( iScoreToAdd );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCTeamManager::IsValidTeam( int iTeam )
{
	if ( ( iTeam >= 0 ) && ( iTeam < GetNumberOfTeams() ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CTDCTeamManager::GetTeamCount( void )
{
	return GetNumberOfTeams();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCTeam *CTDCTeamManager::GetTeam( int iTeam )
{
	Assert( IsValidTeam( iTeam ) );

	return GetGlobalTFTeam( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCTeam *CTDCTeamManager::GetSpectatorTeam()
{
	return GetTeam( 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
color32 CTDCTeamManager::GetUndefinedTeamColor( void )
{
	return m_UndefinedTeamColor;
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the player's screen.
//-----------------------------------------------------------------------------
void CTDCTeamManager::PlayerCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	ClientPrint( pPlayer, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the given teams screen.
//-----------------------------------------------------------------------------
void CTDCTeamManager::TeamCenterPrint( int iTeam, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CTeamRecipientFilter filter( iTeam, true );
	UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the player's teams screen (minus
//          the player).
//-----------------------------------------------------------------------------
void CTDCTeamManager::PlayerTeamCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CTeamRecipientFilter filter( pPlayer->GetTeamNumber(), true );
	filter.RemoveRecipient( pPlayer );
	UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//=============================================================================
//
// TF Team Functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTDCTeam::CTDCTeam()
{
	m_iWins = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCTeam::Init( const char *pName, int iNumber )
{
	BaseClass::Init( pName, iNumber );
}

//-----------------------------------------------------------------------------
// Purpose:
//   Input: pPlayer - print to just that client, NULL = all clients
//-----------------------------------------------------------------------------
void CTDCTeam::ShowScore( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
		ClientPrint( pPlayer, HUD_PRINTNOTIFY,  UTIL_VarArgs( "Team %s: %d\n", GetName(), GetScore() ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "Team %s: %d\n", GetName(), GetScore() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCPlayer *CTDCTeam::GetTDCPlayer( int iIndex )
{
	return ToTDCPlayer( GetPlayer( iIndex ) );
}

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team
//-----------------------------------------------------------------------------
CTDCTeam *GetGlobalTFTeam( int iIndex )
{
	return assert_cast<CTDCTeam *>( GetGlobalTeam( iIndex ) );
}
