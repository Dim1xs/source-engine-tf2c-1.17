//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_WEAPON_CROWBAR_H
#define TDC_WEAPON_CROWBAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CWeaponCrowbar C_WeaponCrowbar
#define CWeaponTireIron C_WeaponTireIron
#define CWeaponLeadPipe C_WeaponLeadPipe
#define CWeaponUmbrella C_WeaponUmbrella
#endif

//=============================================================================
//
// Crowbar class.
//
class CWeaponCrowbar : public CTDCWeaponBaseMelee
{
public:
	DECLARE_CLASS( CWeaponCrowbar, CTDCWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponCrowbar() {}
	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_CROWBAR; }

private:
	CWeaponCrowbar( const CWeaponCrowbar & );
};

class CWeaponTireIron : public CWeaponCrowbar
{
public:
	DECLARE_CLASS( CWeaponTireIron, CWeaponCrowbar );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponTireIron() {}
	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_TIREIRON; }

private:
	CWeaponTireIron( const CWeaponTireIron & );
};

class CWeaponLeadPipe : public CWeaponCrowbar
{
public:
	DECLARE_CLASS( CWeaponLeadPipe, CWeaponCrowbar );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponLeadPipe() {}
	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_LEADPIPE; }

private:
	CWeaponLeadPipe( const CWeaponLeadPipe & );
};

class CWeaponUmbrella : public CWeaponCrowbar
{
public:
	DECLARE_CLASS( CWeaponUmbrella, CWeaponCrowbar );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponUmbrella() {}
	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_UMBRELLA; }

private:
	CWeaponUmbrella (const CWeaponUmbrella &);
};

#endif // TDC_WEAPON_CROWBAR_H
