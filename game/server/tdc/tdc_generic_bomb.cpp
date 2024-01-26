//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tdc_generic_bomb.h"
#include "tdc_fx.h"
#include "props.h"
#include "te_effect_dispatch.h"
#include "tdc_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( prop_generic_bomb, CTDCGenericBomb );

BEGIN_DATADESC( CTDCGenericBomb )
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_iszParticleName, FIELD_STRING, "explode_particle" ),
	DEFINE_KEYFIELD( m_iszExplodeSound, FIELD_SOUNDNAME, "sound" ),
	DEFINE_KEYFIELD( m_bFriendlyFire, FIELD_INTEGER, "friendlyfire" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Detonate", InputDetonate ),
	DEFINE_OUTPUT( m_OnDetonate, "OnDetonate" ),
END_DATADESC()


CTDCGenericBomb::CTDCGenericBomb()
{
	SetMaxHealth( 1 );
	SetHealth( 1 );
	m_flDamage = 50.0f;
	m_flRadius = 100.0f;
	m_bFriendlyFire = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGenericBomb::Precache()
{
	BaseClass::Precache();

	PropBreakablePrecacheAll( GetModelName() );

	if ( m_iszParticleName != NULL_STRING )
		PrecacheParticleSystem( STRING( m_iszParticleName ) );

	if ( m_iszExplodeSound != NULL_STRING )
		PrecacheScriptSound( STRING( m_iszExplodeSound ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGenericBomb::Spawn()
{
	Precache();

	SetModel( STRING( GetModelName() ) );

	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );

	BaseClass::Spawn();

	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCGenericBomb::OnTakeDamage( const CTakeDamageInfo &info )
{
	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGenericBomb::Event_Killed( const CTakeDamageInfo &info )
{
	trace_t	tr;
	Vector vecEnd = GetAbsOrigin() - Vector( 0, 0, 8 );
	UTIL_TraceLine( GetAbsOrigin(), vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

#if 0
	int iAttachment = LookupAttachment("alt-origin");

	if ( iAttachment > 0 )
		GetAttachment( iAttachment, absOrigin, absAngles );
#endif

	CPVSFilter filter( GetAbsOrigin() );

	if ( STRING( m_iszParticleName ) )
		TE_TFParticleEffect( filter, 0.0, STRING( m_iszParticleName ), WorldSpaceCenter(), GetAbsAngles() );

	if ( STRING( m_iszExplodeSound ) )
		EmitSound( STRING( m_iszExplodeSound ) );

	SetSolid( SOLID_NONE );
	m_takedamage = DAMAGE_NO;

	// If destroyed by a player make sure he's the attacker when it explodes.
	CBaseEntity *pAttacker = this;

	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
		pAttacker = info.GetAttacker();

	CTakeDamageInfo info_modified( this, pAttacker, m_flDamage, DMG_BLAST | DMG_HALF_FALLOFF );

	if ( m_bFriendlyFire )
		info_modified.SetForceFriendlyFire( true );

	RadiusDamage( info_modified, WorldSpaceCenter(), m_flRadius, CLASS_NONE, this );

	// Scorch the ground below it.
	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
		UTIL_DecalTrace( &tr, "Scorch" );

	CEffectData data;
	data.m_vOrigin = GetAbsOrigin();
	data.m_vAngles = GetAbsAngles();
	data.m_nMaterial = GetModelIndex();
	
	DispatchEffect( "BreakModel", data );

	m_OnDetonate.FireOutput( this, this );

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGenericBomb::InputDetonate( inputdata_t &inputdata )
{
	CTakeDamageInfo info( this, this, GetHealth(), DMG_GENERIC );
	Event_Killed( info );
}
