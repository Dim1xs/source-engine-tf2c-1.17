//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tdc_shareddefs.h"
#include "tdc_weapon_parse.h"

extern CTDCWeaponInfo *GetTDCWeaponInfo( ETDCWeaponID iWeapon );

void EjectBrass( const CEffectData &data, const char *szBrassModel, bool loaded )
{
	Vector vForward, vRight, vUp;
	AngleVectors( data.m_vAngles, &vForward, &vRight, &vUp );

	QAngle vecShellAngles;
	VectorAngles( -vUp, vecShellAngles );
	
	Vector vecVelocity = random->RandomFloat( 130, 180 ) * vForward +
						 random->RandomFloat( -30, 30 ) * vRight +
						 random->RandomFloat( -30, 30 ) * vUp;

	float flLifeTime = 10.0f;

	const model_t *pModel = engine->LoadModel( szBrassModel );
	if ( !pModel )
		return;

	int flags = FTENT_FADEOUT | FTENT_GRAVITY | FTENT_COLLIDEALL | FTENT_HITSOUND | FTENT_ROTATE;

	vecVelocity += data.m_vStart;

	Assert( pModel );	

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, data.m_vOrigin, vecShellAngles, vecVelocity, flLifeTime, FTENT_NEVERDIE );
	if ( pTemp == NULL )
		return;

	pTemp->m_vecTempEntAngVelocity[0] = random->RandomFloat( -512, 511 );
	pTemp->m_vecTempEntAngVelocity[1] = random->RandomFloat( -255, 255 );
	pTemp->m_vecTempEntAngVelocity[2] = random->RandomFloat( -255, 255 );

	pTemp->hitSound = ( data.m_nDamageType & DMG_BUCKSHOT ) ? BOUNCE_SHOTSHELL : BOUNCE_SHELL;

	pTemp->SetGravity( 0.4 );

	pTemp->m_flSpriteScale = 10;

	pTemp->flags = flags;

	if (loaded)
	{
		pTemp->SetBodygroup( 0, 1 );
	}

	// ::ShouldCollide decides what this collides with
	pTemp->flags |= FTENT_COLLISIONGROUP;
	pTemp->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
}

//-----------------------------------------------------------------------------
// Purpose: TF Eject Brass
//-----------------------------------------------------------------------------
void TDC_EjectBrassCallback( const CEffectData &data )
{
	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( (ETDCWeaponID)data.m_nHitBox );
	if ( !pWeaponInfo )
		return;
	if ( !pWeaponInfo->m_szBrassModel[0] )
		return;

	EjectBrass( data, pWeaponInfo->m_szBrassModel, false );
}

DECLARE_CLIENT_EFFECT( "TDC_EjectBrass", TDC_EjectBrassCallback );

//-----------------------------------------------------------------------------
// Purpose: TF Eject Brass
//-----------------------------------------------------------------------------
void TDC_DumpBrassCallback( const CEffectData &data )
{
	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( (ETDCWeaponID)data.m_nHitBox );
	if ( !pWeaponInfo )
		return;
	if ( !pWeaponInfo->m_szBrassModel[0] )
		return;

	int iLoaded = data.m_nAttachmentIndex;
	for ( int i = 0; i < pWeaponInfo->iMaxClip1; i++ )
	{
		EjectBrass( data, pWeaponInfo->m_szBrassModel, i < iLoaded );
	}
}

DECLARE_CLIENT_EFFECT( "TDC_DumpBrass", TDC_DumpBrassCallback );