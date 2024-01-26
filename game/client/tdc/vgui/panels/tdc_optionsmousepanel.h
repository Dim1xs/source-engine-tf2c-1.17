//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONS_SUB_MOUSE_H
#define OPTIONS_SUB_MOUSE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_dialogpanelbase.h"

class CPanelListPanel;
class CTDCButton;
class CTDCCvarToggleCheckButton;
class CTDCCvarSlider;

namespace vgui
{
    class Label;
    class Panel;
}

//-----------------------------------------------------------------------------
// Purpose: Mouse Details, Part of OptionsDialog
//-----------------------------------------------------------------------------
class CTDCOptionsMousePanel : public CTDCDialogPanelBase
{
	DECLARE_CLASS_SIMPLE(CTDCOptionsMousePanel, CTDCDialogPanelBase);

public:
	CTDCOptionsMousePanel(vgui::Panel* parent, const char *panelName);
	~CTDCOptionsMousePanel();
	virtual void OnResetData();
	virtual void OnApplyChanges();
	virtual void OnCommand(const char *command);
protected:
    void ApplySchemeSettings(vgui::IScheme *pScheme);
	void CreateControls();
	void DestroyControls();
	void UpdateSensitivityLabel();
	void UpdatePanels();

private:
	MESSAGE_FUNC( OnControlModified, "ControlModified" );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

	vgui::Label					*pTitleMouse;
	vgui::Label					*pTitleJoystick;
	CTDCCvarToggleCheckButton		*m_pReverseMouseCheckBox;
	CTDCCvarToggleCheckButton		*m_pRawInputCheckBox;
	CTDCCvarToggleCheckButton		*m_pMouseFilterCheckBox;

	CTDCCvarSlider					*m_pMouseSensitivitySlider;
    vgui::TextEntry             *m_pMouseSensitivityLabel;

	CTDCCvarToggleCheckButton		*m_pMouseAccelCheckBox;
	CTDCCvarSlider					*m_pMouseAccelSlider;

	CTDCCvarToggleCheckButton		*m_pJoystickCheckBox;
	CTDCCvarToggleCheckButton		*m_pJoystickSouthpawCheckBox;
	CTDCCvarToggleCheckButton		*m_pQuickInfoCheckBox;
	CTDCCvarToggleCheckButton		*m_pReverseJoystickCheckBox;

	CTDCCvarSlider					*m_pJoyYawSensitivitySlider;
	vgui::Label					*m_pJoyYawSensitivityPreLabel;
	CTDCCvarSlider					*m_pJoyPitchSensitivitySlider;
	vgui::Label					*m_pJoyPitchSensitivityPreLabel;
};



#endif // OPTIONS_SUB_MOUSE_H