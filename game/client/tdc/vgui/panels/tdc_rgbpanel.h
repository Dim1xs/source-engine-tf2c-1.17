#ifndef TFMAINMENURGBPANEL_H
#define TFMAINMENURGBPANEL_H

#include <vgui_controls/EditablePanel.h>
#include "tdc_shareddefs.h"

class CTDCCvarSlider;
class CCvarComboBox;
class CTDCAdvModelPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCRGBPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCRGBPanel, vgui::EditablePanel );

public:
	CTDCRGBPanel( vgui::Panel *parent, const char *panelName );

	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	MESSAGE_FUNC_PTR( OnControlModified, "ControlModified", panel );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );

	CTDCCvarSlider	*m_pRedScrollBar;
	CTDCCvarSlider	*m_pGrnScrollBar;
	CTDCCvarSlider	*m_pBluScrollBar;
	vgui::ComboBox *m_pColorCombo;
	vgui::ImagePanel *m_pColorBG;
	vgui::ComboBox *m_pAnimCombo;
};

#endif // TFMAINMENURGBPANEL_H