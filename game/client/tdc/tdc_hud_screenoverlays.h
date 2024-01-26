//=============================================================================//
//
// Purpose: HUD for drawing screen overlays.
//
//=============================================================================//
#ifndef TDC_HUD_SCREENOVERLAYS_H
#define TDC_HUD_SCREENOVERLAYS_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "GameEventListener.h"
#include "clienteffectprecachesystem.h"
#include "tdc_shareddefs.h"

enum
{
	TDC_OVERLAY_BERSERK,
	TDC_OVERLAY_HASTE,
	TDC_OVERLAY_BURNING,
	TDC_OVERLAY_INVULN,
	TDC_OVERLAY_HEADSHOT,
	TDC_OVERLAY_ZOOM,
	TDC_OVERLAY_DESAT,
	TDC_OVERLAY_COUNT,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudScreenOverlays : public CAutoGameSystem
{
public:
	CHudScreenOverlays();

	virtual void LevelInitPreEntity( void );
	virtual void LevelShutdownPostEntity( void );

	void DrawOverlays( const CViewSetup &view );

private:
	CMaterialReference m_ScreenOverlays[TDC_OVERLAY_COUNT][TDC_TEAM_COUNT];
};

extern CHudScreenOverlays g_ScreenOverlayManager;

#endif // TDC_HUD_SCREENOVERLAYS_H
