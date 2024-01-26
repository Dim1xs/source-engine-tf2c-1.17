//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC Spawn Point.
//
//=============================================================================//
#ifndef ENTITY_TFSTART_H
#define ENTITY_TFSTART_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// TF team spawning entity.
//

class CTDCPlayerSpawn : public CLogicalEntity, public TAutoList<CTDCPlayerSpawn>
{
public:
	DECLARE_CLASS( CTDCPlayerSpawn, CLogicalEntity );

	CTDCPlayerSpawn();

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void Activate( void );

	bool IsDisabled( void ) { return m_bDisabled; }
	void SetDisabled( bool bDisabled ) { m_bDisabled = bDisabled; }

	bool IsEnabledForMode( int iType ) { return m_bEnabledModes[iType]; }

	// Inputs/Outputs.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	int DrawDebugTextOverlays( void );

private:
	bool	m_bDisabled;		// Enabled/Disabled?
	bool m_bEnabledModes[TDC_GAMETYPE_COUNT];

	DECLARE_DATADESC();
};

#endif // ENTITY_TFSTART_H


