//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
//
//=============================================================================//
#include "cbase.h"
#include "tdc_item.h"
#include "tdc_shareddefs.h"

#ifdef CLIENT_DLL
#include "c_tdc_player.h"
#else
#include "tdc_player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TDCItem, DT_TDCItem )

BEGIN_NETWORK_TABLE( CTDCItem, DT_TDCItem )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Identifier.
//-----------------------------------------------------------------------------
unsigned int CTDCItem::GetItemID( void )
{ 
	return TDC_ITEM_UNDEFINED; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCItem::PickUp( CTDCPlayer *pPlayer, bool bInvisible )
{
	// SetParent with attachment point - look it up later if need be!
	SetOwnerEntity( pPlayer );
	SetParent( pPlayer );
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );

	// Make invisible?
	if ( bInvisible )
	{
		AddEffects( EF_NODRAW );
	}

	// Add the item to the player's item inventory.
	pPlayer->SetItem( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCItem::Drop( CTDCPlayer *pPlayer, bool bVisible, bool bThrown /*= false*/, bool bMessage /*= true*/ )
{
	// Remove the item from the player's item inventory.
	pPlayer->SetItem( NULL );

	// Make visible?
	if ( bVisible )
	{
		RemoveEffects( EF_NODRAW );
	}

	// Clear the parent.
	SetParent( NULL );
	RemoveEffects( EF_BONEMERGE );
	SetOwnerEntity( NULL );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCItem::ShouldDraw()
{
	// Don't draw the flag in first person.
	CBasePlayer *pCarrier = ToBasePlayer( GetMoveParent() );

	if ( pCarrier && !pCarrier->ShouldDrawThisPlayer() )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t CTDCItem::ShadowCastType()
{
	// Don't draw the flag in first person.
	C_BasePlayer *pCarrier = ToBasePlayer( GetMoveParent() );

	if ( pCarrier && !pCarrier->ShouldDrawThisPlayer() )
		return SHADOWS_NONE;

	return BaseClass::ShadowCastType();
}

#endif