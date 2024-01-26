//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "view.h"
#include "collisionutils.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/IEngineSound.h"
#include "tdc_weaponbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_drawtracers;
extern ConVar r_drawtracers_firstperson;

//-----------------------------------------------------------------------------
// Purpose: This is largely a copy of ParticleTracer except it colors tracers in FFA.
//-----------------------------------------------------------------------------
void TFParticleTracerCallback( const CEffectData &data )
{
	if ( !r_drawtracers.GetBool() )
		return;

	// Grab the data
	Vector vecStart = data.m_vStart;
	Vector vecEnd = data.m_vOrigin;
	C_BaseEntity *pEntity = data.GetEntity();

	// Move the starting point to an attachment if possible.
	if ( pEntity && !pEntity->IsDormant() )
	{
		// See if this is a weapon.
		if ( pEntity->IsBaseCombatWeapon() )
		{
			C_TDCWeaponBase *pWeapon = static_cast<C_TDCWeaponBase *>( pEntity );
			if ( pWeapon->UsingViewModel() && !r_drawtracers_firstperson.GetBool() )
				return;

			// Need to get trace position here since the server has no way of knowing
			// the correct one due to differing models.
			pWeapon->GetTracerOrigin( vecStart );
		}
		else if ( data.m_fFlags & TRACER_FLAG_USEATTACHMENT )
		{
			pEntity->GetAttachment( data.m_nAttachmentIndex, vecStart );
		}
	}

	// Create the particle effect
	QAngle vecAngles;
	Vector vecToEnd = vecEnd - vecStart;
	VectorNormalize( vecToEnd );
	VectorAngles( vecToEnd, vecAngles );

	CEffectData	newData;

	newData.m_nHitBox = data.m_nHitBox;
	newData.m_vOrigin = vecStart;
	newData.m_vStart = vecEnd;
	newData.m_vAngles = vecAngles;

	newData.m_bCustomColors = data.m_bCustomColors;
	newData.m_CustomColors.m_vecColor1 = data.m_CustomColors.m_vecColor1;

	DispatchEffect( "ParticleEffect", newData );

	if ( data.m_fFlags & TRACER_FLAG_WHIZ )
	{
		FX_TracerSound( vecStart, vecEnd, TRACER_TYPE_DEFAULT );
	}
}

DECLARE_CLIENT_EFFECT( "TFParticleTracer", TFParticleTracerCallback );
