#ifndef TFMAINMENUPAUSEPANEL_H
#define TFMAINMENUPAUSEPANEL_H

#include "tdc_menupanelbase.h"

class CTDCButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCPauseMenuPanel : public CTDCMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCPauseMenuPanel, CTDCMenuPanelBase );

public:
	CTDCPauseMenuPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTDCPauseMenuPanel();

	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Show();
	virtual void Hide();
	virtual void OnCommand( const char* command );
};

#endif // TFMAINMENUPAUSEPANEL_H