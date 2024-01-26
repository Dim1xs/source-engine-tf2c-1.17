//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_SCOREBOARD_H
#define TDC_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "GameEventListener.h"
#include "tdc_controls.h"
#include "tdc_shareddefs.h"

#define TDC_SCOREBOARD_MAX_DOMINATIONS 16

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTDCClientScoreBoardDialog : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTDCClientScoreBoardDialog, vgui::EditablePanel );

	CTDCClientScoreBoardDialog( IViewPort *pViewPort, const char *pszName );
	virtual ~CTDCClientScoreBoardDialog();

	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual const char *GetName( void ) { return PANEL_SCOREBOARD; }
	virtual void SetData( KeyValues *data ) {}
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	virtual void OnCommand( const char *command );
	virtual void OnThink( void );

	// both vgui::EditablePanel and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

	void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );
	virtual int GetDefaultAvatar( int playerIndex );

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	
protected:
	virtual void InitPlayerList( vgui::SectionedListPanel *pPlayerList, vgui::SectionedListPanel *pPingList );
	void SetPlayerListImages( vgui::SectionedListPanel *pPlayerList );
	virtual void UpdateTeamInfo();
	virtual void UpdatePlayerList();
	virtual void UpdateSpectatorList();
	virtual void UpdateArenaWaitingToPlayList();
	void MoveToCenterOfScreen();

	virtual void FireGameEvent( IGameEvent *event );

	vgui::SectionedListPanel *GetSelectedPlayerList( void );

	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );

	vgui::SectionedListPanel	*m_pPlayerLists[TDC_TEAM_COUNT];
	vgui::SectionedListPanel	*m_pPingLists[TDC_TEAM_COUNT];

	vgui::ImageList				*m_pImageList;
	CUtlMap<CSteamID, int>		m_mapAvatarsToImageList;

	CPanelAnimationVar( int, m_iAvatarWidth, "avatar_width", "34" );		// Avatar width doesn't scale with resolution
	CPanelAnimationVarAliasType( int, m_iNameWidth, "name_width", "136", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iScoreWidth, "score_width", "35", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPingWidth, "ping_width", "23", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iStatusWidth, "status_width", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNemesisWidth, "nemesis_width", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iKillsWidth, "kills_width", "23", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iDeathsWidth, "deaths_width", "23", "proportional_int" );

	int							m_iImageDead;
	int							m_iImageDominated;
	int							m_iImageNemesis;
	int							m_iImageDominations[TDC_SCOREBOARD_MAX_DOMINATIONS];
	int							m_iDefaultAvatars[TDC_TEAM_COUNT];

private:
	MESSAGE_FUNC_INT( OnPollHideCode, "PollHideCode", code );
	MESSAGE_FUNC_PARAMS( ShowContextMenu, "ItemContextMenu", data );

	CExLabel					*m_pLabelPlayerName;
	CExLabel					*m_pLabelMapName;
	CExLabel					*m_pLabelGameTypeName;
	vgui::ImagePanel			*m_pGameTypeIcon;
	CExLabel					*m_pSpectatorsInQueue;
	CExLabel					*m_pServerTimeLeftValue;
	vgui::HFont					m_hTimeLeftFont;
	vgui::HFont					m_hTimeLeftNotSetFont;
	vgui::Menu					*m_pContextMenu;

	float						m_flNextUpdateTime;
	ButtonCode_t				m_nCloseKey;
	int							m_iSelectedPlayerIndex;
};

#endif // TDC_SCOREBOARD_H
