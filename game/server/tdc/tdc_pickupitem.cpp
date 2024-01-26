//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_pickupitem.h"
#include "tdc_gamerules.h"
#include "particle_parse.h"

//=============================================================================
//
// CTDC Powerup tables.
//
IMPLEMENT_SERVERCLASS_ST( CTDCPickupItem, DT_TDCPickupItem )
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropBool( SENDINFO( m_bRespawning ) ),
	SendPropTime( SENDINFO( m_flRespawnStartTime ) ),
	SendPropTime( SENDINFO( m_flRespawnTime ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTDCPickupItem )

	// Keyfields.
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableWithEffect", InputEnableWithEffect ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableWithEffect", InputDisableWithEffect ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RespawnNow", InputRespawnNow ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnRespawn, "OnRespawn" ),
	DEFINE_OUTPUT( m_outputOn15SecBeforeRespawn, "On15SecBeforeRespawn" ),
	DEFINE_OUTPUT( m_outputOnTeam1Touch, "OnTeam1Touch" ),
	DEFINE_OUTPUT( m_outputOnTeam2Touch, "OnTeam2Touch" ),

	DEFINE_THINKFUNC( RespawnThink ),

END_DATADESC();

//=============================================================================
//
// CTDC Powerup functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCPickupItem::CTDCPickupItem()
{
	m_bDisabled = false;
	m_bRespawning = false;
	m_flOwnerPickupEnableTime = 0.0f;

	// 10 seconds respawn time by default. Override in derived classes.
	m_flRespawnDelay = 10.0f;
	m_flInitialSpawnDelay = 0.0f;
	m_bDropped = false;
	m_bFire15SecRemain = false;
	memset( m_bEnabledModes, 0, sizeof( m_bEnabledModes ) );

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::Precache( void )
{
	BaseClass::Precache();

	PrecacheParticleSystem( "ExplosionCore_buildings" );

	PrecacheMaterial( "vgui/flagtime_full" );
	PrecacheMaterial( "vgui/flagtime_empty" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::Spawn( void )
{
	BaseClass::Spawn();

	//SetOriginalSpawnOrigin( GetLocalOrigin() );
	//SetOriginalSpawnAngles( GetAbsAngles() );

	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );

	if ( ValidateForGameType( (ETDCGameType)TDCGameRules()->GetGameType() ) == true )
	{
		SetDisabled( m_bDisabled );

		ResetSequence( LookupSequence( "idle" ) );

		if ( m_flInitialSpawnDelay > 0.0f && !m_bDropped )
		{
			// Don't spawn immediately.
			Respawn();

			// Override the respawn time
			SetRespawnTime( m_flInitialSpawnDelay );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPickupItem::KeyValue( const char *szKeyName, const char *szValue )
{
	for ( int i = 1; i < TDC_GAMETYPE_COUNT; i++ )
	{
		if ( FStrEq( szKeyName, UTIL_VarArgs( "EnabledIn%s", g_aGameTypeInfo[i].name ) ) )
		{
			m_bEnabledModes[i] = !!atoi( szValue );
			return true;
		}
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity* CTDCPickupItem::Respawn( void )
{
	m_bRespawning = true;

	HideOnPickedUp();
	SetTouch( NULL );

	RemoveAllDecals(); //remove any decals

	// Set respawn time.
	SetRespawnTime( m_flRespawnDelay );
	m_flRespawnStartTime = gpGlobals->curtime;

	if ( !m_bDisabled )
	{
		SetContextThink( &CTDCPickupItem::RespawnThink, gpGlobals->curtime, "RespawnThinkContext" );
	}

	return this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::Materialize( void )
{
	m_bRespawning = false;
	UnhideOnRespawn();
	SetTouch( &CItem::ItemTouch );

	m_outputOnRespawn.FireOutput( this, this );
	SetContextThink( NULL, 0, "RespawnThinkContext" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::HideOnPickedUp( void )
{
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::UnhideOnRespawn( void )
{
	EmitSound( "Item.Materialize" );
	RemoveEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPickupItem::ValidTouch( CBasePlayer *pPlayer )
{
	if ( IsRespawning() )
	{
		return false;
	}

	// Is the item enabled?
	if ( IsDisabled() )
	{
		return false;
	}

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsAlive() )
	{
		return false;
	}

	// Team number and does it match?
	int iTeam = GetTeamNumber();
	if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPickupItem::MyTouch( CBasePlayer *pPlayer )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPickupItem::ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer )
{
	// Owner can't pick it up for some time after dropping it.
	if ( pPlayer == GetOwnerEntity() && gpGlobals->curtime < m_flOwnerPickupEnableTime )
		return false;

	return BaseClass::ItemCanBeTouchedByPlayer( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::SetRespawnTime( float flDelay )
{
	m_flRespawnTime = gpGlobals->curtime + flDelay;
	m_bFire15SecRemain = ( flDelay >= 15.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::RespawnThink( void )
{
	if ( m_bFire15SecRemain && m_flRespawnTime - gpGlobals->curtime <= 15.0f )
	{
		m_bFire15SecRemain = false;
		m_outputOn15SecBeforeRespawn.FireOutput( this, this );
		OnIncomingSpawn();
	}

	if ( gpGlobals->curtime >= m_flRespawnTime )
	{
		Materialize();
	}
	else
	{
		SetContextThink( &CTDCPickupItem::RespawnThink, gpGlobals->curtime, "RespawnThinkContext" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPickupItem::DropSingleInstance( const Vector &vecVelocity, CBaseCombatCharacter *pOwner, float flOwnerPickupDelay, float flRestTime, float flRemoveTime /*= 30.0f*/ )
{
	SetOwnerEntity( pOwner );
	AddSpawnFlags( SF_NORESPAWN );
	m_bDropped = true;
	DispatchSpawn( this );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetAbsVelocity( vecVelocity );
	SetSolid( SOLID_BBOX );

	if ( flRestTime != 0.0f )
		ActivateWhenAtRest( flRestTime );

	m_flOwnerPickupEnableTime = gpGlobals->curtime + flOwnerPickupDelay;

	// Remove after 30 seconds.
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + flRemoveTime, "PowerupRemoveThink" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::InputEnableWithEffect( inputdata_t &inputdata )
{
	DispatchParticleEffect( "ExplosionCore_buildings", GetAbsOrigin(), vec3_angle );
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::InputDisableWithEffect( inputdata_t &inputdata )
{
	DispatchParticleEffect( "ExplosionCore_buildings", GetAbsOrigin(), vec3_angle );
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPickupItem::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( m_bDisabled )
	{
		AddEffects( EF_NODRAW );
		SetContextThink( NULL, 0, "RespawnThinkContext" );
	}
	else
	{
		RemoveEffects( EF_NODRAW );
		
		if ( m_bRespawning )
		{
			HideOnPickedUp();
			m_bFire15SecRemain = ( m_flRespawnTime - gpGlobals->curtime >= 15.0f );
			SetContextThink( &CTDCPickupItem::RespawnThink, gpGlobals->curtime, "RespawnThinkContext" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::InputRespawnNow( inputdata_t &inputdata )
{
	if ( m_bRespawning )
	{
		Materialize();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::FireOutputsOnPickup( CBasePlayer *pPlayer )
{
	if ( TDCGameRules()->IsTeamplay() )
	{
		switch ( pPlayer->GetTeamNumber() )
		{
		case TDC_TEAM_RED:
			m_outputOnTeam1Touch.FireOutput( pPlayer, this );
			break;
		case TDC_TEAM_BLUE:
			m_outputOnTeam2Touch.FireOutput( pPlayer, this );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPickupItem::ValidateForGameType( ETDCGameType iType )
{
	if ( m_bDropped )
		return true;

	if ( iType && ( !m_bEnabledModes[iType] || TDCGameRules()->IsInstagib() ) )
	{
		UTIL_Remove( this );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPickupItem::UpdatePowerupsForGameType( ETDCGameType iType )
{
	// Run through all pickups and update their status based on the selected gamemode.
	for ( CTDCPickupItem *pPowerup : CTDCPickupItem::AutoList() )
	{
		pPowerup->ValidateForGameType( iType );
	}
}
