//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: Triggers an intermission
//
//=============================================================================//
#ifndef ENTITY_INTERMISSION_H
#define ENTITY_INTERMISSION_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTDC Intermission class.
//

class CTDCIntermission : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTDCIntermission, CLogicalEntity );

	void InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

#endif // ENTITY_INTERMISSION_H

