//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#ifndef TDC_TEAM_H
#define TDC_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "team.h"
#include "tdc_shareddefs.h"
#include "tdc_gamerules.h"

class CTDCPlayer;

//=============================================================================
// TF Teams.
//
class CTDCTeam : public CTeam
{
	DECLARE_CLASS( CTDCTeam, CTeam );
	DECLARE_SERVERCLASS();

public:

	CTDCTeam();

	virtual void	Init( const char *pName, int iNumber );

	// Score.
	void			ShowScore( CBasePlayer *pPlayer );

	CTDCPlayer		*GetTDCPlayer( int iIndex );

	// Points
	int				GetRoundScore( void ) { return m_iRoundScore; }
	void			AddRoundScore( int nPoints ) { m_iRoundScore += nPoints; }
	void			ResetRoundScore( void ) { m_iRoundScore = 0; }

	int				GetWinCount( void ) { return m_iWins; }
	void			SetWinCount( int iWins ) { m_iWins = iWins; }
	void			IncrementWins( void ) { m_iWins++; }
	void			ResetWins( void ) { m_iWins = 0; }

private:
	CNetworkVar( int, m_iRoundScore );
	int m_iWins;
};

class CTDCTeamManager
{
public:

	CTDCTeamManager();

	// Creation/Destruction.
	bool	Init( void );
	void    Shutdown( void );
	void	RemoveExtraTeams( void );

	bool	IsValidTeam( int iTeam );
	int		GetTeamCount( void );
	CTDCTeam *GetTeam( int iTeam );
	CTDCTeam *GetSpectatorTeam();

	color32 GetUndefinedTeamColor( void );

	void AddTeamScore( int iTeam, int iScoreToAdd );

	void AddRoundScore( int iTeam, int iScore = 1 );
	int GetRoundScore( int iTeam );

	// Screen prints.
	void PlayerCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );
	void TeamCenterPrint( int iTeam, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );
	void PlayerTeamCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1 = NULL, const char *param2 = NULL, const char *param3 = NULL, const char *param4 = NULL );

	// Vox

private:

	int		Create( const char *pName );

private:

	color32	m_UndefinedTeamColor;
};

extern CTDCTeamManager *TFTeamMgr();
extern CTDCTeam *GetGlobalTFTeam( int iIndex );

template<typename Functor>
bool ForEachTFTeam( Functor&& func )
{
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		if ( !func( i ) )
			return false;
	}

	return true;
}

template<typename Functor>
bool ForEachEnemyTFTeam( int iTeam, Functor&& func )
{
	if ( TDCGameRules()->IsTeamplay() )
	{
		// Iterate over every game team except for ourselves.
		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			if ( i != iTeam )
			{
				if ( !func( i ) )
					return false;
			}
		}

		return true;
	}
	else
	{
		return func( FIRST_GAME_TEAM );
	}
}

#endif // TDC_TEAM_H
