#include "cbase.h"
#include "tdc_tospanel.h"
#include "tdc_mainmenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tdc_accepted_tos( "tdc_accepted_tos", "0", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCTOSPanel::CTDCTOSPanel( vgui::Panel* parent, const char *panelName ) : CTDCDialogPanelBase( parent, panelName )
{

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCTOSPanel::~CTDCTOSPanel()
{

}

void CTDCTOSPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/TOSPanel.res" );
}

void CTDCTOSPanel::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "accept" ) )
	{
		tdc_accepted_tos.SetValue( TDC_TOS_VERSION );
		BaseClass::OnCommand( "Ok" );
	}
	else if ( !V_stricmp( command, "decline" ) )
	{
		engine->ClientCmd( "quit" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTDCTOSPanel::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		// Block ESC key press.
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}
