//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TDC_HUD_MATCH_STATUS
#define TDC_HUD_MATCH_STATUS

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tdc_hud_objectivestatus.h"

//-----------------------------------------------------------------------------
// Purpose: Stub HUD element from matchmaking that we need for the timer.
//-----------------------------------------------------------------------------
class CTDCHudMatchStatus : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCHudMatchStatus, vgui::EditablePanel );

	CTDCHudMatchStatus( const char *pElementName );
	
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset( void );
	virtual void OnThink( void );

	virtual int GetRenderGroupPriority( void ) { return 60; }

private:
	CTDCHudTimeStatus *m_pTimePanel;
};

#endif // TDC_HUD_MATCH_STATUS
