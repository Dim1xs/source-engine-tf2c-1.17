//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tdc_fx_muzzleflash.h"
#include "c_te_effect_dispatch.h"
#include "tdc_weaponbase.h"
#include "iclientmode.h"

extern ConVar r_drawviewmodel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TDC_MuzzleFlashCallback( const CEffectData &data )
{	
	C_TDCWeaponBase *pWeapon = dynamic_cast<C_TDCWeaponBase *>( data.GetEntity() );
	if ( !pWeapon || pWeapon->IsDormant() )
		return;

	// Don't draw muzzleflashes if the viewmodel is not drawn.
	if ( pWeapon->UsingViewModel() && ( !g_pClientMode->ShouldDrawViewModel() || !r_drawviewmodel.GetBool() ) )
		return;

	pWeapon->CreateMuzzleFlashEffects( pWeapon->GetWeaponForEffect() );
}

void TDC_3rdPersonMuzzleFlashCallback_SentryGun( const CEffectData &data )
{
	int iMuzzleFlashAttachment = data.m_nAttachmentIndex;
	int iUpgradeLevel	= data.m_fFlags;

	C_BaseEntity *pEnt = data.GetEntity();
	if ( pEnt && !pEnt->IsDormant() )
	{
		// The created entity kills itself
		//C_MuzzleFlashModel::CreateMuzzleFlashModel( "models/effects/sentry1_muzzle/sentry1_muzzle.mdl", pEnt, iMuzzleFlashAttachment );

		char *pszMuzzleFlashParticleEffect = NULL;
		switch( iUpgradeLevel )
		{
		case 1:
		default:
			pszMuzzleFlashParticleEffect = "muzzle_sentry";
			break;
		case 2:
		case 3:
			pszMuzzleFlashParticleEffect = "muzzle_sentry2";
			break;
		}

		pEnt->ParticleProp()->Create( pszMuzzleFlashParticleEffect, PATTACH_POINT_FOLLOW, iMuzzleFlashAttachment );
	}
}

DECLARE_CLIENT_EFFECT( "TDC_MuzzleFlash", TDC_MuzzleFlashCallback );
DECLARE_CLIENT_EFFECT( "TDC_3rdPersonMuzzleFlash_SentryGun", TDC_3rdPersonMuzzleFlashCallback_SentryGun );


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszModelName - 
//			vecOrigin - 
//			vecForceDir - 
//			vecAngularImp - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
C_MuzzleFlashModel *C_MuzzleFlashModel::CreateMuzzleFlashModel( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime )
{
	C_MuzzleFlashModel *pFlash = new C_MuzzleFlashModel;
	if ( !pFlash )
		return NULL;

	if ( !pFlash->InitializeMuzzleFlash( pszModelName, pParent, iAttachment, flLifetime ) )
		return NULL;

	return pFlash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_MuzzleFlashModel::InitializeMuzzleFlash( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime )
{
	AddEffects( EF_NORECEIVESHADOW | EF_NOSHADOW );
	if ( InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		Release();
		return false;
	}

	SetParent( pParent, iAttachment );
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );

	AddSolidFlags( FSOLID_NOT_SOLID );

	m_flRotateAt = gpGlobals->curtime + 0.2;
	SetLifetime( flLifetime );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	SetCycle( 0 );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MuzzleFlashModel::SetLifetime( float flLifetime )
{
	// Expire when the lifetime is up
	m_flExpiresAt = gpGlobals->curtime + flLifetime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MuzzleFlashModel::ClientThink( void )
{
	if ( !GetMoveParent() || gpGlobals->curtime > m_flExpiresAt )
	{
		Release();
		return;
	}

	if ( gpGlobals->curtime > m_flRotateAt )
	{
		// Pick a new anim frame
		float flDelta = RandomFloat(0.2,0.4) * (RandomInt(0,1) == 1 ? 1 : -1);
		float flCycle = clamp( GetCycle() + flDelta, 0, 1 );
		SetCycle( flCycle );

		SetLocalAngles( QAngle(0,0,RandomFloat(0,360)) );
		m_flRotateAt = gpGlobals->curtime + 0.075;
	}
}
