//=============================================================================//
//
// Purpose: FILL IN
//
//=============================================================================//

#ifndef POWERUP_SPEEDBOOST_H
#define POWERUP_SPEEDBOOST_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_powerupbase.h"

//=============================================================================

class CTDCPowerupSpeedBoost : public CTDCPowerupBase
{
public:
	DECLARE_CLASS( CTDCPowerupSpeedBoost, CTDCPowerupBase );

	virtual const char *GetPowerupModel( void ) { return "models/items/powerup_speed.mdl"; }
	virtual const char *GetPickupSound( void ) { return "PowerupSpeedBoost.Touch"; }

	virtual ETDCCond	GetCondition( void ) { return TDC_COND_POWERUP_SPEEDBOOST; }

	virtual int GetIncomingAnnouncement( void ) { return TDC_ANNOUNCER_DM_HASTE_INCOMING; }
	virtual int GetSpawnAnnouncement( void ) { return TDC_ANNOUNCER_DM_HASTE_SPAWN; }
	virtual int GetTeamPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_HASTE_TEAMPICKUP; }
	virtual int GetEnemyPickupAnnouncement( void ) { return TDC_ANNOUNCER_DM_HASTE_ENEMYPICKUP; }
};

#endif // POWERUP_SPEEDBOOST_H
