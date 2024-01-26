#ifndef TFMAINMENULOADOUTPANEL_H
#define TFMAINMENULOADOUTPANEL_H

#include "tdc_dialogpanelbase.h"
#include <vgui_controls/ComboBox.h>

class CTDCPlayerModelPanel;
class CTDCRGBPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCLoadoutPanel : public CTDCDialogPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCLoadoutPanel, CTDCDialogPanelBase );

public:
	CTDCLoadoutPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CTDCLoadoutPanel();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnCommand( const char* command );
	virtual void Hide();
	virtual void Show();

	void RecalcComboBoxes( void );

	void SetClass( int _class );
	void SetTeam( int team );

private:
	void UpdateClassModel( void );
	void UpdateClassModelItems( void );

	MESSAGE_FUNC_PTR( OnControlModified, "ControlModified", panel );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );

	CTDCPlayerModelPanel *m_pClassModelPanel;
	CTDCRGBPanel		*m_pRGBPanel;
	vgui::ComboBox *m_pWearableCombo[TDC_WEARABLE_COUNT];
	vgui::ComboBox *m_pSkinTonesCombo;

	int m_iSelectedClass;
	int m_iSelectedTeam;
};

#endif // TFMAINMENULOADOUTPANEL_H
