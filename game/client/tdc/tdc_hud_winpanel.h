//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFWINPANEL_H
#define TFWINPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"

using namespace vgui;

#define NUM_NOTABLE_PLAYERS	3
#define NUM_CATEGORIES	2
#define NUM_ITEMS_PER_CATEGORY	3

class CTDCWinPanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTDCWinPanel, EditablePanel );

public:
	CTDCWinPanel( const char *pElementName );

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnThink();
	virtual bool ShouldDraw( void );
	virtual void SetVisible( bool state );

	virtual int GetRenderGroupPriority() { return 70; }

	int GetLeftTeam( void );
	int GetRightTeam( void );
	int GetTeamScore( int iTeam, bool bPrevious );

private:
	EditablePanel *m_pBGPanel;
	EditablePanel *m_pTeamScorePanel;
	EditablePanel *m_pBlueBG;
	EditablePanel *m_pRedBG;

	vgui::IBorder *m_pBlackBorder;
	vgui::IBorder *m_pBlueBorder;
	vgui::IBorder *m_pRedBorder;

	float	m_flTimeUpdateTeamScore;
	int		m_iBlueTeamScore;
	int		m_iRedTeamScore;
	int		m_iScoringTeam;

	bool	m_bShouldBeVisible;
};

#endif //TFWINPANEL_H