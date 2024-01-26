//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Dropped DM weapon
//
//=============================================================================//
#include "cbase.h"
#include "tdc_dropped_weapon.h"
#include "tdc_gamerules.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST( CTDCDroppedWeapon, DT_TDCDroppedWeapon )
	SendPropInt( SENDINFO( m_iAmmo ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iMaxAmmo ), 10, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bDissolving ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( item_dropped_weapon, CTDCDroppedWeapon );

CTDCDroppedWeapon::CTDCDroppedWeapon()
{
	m_iWeaponID = WEAPON_NONE;
	m_iClip = 0;
	m_iMaxAmmo = 0;
	m_flCreationTime = 0.0f;
	m_flRemoveTime = 0.0f;
	m_pWeaponInfo = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function 
//-----------------------------------------------------------------------------
void CTDCDroppedWeapon::Spawn( void )
{
	m_pWeaponInfo = GetTDCWeaponInfo( m_iWeaponID );
	Assert( m_pWeaponInfo );

	SetModel( STRING( GetModelName() ) );
	AddSpawnFlags( SF_NORESPAWN );

	BaseClass::Spawn();

	if ( VPhysicsGetObject() )
	{
		// All weapons must have same weight.
		VPhysicsGetObject()->SetMass( 25.0f );
	}

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	m_flCreationTime = gpGlobals->curtime;

	// Dropped weapons stay indefinitely in Infection.
	if ( ShouldDespawn() )
	{
		// Remove 30s after spawning
		if ( m_iWeaponID == WEAPON_PISTOL )
		{
			m_flRemoveTime = gpGlobals->curtime + 8.0f;
		}
		else
		{
			m_flRemoveTime = gpGlobals->curtime + 30.0f;
		}

		SetThink( &CTDCDroppedWeapon::RemovalThink );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTDCDroppedWeapon::PhysicsSolidMaskForEntity( void ) const
{
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCDroppedWeapon::RemovalThink( void )
{
	if ( gpGlobals->curtime >= m_flRemoveTime )
		UTIL_Remove( this );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCDroppedWeapon::ShouldDespawn( void )
{
	return ( !TDCGameRules()->IsInfectionMode() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCDroppedWeapon *CTDCDroppedWeapon::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CTDCWeaponBase *pWeapon, bool bDissolve )
{
	CTDCDroppedWeapon *pDroppedWeapon = static_cast<CTDCDroppedWeapon *>( CBaseAnimating::CreateNoSpawn( "item_dropped_weapon", vecOrigin, vecAngles, pOwner ) );
	if ( pDroppedWeapon )
	{
		pDroppedWeapon->SetModelName( AllocPooledString( pWeapon->GetWorldModel() ) );
		pDroppedWeapon->SetWeaponID( pWeapon->GetWeaponID() );

		DispatchSpawn( pDroppedWeapon );

		if ( bDissolve )
		{
			pDroppedWeapon->Dissolve( NULL, gpGlobals->curtime, false );
			pDroppedWeapon->SetTouch( NULL );
			pDroppedWeapon->m_bDissolving = true;
		}
	}

	return pDroppedWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCDroppedWeapon::ValidTouch( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return false;
	}

	// Dropper can't pick us up for 3 seconds.
	if ( pPlayer == GetOwnerEntity() && gpGlobals->curtime - m_flCreationTime < 2.0f )
	{
		return false;
	}

	if ( pTFPlayer->m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TDC_COND_SPRINT ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCDroppedWeapon::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	if ( ValidTouch( pTFPlayer ) && pTFPlayer->IsNormalClass() && pTFPlayer->CanPickUpWeapon( this ) )
	{
		// Don't remove weapon while a player is standing over it.
		SetThink( NULL );

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
			CTDCWeaponBase *pNewWeapon = dynamic_cast<CTDCWeaponBase *>( pTFPlayer->GiveNamedItem( m_pWeaponInfo->szClassName, 0, m_iAmmo ) );

			if ( pNewWeapon )
			{
				// If this is the same guy who dropped it restore old clip size to avoid exploiting swapping
				// weapons for faster reload.
				if ( pPlayer == GetOwnerEntity() )
				{
					pNewWeapon->m_iClip1 = m_iClip;
				}

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
		}
	}

	return bSuccess;
}

void CTDCDroppedWeapon::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	CTDCPlayer *pTFPlayer = ToTDCPlayer( pOther );

	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.SetDesiredWeaponIndex( WEAPON_NONE );

		if ( m_flRemoveTime != 0.0f )
		{
			SetThink( &CTDCDroppedWeapon::RemovalThink );
			// Don't remove weapon immediately after player stopped touching it.
			SetNextThink( gpGlobals->curtime + 3.5f );
		}
	}
}
