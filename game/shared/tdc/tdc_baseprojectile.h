//=============================================================================//
//
// Purpose: TF2 projectile base.
//
//=============================================================================//
#ifndef TDC_BASEPROJECTILE_H
#define TDC_BASEPROJECTILE_H

#ifdef _WIN32
#pragma once
#endif

#include "baseprojectile.h"
#include "tdc_shareddefs.h"

#ifdef CLIENT_DLL
#define CTDCBaseProjectile C_TDCBaseProjectile
#endif

class CTDCBaseProjectile : public CBaseProjectile
{
public:
	DECLARE_CLASS( CTDCBaseProjectile, CBaseProjectile );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	virtual ETDCWeaponID GetWeaponID( void ) const;

	virtual void Spawn( void );
#else
	virtual bool HasTeamSkins( void ) { return false; }
	virtual int GetSkin( void );
	void SetParticleColor( CNewParticleEffect *pEffect );
#endif
};

#endif // TDC_BASEPROJECTILE_H
