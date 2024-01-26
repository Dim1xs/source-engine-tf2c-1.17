//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTDC Flag detection trigger.
//
//=============================================================================//
#ifndef FUNC_FLAGDETECTION_ZONE_H
#define FUNC_FLAGDETECTION_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "tdc_player.h"

//=============================================================================

class CFlagDetectionZone : public CBaseTrigger, public TAutoList<CFlagDetectionZone>
{
public:
	DECLARE_CLASS( CFlagDetectionZone, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void	Spawn( void );
	virtual void	StartTouch( CBaseEntity *pOther );
	virtual void	EndTouch( CBaseEntity *pOther );

	virtual void	SetDisabled( bool bDisabled );

	bool	EntityIsFlagCarrier( CBaseEntity *pEntity );

	void	FlagCaptured( CTDCPlayer *pPlayer );
	void	FlagDropped( CTDCPlayer *pPlayer );
	void	FlagPickedUp( CTDCPlayer *pPlayer );

	bool	IsDisabled( void ) { return m_bDisabled; };

	// Input handlers
	virtual void	InputEnable( inputdata_t &inputdata );
	virtual void	InputDisable( inputdata_t &inputdata );
	virtual void	InputTest( inputdata_t &inputdata );

private:
	bool	m_bDisabled;
	bool	m_bShouldAlarm;

	COutputEvent m_outOnStartTouchFlag; // Sent when a flag or flag carrier first touches the zone.
	COutputEvent m_outOnEndTouchFlag; // Sent when a flag or flag carrier stops touching the zone.
	COutputEvent m_outOnDroppedFlag; // Sent when a flag is dropped in the zone.
	COutputEvent m_outOnPickedUpFlag; // Sent when a flag is picked up in the zone.
};

void HandleFlagPickedUpInDetectionZone( CTDCPlayer *pPlayer );
void HandleFlagDroppedInDetectionZone( CTDCPlayer *pPlayer );
void HandleFlagCapturedInDetectionZone( CTDCPlayer *pPlayer );

#endif // FUNC_FLAGDETECTION_ZONE_H



