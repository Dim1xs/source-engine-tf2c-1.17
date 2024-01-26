//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef TDC_IMAGEPANEL_H
#define TDC_IMAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/ScalableImagePanel.h"
#include "GameEventListener.h"
#include "tdc_shareddefs.h"

class CTDCImagePanel : public vgui::ScalableImagePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTDCImagePanel, vgui::ScalableImagePanel );

	CTDCImagePanel(vgui::Panel *parent, const char *name);

	virtual void ApplySettings( KeyValues *inResourceData );
	void UpdateBGImage( void );
	void SetBGImage( int iTeamNum );
	void UpdateBGTeam( void );

	virtual Color GetDrawColor( void );

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

private:
	char	m_szTeamBG[TDC_TEAM_COUNT][128];
	int		m_iBGTeam;
	bool	m_bAlwaysColored;
};


#endif // TDC_IMAGEPANEL_H
