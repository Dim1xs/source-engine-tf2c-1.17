//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "entity_ammopack.h"
#include "tdc_shareddefs.h"
#include "tdc_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTDC AmmoPack defines.
//

LINK_ENTITY_TO_CLASS( item_ammopack_full, CAmmoPack );
LINK_ENTITY_TO_CLASS( item_ammopack_small, CAmmoPackSmall );
LINK_ENTITY_TO_CLASS( item_ammopack_medium, CAmmoPackMedium );

//=============================================================================
//
// CTDC AmmoPack functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TDC_AMMOPACK_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CAmmoPack::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		for ( int iAmmo = TDC_AMMO_PRIMARY; iAmmo < TDC_AMMO_GRENADES1; iAmmo++ )
		{
			// Full ammo packs re-fill all ammo and account for fired clip.
			int iMax = pTFPlayer->GetMaxAmmo( iAmmo, GetAmmoPerc() == 1.0f );

			if ( pTFPlayer->GiveAmmo( ceil( iMax * GetAmmoPerc() ), iAmmo, true, TDC_AMMO_SOURCE_AMMOPACK ) )
			{
				bSuccess = true;
			}
		}

		// did we give them anything?
		if ( bSuccess )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), TDC_AMMOPACK_PICKUP_SOUND );
		}
	}

	return bSuccess;
}
