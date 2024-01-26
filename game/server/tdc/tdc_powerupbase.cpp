//=============================================================================//
//
// Purpose: Base class for Deathmatch powerups 
//
//=============================================================================//

#include "cbase.h"
#include "items.h"
#include "tdc_gamerules.h"
#include "tdc_shareddefs.h"
#include "tdc_player.h"
#include "tdc_team.h"
#include "engine/IEngineSound.h"
#include "tdc_powerupbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tdc_powerup_respawn_timer( "tdc_powerup_respawn_timer", "1", FCVAR_REPLICATED, "Show visual respawn timers for power-ups in Deathmatch." );

//=============================================================================

BEGIN_DATADESC( CTDCPowerupBase )
	DEFINE_KEYFIELD( m_flRespawnDelay, FIELD_FLOAT, "RespawnTime" ),
	DEFINE_KEYFIELD( m_flInitialSpawnDelay, FIELD_FLOAT, "InitialSpawnDelay" ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTDCPowerupBase, DT_TDCPowerupBase )
END_SEND_TABLE()

//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor 
//-----------------------------------------------------------------------------
CTDCPowerupBase::CTDCPowerupBase()
{
	// Default duration to 30 seconds, adjust in base classes as necessary.
	m_flEffectDuration = 30.0f;

	// 2 minutes respawn time.
	m_flRespawnDelay = 120.0f;
}

CTDCPowerupBase *CTDCPowerupBase::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszClassname, float flDuration )
{
	CTDCPowerupBase *pPowerup = dynamic_cast<CTDCPowerupBase *>( CBaseEntity::CreateNoSpawn( pszClassname, vecOrigin, vecAngles, pOwner ) );

	if ( pPowerup )
	{
		pPowerup->SetEffectDuration( flDuration );
		pPowerup->AddSpawnFlags( SF_NORESPAWN );
		pPowerup->DropSingleInstance( vec3_origin, ToBaseCombatCharacter( pOwner ), 0.3f, 0.1f );
	}

	return pPowerup;
}

//-----------------------------------------------------------------------------
// Purpose: Precache 
//-----------------------------------------------------------------------------
void CTDCPowerupBase::Precache( void )
{
	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( GetPickupSound() );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function 
//-----------------------------------------------------------------------------
void CTDCPowerupBase::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );
	SetRenderMode( kRenderTransColor );

	BaseClass::Spawn();

	// Don't bounce.
	SetElasticity( 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPowerupBase::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPowerupBase::HideOnPickedUp( void )
{
	SetRenderColorA( 80 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPowerupBase::UnhideOnRespawn( void )
{
	SetRenderColorA( 255 );
	EmitSound( "Item.Materialize" );
	g_TFAnnouncer.Speak( GetSpawnAnnouncement() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPowerupBase::OnIncomingSpawn( void )
{
	g_TFAnnouncer.Speak( GetIncomingAnnouncement() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPowerupBase::ValidTouch( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	if ( !pTFPlayer || pTFPlayer->HasTheFlag() )
		return false;

	return BaseClass::ValidTouch( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Touch function
//-----------------------------------------------------------------------------
bool CTDCPowerupBase::MyTouch( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	if ( pTFPlayer && ValidTouch( pPlayer ) )
	{
		// Keep the higher duration
		if ( pTFPlayer->m_Shared.GetConditionDuration( GetCondition() ) < GetEffectDuration() )
		{
			// Add the condition and duration from derived classes
			pTFPlayer->m_Shared.AddCond( GetCondition(), GetEffectDuration() );
		}

		// Give full health
		if ( !m_bDropped )
		{
			pTFPlayer->TakeHealth( pTFPlayer->GetMaxHealth(), HEAL_NOTIFY );
			pTFPlayer->m_Shared.HealNegativeConds();
		}

		pTFPlayer->EmitSound( GetPickupSound() );

		if ( TDCGameRules()->IsTeamplay() )
		{
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				if ( i != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( i, true );
					g_TFAnnouncer.Speak( filter, GetEnemyPickupAnnouncement() );
				}
				else
				{
					CTeamRecipientFilter filter( i, true );
					filter.RemoveRecipient( pPlayer );
					g_TFAnnouncer.Speak( filter, GetTeamPickupAnnouncement() );
				}
			}
		}
		else
		{
			CTeamRecipientFilter filter( FIRST_GAME_TEAM, true );
			filter.RemoveRecipient( pPlayer );
			g_TFAnnouncer.Speak( filter, GetEnemyPickupAnnouncement() );
		}

		return true;
	}

	return false;
}
