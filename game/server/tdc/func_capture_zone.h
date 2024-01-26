//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTDC Flag Capture Zone.
//
//=============================================================================//
#ifndef FUNC_CAPTURE_ZONE_H
#define FUNC_CAPTURE_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CCaptureFlag;

//=============================================================================
//
// CTDC Flag Capture Zone class.
//
class CCaptureZone : public CBaseTrigger, public TAutoList<CCaptureZone>
{
public:
	DECLARE_CLASS( CCaptureZone, CBaseTrigger );
	DECLARE_SERVERCLASS();

	CCaptureZone();

	void	Spawn( void );
	void	Activate( void );
	void	Touch( CBaseEntity *pOther );

	int		UpdateTransmitState( void );

private:
	int				m_nCapturePoint;	// Used in non-CTDC maps to identify this capture point
	bool			m_bWinOnCapture;
	bool			m_bForceMapReset;
	bool			m_bSwitchTeamsOnWin;

	string_t		m_strAssociatedFlag;
	CHandle<CCaptureFlag> m_hAssociatedFlag;

	COutputEvent	m_outputOnCapture;	// Fired a flag is captured on this point.
	COutputEvent	m_outputOnCapTeam1;
	COutputEvent	m_outputOnCapTeam2;

	DECLARE_DATADESC();

	float			m_flNextTouchingEnemyZoneWarning;	// don't spew warnings to the player who is touching the wrong cap
};

#endif // FUNC_CAPTURE_ZONE_H
