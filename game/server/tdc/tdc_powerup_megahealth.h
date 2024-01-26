//=============================================================================//
//
// Purpose: FILL IN
//
//=============================================================================//

#ifndef POWERUP_MEGAHEALTH_H
#define POWERUP_MEGAHEALTH_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_powerupbase.h"

//=============================================================================

class CTDCPowerupMegaHealth : public CTDCPowerupBase
{
public:
	DECLARE_CLASS( CTDCPowerupMegaHealth, CTDCPowerupBase );

	virtual const char *GetPowerupModel( void ) { return "models/items/powerup_health.mdl"; }
	virtual const char *GetPickupSound( void ) { return "PowerupMegaHealth.Touch"; }

	virtual int GetIncomingAnnouncement( void ) { return TDC_ANNOUNCER_DM_MEGAHEALTH_INCOMING; }
	virtual int GetSpawnAnnouncement( void ) { return TDC_ANNOUNCER_DM_MEGAHEALTH_SPAWN; }
	virtual int GetTeamPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_MEGAHEALTH_TEAMPICKUP; }
	virtual int GetEnemyPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_MEGAHEALTH_ENEMYPICKUP; }

	virtual bool MyTouch( CBasePlayer *pPlayer );
};

#endif // POWERUP_MEGAHEALTH_H
