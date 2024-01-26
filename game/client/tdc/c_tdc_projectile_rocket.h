//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef C_TDC_PROJECTILE_ROCKET_H
#define C_TDC_PROJECTILE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_rocket.h"

#define CProjectile_Rocket C_Projectile_Rocket

//-----------------------------------------------------------------------------
// Purpose: Rocket projectile.
//-----------------------------------------------------------------------------
class C_Projectile_Rocket : public C_TDCBaseRocket
{
	DECLARE_CLASS( C_Projectile_Rocket, C_TDCBaseRocket );
public:
	DECLARE_NETWORKCLASS();

	C_Projectile_Rocket();
	~C_Projectile_Rocket();

	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual void	CreateRocketTrails( void );
	virtual const char *GetTrailParticleName( void );
};

#endif // C_TDC_PROJECTILE_ROCKET_H
