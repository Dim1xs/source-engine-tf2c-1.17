#ifndef TDC_TOSPANEL_H
#define TDC_TOSPANEL_H

#include "tdc_dialogpanelbase.h"

#define TDC_TOS_VERSION 2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCTOSPanel : public CTDCDialogPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCTOSPanel, CTDCDialogPanelBase );

public:
	CTDCTOSPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTDCTOSPanel();

	void OnCommand( const char* command );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnKeyCodeTyped( vgui::KeyCode code );
};

extern ConVar tdc_accepted_tos;

#endif // TDC_TOSPANEL_H
