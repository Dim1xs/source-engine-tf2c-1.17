//=============================================================================//
//
// Purpose: Deathmatch HUD
//
//=============================================================================//

#ifndef TDC_HUD_DEATHMATCHSTATUS_H
#define TDC_HUD_DEATHMATCHSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "GameEventListener.h"
#include "tdc_controls.h"
#include "vgui_avatarimage.h"

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudDeathMatchObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTDCHudDeathMatchObjectives, vgui::EditablePanel );

public:

	CTDCHudDeathMatchObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void LevelInit( void );
	virtual void Reset( void );
	virtual void OnTick( void );

	virtual void FireGameEvent( IGameEvent *event );

private:
	void UpdateStatus( void );
	void SetPlayingToLabelVisible( bool bVisible );

private:
	vgui::EditablePanel *m_pLocalPlayerPanel;
	vgui::EditablePanel *m_pBestPlayerPanel;
	CAvatarImagePanel *m_pPlayerAvatar;
	CAvatarImagePanel *m_pRivalAvatar;

	CExLabel *m_pPlayingTo;
	CTDCImagePanel *m_pPlayingToBG;

	int m_iRivalPlayer;

	enum
	{
		DM_STATUS_NONE,
		DM_STATUS_LEADTAKEN,
		DM_STATUS_LEADLOST,
		DM_STATUS_LEADTIED,
	};

	int m_iLeadStatus;

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudTeamDeathMatchObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTDCHudTeamDeathMatchObjectives, vgui::EditablePanel );

public:

	CTDCHudTeamDeathMatchObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void LevelInit( void );
	virtual void OnTick( void );

	virtual void FireGameEvent( IGameEvent *event );

private:
	void UpdateStatus( void );
	void SetPlayingToLabelVisible( bool bVisible );
	vgui::EditablePanel *GetTeamPanel( int iTeam );

private:
	vgui::EditablePanel *m_pRedPanel;
	vgui::EditablePanel *m_pBluePanel;

	CExLabel *m_pPlayingTo;
	CTDCImagePanel *m_pPlayingToBG;

	enum
	{
		DM_STATUS_NONE,
		DM_STATUS_LEADTAKEN,
		DM_STATUS_LEADLOST,
		DM_STATUS_LEADTIED,
	};

	int m_iLeadStatus;
};

#endif	// TDC_HUD_DEATHMATCHSTATUS_H
