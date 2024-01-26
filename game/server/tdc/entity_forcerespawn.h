//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC Reset Entity (resets the teams).
//
//=============================================================================//
#ifndef ENTITY_FORCERESPAWN_H
#define ENTITY_FORCERESPAWN_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTDC Force Respawn Entity.
//

class CForceRespawn : public CLogicalEntity
{
public:
	DECLARE_CLASS( CForceRespawn, CLogicalEntity );

	CForceRespawn();
	void Reset( void );

	void ForceRespawn( bool bSwitchTeams, int iTeam = TEAM_UNASSIGNED, bool bRemoveOwnedEnts = true );

	// Input.
	void InputForceRespawn( inputdata_t &inputdata );
	void InputForceRespawnSwitchTeams( inputdata_t &inputdata );
	void InputForceTeamRespawn( inputdata_t &inputdata );

private:

	COutputEvent	m_outputOnForceRespawn;	// Fired when the entity is done respawning the players.

	DECLARE_DATADESC();
};

#endif // ENTITY_FORCERESPAWN_H


