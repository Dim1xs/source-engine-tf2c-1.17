//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "entity_healthkit.h"
#include "tdc_shareddefs.h"
#include "tdc_player.h"
#include "tdc_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTDC HealthKit defines.
//

#define TDC_HEALTHKIT_MODEL			"models/items/healthkit.mdl"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );
LINK_ENTITY_TO_CLASS( item_healthkit_tiny, CHealthKitTiny );

//=============================================================================
//
// CTDC HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( "HealthKit.Touch" );
	PrecacheScriptSound( "OverhealPillRattle.Touch" );
	PrecacheScriptSound( "OverhealPillNoRattle.Touch" );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );
		Assert( pTFPlayer );

		int iHealthToAdd = GetHealthAmount();
		bool bTiny = iHealthToAdd < 30;
		int iHealthRestored = 0;

		// Overheal pellets, well, overheal.
		if ( bTiny )
		{
			iHealthRestored = pTFPlayer->TakeHealth( iHealthToAdd, HEAL_IGNORE_MAXHEALTH | HEAL_MAXBUFFCAP );
		}
		else
		{
			iHealthRestored = pTFPlayer->TakeHealth( iHealthToAdd, HEAL_NOTIFY );
		}

		if ( iHealthRestored )
			bSuccess = true;

		if ( !bTiny )
		{
			// Remove any negative conditions whether player got healed or not.
			if ( pTFPlayer->m_Shared.HealNegativeConds() )
				bSuccess = true;
		}

		if ( bSuccess )
		{
			const char *pszSound = "HealthKit.Touch";

			if ( bTiny )
			{
				if ( pPlayer->GetHealth() > pPlayer->GetMaxHealth() )
					pszSound = "OverhealPillRattle.Touch";
				else
					pszSound = "OverhealPillNoRattle.Touch";
			}

			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			EmitSound( user, entindex(), pszSound );
		}
	}

	return bSuccess;
}
