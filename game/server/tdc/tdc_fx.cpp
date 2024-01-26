//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//  
//
//=============================================================================

#include "cbase.h"
#include "basetempentity.h"
#include "tdc_fx.h"
#include "tdc_shareddefs.h"
#include "coordsize.h"
#include "tdc_player.h"
#include "te_effect_dispatch.h"

//=============================================================================
//
// Explosions.
//
class CTETDCExplosion : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTETDCExplosion, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTETDCExplosion( const char *name );

public:

	Vector m_vecOrigin;
	Vector m_vecNormal;
	ETDCWeaponID m_iWeaponID;
	int m_nEntIndex;
	int m_iPlayerIndex;
	int m_iTeamNum;
	bool m_bCritical;
	ETDCDmgCustom m_iCustomDmg;
};

// Singleton to fire explosion objects
static CTETDCExplosion g_TETDCExplosion( "TFExplosion" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETDCExplosion::CTETDCExplosion( const char *name ) : CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = WEAPON_NONE;
	m_nEntIndex = 0;
	m_iPlayerIndex = 0;
	m_iTeamNum = TEAM_UNASSIGNED;
	m_bCritical = false;
	m_iCustomDmg = TDC_DMG_CUSTOM_NONE;
}

IMPLEMENT_SERVERCLASS_ST( CTETDCExplosion, DT_TETDCExplosion )
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropVector( SENDINFO_NOCHECK( m_vecNormal ), 6, 0, -1.0f, 1.0f ),
	SendPropInt( SENDINFO_NOCHECK( m_iWeaponID ), Q_log2( WEAPON_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_iPlayerIndex ), Q_log2( MAX_PLAYERS ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_iTeamNum ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO_NOCHECK( m_bCritical ) ),
	SendPropInt( SENDINFO_NOCHECK( m_iCustomDmg ), Q_log2( TDC_DMG_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS ),
END_SEND_TABLE()

void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, ETDCWeaponID iWeaponID, int nEntIndex, CBasePlayer *pPlayer /*= NULL*/, int iTeam /*= TEAM_UNASSIGNED*/, bool bCrit /*= false*/, ETDCDmgCustom iCustomDmg /*= TDC_DMG_CUSTOM_NONE*/ )
{
	VectorCopy( vecOrigin, g_TETDCExplosion.m_vecOrigin );
	VectorCopy( vecNormal, g_TETDCExplosion.m_vecNormal );
	g_TETDCExplosion.m_iWeaponID	= iWeaponID;	
	g_TETDCExplosion.m_nEntIndex	= nEntIndex;
	g_TETDCExplosion.m_iPlayerIndex = pPlayer ? pPlayer->entindex() : 0;
	g_TETDCExplosion.m_iTeamNum = iTeam;
	g_TETDCExplosion.m_bCritical = bCrit;
	g_TETDCExplosion.m_iCustomDmg = iCustomDmg;

	// Send it over the wire
	g_TETDCExplosion.Create( filter, flDelay );
}

//=============================================================================
//
// TF ParticleEffect
//
class CTETDCParticleEffect : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTETDCParticleEffect, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTETDCParticleEffect( const char *name );

	void Init( void );

public:

	Vector m_vecOrigin;
	Vector m_vecStart;
	QAngle m_vecAngles;

	int m_iParticleSystemIndex;

	int m_nEntIndex;

	int m_iAttachType;
	int m_iAttachmentPointIndex;

	bool m_bResetParticles;
};

// Singleton to fire explosion objects
static CTETDCParticleEffect g_TETDCParticleEffect( "TFParticleEffect" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETDCParticleEffect::CTETDCParticleEffect( const char *name ) : CBaseTempEntity( name )
{
	Init();
}

void CTETDCParticleEffect::Init( void )
{
	m_vecOrigin.Init();
	m_vecStart.Init();
	m_vecAngles.Init();

	m_iParticleSystemIndex = 0;

	m_nEntIndex = -1;

	m_iAttachType = PATTACH_ABSORIGIN;
	m_iAttachmentPointIndex = 0;

	m_bResetParticles = false;
}


IMPLEMENT_SERVERCLASS_ST( CTETDCParticleEffect, DT_TETDCParticleEffect )
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecStart[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecStart[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecStart[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropQAngles( SENDINFO_NOCHECK( m_vecAngles ), 7 ),
	SendPropInt( SENDINFO_NOCHECK( m_iParticleSystemIndex ), 16, SPROP_UNSIGNED ),	// probably way too high
	SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS ),
	SendPropInt( SENDINFO_NOCHECK( m_iAttachType ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_iAttachmentPointIndex ), Q_log2(MAX_PATTACH_TYPES) + 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO_NOCHECK( m_bResetParticles ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity )
{
	int iAttachment = -1;
	if ( pEntity && pEntity->GetBaseAnimating() )
	{
		// Find the attachment point index
		iAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
		if ( iAttachment == -1 )
		{
			Warning("Model '%s' doesn't have attachment '%s' to attach particle system '%s' to.\n", STRING(pEntity->GetBaseAnimating()->GetModelName()), pszAttachmentName, pszParticleName );
			return;
		}
	}

	TE_TFParticleEffect( filter, flDelay, pszParticleName, iAttachType, pEntity, iAttachment, bResetAllParticlesOnEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Yet another overload, lets us supply vecStart
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity )
{
	int iIndex = GetParticleSystemIndex( pszParticleName );
	TE_TFParticleEffect( filter, flDelay, iIndex, vecOrigin, vecStart, vecAngles, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, int iAttachmentPoint, bool bResetAllParticlesOnEntity  )
{
	g_TETDCParticleEffect.Init();

	g_TETDCParticleEffect.m_iParticleSystemIndex = GetParticleSystemIndex( pszParticleName );
	if ( pEntity )
	{
		g_TETDCParticleEffect.m_nEntIndex = pEntity->entindex();
	}

	g_TETDCParticleEffect.m_iAttachType = iAttachType;
	g_TETDCParticleEffect.m_iAttachmentPointIndex = iAttachmentPoint;

	if ( bResetAllParticlesOnEntity )
	{
		g_TETDCParticleEffect.m_bResetParticles = true;
	}

	// Send it over the wire
	g_TETDCParticleEffect.Create( filter, flDelay );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity /*= NULL*/, int iAttachType /*= PATTACH_CUSTOMORIGIN*/ )
{
	g_TETDCParticleEffect.Init();

	g_TETDCParticleEffect.m_iParticleSystemIndex = GetParticleSystemIndex( pszParticleName );

	VectorCopy( vecOrigin, g_TETDCParticleEffect.m_vecOrigin );
	VectorCopy( vecAngles, g_TETDCParticleEffect.m_vecAngles );

	if ( pEntity )
	{
		g_TETDCParticleEffect.m_nEntIndex = pEntity->entindex();
		g_TETDCParticleEffect.m_iAttachType = iAttachType;
	}

	// Send it over the wire
	g_TETDCParticleEffect.Create( filter, flDelay );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_TFParticleEffect( IRecipientFilter &filter, float flDelay, int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity )
{
	g_TETDCParticleEffect.Init();

	g_TETDCParticleEffect.m_iParticleSystemIndex = iEffectIndex;

	VectorCopy( vecOrigin, g_TETDCParticleEffect.m_vecOrigin );
	VectorCopy( vecStart, g_TETDCParticleEffect.m_vecStart );
	VectorCopy( vecAngles, g_TETDCParticleEffect.m_vecAngles );

	if ( pEntity )
	{
		g_TETDCParticleEffect.m_nEntIndex = pEntity->entindex();
		g_TETDCParticleEffect.m_iAttachType = PATTACH_CUSTOMORIGIN;
	}

	// Send it over the wire
	g_TETDCParticleEffect.Create( filter, flDelay );
}
