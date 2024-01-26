#ifndef TFMAINMENUPANELBASE_H
#define TFMAINMENUPANELBASE_H

#include "tdc_controls.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCMenuPanelBase : public vgui::EditablePanel, public CAutoGameSystem
{
	DECLARE_CLASS_SIMPLE( CTDCMenuPanelBase, vgui::EditablePanel );

public:
	CTDCMenuPanelBase( vgui::Panel *parent, const char *panelName );

	virtual ~CTDCMenuPanelBase();
	virtual void SetShowSingle( bool ShowSingle );
	virtual void Show();
	virtual void Hide();

protected:
	bool				m_bShowSingle;
};

#endif // TFMAINMENUPANELBASE_H