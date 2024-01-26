//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef TDC_HUD_WINPANEL_DEATHMATCH_H
#define TDC_HUD_WINPANEL_DEATHMATCH_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tdc_playermodelpanel.h"
#include "hudelement.h"
#include "tdc_controls.h"

class CTDCWinPanelDeathmatch : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCWinPanelDeathmatch, vgui::EditablePanel );

	CTDCWinPanelDeathmatch( const char *pszName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void LevelInit( void );
	virtual void FireGameEvent( IGameEvent *event );

private:
	float m_flShowAt;
	bool m_bHiddenScoreboard;

	CExLabel *m_pPlayerNames[3];
	CExLabel *m_pPlayerKills[3];
	CExLabel *m_pPlayerDeaths[3];
	CTDCPlayerModelPanel *m_pPlayerModels[3];
	int m_iWinAnimation;
};

#endif // TDC_HUD_WINPANEL_DEATHMATCH_H