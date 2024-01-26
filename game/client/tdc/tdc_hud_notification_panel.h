//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_HUD_NOTIFICATION_PANEL_H
#define TDC_HUD_NOTIFICATION_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "IconPanel.h"
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudNotificationPanel : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudNotificationPanel, EditablePanel );

public:
	CHudNotificationPanel( const char *pElementName );
	~CHudNotificationPanel();

	virtual void	Init( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	virtual void	PerformLayout( void );

	const char *GetNotificationByType( int iType );

	void	MsgFunc_HudNotify( bf_read &msg );
	void	MsgFunc_HudNotifyCustom( bf_read &msg );

	void	SetupNotifyCustom( const char *pszText, const char *pszIcon, int iBackgroundTeam );

	virtual void LevelInit( void ) { m_flFadeTime = 0; };

	static void SetupNotificationPanel( const char *pszText, const char *pszIcon, int iBackgroundTeam );

private:
	float m_flFadeTime;

	Label *m_pText;
	CIconPanel *m_pIcon;
	ImagePanel *m_pBackground;
};

#endif // TDC_HUD_NOTIFICATION_PANEL_H
