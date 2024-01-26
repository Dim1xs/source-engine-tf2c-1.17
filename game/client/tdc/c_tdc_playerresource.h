//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TDC_PLAYERRESOURCE_H
#define C_TDC_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"
#include "c_playerresource.h"

class C_TDC_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_TDC_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_TDC_PlayerResource();
	virtual ~C_TDC_PlayerResource();

	int	GetTotalScore( int iIndex ) { return GetArrayValue( iIndex, m_iTotalScore, 0 ); }
	int GetMaxHealth( int iIndex ) { return GetArrayValue( iIndex, m_iMaxHealth, 1 ); }
	int GetNumberOfDominations( int iIndex ) { return GetArrayValue( iIndex, m_iActiveDominations, 0 ); }
	bool IsArenaSpectator( int iIndex ) { return GetArrayValue( iIndex, m_bArenaSpectator ); }
	const Vector &GetPlayerColorVector( int iIndex );
	Color GetPlayerColor( int iIndex );
	int GetWinAnimation( int iIndex ) { return GetArrayValue( iIndex, m_iWinAnimations, 1 ); }
	int GetKillstreak( int iIndex ) { return GetArrayValue( iIndex, m_iKillstreak, 0 ); }
	int GetNumMoneyPacks( int iIndex ) { return GetArrayValue( iIndex, m_nMoneyPacks, 0 ); }

	bool IsPlayerDominated( int iIndex );
	bool IsPlayerDominating( int iIndex );
	bool IsEnemyPlayer( int iIndex );
	int GetUserID( int iIndex );
	bool GetSteamID( int iIndex, CSteamID *pID );

	int GetNumPlayersForTeam( int iTeam, bool bAlive = false );
	int GetTeamMoneyPacks( int iTeam );
	int GetPlayerItemPreset( int iIndex, ETDCWearableSlot iSlot );

	const Vector &GetPlayerSkinTone( int iIndex );

protected:
	int GetArrayValue( int iIndex, int *pArray, int defaultVal );
	bool GetArrayValue( int iIndex, bool *pArray, bool defaultVal = false );

	int		m_iTotalScore[MAX_PLAYERS + 1];
	int		m_iMaxHealth[MAX_PLAYERS + 1];
	int		m_iActiveDominations[MAX_PLAYERS + 1];
	bool	m_bArenaSpectator[MAX_PLAYERS + 1];
	int		m_iKillstreak[MAX_PLAYERS + 1];
	int		m_iWinAnimations[MAX_PLAYERS + 1];
	Vector	m_vecColors[MAX_PLAYERS + 1];
	int		m_nMoneyPacks[MAX_PLAYERS + 1];
	int		m_iWearableItemIDs[ ( MAX_PLAYERS + 1 ) * TDC_WEARABLE_COUNT ];
	Vector	m_vecSkinTones[MAX_PLAYERS + 1];
};

extern C_TDC_PlayerResource *g_TDC_PR;

#endif // C_TDC_PLAYERRESOURCE_H
