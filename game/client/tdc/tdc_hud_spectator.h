//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_SPECTATORGUI_H
#define TDC_SPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tdc_hud_playerstatus.h"

//-----------------------------------------------------------------------------
// Purpose: Custom health panel used to show spectator target's health
//-----------------------------------------------------------------------------
class CTDCSpectatorGUIHealth : public CTDCHudPlayerHealth
{
public:
	CTDCSpectatorGUIHealth( Panel *parent, const char *name ) : CTDCHudPlayerHealth( parent, name )
	{
	}

	virtual const char *GetResFilename( void )
	{
		return "resource/UI/SpectatorGUIHealth.res";
	}
	virtual void OnThink()
	{
		// Do nothing. We're just preventing the base health panel from updating.
	}
};

//-----------------------------------------------------------------------------
// Purpose: TF Spectator UI
//-----------------------------------------------------------------------------
class CTDCSpectatorGUI : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCSpectatorGUI, vgui::EditablePanel );

	CTDCSpectatorGUI( const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	void			UpdateRespawnLabel(void);
	void			UpdateReinforcements( void );
	virtual void	SetVisible( bool bVisible );
	virtual Color	GetBlackBarColor( void ) { return Color( 52, 48, 45, 255 ); }

	void			UpdateKeyLabels( void );
protected:
	int		m_nLastSpecMode;
	CBaseEntity	*m_nLastSpecTarget;
	float	m_flNextTipChangeTime;		// time at which to next change the tip
	int		m_iTipClass;				// class that current tip is for

	vgui::Label				*m_pReinforcementsLabel;
	vgui::Label				*m_pClassOrTeamLabel;
	vgui::Label				*m_pRespawnLabel;
	vgui::Label				*m_pSwitchCamModeKeyLabel;
	vgui::Label				*m_pCycleTargetFwdKeyLabel;
	vgui::Label				*m_pCycleTargetRevKeyLabel;
	vgui::Label				*m_pMapLabel;
};

#endif // TDC_SPECTATORGUI_H
