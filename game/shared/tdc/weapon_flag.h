//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_WEAPON_FLAG_H
#define TDC_WEAPON_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_melee.h"
#include "entity_capture_flag.h"

#ifdef CLIENT_DLL
#define CWeaponFlag C_WeaponFlag
#endif

//=============================================================================
//
// Flag class.
//
class CWeaponFlag : public CTDCWeaponBaseMelee
{
public:
	DECLARE_CLASS( CWeaponFlag, CTDCWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponFlag();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_FLAG; }
	virtual void		SecondaryAttack();
	virtual bool		CanHolster( void ) const;
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual bool		ForceWeaponSwitch( void ) const { return true; }

	CCaptureFlag		*GetOwnerFlag( void ) const;

#ifdef CLIENT_DLL
	virtual int			GetSkin( void );
#endif

private:
	bool m_bDropping;

	CWeaponFlag( const CWeaponFlag & ) {}
};

#endif // TDC_WEAPON_FLAG_H
