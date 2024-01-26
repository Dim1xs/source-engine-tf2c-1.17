//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail Projectile
//
//=============================================================================
#ifndef TDC_PROJECTILE_NAIL_H
#define TDC_PROJECTILE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_nail.h"

//-----------------------------------------------------------------------------
// Purpose: Identical to a nail except for model used
//-----------------------------------------------------------------------------
class CProjectile_Syringe : public CTDCBaseNail
{
	DECLARE_CLASS( CProjectile_Syringe, CTDCBaseNail );

public:
	// Creation.
	static CProjectile_Syringe *Create( const Vector &vecOrigin, const Vector &vecVelocity,
		int iType, float flDamage, bool bCritical,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );

#ifdef GAME_DLL
	virtual ETDCWeaponID GetWeaponID( void ) const;
#endif
};

#endif	//TDC_PROJECTILE_NAIL_H