#ifndef TDC_MAINMENU_TABS_H
#define TDC_MAINMENU_TABS_H

#ifdef _WIN32
#pragma once
#endif

class CTDCButton;

class CAdvTabs : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CAdvTabs, vgui::EditablePanel );

	CAdvTabs( vgui::Panel *parent, char const *panelName );
	~CAdvTabs();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );

	void SetSelectedButton( const char *pszName );

protected:
	MESSAGE_FUNC_PTR( OnButtonPressed, "ButtonPressed", panel );

	CUtlVector<CTDCButton *> m_pButtons;
	CTDCButton *m_pCurrentButton;
};

#endif // TDC_MAINMENU_TABS_H
