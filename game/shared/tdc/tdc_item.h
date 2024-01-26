//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
//
//=============================================================================//
#ifndef TDC_ITEM_H
#define TDC_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"

#ifdef CLIENT_DLL
#define CTDCPlayer C_TDCPlayer
#define CTDCItem C_TDCItem
#endif

class CTDCPlayer;

//=============================================================================
//
// TF Item
//
class CTDCItem : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTDCItem, CBaseAnimating )
	DECLARE_NETWORKCLASS();

	// Unique identifier.
	virtual unsigned int GetItemID();
	
	// Pick up and drop.
	virtual void PickUp( CTDCPlayer *pPlayer, bool bInvisible );
	virtual void Drop( CTDCPlayer *pPlayer, bool bVisible, bool bThrown = false, bool bMessage = true );

#ifdef CLIENT_DLL
	virtual bool ShouldDraw();
	virtual ShadowType_t ShadowCastType();
#endif
};

#endif // TDC_ITEM_H