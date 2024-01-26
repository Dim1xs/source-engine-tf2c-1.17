//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTDC AmmoPack.
//
//=============================================================================//
#ifndef TDC_POWERUP_H
#define TDC_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

#include "items.h"
#include "tdc_shareddefs.h"

//=============================================================================
//
// CTDC Powerup class.
//

class CTDCPickupItem : public CItem, public TAutoList<CTDCPickupItem>
{
public:
	DECLARE_CLASS( CTDCPickupItem, CItem );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTDCPickupItem();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );
	virtual CBaseEntity*	Respawn( void );
	virtual void	Materialize( void );
	virtual void	HideOnPickedUp( void );
	virtual void	UnhideOnRespawn( void );
	virtual void	OnIncomingSpawn( void ) {}
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual bool	MyTouch( CBasePlayer *pPlayer );
	virtual bool	ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer );

	void			SetRespawnTime( float flDelay );
	void			RespawnThink( void );
	bool			IsDisabled( void );
	void			SetDisabled( bool bDisabled );
	void			FireOutputsOnPickup( CBasePlayer *pPlayer );

	void			DropSingleInstance( const Vector &vecVelocity, CBaseCombatCharacter *pOwner, float flOwnerPickupDelay, float flRestTime, float flRemoveTime = 30.0f );

	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputEnableWithEffect( inputdata_t &inputdata );
	void			InputDisableWithEffect( inputdata_t &inputdata );
	void			InputToggle( inputdata_t &inputdata );
	void			InputRespawnNow( inputdata_t &inputdata );

	bool			IsRespawning() const { return m_bRespawning; }

	bool			ValidateForGameType( ETDCGameType iType );
	static void		UpdatePowerupsForGameType( ETDCGameType iType );

protected:
	float m_flRespawnDelay;
	float m_flInitialSpawnDelay;
	bool m_bDropped;

private:
	CNetworkVar( bool, m_bDisabled );
	CNetworkVar( bool, m_bRespawning );
	CNetworkVar( float, m_flRespawnStartTime );
	CNetworkVar( float, m_flRespawnTime );

	float m_flOwnerPickupEnableTime;
	bool m_bFire15SecRemain;
	bool m_bEnabledModes[TDC_GAMETYPE_COUNT];

	COutputEvent m_outputOnRespawn;
	COutputEvent m_outputOn15SecBeforeRespawn;
	COutputEvent m_outputOnTeam1Touch;
	COutputEvent m_outputOnTeam2Touch;
};

#endif // TDC_POWERUP_H


