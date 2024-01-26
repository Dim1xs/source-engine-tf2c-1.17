//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tdc_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tdc_weapon_parse.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "tdc_gamerules.h"
#include "c_tdc_playerresource.h"

//--------------------------------------------------------------------------------------------------------------
extern CTDCWeaponInfo *GetTDCWeaponInfo( ETDCWeaponID iWeapon );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFExplosionCallback( const Vector &vecOrigin, const Vector &vecNormal, ETDCWeaponID iWeaponID, ClientEntityHandle_t hEntity, int iPlayerIndex, int iTeam, bool bCrit, ETDCDmgCustom iCustomDmg )
{
	// Get the weapon information.
	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( iWeaponID );
	Assert( pWeaponInfo );

	bool bIsPlayer = false;
	if ( hEntity.Get() )
	{
		C_BaseEntity *pEntity = C_BaseEntity::Instance( hEntity );
		if ( pEntity && pEntity->IsPlayer() )
		{
			bIsPlayer = true;
		}
	}

	// Calculate the angles, given the normal.
	bool bIsWater = ( UTIL_PointContents( vecOrigin ) & CONTENTS_WATER );
	bool bInAir = false;
	QAngle angExplosion( 0.0f, 0.0f, 0.0f );

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if ( fabs( vecNormal.x ) < 0.05f && fabs( vecNormal.y ) < 0.05f && fabs( vecNormal.z ) < 0.05f )
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles( vecNormal, angExplosion );
		bInAir = false;
	}

	// Cheating here but I don't want to add another bunch of keyvalues just for Displacer.
	if ( iCustomDmg == TDC_DMG_DISPLACER_BOMB )
	{
		bIsWater = true;
	}
	else if ( iWeaponID == WEAPON_DISPLACER )
	{
		bIsWater = false;
	}

	// Base explosion effect and sound.
	const char *pszFormat = "explosion";
	const char *pszSound = "BaseExplosionEffect.Sound";
	bool bColored = false;

	if ( pWeaponInfo )
	{
		bColored = pWeaponInfo->m_bHasTeamColoredExplosions;

		if ( bCrit )
		{
			if ( !pWeaponInfo->m_bHasCritExplosions )
			{
				// Not supporting crit explosions.
				bCrit = false;
			}
			else
			{
				bColored = true;
			}
		}

		// Explosions.
		if ( bIsWater )
		{
			pszFormat = pWeaponInfo->m_szExplosionEffects[TDC_EXPLOSION_WATER];
		}
		else
		{
			if ( bIsPlayer || bInAir )
			{
				pszFormat = pWeaponInfo->m_szExplosionEffects[TDC_EXPLOSION_AIR];
			}
			else
			{
				pszFormat = pWeaponInfo->m_szExplosionEffects[TDC_EXPLOSION_WALL];
			}
		}

		// Sound.
		if ( pWeaponInfo->m_szExplosionSound[0] != '\0' )
		{
			pszSound = pWeaponInfo->m_szExplosionSound;
		}

		if ( iCustomDmg == TDC_DMG_DISPLACER_BOMB )
		{
			// Cheating again.
			pszSound = pWeaponInfo->aShootSounds[SPECIAL1];
		}
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, pszSound, &vecOrigin );

	char szEffect[128];

	if ( bCrit )
	{
		// Make a team-colored particle.
		V_sprintf_safe( szEffect, "%s_%s_crit", pszFormat, GetTeamSuffix( iTeam, true ) );
	}
	else if ( bColored )
	{
		V_sprintf_safe( szEffect, "%s_%s", pszFormat, GetTeamSuffix( iTeam, true ) );
	}
	else
	{
		// Just take the name as it is.
		V_strcpy_safe( szEffect, pszFormat );
	}

	const Vector &vecColor = g_TDC_PR ? g_TDC_PR->GetPlayerColorVector( iPlayerIndex ) : vec3_origin;

	DispatchParticleEffect( szEffect, vecOrigin, angExplosion, vecColor, vec3_origin, bColored && !TDCGameRules()->IsTeamplay() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETDCExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETDCExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETDCExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	Vector			m_vecOrigin;
	Vector			m_vecNormal;
	ETDCWeaponID	m_iWeaponID;
	int				m_iPlayerIndex;
	int				m_iTeamNum;
	bool			m_bCritical;
	ETDCDmgCustom	m_iCustomDmg;
	ClientEntityHandle_t m_hEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETDCExplosion::C_TETDCExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = WEAPON_NONE;
	m_iPlayerIndex = 0;
	m_iTeamNum = TEAM_UNASSIGNED;
	m_bCritical = false;
	m_hEntity = INVALID_EHANDLE_INDEX;
	m_iCustomDmg = TDC_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETDCExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETDCExplosion::PostDataUpdate" );

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_iWeaponID, m_hEntity, m_iPlayerIndex, m_iTeamNum, m_bCritical, m_iCustomDmg );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	((C_TETDCExplosion*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE_INDEX : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETDCExplosion, DT_TETDCExplosion, CTETDCExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iTeamNum ) ),
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropInt( RECVINFO( m_iCustomDmg ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
END_RECV_TABLE()

