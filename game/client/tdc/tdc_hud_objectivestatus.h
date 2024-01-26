//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_HUD_OBJECTIVESTATUS_H
#define TDC_HUD_OBJECTIVESTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_controls.h"
#include "tdc_imagepanel.h"
#include "tdc_hud_flagstatus.h"
#include "tdc_hud_deathmatchstatus.h"
#include "tdc_hud_bloodmoney.h"
#include "GameEventListener.h"
#include "c_tdc_player.h"
#include "tdc_imagepanel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCProgressBar : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCProgressBar, vgui::ImagePanel );

	CTDCProgressBar( vgui::Panel *parent, const char *name );

	virtual void Paint();
	void SetPercentage( float flPercentage ){ m_flPercent = flPercentage; }	
	void SetIcon( const char* szIcon );

private:

	float	m_flPercent;
	int		m_iTexture;

	CPanelAnimationVar( Color, m_clrActive, "color_active", "TimerProgress.Active" );
	CPanelAnimationVar( Color, m_clrInActive, "color_inactive", "TimerProgress.InActive" );
	CPanelAnimationVar( Color, m_clrWarning, "color_warning", "TimerProgress.Active" );
	CPanelAnimationVar( float, m_flPercentWarning, "percent_warning", "0.75" );
};


// Floating delta text items, float off the top of the frame to 
// show changes to the timer value
typedef struct 
{
	// amount of delta
	int m_nAmount;

	// die time
	float m_flDieTime;

} timer_delta_t;

#define NUM_TIMER_DELTA_ITEMS 10

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudTimeStatus : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTDCHudTimeStatus, EditablePanel );

public:

	CTDCHudTimeStatus( Panel *parent, const char *name );

	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	int GetTimerIndex( void ){ return m_iTimerIndex; }
	void SetTimerIndex( int index ){ m_iTimerIndex = ( index >= 0 ) ? index : 0; SetExtraTimePanels(); }
	
	virtual void FireGameEvent( IGameEvent *event );

	void SetExtraTimePanels();

protected:

	virtual void OnThink();

private:

	void SetTimeAdded( int iIndex, int nSeconds );
	void CheckClockLabelLength( CExLabel *pLabel, CTDCImagePanel *pBG );
	void SetTeamBackground();

public:

	int					m_iTeamIndex;

private:

	float				m_flNextThink;
	int					m_iTimerIndex;
	bool				m_bSuddenDeath;
	bool				m_bOvertime;

	CExLabel			*m_pTimeValue;
	CTDCProgressBar		*m_pProgressBar;

	CExLabel			*m_pWaitingForPlayersLabel;
	CTDCImagePanel		*m_pWaitingForPlayersBG;

	CExLabel			*m_pOvertimeLabel;
	CTDCImagePanel		*m_pOvertimeBG;

	CExLabel			*m_pSetupLabel;
	CTDCImagePanel		*m_pSetupBG;

	// we'll have a second label/bg set for the SuddenDeath panel in case we want to change the look from the Overtime label
	CExLabel			*m_pSuddenDeathLabel;
	CTDCImagePanel		*m_pSuddenDeathBG;

	vgui::ScalableImagePanel  *m_pTimePanelBG;

	// delta stuff
	int m_iTimerDeltaHead;
	timer_delta_t m_TimerDeltaItems[NUM_TIMER_DELTA_ITEMS];
	CPanelAnimationVarAliasType( float, m_flDeltaItemStartPos, "delta_item_start_y", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flDeltaItemX, "delta_item_x", "0", "proportional_float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
};

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the various objective displays
//-----------------------------------------------------------------------------
class CTDCHudObjectiveStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudObjectiveStatus, vgui::EditablePanel );

public:
	CTDCHudObjectiveStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();
	virtual void OnThink( void );

	virtual int GetRenderGroupPriority( void ) { return 60; }	// higher than build menus

private:

	void	SetVisiblePanels( void );
	void	TurnOffPanels( void );

private:
	CTDCHudFlagObjectives	*m_pFlagPanel;
//	CTDCHudKothTimeStatus	*m_pKothTimePanel;
	CTDCHudDeathMatchObjectives *m_pDMPanel;
	CTDCHudTeamDeathMatchObjectives *m_pTDMPanel;
	CHudBloodMoney			*m_pBloodMoneyPanel;
	CHudTeamBloodMoney		*m_pTeamBloodMoneyPanel;
};

#endif	// TDC_HUD_OBJECTIVESTATUS_H