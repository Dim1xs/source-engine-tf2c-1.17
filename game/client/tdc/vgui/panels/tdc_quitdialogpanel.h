#ifndef TFMAINMENUQUITPANEL_H
#define TFMAINMENUQUITPANEL_H

#include "tdc_dialogpanelbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCQuitDialogPanel : public CTDCDialogPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCQuitDialogPanel, CTDCDialogPanelBase );

public:
	CTDCQuitDialogPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTDCQuitDialogPanel();

	void OnCommand( const char* command );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
};

#endif // TFTESTMENU_H