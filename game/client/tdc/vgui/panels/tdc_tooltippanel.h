#ifndef TDC_MAINMENUTOOLTIPPANEL_H
#define TDC_MAINMENUTOOLTIPPANEL_H

#include "tdc_dialogpanelbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCToolTipPanel : public CTDCMenuPanelBase
{
	DECLARE_CLASS_SIMPLE(CTDCToolTipPanel, CTDCMenuPanelBase);

public:
	CTDCToolTipPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTDCToolTipPanel();
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink( void );
	virtual void Show( void );
	virtual void Hide( void );
	virtual void ShowToolTip( const char *pszText );
	virtual void HideToolTip( void );
	virtual void AdjustToolTipSize( void );

	virtual const char *GetResFilename( void ) { return "resource/UI/main_menu/ToolTipPanel.res"; }

protected:
	CExLabel	*m_pText;
};

#endif // TDC_MAINMENUTOOLTIPPANEL_H