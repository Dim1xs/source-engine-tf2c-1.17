//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tdc_playerresource.h"
#include <shareddefs.h>
#include "tdc_shareddefs.h"
#include "hud.h"
#include "tdc_gamerules.h"
#include "clientsteamcontext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT( C_TDC_PlayerResource, DT_TDCPlayerResource, CTDCPlayerResource )
	RecvPropArray3( RECVINFO_ARRAY( m_iTotalScore ), RecvPropInt( RECVINFO( m_iTotalScore[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iMaxHealth ), RecvPropInt( RECVINFO( m_iMaxHealth[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iActiveDominations ), RecvPropInt( RECVINFO( m_iActiveDominations[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bArenaSpectator ), RecvPropBool( RECVINFO( m_bArenaSpectator[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_vecColors ), RecvPropVector( RECVINFO( m_vecColors[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iWinAnimations ), RecvPropInt( RECVINFO( m_iWinAnimations[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iKillstreak ), RecvPropInt( RECVINFO( m_iKillstreak[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nMoneyPacks ), RecvPropInt( RECVINFO( m_nMoneyPacks[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iWearableItemIDs ), RecvPropInt( RECVINFO( m_iWearableItemIDs[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_vecSkinTones ), RecvPropVector( RECVINFO( m_vecSkinTones[0] ) ) ),
END_RECV_TABLE()

C_TDC_PlayerResource *g_TDC_PR = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDC_PlayerResource::C_TDC_PlayerResource()
{
	memcpy( m_Colors, g_aTeamHUDColors, sizeof( g_aTeamHUDColors ) );

	g_TDC_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDC_PlayerResource::~C_TDC_PlayerResource()
{
	g_TDC_PR = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int C_TDC_PlayerResource::GetArrayValue( int iIndex, int *pArray, int iDefaultVal )
{
	if ( !IsConnected( iIndex ) )
	{
		return iDefaultVal;
	}
	return pArray[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
bool C_TDC_PlayerResource::GetArrayValue( int iIndex, bool *pArray, bool bDefaultVal )
{
	if ( !IsConnected( iIndex ) )
	{
		return bDefaultVal;
	}
	return pArray[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector &C_TDC_PlayerResource::GetPlayerColorVector( int iIndex )
{
	if ( !IsConnected( iIndex ) )
	{
		// White color.
		static Vector vecWhite( 1, 1, 1 );
		return vecWhite;
	}

	return m_vecColors[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color C_TDC_PlayerResource::GetPlayerColor( int iIndex )
{
	const Vector &vecColor = GetPlayerColorVector( iIndex );

	return Color( vecColor.x * 255.0f, vecColor.y * 255.0f, vecColor.z * 255.0f, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector &C_TDC_PlayerResource::GetPlayerSkinTone( int iIndex )
{
	if ( !IsConnected( iIndex ) )
	{
		// White color.
		static Vector vecWhite( 1, 1, 1 );
		return vecWhite;
	}

	return m_vecSkinTones[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Returns if this player is an enemy of the local player.
//-----------------------------------------------------------------------------
bool C_TDC_PlayerResource::IsEnemyPlayer( int iIndex )
{
	int iTeam = GetTeam( iIndex );
	int iLocalTeam = GetLocalPlayerTeam();

	// Spectators are nobody's enemy.
	if ( iLocalTeam < FIRST_GAME_TEAM )
		return false;

	// In FFA everybody is an enemy. Except for ourselves.
	if ( !TDCGameRules()->IsTeamplay() )
		return !IsLocalPlayer( iIndex );

	// Players from other teams are enemies.
	return ( iTeam != iLocalTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Returns if this player is dominated by the local player.
//-----------------------------------------------------------------------------
bool C_TDC_PlayerResource::IsPlayerDominated( int iIndex )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return false;

	return pPlayer->m_Shared.IsPlayerDominated( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Returns if this player is dominating the local player.
//-----------------------------------------------------------------------------
bool C_TDC_PlayerResource::IsPlayerDominating( int iIndex )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return false;

	return pPlayer->m_Shared.IsPlayerDominatingMe( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDC_PlayerResource::GetUserID( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	player_info_t pi;
	if ( engine->GetPlayerInfo( iIndex, &pi ) )
	{
		return pi.userID;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TDC_PlayerResource::GetSteamID( int iIndex, CSteamID *pID )
{
	if ( !IsConnected( iIndex ) )
		return false;

	// Copied from C_BasePlayer.
	player_info_t pi;
	if ( engine->GetPlayerInfo( iIndex, &pi ) && pi.friendsID && ClientSteamContext().BLoggedOn() )
	{
		pID->InstancedSet( pi.friendsID, 1, ClientSteamContext().GetConnectedUniverse(), k_EAccountTypeIndividual );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDC_PlayerResource::GetNumPlayersForTeam( int iTeam, bool bAlive /*= false*/ )
{
	int count = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !IsConnected( i ) )
			continue;

		if ( GetTeam( i ) == iTeam && ( !bAlive || IsAlive( i ) ) )
		{
			count++;
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDC_PlayerResource::GetTeamMoneyPacks( int iTeam )
{
	int nMoney = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( IsConnected( i ) && GetTeam( i ) == iTeam )
		{
			nMoney += m_nMoneyPacks[i];
		}
	}

	return nMoney;
}

//-----------------------------------------------------------------------------
// Purpose: Returns player's wearable item preset for the specified slot
//-----------------------------------------------------------------------------
int C_TDC_PlayerResource::GetPlayerItemPreset( int iIndex, ETDCWearableSlot iSlot )
{
	if ( !IsConnected( iIndex ) )
		return TDC_WEARABLE_INVALID;

	return m_iWearableItemIDs[ ( TDC_WEARABLE_COUNT * iIndex ) + iSlot ];
}
