//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef TDC_HUD_CONDSTATUS_H
#define TDC_HUD_CONDSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_hud_objectivestatus.h"

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCPowerupPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCPowerupPanel, vgui::EditablePanel );

	CTDCPowerupPanel( vgui::Panel *parent, const char *name );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void UpdateStatus( void );

	void SetData( ETDCCond cond, float dur, float initdur );

	ETDCCond m_nCond;
	float m_flDuration;
	float m_flInitDuration;

private:
	CTDCProgressBar *m_pProgressBar;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudCondStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudCondStatus, vgui::EditablePanel );

public:
	CTDCHudCondStatus( const char *pElementName );
	~CTDCHudCondStatus();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual bool ShouldDraw( void );
	virtual void OnTick( void );

private:
	CUtlVector<CTDCPowerupPanel *>	m_pPowerups;
};

#endif	// TDC_HUD_CONDSTATUS_H
