//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tdc_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTDCObjectiveResource, DT_TDCObjectiveResource )
	SendPropInt( SENDINFO( m_iTimerToShowInHUD ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iStopWatchTimer ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iActiveMoneyZone ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flMoneyZoneSwitchTime ) ),
	SendPropTime( SENDINFO( m_flMoneyZoneDuration ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTDCObjectiveResource )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tdc_objective_resource, CTDCObjectiveResource );

CTDCObjectiveResource *g_pObjectiveResource = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCObjectiveResource::CTDCObjectiveResource()
{
	g_pObjectiveResource = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCObjectiveResource::~CTDCObjectiveResource()
{
	Assert( g_pObjectiveResource == this );
	g_pObjectiveResource = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: The objective resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CTDCObjectiveResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
