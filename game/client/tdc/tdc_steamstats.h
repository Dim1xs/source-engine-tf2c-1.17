//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_STEAMSTATS_H
#define TDC_STEAMSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include "GameEventListener.h"

class CTDCSteamStats : public CAutoGameSystem, public CGameEventListener
{
public:
	CTDCSteamStats();
	virtual void PostInit();
	virtual void LevelShutdownPreEntity();
	virtual void UploadStats();

private:
	void FireGameEvent( IGameEvent *event );
	void SetNextForceUploadTime();
	void ReportLiveStats();	// Xbox 360
	float m_flTimeNextForceUpload;
};

extern CTDCSteamStats g_TFSteamStats;

#endif //TDC_STEAMSTATS_H