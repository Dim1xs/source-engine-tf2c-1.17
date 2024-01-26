//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: DOD's objective resource, transmits all objective states to players
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_OBJECTIVE_RESOURCE_H
#define TDC_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"

#define TEAM_ARRAY( index, team )		(index + (team * MAX_CONTROL_POINTS))

class CTDCObjectiveResource : public CBaseEntity
{
public:
	DECLARE_CLASS( CTDCObjectiveResource, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTDCObjectiveResource();
	~CTDCObjectiveResource();

	virtual int UpdateTransmitState( void );

	void SetTimerInHUD( CBaseEntity *pTimer )
	{
		m_iTimerToShowInHUD = pTimer ? pTimer->entindex() : 0;
	}

	void SetStopWatchTimer( CBaseEntity *pTimer )
	{
		m_iStopWatchTimer = pTimer ? pTimer->entindex() : 0;
	}

	int GetTimerInHUD( void ) { return m_iTimerToShowInHUD; }
	bool IsActiveTimer( CBaseEntity *pTimer ) { return ( pTimer->entindex() == m_iTimerToShowInHUD ); }

	void SetActiveMoneyZone( int iEnt ) { m_iActiveMoneyZone = iEnt; }
	void SetMoneyZoneSwitchTime( float flTime, float flDuration ) { m_flMoneyZoneSwitchTime = flTime; m_flMoneyZoneDuration = flDuration; }

private:
	CNetworkVar( int, m_iTimerToShowInHUD );
	CNetworkVar( int, m_iStopWatchTimer );

	CNetworkVar( int, m_iActiveMoneyZone );
	CNetworkVar( float, m_flMoneyZoneSwitchTime );
	CNetworkVar( float, m_flMoneyZoneDuration );
};

extern CTDCObjectiveResource *g_pObjectiveResource;

inline CTDCObjectiveResource *ObjectiveResource()
{
	return g_pObjectiveResource;
}

inline CTDCObjectiveResource *TFObjectiveResource( void )
{
	return g_pObjectiveResource;
}

#endif	// TDC_OBJECTIVE_RESOURCE_H

