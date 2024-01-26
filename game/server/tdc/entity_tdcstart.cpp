//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tdc_shareddefs.h"
#include "entity_tdcstart.h"
#include "tdc_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTDCPlayerSpawn tables.
//
BEGIN_DATADESC( CTDCPlayerSpawn )

	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs.

END_DATADESC()

LINK_ENTITY_TO_CLASS( info_player_teamspawn, CTDCPlayerSpawn );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTDCPlayerSpawn::CTDCPlayerSpawn()
{
	m_bDisabled = false;
	memset( m_bEnabledModes, 0, sizeof( m_bEnabledModes ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerSpawn::KeyValue( const char *szKeyName, const char *szValue )
{
	for ( int i = 1; i < TDC_GAMETYPE_COUNT; i++ )
	{
		if ( FStrEq( szKeyName, UTIL_VarArgs( "EnabledIn%s", g_aGameTypeInfo[i].name ) ) )
		{
			m_bEnabledModes[i] = !!atoi( szValue );
			return true;
		}
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerSpawn::Activate( void )
{
	BaseClass::Activate();

	trace_t trace;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	if ( trace.DidHit() )
	{
		Warning("Spawnpoint at (%.2f %.2f %.2f) is not clear.\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		// m_debugOverlays |= OVERLAY_TEXT_BIT;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerSpawn::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerSpawn::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTDCPlayerSpawn::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		V_sprintf_safe(tempstr,"TeamNumber: %d", GetTeamNumber() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color32 teamcolor = g_aTeamColors[ GetTeamNumber() ];
		teamcolor.a = 0;

		if ( m_bDisabled )
		{
			V_sprintf_safe(tempstr,"DISABLED" );
			EntityText(text_offset,tempstr,0);
			text_offset++;

			teamcolor.a = 255;
		}

		// Make sure it's empty
		Vector vTestMins = GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = GetAbsOrigin() + VEC_HULL_MAX;

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( NULL, vTestMins, vTestMaxs ) )
		{
			NDebugOverlay::Box( GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, teamcolor.r, teamcolor.g, teamcolor.b, teamcolor.a, 0.1 );
		}
		else
		{
			NDebugOverlay::Box( GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 0, 0.1 );
		}
	}
	return text_offset;
}
