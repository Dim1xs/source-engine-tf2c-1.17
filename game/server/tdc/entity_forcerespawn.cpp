//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tdc_shareddefs.h"
#include "entity_forcerespawn.h"
#include "tdc_player.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTDCReset tables.
//
BEGIN_DATADESC( CForceRespawn )
	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRespawn", InputForceRespawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceRespawnSwitchTeams", InputForceRespawnSwitchTeams ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ForceTeamRespawn", InputForceTeamRespawn ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnForceRespawn, "OnForceRespawn" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_forcerespawn, CForceRespawn );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CForceRespawn::CForceRespawn()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CForceRespawn::ForceRespawn( bool bSwitchTeams, int iTeam /*= TEAM_UNASSIGNED*/, bool bRemoveOwnedEnts /*= true*/ )
{
	if ( bSwitchTeams )
	{
		TDCGameRules()->HandleSwitchTeams();
	}

	// respawn the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			continue;

		if ( iTeam && pPlayer->GetTeamNumber() != iTeam )
			continue;

		// Ignore players who haven't picked a class yet
		if ( pPlayer->GetPlayerClass()->GetClassIndex() == TDC_CLASS_UNDEFINED )
		{
			// Allow them to spawn instantly when they do choose
			pPlayer->AllowInstantSpawn();
			continue;
		}

		if ( bRemoveOwnedEnts )
		{
			pPlayer->RemoveAllOwnedEntitiesFromWorld();
		}

		pPlayer->ForceRespawn();
	}

	// Output.
	m_outputOnForceRespawn.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CForceRespawn::InputForceRespawn( inputdata_t &inputdata )
{
	ForceRespawn( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CForceRespawn::InputForceRespawnSwitchTeams( inputdata_t &inputdata )
{
	ForceRespawn( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CForceRespawn::InputForceTeamRespawn( inputdata_t &inputdata )
{
	ForceRespawn( false, inputdata.value.Int(), false );
}
