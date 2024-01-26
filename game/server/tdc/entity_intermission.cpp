//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC Intermission.
//
//=============================================================================//

#include "cbase.h"
#include "tdc_gamerules.h"
#include "entity_intermission.h"



LINK_ENTITY_TO_CLASS( point_intermission, CTDCIntermission );

BEGIN_DATADESC(CTDCIntermission)
	// Keys

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Activate", InputActivate ),

	// Outputs

END_DATADESC()

//=============================================================================
//
// CTDC Intermission functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CTDCIntermission::InputActivate( inputdata_t &inputdata )
{
	CTDCGameRules *pCTDCGameRules = TDCGameRules();
	if ( pCTDCGameRules )
	{
		pCTDCGameRules->GoToIntermission();
	}
}


