#ifndef TDC_MAINMENU_SCROLLBAR_H
#define TDC_MAINMENU_SCROLLBAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_advbuttonbase.h"
#include "tdc_imagepanel.h"

class CTDCScrollButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCSlider : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCSlider, vgui::EditablePanel );

	CTDCSlider( vgui::Panel *parent, const char *panelName );
	~CTDCSlider();

	virtual void Init();
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnThink( void );

	void SetFont( vgui::HFont font );
	void SetFont( const char *pszFont );
	float GetValue();
	const char *GetFinalValue();
	void SetPercentage( float fPerc, bool bInstant = false );
	void SetValue( float flValue );
	void SetRange( float flMin, float flMax );
	bool IsVertical() { return m_bVertical; }

	void SendSliderMovedMessage();
	void SendSliderDragStartMessage();
	void SendSliderDragEndMessage();

	void SetToolTip( const char *pszText );

	CTDCScrollButton *GetButton() { return m_pButton; };

	void ClampValue();

protected:
	CTDCScrollButton	*m_pButton;
	vgui::Label			*m_pValueLabel;
	vgui::EditablePanel *m_pBGBorder;

	char			m_szFont[64];

	float			m_flMinValue;
	float			m_flMaxValue;
	float			m_flValue;
	bool			m_bValueVisible;
	bool			m_bVertical;
	bool			m_bShowFrac;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCScrollButton : public CTDCButtonBase
{
public:
	DECLARE_CLASS_SIMPLE( CTDCScrollButton, CTDCButtonBase );

	CTDCScrollButton( vgui::Panel *parent, const char *panelName, const char *text );

	void ApplySettings( KeyValues *inResourceData );
	void ApplySchemeSettings( vgui::IScheme *pScheme );
	void PerformLayout();
	void OnCursorExited();
	void OnCursorEntered();
	void OnMousePressed( vgui::MouseCode code );
	void OnMouseReleased( vgui::MouseCode code );
	void SetMouseEnteredState( MouseState flag );
	void SetSliderPanel( CTDCSlider *m_pButton ) { m_pSliderPanel = m_pButton; };

private:
	CTDCSlider *m_pSliderPanel;
};


#endif // TDC_MAINMENU_SCROLLBAR_H
