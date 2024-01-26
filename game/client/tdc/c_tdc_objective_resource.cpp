//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates objective data
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_tdc.h"
#include "c_tdc_objective_resource.h"
#include "tdc_gamerules.h"
#include "tdc_round_timer.h"
#include "tdc_announcer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TDCObjectiveResource, DT_TDCObjectiveResource, CTDCObjectiveResource )
	RecvPropInt( RECVINFO( m_iTimerToShowInHUD ) ),
	RecvPropInt( RECVINFO( m_iStopWatchTimer ) ),
	RecvPropInt( RECVINFO( m_iActiveMoneyZone ) ),
	RecvPropTime( RECVINFO( m_flMoneyZoneSwitchTime ) ),
	RecvPropTime( RECVINFO( m_flMoneyZoneDuration ) ),
END_RECV_TABLE()

C_TDCObjectiveResource *g_pObjectiveResource = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDCObjectiveResource::C_TDCObjectiveResource()
{
	g_pObjectiveResource = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDCObjectiveResource::~C_TDCObjectiveResource()
{
	g_pObjectiveResource = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TeamRoundTimer *C_TDCObjectiveResource::GetTimerToShowInHUD( void )
{
	// Certain game-created timers override map placed active timer.
	CTeamRoundTimer *pTimer = TDCGameRules()->GetWaitingForPlayersTimer();
	if ( pTimer )
		return pTimer;

	pTimer = TDCGameRules()->GetStalemateTimer();
	if ( pTimer )
		return pTimer;

	pTimer = TDCGameRules()->GetRoundTimer();
	if ( pTimer )
		return pTimer;

	pTimer = dynamic_cast<C_TeamRoundTimer *>( ClientEntityList().GetBaseEntity( m_iTimerToShowInHUD ) );
	if ( pTimer )
		return pTimer;

	// IMPORTANT: mp_timelimit timer has the lowest priority.
	return TDCGameRules()->GetTimeLimitTimer();
}
