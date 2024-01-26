//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef FUNC_FLAG_ALERT_H
#define FUNC_FLAG_ALERT_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "tdc_shareddefs.h"

class CFuncFlagAlertZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFuncFlagAlertZone, CBaseTrigger );
	DECLARE_DATADESC();

	CFuncFlagAlertZone();

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );

private:
	bool m_bPlaySound;
	float m_flNextAlertTime[TDC_TEAM_COUNT];

	COutputEvent m_OnTriggeredByTeam1;
	COutputEvent m_OnTriggeredByTeam2;
};

#endif // FUNC_FLAG_ALERT_H
