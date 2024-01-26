//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_HUD_PLAYERSTATUS_H
#define TDC_HUD_PLAYERSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/ImagePanel.h>
#include "tdc_controls.h"
#include "tdc_imagepanel.h"
#include "tdc_playermodelpanel.h"
#include "hudelement.h"
#include "GameEventListener.h"
#include "tdc_weaponbase.h"

//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTDCHealthPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCHealthPanel, vgui::Panel );

	CTDCHealthPanel( vgui::Panel *parent, const char *name );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void Paint();
	void SetHealth( float flHealth ){ m_flHealth = ( flHealth <= 1.0 ) ? flHealth : 1.0f; }

private:
	float	m_flHealth; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
	CPanelAnimationVar( bool, m_bLeftToRight, "leftToRight", "0" );
};

//-----------------------------------------------------------------------------
// Purpose:  Displays player health data
//-----------------------------------------------------------------------------
class CTDCHudPlayerHealth : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudPlayerHealth, EditablePanel );

public:

	CTDCHudPlayerHealth( Panel *parent, const char *name );

	virtual const char *GetResFilename( void ) { return "resource/UI/HudPlayerHealth.res"; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	void	SetHealth( int iNewHealth, int iMaxHealth, int iMaxBuffedHealth );
	void	HideHealthBonusImage( void );
	void	SetClass( int iClass );

protected:

	virtual void OnThink();

protected:
	float				m_flNextThink;

private:
	CTDCHealthPanel		*m_pHealthImage;
	vgui::ImagePanel	*m_pHealthBonusImage;
	vgui::ImagePanel	*m_pHealthImageBG;

	int					m_nHealth;
	int					m_nMaxHealth;
	int					m_iClass;

	int					m_nBonusHealthOrigX;
	int					m_nBonusHealthOrigY;
	int					m_nBonusHealthOrigW;
	int					m_nBonusHealthOrigH;

	CPanelAnimationVar( int, m_nHealthBonusPosAdj, "HealthBonusPosAdj", "25" );
	CPanelAnimationVar( float, m_flHealthDeathWarning, "HealthDeathWarning", "0.49" );
	CPanelAnimationVar( Color, m_clrHealthDeathWarningColor, "HealthDeathWarningColor", "HUDDeathWarning" );
};

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the player class/health displays
//-----------------------------------------------------------------------------
class CTDCHudPlayerStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudPlayerStatus, vgui::EditablePanel );

public:
	CTDCHudPlayerStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

private:
	CTDCHudPlayerHealth	*m_pHudPlayerHealth;
};

#endif	// TDC_HUD_PLAYERSTATUS_H