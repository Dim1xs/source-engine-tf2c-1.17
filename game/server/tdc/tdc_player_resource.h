//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_PLAYER_RESOURCE_H
#define TDC_PLAYER_RESOURCE_H

#ifdef _WIN32
#pragma once
#endif

#include "player_resource.h"

class CTDCPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CTDCPlayerResource, CPlayerResource );
	
public:
	DECLARE_SERVERCLASS();

	CTDCPlayerResource();

	virtual void UpdatePlayerData( void );
	virtual void Spawn( void );

	int	GetTotalScore( int iIndex );
	Color GetPlayerColor( int iIndex );

protected:
	CNetworkArray( int, m_iTotalScore, MAX_PLAYERS + 1 );
	CNetworkArray( int, m_iMaxHealth, MAX_PLAYERS + 1 );
	CNetworkArray( int, m_iActiveDominations, MAX_PLAYERS + 1 );
	CNetworkArray( bool, m_bArenaSpectator, MAX_PLAYERS + 1 );
	CNetworkArray( Vector, m_vecColors, MAX_PLAYERS + 1 );
	CNetworkArray( int, m_iWinAnimations, MAX_PLAYERS + 1 );
	CNetworkArray( int, m_iKillstreak, MAX_PLAYERS + 1 );
	CNetworkArray( int, m_nMoneyPacks, MAX_PLAYERS + 1 );
	CNetworkArray( int, m_iWearableItemIDs, ( MAX_PLAYERS + 1 ) * TDC_WEARABLE_COUNT );
	CNetworkArray( Vector, m_vecSkinTones, MAX_PLAYERS + 1 );
};

inline CTDCPlayerResource *GetTDCPlayerResource( void )
{
	if ( !g_pPlayerResource )
		return NULL;

	return assert_cast<CTDCPlayerResource *>( g_pPlayerResource );
}

#endif // TDC_PLAYER_RESOURCE_H
