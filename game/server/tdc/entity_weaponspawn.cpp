//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawning entity.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_gamerules.h"
#include "tdc_shareddefs.h"
#include "tdc_player.h"
#include "tdc_team.h"
#include "engine/IEngineSound.h"
#include "entity_weaponspawn.h"
#include "tdc_weaponbase.h"
#include "basecombatcharacter.h"
#include "in_buttons.h"
#include "tdc_fx.h"
#include "tdc_dropped_weapon.h"
#include "tdc_announcer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tdc_weapon_respawn_timer( "tdc_weapon_respawn_timer", "1", FCVAR_REPLICATED, "Show visual respawn timers for weapons in Deathmatch." );

IMPLEMENT_SERVERCLASS_ST( CWeaponSpawner, DT_WeaponSpawner )
	SendPropBool( SENDINFO( m_bStaticSpawner ) ),
	SendPropBool( SENDINFO( m_bOutlineDisabled ) ),
	SendPropBool( SENDINFO( m_bSpecialGlow ) ),
	SendPropStringT( SENDINFO( m_iszWeaponName ) )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponSpawner )
	DEFINE_KEYFIELD( m_iszWeaponName, FIELD_STRING, "weapon" ),
	DEFINE_KEYFIELD( m_flRespawnDelay, FIELD_FLOAT, "RespawnTime" ),
	DEFINE_KEYFIELD( m_flInitialSpawnDelay, FIELD_FLOAT, "InitialSpawnDelay" ),
	DEFINE_KEYFIELD( m_bStaticSpawner, FIELD_BOOLEAN, "StaticSpawner" ),
	DEFINE_KEYFIELD( m_bOutlineDisabled, FIELD_BOOLEAN, "DisableWeaponOutline" ),
	DEFINE_KEYFIELD( m_bSpecialGlow, FIELD_BOOLEAN, "SpecialGlow" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_weaponspawner, CWeaponSpawner );

CWeaponSpawner::CWeaponSpawner()
{
	m_flRespawnDelay = 10.0f;
	m_bEnableAnnouncements = false;
	m_pWeaponInfo = NULL;
	m_iWeaponID = WEAPON_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function 
//-----------------------------------------------------------------------------
void CWeaponSpawner::Spawn( void )
{
	const char *pszWeaponName = STRING( m_iszWeaponName.Get() );

	// Handle temp translations.
	if ( V_strcmp( pszWeaponName, "WEAPON_TOMMYGUN" ) == 0 )
	{
		pszWeaponName = "WEAPON_REMOTEBOMB";
	}
	else if ( V_strcmp( pszWeaponName, "WEAPON_SIXSHOOTER" ) == 0 )
	{
		pszWeaponName = "WEAPON_REVOLVER";
	}
	else if ( V_strcmp( pszWeaponName, "WEAPON_SNIPERRIFLE" ) == 0 )
	{
		pszWeaponName = "WEAPON_LEVERRIFLE";
	}

	m_iWeaponID = GetWeaponId( pszWeaponName );

	if ( m_iWeaponID == WEAPON_NONE )
	{
		Warning( "tf_weaponspawner has incorrect weapon name %s. DELETED\n", pszWeaponName );
		UTIL_Remove( this );
		return;
	}

	m_pWeaponInfo = GetTDCWeaponInfo( m_iWeaponID );
	Assert( m_pWeaponInfo );

	Precache();
	SetModel( m_pWeaponInfo->szWorldModel );

	if ( m_iWeaponID == WEAPON_DISPLACER )
	{
		m_Announcements.incoming = TDC_ANNOUNCER_DM_DISPLACER_INCOMING;
		m_Announcements.spawn = TDC_ANNOUNCER_DM_DISPLACER_SPAWN;
		m_Announcements.teampickup = TDC_ANNOUNCER_DM_DISPLACER_TEAMPICKUP;
		m_Announcements.enemypickup = TDC_ANNOUNCER_DM_DISPLACER_ENEMYPICKUP;
		m_bEnableAnnouncements = true;
	}
	else
	{
		m_bEnableAnnouncements = false;
	}

	BaseClass::Spawn();

	// Ensures consistent trigger bounds for all weapons. (danielmm8888)
	SetSolid( SOLID_BBOX );
	SetCollisionBounds( -Vector( 22, 22, 15 ), Vector( 22, 22, 15 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function 
//-----------------------------------------------------------------------------
void CWeaponSpawner::Precache( void )
{
	PrecacheModel( m_pWeaponInfo->szWorldModel );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWeaponSpawner::UpdateTransmitState( void )
{
	if ( m_bSpecialGlow )
		return SetTransmitState( FL_EDICT_ALWAYS );

	return BaseClass::UpdateTransmitState();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSpawner::HideOnPickedUp( void )
{
	RemoveEffects( EF_ITEM_BLINK );
	m_nRenderFX = kRenderFxDistort;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSpawner::UnhideOnRespawn( void )
{
	AddEffects( EF_ITEM_BLINK );
	m_nRenderFX = kRenderFxNone;
	EmitSound( "Item.Materialize" );

	if ( m_bEnableAnnouncements )
	{
		g_TFAnnouncer.Speak( m_Announcements.spawn );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSpawner::OnIncomingSpawn( void )
{
	if ( m_bEnableAnnouncements )
	{
		g_TFAnnouncer.Speak( m_Announcements.incoming );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponSpawner::ValidTouch( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	if ( pTFPlayer->m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TDC_COND_SPRINT ) )
		return false;

	return BaseClass::ValidTouch( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponSpawner::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	if ( ValidTouch( pTFPlayer ) && pTFPlayer->CanPickUpWeapon( this ) )
	{
		int iSlot = m_pWeaponInfo->iSlot;
		CTDCWeaponBase *pWeapon = (CTDCWeaponBase *)pTFPlayer->Weapon_GetSlot( iSlot );
		bool bWasLastWeapon = false;

		if ( pWeapon )
		{
			if ( pWeapon->IsWeapon( m_iWeaponID ) )
			{
				if ( pTFPlayer->GiveAmmo( pWeapon->GetInitialAmmo(), pWeapon->GetPrimaryAmmoType(), true, TDC_AMMO_SOURCE_AMMOPACK ) )
					bSuccess = true;
			}
			else if ( !pWeapon->CanHolster() )
			{
				pTFPlayer->m_Shared.SetDesiredWeaponIndex( WEAPON_NONE );
			}
			else if ( !( pTFPlayer->m_nButtons & IN_ATTACK ) && ( pTFPlayer->m_nButtons & IN_USE ) )
			{
				// Drop a usable weapon
				pTFPlayer->DropWeapon( pWeapon );
				bWasLastWeapon = ( pTFPlayer->GetLastWeapon() == pWeapon );
				pWeapon->UnEquip();
				pWeapon = NULL;
			}
			else
			{
				pTFPlayer->m_Shared.SetDesiredWeaponIndex( m_iWeaponID );
			}
		}

		if ( !pWeapon )
		{
			CTDCWeaponBase *pNewWeapon = dynamic_cast<CTDCWeaponBase *>( pTFPlayer->GiveNamedItem( m_pWeaponInfo->szClassName, 0, TDC_GIVEAMMO_INITIAL ) );

			if ( pNewWeapon )
			{
				if ( bWasLastWeapon && pPlayer->GetActiveWeapon() != pNewWeapon )
				{
					pPlayer->Weapon_SetLast( pNewWeapon );
				}

				pTFPlayer->OnPickedUpWeapon( this );
				bSuccess = true;
			}
		}

		if ( bSuccess )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			EmitSound( user, entindex(), "BaseCombatCharacter.AmmoPickup" );

			if ( m_bEnableAnnouncements )
			{
				if ( TDCGameRules()->IsTeamplay() )
				{
					for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
					{
						if ( i != pPlayer->GetTeamNumber() )
						{
							CTeamRecipientFilter filter( i, true );
							g_TFAnnouncer.Speak( filter, m_Announcements.enemypickup );
						}
						else
						{
							CTeamRecipientFilter filter( i, true );
							filter.RemoveRecipient( pPlayer );
							g_TFAnnouncer.Speak( filter, m_Announcements.teampickup );
						}
					}
				}
				else
				{
					CTeamRecipientFilter filter( FIRST_GAME_TEAM, true );
					filter.RemoveRecipient( pPlayer );
					g_TFAnnouncer.Speak( filter, m_Announcements.enemypickup );
				}
			}
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CWeaponSpawner::EndTouch( CBaseEntity *pOther )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pOther );

	if ( pTFPlayer )
	{
		int iCurrentWeaponID = pTFPlayer->m_Shared.GetDesiredWeaponIndex();
		if ( iCurrentWeaponID == m_iWeaponID )
		{
			pTFPlayer->m_Shared.SetDesiredWeaponIndex( WEAPON_NONE );
		}
	}
}
