//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TDC_OBJECTIVE_RESOURCE_H
#define C_TDC_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>

class C_TeamRoundTimer;

#define TEAM_ARRAY( index, team )		(index + (team * MAX_CONTROL_POINTS))

class C_TDCObjectiveResource : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TDCObjectiveResource, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TDCObjectiveResource();
	~C_TDCObjectiveResource();

	C_TeamRoundTimer *GetTimerToShowInHUD( void );
	int GetStopWatchTimer( void ) { return m_iStopWatchTimer; }

	int GetActiveMoneyZone( void ) { return m_iActiveMoneyZone; }
	float GetMoneyZoneSwitchTime( void ) { return m_flMoneyZoneSwitchTime; }
	float GetMoneyZoneDuration( void ) { return m_flMoneyZoneDuration; }

private:
	int		m_iTimerToShowInHUD;
	int		m_iStopWatchTimer;

	int		m_iActiveMoneyZone;
	float	m_flMoneyZoneSwitchTime;
	float	m_flMoneyZoneDuration;
};

extern C_TDCObjectiveResource *g_pObjectiveResource;

inline C_TDCObjectiveResource *ObjectiveResource()
{
	return g_pObjectiveResource;
}

inline C_TDCObjectiveResource *TFObjectiveResource()
{
	return g_pObjectiveResource;
}

#endif // C_TDC_OBJECTIVE_RESOURCE_H
