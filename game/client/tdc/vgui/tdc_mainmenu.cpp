#include "cbase.h"
#include "tdc_mainmenu.h"

#include "panels/tdc_mainmenupanel.h"
#include "panels/tdc_pausemenupanel.h"
#include "panels/tdc_backgroundpanel.h"
#include "panels/tdc_loadoutpanel.h"
#include "panels/tdc_shadebackgroundpanel.h"
#include "panels/tdc_optionsdialog.h"
#include "panels/tdc_quitdialogpanel.h"
#include "panels/tdc_tooltippanel.h"
#include "panels/tdc_createserverdialog.h"
#include "panels/tdc_tospanel.h"
#include "engine/IEngineSound.h"
#include "tdc_hud_statpanel.h"
#include "ienginevgui.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CTDCMainMenu *guiroot = NULL;

CON_COMMAND( tdc_mainmenu_reload, "Reload Main Menu" )
{
	if ( guiroot )
	{
		guiroot->InvalidatePanelsLayout( true, true );
	}
}

CON_COMMAND( showloadout, "Show loadout screen (new)" )
{
	if ( !guiroot )
		return;

	engine->ClientCmd( "gameui_activate" );
	guiroot->ShowPanel( LOADOUT_MENU, true );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCMainMenu::CTDCMainMenu() : EditablePanel( NULL, "MainMenu" )
{
	guiroot = this;

	SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	SetScheme( scheme()->LoadSchemeFromFile( "resource/ClientScheme_tf2c.res", "ClientScheme_tf2c" ) );

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	SetDragEnabled( false );
	SetShowDragHelper( false );
	SetProportional( true );
	SetVisible( true );

	int width, height;
	surface()->GetScreenSize( width, height );
	SetSize( width, height );
	SetPos( 0, 0 );

	memset( m_pPanels, 0, sizeof( m_pPanels ) );
	AddMenuPanel( new CTDCMainMenuPanel( this, "CTDCMainMenuPanel" ), MAIN_MENU );
	AddMenuPanel( new CTDCPauseMenuPanel( this, "CTDCPauseMenuPanel" ), PAUSE_MENU );
	AddMenuPanel( new CTDCBackgroundPanel( this, "CTDCBackgroundPanel" ), BACKGROUND_MENU );
	AddMenuPanel( new CTDCLoadoutPanel( this, "CTDCLoadoutPanel" ), LOADOUT_MENU );
	AddMenuPanel( new CTDCShadeBackgroundPanel( this, "CTDCShadeBackgroundPanel" ), SHADEBACKGROUND_MENU );
	AddMenuPanel( new CTDCQuitDialogPanel( this, "CTDCQuitDialogPanel" ), QUIT_MENU );
	AddMenuPanel( new CTDCOptionsDialog( this, "CTDCOptionsDialog" ), OPTIONSDIALOG_MENU );
	AddMenuPanel( new CTDCCreateServerDialog( this, "CTDCCreateServerDialog" ), CREATESERVER_MENU );
	AddMenuPanel( new CTDCTOSPanel( this, "CTDCTOSPanel" ), TOS_MENU );
	AddMenuPanel( new CTDCToolTipPanel( this, "CTDCToolTipPanel" ), TOOLTIP_MENU );

	m_iMainMenuStatus = TFMAINMENU_STATUS_UNDEFINED;
	m_iUpdateLayout = 1;

	ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCMainMenu::~CTDCMainMenu()
{
	guiroot = NULL;
}

void CTDCMainMenu::AddMenuPanel( CTDCMenuPanelBase *pPanel, int iPanel )
{
	m_pPanels[iPanel] = pPanel;
	pPanel->SetZPos( iPanel );
	pPanel->SetVisible( false );
}

CTDCMenuPanelBase* CTDCMainMenu::GetMenuPanel( int iPanel )
{
	return m_pPanels[iPanel];
}

CTDCMenuPanelBase* CTDCMainMenu::GetMenuPanel( const char *name )
{
	for ( int i = FIRST_MENU; i < COUNT_MENU; i++ )
	{
		CTDCMenuPanelBase* pMenu = GetMenuPanel( i );
		if ( pMenu && ( Q_strcmp( pMenu->GetName(), name ) == 0 ) )
		{
			return pMenu;
		}
	}
	return NULL;
}

void CTDCMainMenu::ShowPanel( MenuPanel iPanel, bool m_bShowSingle /*= false*/ )
{
	GetMenuPanel( iPanel )->SetShowSingle( m_bShowSingle );
	GetMenuPanel( iPanel )->Show();
	if ( m_bShowSingle )
	{
		GetMenuPanel( guiroot->GetCurrentMainMenu() )->Hide();
	}
}

void CTDCMainMenu::HidePanel( MenuPanel iPanel )
{
	GetMenuPanel( iPanel )->Hide();
}

void CTDCMainMenu::InvalidatePanelsLayout( bool layoutNow, bool reloadScheme )
{
	for ( int i = FIRST_MENU; i < COUNT_MENU; i++ )
	{
		if ( GetMenuPanel( i ) )
		{
			bool bVisible = GetMenuPanel( i )->IsVisible();
			GetMenuPanel( i )->InvalidateLayout( layoutNow, reloadScheme );
			GetMenuPanel( i )->SetVisible( bVisible );
		}
	}
	
	UpdateCurrentMainMenu();
}

void CTDCMainMenu::LaunchInvalidatePanelsLayout()
{
	m_iUpdateLayout = 4;
}

void CTDCMainMenu::OnTick()
{
	// Don't draw during loading.
	SetVisible( !engine->IsDrawingLoadingImage() );

	int iStatus;

	// HACK to get rid of the main menu after changing the game resolution -DAN_H
	if (GetMenuPanel(OPTIONSDIALOG_MENU)->IsVisible())
	{
		HidePanel( MAIN_MENU );
		ShowPanel( SHADEBACKGROUND_MENU );
	}

	const char *levelName = engine->GetLevelName();
	if ( levelName && levelName[0] )
	{
		iStatus = engine->IsLevelMainMenuBackground() ? TFMAINMENU_STATUS_BACKGROUNDMAP : TFMAINMENU_STATUS_INGAME;
	}
	else
	{
		iStatus = TFMAINMENU_STATUS_MENU;
	}

	if ( iStatus != m_iMainMenuStatus )
	{
		m_iMainMenuStatus = iStatus;
		UpdateCurrentMainMenu();
	}

	if ( m_iUpdateLayout > 0 )
	{
		m_iUpdateLayout--;
		if ( !m_iUpdateLayout )
		{
			InvalidatePanelsLayout( true, true );
		}
	}
}

bool CTDCMainMenu::IsInLevel()
{
	const char *levelName = engine->GetLevelName();
	if ( levelName && levelName[0] && !engine->IsLevelMainMenuBackground() )
	{
		return true;
	}
	return false;
}

bool CTDCMainMenu::IsInBackgroundLevel()
{
	const char *levelName = engine->GetLevelName();
	if ( levelName && levelName[0] && engine->IsLevelMainMenuBackground() )
	{
		return true;
	}
	return false;
}

void CTDCMainMenu::UpdateCurrentMainMenu()
{
	switch ( m_iMainMenuStatus )
	{
	case TFMAINMENU_STATUS_MENU:
		// Show Main Menu and Video BG.
		m_pPanels[MAIN_MENU]->SetVisible( true );
		m_pPanels[PAUSE_MENU]->SetVisible( false );
		m_pPanels[BACKGROUND_MENU]->SetVisible( true );

//#ifdef STAGING_ONLY
		if ( tdc_accepted_tos.GetInt() < TDC_TOS_VERSION )
		{
			guiroot->ShowPanel( TOS_MENU );
			((CTDCMainMenuPanel *)m_pPanels[MAIN_MENU])->HideFakeIntro();
		}
//#endif
		break;
	case TFMAINMENU_STATUS_INGAME:
		// Show Pause Menu.
		m_pPanels[MAIN_MENU]->SetVisible( false );
		m_pPanels[PAUSE_MENU]->SetVisible( true );
		m_pPanels[BACKGROUND_MENU]->SetVisible( false );
		break;
	case TFMAINMENU_STATUS_BACKGROUNDMAP:
		// Show Main Menu without Video BG.
		m_pPanels[MAIN_MENU]->SetVisible( true );
		m_pPanels[PAUSE_MENU]->SetVisible( false );
		m_pPanels[BACKGROUND_MENU]->SetVisible( false );
		break;
	case TFMAINMENU_STATUS_UNDEFINED:
	default:
		Assert( false );
		m_pPanels[MAIN_MENU]->SetVisible( false );
		m_pPanels[PAUSE_MENU]->SetVisible( false );
		m_pPanels[BACKGROUND_MENU]->SetVisible( false );
		break;
	}

	m_pPanels[GetCurrentMainMenu()]->RequestFocus();
}

void CTDCMainMenu::SetStats( CUtlVector<ClassStats_t> &vecClassStats )
{
	//GET_MAINMENUPANEL( CTDCStatsSummaryDialog )->SetStats( vecClassStats );
}

void CTDCMainMenu::ShowToolTip( const char *pszText )
{
	GET_MAINMENUPANEL( CTDCToolTipPanel )->ShowToolTip( pszText );
}

void CTDCMainMenu::HideToolTip()
{
	GET_MAINMENUPANEL( CTDCToolTipPanel )->HideToolTip();
}

void CTDCMainMenu::FadeMainMenuIn( void )
{
	CTDCMenuPanelBase *pMenu = m_pPanels[GetCurrentMainMenu()];
	pMenu->Show();
	pMenu->SetAlpha( 0 );
	GetAnimationController()->RunAnimationCommand( pMenu, "Alpha", 255, 0.05f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE );
}
