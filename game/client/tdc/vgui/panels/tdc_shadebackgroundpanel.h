#ifndef TFMAINMENUSHADEBACKGROUNDPANEL_H
#define TFMAINMENUSHADEBACKGROUNDPANEL_H

#include "tdc_menupanelbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCShadeBackgroundPanel : public CTDCMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCShadeBackgroundPanel, CTDCMenuPanelBase );

public:
	CTDCShadeBackgroundPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CTDCShadeBackgroundPanel();
	virtual void Show();
	virtual void Hide();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	vgui::ImagePanel *m_pShadedBG;
};

#endif // TFMAINMENUSHADEBACKGROUNDPANEL_H