#include "cbase.h"
#include "tdc_quitdialogpanel.h"
#include "tdc_mainmenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCQuitDialogPanel::CTDCQuitDialogPanel( vgui::Panel* parent, const char *panelName ) : CTDCDialogPanelBase( parent, panelName )
{

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCQuitDialogPanel::~CTDCQuitDialogPanel()
{

}

void CTDCQuitDialogPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/QuitDialogPanel.res" );
}

void CTDCQuitDialogPanel::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "quitconfirm" ) )
	{
		engine->ClientCmd( "quit" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}
