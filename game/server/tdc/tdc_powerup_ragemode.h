//=============================================================================//
//
// Purpose: The ragemode powerup (name is a W.I.P). Equips the mercenary with
//			the Hammerfist weapon for a short period of time.
//
//=============================================================================//

#ifndef POWERUP_RAGEMODE_H
#define POWERUP_RAGEMODE_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_powerupbase.h"

//=============================================================================

class CTDCPowerupRagemode : public CTDCPowerupBase
{
public:
	DECLARE_CLASS( CTDCPowerupRagemode, CTDCPowerupBase );

	virtual const char *GetPowerupModel( void ) { return "models/items/powerup_berserk.mdl"; }
	virtual const char *GetPickupSound( void ) { return "PowerupBerserk.Touch"; }

	virtual ETDCCond	GetCondition( void ) { return TDC_COND_POWERUP_RAGEMODE; }

	virtual int GetIncomingAnnouncement( void ) { return TDC_ANNOUNCER_DM_BERSERK_INCOMING; }
	virtual int GetSpawnAnnouncement( void ) { return TDC_ANNOUNCER_DM_BERSERK_SPAWN; }
	virtual int GetTeamPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_BERSERK_TEAMPICKUP; }
	virtual int GetEnemyPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_BERSERK_ENEMYPICKUP; }
};

#endif // POWERUP_RAGEMODE_H
