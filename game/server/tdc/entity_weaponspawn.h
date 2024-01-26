//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawning entity.
//
//=============================================================================//
#ifndef ENTITY_WEAPONSPAWN_H
#define ENTITY_WEAPONSPAWN_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_pickupitem.h"

//=============================================================================

class CWeaponSpawner : public CTDCPickupItem
{
public:
	DECLARE_CLASS( CWeaponSpawner, CTDCPickupItem );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CWeaponSpawner();

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual int		UpdateTransmitState( void );

	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual void	HideOnPickedUp( void );
	virtual void	UnhideOnRespawn( void );
	virtual void	OnIncomingSpawn( void );
	virtual bool	MyTouch( CBasePlayer *pPlayer );
	virtual void	EndTouch( CBaseEntity *pOther );

private:

	ETDCWeaponID m_iWeaponID;
	CTDCWeaponInfo *m_pWeaponInfo;

	CNetworkVar( string_t, m_iszWeaponName );
	CNetworkVar( bool, m_bStaticSpawner );
	CNetworkVar( bool, m_bOutlineDisabled );
	CNetworkVar( bool, m_bSpecialGlow );

	struct powerupvoices
	{
		int incoming;
		int spawn;
		int teampickup;
		int enemypickup;
	} m_Announcements;
	bool m_bEnableAnnouncements;
};

#endif // ENTITY_HEALTHKIT_H


