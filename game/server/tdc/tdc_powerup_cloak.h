//=============================================================================//
//
// Purpose: FILL IN
//
//=============================================================================//

#ifndef POWERUP_CLOAK_H
#define POWERUP_CLOAK_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_powerupbase.h"

//=============================================================================

class CTDCPowerupCloak : public CTDCPowerupBase
{
public:
	DECLARE_CLASS( CTDCPowerupCloak, CTDCPowerupBase );

	virtual const char *GetPowerupModel( void ) { return "models/items/powerup_invis.mdl"; }
	virtual const char *GetPickupSound( void ) { return "PowerupCloak.Touch"; }

	virtual ETDCCond	GetCondition( void ) { return TDC_COND_POWERUP_CLOAK; }

	virtual int GetIncomingAnnouncement( void ) { return TDC_ANNOUNCER_DM_CLOAK_INCOMING; }
	virtual int GetSpawnAnnouncement( void ) { return TDC_ANNOUNCER_DM_CLOAK_SPAWN; }
	virtual int GetTeamPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_CLOAK_TEAMPICKUP; }
	virtual int GetEnemyPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_CLOAK_ENEMYPICKUP; }
};

#endif // POWERUP_CLOAK_H
