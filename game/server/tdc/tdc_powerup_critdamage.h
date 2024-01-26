//=============================================================================//
//
// Purpose: Critical Damage: Crits for 30/15 seconds with a crit glow. 
// Looks like an amplifier device that attaches onto weapons somehow, 
// looks similar to a car battery. 
// Crit glow effect would match the player’s merc color or be blue.
//
//=============================================================================//

#ifndef POWERUP_CRITDAMAGE_H
#define POWERUP_CRITDAMAGE_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_powerupbase.h"

//=============================================================================

class CTDCPowerupCritdamage : public CTDCPowerupBase
{
public:
	DECLARE_CLASS( CTDCPowerupCritdamage, CTDCPowerupBase );

	virtual const char *GetPowerupModel( void ) { return "models/items/powerup_crit.mdl"; }
	virtual const char *GetPickupSound( void ) { return "PowerupCrit.Touch"; }

	virtual ETDCCond	GetCondition( void ) { return TDC_COND_POWERUP_CRITDAMAGE; }

	virtual int GetIncomingAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_INCOMING; }
	virtual int GetSpawnAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_SPAWN; }
	virtual int GetTeamPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_TEAMPICKUP; }
	virtual int GetEnemyPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_CRIT_ENEMYPICKUP; }
};

#endif // POWERUP_CRITDAMAGE_H
