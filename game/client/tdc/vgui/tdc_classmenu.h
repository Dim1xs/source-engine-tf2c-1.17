//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_CLASSMENU_H
#define TDC_CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Frame.h>
#include "vgui_controls/KeyRepeat.h"
#include <tdc_shareddefs.h>
#include "tdc_controls.h"
#include "tdc_playermodelpanel.h"
#include <game/client/iviewport.h>
#include <vgui_controls/PanelListPanel.h>

class CTDCClassTipsPanel;
class CTDCClassTipsListPanel;

#define CLASS_COUNT_IMAGES	11

//-----------------------------------------------------------------------------
// Purpose: Class selection menu
//-----------------------------------------------------------------------------
class CTDCClassMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTDCClassMenu, vgui::Frame );

public:
	CTDCClassMenu( IViewPort *pViewPort );

	virtual const char *GetName( void ) { return PANEL_CLASS; }
	virtual void SetData( KeyValues *data );
	virtual void Reset() {}
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void SetVisible( bool state );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );

	virtual void OnClose();

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

	CON_COMMAND_MEMBER_F( CTDCClassMenu, "join_class", Join_Class, "Send a joinclass command", 0 );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	int GetCurrentClass( void );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnThink();

	void SetTeam( int iTeam );
	void SelectClass( int iClass );

	MESSAGE_FUNC_INT( OnShowToTeam, "ShowToTeam", iTeam );
	MESSAGE_FUNC( OnLoadoutChanged, "LoadoutChanged" );

private:
	CTDCPlayerModelPanel *m_pClassModels[TDC_CLASS_MENU_BUTTONS];
	CExButton *m_pClassButtons[TDC_CLASS_MENU_BUTTONS];
	CTDCClassTipsPanel *m_pTipsPanel;

	CExButton *m_pCancelButton;
	CExButton *m_pLoadoutButton;

#ifdef _X360
	CTFFooter		*m_pFooter;
#endif

	ButtonCode_t	m_iClassMenuKey;
	ButtonCode_t	m_iScoreBoardKey;
	vgui::CKeyRepeatHandler	m_KeyRepeat;
	int				m_iCurrentButtonIndex;
	int				m_iTeamNum;

#ifndef _X360
	CTDCImagePanel *m_ClassCountImages[CLASS_COUNT_IMAGES];
	CExLabel *m_pCountLabel;
#endif
};

#endif // TDC_CLASSMENU_H

