#include "cbase.h"
#include "tdc_pausemenupanel.h"
#include "controls/tdc_advbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCPauseMenuPanel::CTDCPauseMenuPanel( vgui::Panel* parent, const char *panelName ) : CTDCMenuPanelBase( parent, panelName )
{
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCPauseMenuPanel::~CTDCPauseMenuPanel()
{

}

void CTDCPauseMenuPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/PauseMenuPanel.res" );
}

void CTDCPauseMenuPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CTDCPauseMenuPanel::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "newquit" ) )
	{
		guiroot->ShowPanel( QUIT_MENU );
	}
	else if ( !V_stricmp( command, "newoptionsdialog" ) )
	{
		guiroot->ShowPanel( OPTIONSDIALOG_MENU );
	}
	else if ( !V_stricmp( command, "newloadout" ) )
	{
		guiroot->ShowPanel( LOADOUT_MENU );
	}
	else if ( !V_stricmp( command, "newcreateserver" ) )
	{
		guiroot->ShowPanel( CREATESERVER_MENU );
	}
	else if ( !V_stricmp( command, "newstats" ) )
	{
		//guiroot->ShowPanel( STATSUMMARY_MENU );
	}
	else if ( !V_stricmp( command, "callvote" ) )
	{
		engine->ClientCmd( "gameui_hide" );
		engine->ClientCmd( command );
	}
	else if ( V_stristr( command, "gamemenucommand " ) )
	{
		engine->ClientCmd( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTDCPauseMenuPanel::Show()
{
	BaseClass::Show();

	RequestFocus();
}

void CTDCPauseMenuPanel::Hide()
{
	BaseClass::Hide();
}
