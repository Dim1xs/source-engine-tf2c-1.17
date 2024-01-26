//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tdc_fx_shared.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include <cliententitylist.h>
#include "c_te_effect_dispatch.h"
#include "props_shared.h"

//-----------------------------------------------------------------------------
// Purpose: Live TF2 uses BreakModel user message but screw that.
//-----------------------------------------------------------------------------
void BreakModelCallback( const CEffectData &data )
{
	int nModelIndex = data.m_nMaterial;

	CUtlVector<breakmodel_t> aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( aGibs.IsEmpty() )
		return;

	Vector vecVelocity( 0, 0, 200 );
	AngularImpulse angularVelocity( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

	breakablepropparams_t params( data.m_vOrigin, data.m_vAngles, vecVelocity, angularVelocity );
	CreateGibsFromList( aGibs, nModelIndex, NULL, params, NULL, -1, false );
}

DECLARE_CLIENT_EFFECT( "BreakModel", BreakModelCallback );
