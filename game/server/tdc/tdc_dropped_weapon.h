//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Dropped DM weapon
//
//=============================================================================//
#ifndef TDC_DROPPED_WEAPON
#define TDC_DROPPED_WEAPON
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tdc_item.h"
#include "items.h"
#include "tdc_weaponbase.h"

class CTDCDroppedWeapon : public CItem, public TAutoList<CTDCDroppedWeapon>
{
public:
	DECLARE_CLASS( CTDCDroppedWeapon, CItem );
	DECLARE_SERVERCLASS();

	CTDCDroppedWeapon();

	virtual void			Spawn( void );
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	bool			MyTouch( CBasePlayer *pPlayer );
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual void	EndTouch( CBaseEntity *pOther );
	void			RemovalThink( void );
	bool			ShouldDespawn( void );
	void			SetWeaponID( ETDCWeaponID iWeapon ) { m_iWeaponID = iWeapon; }
	float			GetCreationTime( void ) { return m_flCreationTime; }
	void			SetClip( int iClip ) { m_iClip = iClip; }
	void			SetAmmo( int iAmmo ) { m_iAmmo = iAmmo; }
	void			SetMaxAmmo( int iAmmo ) { m_iMaxAmmo = iAmmo; }

	static CTDCDroppedWeapon *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CTDCWeaponBase *pWeapon, bool bDissolve );

private:
	float m_flCreationTime;
	float m_flRemoveTime;
	ETDCWeaponID m_iWeaponID;
	CTDCWeaponInfo *m_pWeaponInfo;
	
	int m_iClip;
	CNetworkVar( int, m_iAmmo );
	CNetworkVar( int, m_iMaxAmmo );
	CNetworkVar( bool, m_bDissolving );
};

#endif