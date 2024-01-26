#ifndef TFMAINMENU_H
#define TFMAINMENU_H

#include <vgui_controls/EditablePanel.h>

struct ClassStats_t;

enum MenuPanel //position in this enum = zpos on the screen
{
	NONE_MENU,
	BACKGROUND_MENU,
	MAIN_MENU,
	PAUSE_MENU,
	SHADEBACKGROUND_MENU, //add popup/additional menus below:		
	LOADOUT_MENU,
	OPTIONSDIALOG_MENU,
	CREATESERVER_MENU,
	TOS_MENU,
	QUIT_MENU,
	TOOLTIP_MENU,
	COUNT_MENU,

	FIRST_MENU = NONE_MENU + 1
};

#define GET_MAINMENUPANEL( className ) assert_cast<className*>(guiroot->GetMenuPanel(#className))

class CTDCMenuPanelBase;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCMainMenu : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCMainMenu, vgui::EditablePanel );

public:
	CTDCMainMenu();
	virtual ~CTDCMainMenu();

	enum
	{
		TFMAINMENU_STATUS_UNDEFINED = 0,
		TFMAINMENU_STATUS_MENU,
		TFMAINMENU_STATUS_BACKGROUNDMAP,
		TFMAINMENU_STATUS_INGAME,
	};

	CTDCMenuPanelBase* GetMenuPanel( int iPanel );
	CTDCMenuPanelBase* GetMenuPanel( const char *name );
	MenuPanel GetCurrentMainMenu( void ) { return ( !IsInLevel() ? MAIN_MENU : PAUSE_MENU ); }
	int GetMainMenuStatus( void ) { return m_iMainMenuStatus; }

	virtual void OnTick();

	void ShowPanel( MenuPanel iPanel, bool m_bShowSingle = false );
	void HidePanel( MenuPanel iPanel );
	void InvalidatePanelsLayout( bool layoutNow = false, bool reloadScheme = false );
	void LaunchInvalidatePanelsLayout();
	bool IsInLevel();
	bool IsInBackgroundLevel();
	void UpdateCurrentMainMenu();
	void SetStats( CUtlVector<ClassStats_t> &vecClassStats );
	void ShowToolTip( const char *pszText );
	void HideToolTip();
	void FadeMainMenuIn();

private:
	CTDCMenuPanelBase	*m_pPanels[COUNT_MENU];
	void				AddMenuPanel( CTDCMenuPanelBase *m_pPanel, int iPanel );

	int					m_iMainMenuStatus;
	int					m_iUpdateLayout;
};

extern CTDCMainMenu *guiroot;

#endif // TFMAINMENU_H