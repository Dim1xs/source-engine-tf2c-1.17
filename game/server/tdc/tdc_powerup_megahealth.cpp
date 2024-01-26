//=============================================================================//
//
// Purpose: FILL IN
//
//=============================================================================//
#include "cbase.h"
#include "tdc_powerup_megahealth.h"
#include "tdc_player.h"
#include "tdc_gamerules.h"
#include "tdc_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================

LINK_ENTITY_TO_CLASS( item_powerup_megahealth, CTDCPowerupMegaHealth );

//-----------------------------------------------------------------------------
// Purpose: Touch function
//-----------------------------------------------------------------------------
bool CTDCPowerupMegaHealth::MyTouch( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	if ( pTFPlayer && ValidTouch( pPlayer ) )
	{
		// Give 200% health
		pTFPlayer->TakeHealth( pTFPlayer->GetMaxHealth() * 2, HEAL_NOTIFY | HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
		pTFPlayer->m_Shared.HealNegativeConds();
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
