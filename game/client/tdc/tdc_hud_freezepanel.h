//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_HUD_FREEZEPANEL_H
#define TDC_HUD_FREEZEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tdc_imagepanel.h"
#include "tdc_hud_playerstatus.h"
#include "vgui_avatarimage.h"
#include "basemodelpanel.h"

using namespace vgui;

bool IsTakingAFreezecamScreenshot( void );

//-----------------------------------------------------------------------------
// Purpose: Custom health panel used in the freeze panel to show killer's health
//-----------------------------------------------------------------------------
class CTDCFreezePanelHealth : public CTDCHudPlayerHealth
{
public:
	CTDCFreezePanelHealth( Panel *parent, const char *name ) : CTDCHudPlayerHealth( parent, name )
	{
	}

	virtual const char *GetResFilename( void ) { return "resource/UI/FreezePanelKillerHealth.res"; }
	virtual void OnThink()
	{
		// Do nothing. We're just preventing the base health panel from updating.
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCFreezePanelCallout : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCFreezePanelCallout, EditablePanel );
public:
	CTDCFreezePanelCallout( Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void	UpdateForGib( int iGib, int iCount );
	void	UpdateForRagdoll( void );
	void	SetTeam( int iTeam );

private:
	vgui::Label	*m_pGibLabel;
	CTDCImagePanel *m_pCalloutBG;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCFreezePanel : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTDCFreezePanel, EditablePanel );

public:
	CTDCFreezePanel( const char *pElementName );

	virtual void Reset();
	virtual void Init();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent * event );

	void ShowSnapshotPanel( bool bShow );
	void UpdateCallout( void );
	void ShowCalloutsIn( float flTime );
	void ShowSnapshotPanelIn( float flTime );
	void Show();
	void Hide();
	virtual bool ShouldDraw( void );
	void OnThink( void );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	bool IsHoldingAfterScreenShot( void ) { return m_bHoldingAfterScreenshot; }

	const CUtlVector<CSteamID> &GetUsersInScreenshot( void ) { return m_UsersInFreezecam; }

protected:
	CTDCFreezePanelCallout *TestAndAddCallout( Vector &origin, Vector &vMins, Vector &vMaxs, CUtlVector<Vector> *vecCalloutsTL, 
		CUtlVector<Vector> *vecCalloutsBR, Vector &vecFreezeTL, Vector &vecFreezeBR, Vector &vecStatTL, Vector &vecStatBR, int *iX, int *iY );

private:
	void ShowNemesisPanel( bool bShow );

	int						m_iYBase;
	int						m_iKillerIndex;
	CTDCHudPlayerHealth		*m_pKillerHealth;
	int						m_iShowNemesisPanel;
	CUtlVector<CTDCFreezePanelCallout*>	m_pCalloutPanels;
	float					m_flShowCalloutsAt;
	float					m_flShowSnapshotReminderAt;
	EditablePanel			*m_pNemesisSubPanel;
	vgui::Label				*m_pFreezeLabel;
	vgui::Label				*m_pFreezeLabelKiller;
	CTDCImagePanel			*m_pFreezePanelBG;
	CAvatarImagePanel		*m_pAvatar;
	vgui::EditablePanel		*m_pScreenshotPanel;
	CModelPanel				*m_pModelBG;
	vgui::EditablePanel		*m_pBasePanel;

	int 					m_iBasePanelOriginalX;
	int 					m_iBasePanelOriginalY;

	bool					m_bHoldingAfterScreenshot;

	CUtlVector<CSteamID>	m_UsersInFreezecam;

	enum 
	{
		SHOW_NO_NEMESIS = 0,
		SHOW_NEW_NEMESIS,
		SHOW_REVENGE
	};
};

#endif // TDC_HUD_FREEZEPANEL_H
