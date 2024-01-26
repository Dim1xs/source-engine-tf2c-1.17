//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_MAPINFOMENU_H
#define TDC_MAPINFOMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include "vgui_controls/KeyRepeat.h"

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTDCMapInfoMenu : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTDCMapInfoMenu, vgui::Frame );

public:
	CTDCMapInfoMenu( IViewPort *pViewPort );
	virtual ~CTDCMapInfoMenu();

	virtual const char *GetName( void ){ return PANEL_MAPINFO; }
	virtual void SetData( KeyValues *data ){}
	virtual void Reset(){ Update(); }
	virtual void Update() {}
	virtual bool NeedsUpdate( void ){ return false; }
	virtual bool HasInputElements( void ){ return true; }
	virtual void ShowPanel( bool bShow );
	virtual void FireGameEvent( IGameEvent *event );

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible(){ return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

	// static const char *GetMapType( const char *mapName );

protected:
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnThink();
	
private:
	// helper functions
	void CheckBackContinueButtons();

protected:
	IViewPort			*m_pViewPort;

	CExButton			*m_pContinue;
	vgui::Label			*m_pNameLabel;
	vgui::Label			*m_pGoalLabel;
	CTDCImagePanel		*m_pGoalImage;

	vgui::CKeyRepeatHandler	m_KeyRepeat;
};


#endif // TDC_MAPINFOMENU_H
