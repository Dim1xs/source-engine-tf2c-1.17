#include "cbase.h"
#include "tdc_dialogpanelbase.h"
#include "tdc_mainmenu.h"
#include "controls/tdc_advbutton.h"
#include "controls/tdc_advpanellistpanel.h"
#include "controls/tdc_cvartogglecheckbutton.h"
#include "controls/tdc_cvarslider.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Tooltip.h"
#include "inputsystem/iinputsystem.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include "tier1/convar.h"
#include <stdio.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CTDCDialogPanelBase::CTDCDialogPanelBase( vgui::Panel *parent, const char *panelName ) : CTDCMenuPanelBase( parent, panelName )
{
	m_pListPanel = NULL;
	m_bEmbedded = false;
	m_bHideMainMenu = false;
	m_PassUnhandledInput = false;
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCDialogPanelBase::~CTDCDialogPanelBase()
{
	DestroyControls();
}

//-----------------------------------------------------------------------------
// Purpose: sets background color & border
//-----------------------------------------------------------------------------
void CTDCDialogPanelBase::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	if ( m_bEmbedded )
	{
		OnCreateControls();
	}
	else
	{
		//Show();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCDialogPanelBase::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_bHideMainMenu = inResourceData->GetBool( "hide_mainmenu", false );
}

void CTDCDialogPanelBase::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "vguicancel" ) )
	{
		PostActionSignal( new KeyValues( "CancelPressed" ) );
		OnResetData();
		Hide();
	}
	else if ( !V_stricmp( command, "Ok" ) )
	{
		PostActionSignal( new KeyValues( "OkPressed" ) );
		OnApplyChanges();
		Hide();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTDCDialogPanelBase::Show()
{
	BaseClass::Show();

	if ( !m_bEmbedded )
	{
		RequestFocus();
		MakePopup();

		// Fade in.
		SetAlpha( 0 );
		GetAnimationController()->RunAnimationCommand( this, "Alpha", 255, 0.05f, 0.3f, AnimationController::INTERPOLATOR_SIMPLESPLINE );

		// Offset the dialog and make it slide back into normal position.
		int x, y;
		GetPos( x, y );
		SetPos( x - YRES( 20 ), y );

		GetAnimationController()->RunAnimationCommand( this, "xpos", x, 0.0f, 0.3f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );

		guiroot->ShowPanel( SHADEBACKGROUND_MENU );

		if ( m_bHideMainMenu )
		{
			guiroot->HidePanel( guiroot->GetCurrentMainMenu() );
		}
	}
}

void CTDCDialogPanelBase::Hide()
{
	BaseClass::Hide();

	if ( !m_bEmbedded )
	{
		guiroot->HidePanel( SHADEBACKGROUND_MENU );
		guiroot->HideToolTip();
		guiroot->ShowPanel( guiroot->GetCurrentMainMenu() );

		if ( m_bShowSingle )
		{
			engine->ClientCmd( "gameui_hide" );
		}

		if ( m_bHideMainMenu )
		{
			guiroot->FadeMainMenuIn();
		}
	}
}

void CTDCDialogPanelBase::OnKeyCodeTyped( KeyCode code )
{
	if ( m_bEmbedded )
	{
		if ( code == KEY_ESCAPE )
		{
			Hide();
			return;
		}
	}

	BaseClass::OnKeyCodePressed( code );
}

void CTDCDialogPanelBase::OnKeyTyped( wchar_t key )
{
	if ( !m_bEmbedded )
	{
		Panel *pPanel = HasHotkey( key );
		if ( pPanel )
		{
			PostMessage( pPanel, new KeyValues( "Hotkey" ) );
		}
	}

	BaseClass::OnKeyTyped( key );
}

void CTDCDialogPanelBase::AddControl( Panel* panel, objtype_t iType, const char* text /*= ""*/, const char *pszToolTip /*= ""*/, Label **pLabel /*= NULL*/ )
{
	if ( !m_pListPanel )
		return;

	mpcontrol_t	*pCtrl = new mpcontrol_t( m_pListPanel, "mpcontrol_t" );
	pCtrl->type = iType;
	HFont hFont = GETSCHEME()->GetFont( "TF2CMenuNormal", true );

	switch ( pCtrl->type )
	{
	case O_CATEGORY:
	{
		Label *pTitle = assert_cast<Label*>( panel );
		pTitle->MakeReadyForUse();

		pTitle->SetFont( GETSCHEME()->GetFont( "TF2CMenuHeader", true ) );
		pTitle->SetBorder( GETSCHEME()->GetBorder( "AdvSettingsTitleBorder" ) );
		pTitle->SetFgColor( GETSCHEME()->GetColor( "TanLight", COLOR_WHITE ) );
		break;
	}
	case O_BOOL:
	{
		CTDCCheckButton *pBox = assert_cast<CTDCCheckButton*>( panel );
		pBox->MakeReadyForUse();

		pBox->SetFont( hFont );
		pBox->SetToolTip( pszToolTip );
		break;
	}
	case O_SLIDER:
	{
		CTDCSlider *pScroll = assert_cast<CTDCSlider*>( panel );
		pScroll->MakeReadyForUse();

		pScroll->SetFont( "TF2CMenuNormal" );
		pScroll->SetToolTip( pszToolTip );
		break;
	}
	case O_LIST:
	{
		ComboBox *pCombo = assert_cast<ComboBox*>( panel );
		pCombo->MakeReadyForUse();

		pCombo->SetFont( hFont );
		break;
	}
	default:
		break;
	}

	if ( pCtrl->type != O_BOOL && pCtrl->type != O_CATEGORY )
	{
		pCtrl->pPrompt = new Label( pCtrl, "DescLabel", "" );
		pCtrl->pPrompt->MakeReadyForUse();

		pCtrl->pPrompt->SetFont( hFont );
		pCtrl->pPrompt->SetContentAlignment( vgui::Label::a_west );
		pCtrl->pPrompt->SetTextInset( 5, 0 );
		pCtrl->pPrompt->SetText( text );

		if ( pLabel )
			*pLabel = pCtrl->pPrompt;
	}

	panel->SetParent( pCtrl );
	pCtrl->pControl = panel;
	int h = m_pListPanel->GetTall() / 13.0; //(float)GetParent()->GetTall() / 15.0;
	pCtrl->SetSize( 800, h );
	m_pListPanel->AddItem( pCtrl );
}

void CTDCDialogPanelBase::CreateControls()
{
	DestroyControls();
}

void CTDCDialogPanelBase::DestroyControls()
{
	if ( !m_pListPanel )
		return;

	m_pListPanel->DeleteAllItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCDialogPanelBase::OnResetData()
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCDialogPanelBase::OnApplyChanges()
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCDialogPanelBase::OnSetDefaults()
{

}
