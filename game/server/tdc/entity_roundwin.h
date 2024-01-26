//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTeamplayRoundWin Entity
//
//=============================================================================//
#ifndef ENTITY_ROUND_WIN_H
#define ENTITY_ROUND_WIN_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTeamplayRoundWin Entity.
//

class CTeamplayRoundWin : public CLogicalEntity 
{
public:
	DECLARE_CLASS( CTeamplayRoundWin, CLogicalEntity );

	CTeamplayRoundWin();

	virtual void Spawn( void );

	// Input
	void InputRoundWin( inputdata_t &inputdata );

private:

	void RoundWin( void );

private:

	bool m_bForceMapReset;
	bool m_bSwitchTeamsOnWin;
	int	 m_iWinReason;
	string_t m_strWinReason;

	COutputEvent m_outputOnRoundWin; // Fired when the entity tells the game rules a team has won the round

	DECLARE_DATADESC();
};

#endif // ENTITY_ROUND_WIN_H


