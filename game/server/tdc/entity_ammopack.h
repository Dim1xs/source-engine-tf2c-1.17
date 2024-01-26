//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC AmmoPack.
//
//=============================================================================//
#ifndef ENTITY_AMMOPACK_H
#define ENTITY_AMMOPACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_pickupitem.h"

#define TDC_AMMOPACK_PICKUP_SOUND	"AmmoPack.Touch"

//=============================================================================
//
// CTDC AmmoPack class.
//

class CAmmoPack : public CTDCPickupItem
{
public:
	DECLARE_CLASS( CAmmoPack, CTDCPickupItem );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );

	virtual float GetAmmoPerc( void ) { return 1.0f; }

	virtual const char *GetPowerupModel( void ) { return "models/items/ammopack_large.mdl"; }
};

class CAmmoPackSmall : public CAmmoPack
{
public:
	DECLARE_CLASS( CAmmoPackSmall, CAmmoPack );
	virtual float GetAmmoPerc( void ) { return 0.2f; }

	virtual const char *GetPowerupModel( void ) { return "models/items/ammopack_small.mdl"; }
};

class CAmmoPackMedium : public CAmmoPack
{
public:
	DECLARE_CLASS( CAmmoPackMedium, CAmmoPack );
	virtual float GetAmmoPerc( void ) { return 0.5; }

	virtual const char *GetPowerupModel( void ) { return "models/items/ammopack_medium.mdl"; }
};

#endif // ENTITY_AMMOPACK_H


