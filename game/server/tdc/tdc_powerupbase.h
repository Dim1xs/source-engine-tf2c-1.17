//=============================================================================//
//
// Purpose: Base class for Deathmatch powerups 
//
//=============================================================================//

#ifndef BASE_DM_POWERUP_H
#define BASE_DM_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_pickupitem.h"
#include "tdc_shareddefs.h"
#include "tdc_announcer.h"

//=============================================================================

class CTDCPowerupBase : public CTDCPickupItem
{
public:
	DECLARE_CLASS( CTDCPowerupBase, CTDCPickupItem );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CTDCPowerupBase();

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual int		UpdateTransmitState( void );

	virtual void	HideOnPickedUp( void );
	virtual void	UnhideOnRespawn( void );
	virtual void	OnIncomingSpawn( void );
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual bool	MyTouch( CBasePlayer *pPlayer );

	float	GetEffectDuration( void ) { return m_flEffectDuration; }
	void	SetEffectDuration( float flTime ) { m_flEffectDuration = flTime; }

	virtual const char *GetPickupSound( void ) { return "HealthKit.Touch"; }
	virtual const char *GetPowerupModel( void ) { return "models/class_menu/random_class_icon.mdl"; }
	virtual ETDCCond GetCondition( void ) { return TDC_COND_LAST; } // Should trigger an assert.
	virtual int GetIncomingAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_INCOMING; }
	virtual int GetSpawnAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_SPAWN; }
	virtual int GetTeamPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_TEAMPICKUP; }
	virtual int GetEnemyPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_ENEMYPICKUP; }

	static CTDCPowerupBase *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszClassname, float flDuration );

protected:
	float		m_flEffectDuration;
};

#endif // BASE_DM_POWERUP_H
