//========= Copyright  1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_HUD_WEAPONDESIRE_H
#define TDC_HUD_WEAPONDESIRE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "tdc_imagepanel.h"

class CTDCWeaponInfo;

class CTDCWeaponSwitchIcon : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CTDCWeaponSwitchIcon, vgui::Panel );

	CTDCWeaponSwitchIcon( vgui::Panel *parent, const char *name );

	void SetIcon( const CHudTexture *pIcon );
	void PaintBackground( void );

private:
	const CHudTexture	*m_icon;
};

class CItemModelPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CItemModelPanel, vgui::EditablePanel );

public:
	CItemModelPanel( Panel *parent, const char* name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );

	void SetWeapon( C_BaseCombatWeapon *pWeapon, int iBorderStyle = -1, int ID = -1 );
	void SetWeapon( ETDCWeaponID iWeapon, int iBorderStyle = -1, int ID = -1 );

	void ShowQuality( bool bShow ) { m_bShowQuality = bShow; }

private:
	CHandle<C_BaseCombatWeapon> m_hWeapon;
	int					m_iBorderStyle;
	int					m_iSlot;

	CTDCWeaponSwitchIcon		*m_pWeaponIcon;

	vgui::IBorder		*m_pDefaultBorder;
	vgui::IBorder		*m_pSelectedRedBorder;
	vgui::IBorder		*m_pSelectedBlueBorder;

	bool				m_bModelOnly;
	bool				m_bShowQuality;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudWeaponSwitch : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudWeaponSwitch, vgui::EditablePanel );

public:

	CTDCHudWeaponSwitch( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void SetVisible( bool bVisible );
	void OnTick();

private:
	ETDCWeaponID m_iWeaponFrom;
	ETDCWeaponID m_iWeaponTo;

	CItemModelPanel *m_pWeaponFrom;
	CItemModelPanel *m_pWeaponTo;
};

#endif	// TDC_HUD_WEAPONDESIRE_H
