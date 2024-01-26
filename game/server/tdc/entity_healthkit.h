//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC HealthKit.
//
//=============================================================================//
#ifndef ENTITY_HEALTHKIT_H
#define ENTITY_HEALTHKIT_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_pickupitem.h"

//=============================================================================
//
// CTDC HealthKit class.
//

class CHealthKit : public CTDCPickupItem
{
public:
	DECLARE_CLASS( CHealthKit, CTDCPickupItem );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );

	virtual const char *GetPowerupModel( void ) { return "models/items/medkit_large.mdl"; }

	virtual int	GetHealthAmount( void ) { return 150; }
};

class CHealthKitSmall : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitSmall, CHealthKit );
	virtual int	GetHealthAmount( void ) { return 30; }

	virtual const char *GetPowerupModel( void ) { return "models/items/medkit_small.mdl"; }
};

class CHealthKitMedium : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitMedium, CHealthKit );
	virtual int	GetHealthAmount( void ) { return 75; }

	virtual const char *GetPowerupModel( void ) { return "models/items/medkit_medium.mdl"; }
};

class CHealthKitTiny : public CHealthKit
{
public:
	DECLARE_CLASS(CHealthKitTiny, CHealthKit);
	virtual int	GetHealthAmount( void ) { return 5; }

	virtual const char *GetPowerupModel(void) { return "models/items/medkit_overheal.mdl"; } 
};

#endif // ENTITY_HEALTHKIT_H


