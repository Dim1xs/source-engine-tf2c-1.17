//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_HUD_AMMOSTATUS_H
#define TDC_HUD_AMMOSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tdc_controls.h"

//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTDCAmmoBar : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCAmmoBar, vgui::Panel );

	CTDCAmmoBar( vgui::Panel *parent, const char *name );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void Paint();
	void SetAmmo( float flAmmo ) { m_flAmmo = clamp( flAmmo, 0.0f, 1.0f ); }

private:
	CPanelAnimationVar( float, m_flAmmoWarning, "AmmoWarning", "0.49" );
	CPanelAnimationVar( Color, m_clrAmmoWarningColor, "AmmoWarningColor", "HUDDeathWarning" );

	float	m_flAmmo; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
};

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTDCHudWeaponAmmo : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudWeaponAmmo, vgui::EditablePanel );

public:

	CTDCHudWeaponAmmo( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip );

private:
	float							m_flNextThink;

	int								m_iClass;
	CHandle<C_BaseCombatWeapon>		m_hCurrentActiveWeapon;
	int								m_nAmmo;
	int								m_nAmmo2;

	CExLabel						*m_pInClip;
	CExLabel						*m_pInClipShadow;
	CExLabel						*m_pInReserve;
	CExLabel						*m_pInReserveShadow;
	CExLabel						*m_pNoClip;
	CExLabel						*m_pNoClipShadow;
	vgui::ImagePanel				*m_pLowAmmoImage;
	CTDCAmmoBar						*m_pAmmoBar;
	vgui::ImagePanel				*m_pAmmoIcon;

	int								m_iLowAmmoX;
	int								m_iLowAmmoY;
	int								m_iLowAmmoWide;
	int								m_iLowAmmoTall;

	CPanelAnimationVar( int, m_nAmmoWarningPosAdj, "AmmoWarningPosAdj", "25" );
	CPanelAnimationVar( float, m_flAmmoWarning, "AmmoWarning", "0.49" );
	CPanelAnimationVar( Color, m_clrAmmoWarningColor, "AmmoWarningColor", "HUDDeathWarning" );
};

#endif	// TDC_HUD_AMMOSTATUS_H