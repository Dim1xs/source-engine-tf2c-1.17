//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tdc_optionsdialog.h"
#include "tdc_mainmenu.h"

#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/QueryBox.h"
#include "controls/tdc_advbutton.h"

#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"

#include "KeyValues.h"
#include "tdc_optionsmousepanel.h"
#include "tdc_optionskeyboardpanel.h"
#include "tdc_optionsaudiopanel.h"
#include "tdc_optionsvideopanel.h"
#include "tdc_optionsadvancedpanel.h"

#include "tdc_mainmenupanel.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CTDCOptionsDialog::CTDCOptionsDialog( vgui::Panel *parent, const char *panelName ) : CTDCDialogPanelBase( parent, panelName )
{
	m_pPanels.SetSize( PANEL_COUNT );

	AddPanel( new CTDCOptionsAdvancedPanel( this, "MultiplayerOptions" ), PANEL_ADV );
	AddPanel( new CTDCOptionsMousePanel( this, "MouseOptions" ), PANEL_MOUSE );
	AddPanel( new CTDCOptionsKeyboardPanel( this, "KeyboardOptions" ), PANEL_KEYBOARD );
	AddPanel( new CTDCOptionsAudioPanel( this, "AudioOptions" ), PANEL_AUDIO );
	AddPanel( new CTDCOptionsVideoPanel( this, "VideoOptions" ), PANEL_VIDEO );

	m_pApplyButton = new CTDCButton( this, "Apply", "" );
	m_pDefaultsButton = new CTDCButton( this, "Defaults", "" );

	m_pOptionsCurrent = PANEL_ADV;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCOptionsDialog::~CTDCOptionsDialog()
{
}

void CTDCOptionsDialog::AddPanel( CTDCDialogPanelBase *m_pPanel, int iPanel )
{
	m_pPanels[iPanel] = m_pPanel;
	m_pPanel->SetEmbedded( true );
	m_pPanel->SetVisible( false );
}

void CTDCOptionsDialog::SetCurrentPanel( OptionPanel pCurrentPanel )
{
	if ( m_pOptionsCurrent == pCurrentPanel )
		return;
	GetPanel( m_pOptionsCurrent )->Hide();
	GetPanel( pCurrentPanel )->Show();
	m_pOptionsCurrent = pCurrentPanel;

	m_pDefaultsButton->SetVisible( ( pCurrentPanel == PANEL_KEYBOARD ) );
}

CTDCDialogPanelBase*	CTDCOptionsDialog::GetPanel( int iPanel )
{
	return m_pPanels[iPanel];
}

void CTDCOptionsDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/OptionsDialog.res" );
}

void CTDCOptionsDialog::Show()
{
	BaseClass::Show();
	GetPanel( m_pOptionsCurrent )->Show();

	for ( int i = 0; i < PANEL_COUNT; i++ )
		GetPanel( i )->OnCreateControls();

	// Disable apply button, it'll get enabled once any controls change.
	m_pApplyButton->SetEnabled( false );
}

void CTDCOptionsDialog::Hide()
{
	BaseClass::Hide();
	GetPanel( m_pOptionsCurrent )->Hide();

	for ( int i = 0; i < PANEL_COUNT; i++ )
		GetPanel( i )->OnDestroyControls();
};

void CTDCOptionsDialog::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "vguicancel" ) )
	{
		OnCancelPressed();
	}
	else if ( !V_stricmp( command, "Ok" ) )
	{
		OnOkPressed();
	}
	else if ( !V_stricmp( command, "Apply" ) )
	{
		OnApplyPressed();
	}
	else if ( !V_stricmp( command, "DefaultsOK" ) )
	{
		OnDefaultPressed();
	}
	else if ( !V_stricmp( command, "newoptionsadv" ) )
	{
		SetCurrentPanel( PANEL_ADV );
	}
	else if ( !V_stricmp( command, "newoptionsmouse" ) )
	{
		SetCurrentPanel( PANEL_MOUSE );
	}
	else if ( !V_stricmp( command, "newoptionskeyboard" ) )
	{
		SetCurrentPanel( PANEL_KEYBOARD );
	}
	else if ( !V_stricmp( command, "newoptionsaudio" ) )
	{
		SetCurrentPanel( PANEL_AUDIO );
	}
	else if ( !V_stricmp( command, "newoptionsvideo" ) )
	{
		SetCurrentPanel( PANEL_VIDEO );
	}
	else if ( !V_stricmp( command, "randommusic" ) )
	{
		GET_MAINMENUPANEL( CTDCMainMenuPanel )->OnCommand( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCOptionsDialog::OnOkPressed()
{
	OnApplyPressed();
	Hide();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCOptionsDialog::OnApplyPressed()
{
	m_pApplyButton->SetEnabled( false );

	for ( int i = 0; i < PANEL_COUNT; i++ )
		GetPanel( i )->OnApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCOptionsDialog::OnCancelPressed()
{
	for ( int i = 0; i < PANEL_COUNT; i++ )
		GetPanel( i )->OnResetData();

	Hide();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCOptionsDialog::OnDefaultPressed()
{
	for ( int i = 0; i < PANEL_COUNT; i++ )
		GetPanel( i )->OnSetDefaults();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the GameUI is hidden
//-----------------------------------------------------------------------------
void CTDCOptionsDialog::OnGameUIHidden()
{
	// tell our children about it
	for ( int i = 0; i < GetChildCount(); i++ )
	{
		Panel *pChild = GetChild( i );
		if ( pChild )
		{
			PostMessage( pChild, new KeyValues( "GameUIHidden" ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable apply button when something is changed.
//-----------------------------------------------------------------------------
void CTDCOptionsDialog::OnApplyButtonEnabled( void )
{
	if ( !m_pApplyButton->IsEnabled() )
	{
		m_pApplyButton->SetEnabled( true );
	}
}
