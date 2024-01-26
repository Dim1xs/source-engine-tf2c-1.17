//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tdc_shareddefs.h"
#include "tdc_player_resource.h"
#include "tdc_player.h"
#include "tdc_gamestats.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTDCPlayerResource, DT_TDCPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 13 ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iActiveDominations ), SendPropInt( SENDINFO_ARRAY( m_iActiveDominations ), Q_log2( MAX_PLAYERS ) + 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bArenaSpectator ), SendPropBool( SENDINFO_ARRAY( m_bArenaSpectator ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_vecColors ), SendPropVector( SENDINFO_ARRAY( m_vecColors ), 8, 0, 0.0f, 1.0f ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iWinAnimations ), SendPropInt( SENDINFO_ARRAY( m_iWinAnimations ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iKillstreak ), SendPropInt( SENDINFO_ARRAY( m_iKillstreak ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nMoneyPacks ), SendPropInt( SENDINFO_ARRAY( m_nMoneyPacks ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iWearableItemIDs ), SendPropInt( SENDINFO_ARRAY( m_iWearableItemIDs ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_vecSkinTones ), SendPropVector( SENDINFO_ARRAY( m_vecSkinTones ), 8, 0, 0.0f, 1.0f ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tdc_player_manager, CTDCPlayerResource );

CTDCPlayerResource::CTDCPlayerResource( void )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		for ( int j = 0; j < TDC_WEARABLE_COUNT; j++ )
			m_iWearableItemIDs.Set( ( i * TDC_WEARABLE_COUNT ) + j, TDC_WEARABLE_INVALID );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerResource::UpdatePlayerData( void )
{
	BaseClass::UpdatePlayerData();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = (CTDCPlayer *)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->IsConnected() )
		{
			CTDCPlayerClass *pClass = pPlayer->GetPlayerClass();
			m_iMaxHealth.Set( i, pClass->GetMaxHealth() );

			RoundStats_t *pRoundStats = &CTDC_GameStats.FindPlayerStats( pPlayer )->statsAccumulated;

			if ( TDCGameRules()->IsInDuelMode() )
			{
				// In Duel, we want to show per-round stats instead.
				pRoundStats = &CTDC_GameStats.FindPlayerStats( pPlayer )->statsCurrentRound;
			}

			int iTotalScore = CTDCGameRules::CalcPlayerScore( pRoundStats );
			m_iTotalScore.Set( i, iTotalScore );

			m_iActiveDominations.Set( i, pPlayer->GetNumberOfDominations() );
			m_bArenaSpectator.Set( i, pPlayer->IsArenaSpectator() );
			m_vecColors.Set( i, pPlayer->m_vecPlayerColor );
			m_iKillstreak.Set( i, pPlayer->m_Shared.GetKillstreak() );
			m_iWinAnimations.Set( i, pPlayer->GetClientConVarIntValue( "tdc_merc_winanim" ) );
			m_nMoneyPacks.Set( i, pPlayer->GetNumMoneyPacks() );
			m_vecSkinTones.Set( i, pPlayer->m_vecPlayerSkinTone );

			for ( int j = 0; j < TDC_WEARABLE_COUNT; j++ )
			{
				int iItemID = pPlayer->GetItemPreset( pClass->GetClassIndex(), (ETDCWearableSlot)j );
				m_iWearableItemIDs.Set( ( i * TDC_WEARABLE_COUNT ) + j, iItemID );
			}
		}
	}
}

void CTDCPlayerResource::Spawn( void )
{
	int i;

	for ( i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iTotalScore.Set( i, 0 );
		m_iMaxHealth.Set( i, 1 );
		m_vecColors.Set( i, Vector( 0.0, 0.0, 0.0 ) );
		m_iKillstreak.Set( i, 0 );
		m_vecSkinTones.Set( i, Vector( 0.0, 0.0, 0.0 ) );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTDCPlayerResource::GetTotalScore( int iIndex )
{
	CTDCPlayer *pPlayer = (CTDCPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{
		return m_iTotalScore[iIndex];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTDCPlayerResource::GetPlayerColor( int iIndex )
{
	return Color( m_vecColors[iIndex].x * 255.0, m_vecColors[iIndex].y * 255.0, m_vecColors[iIndex].z * 255.0, 255 );
}
