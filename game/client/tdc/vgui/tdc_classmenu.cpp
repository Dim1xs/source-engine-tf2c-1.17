//========= Copyright  1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tdc_classmenu.h"
#include <vgui/IVGui.h>
#include "IGameUIFuncs.h" // for key bindings
#include <vgui_controls/ScrollBarSlider.h>
#include "fmtstr.h"
#include "c_tdc_player.h"
#include "c_tdc_team.h"
#include "c_tdc_playerresource.h"
#include "engine/IEngineSound.h"
#include "tdc_viewport.h"
#include "tdc_gamerules.h"
#include "tdc_mainmenu.h"
#include <vgui/ILocalize.h>

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

ConVar hud_classautokill( "hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Automatically kill player after choosing a new playerclass." );

//=============================================================================//
// CTDCClassMenu
//=============================================================================//

// menu buttons are not in the same order as the defines
static int iRemapButtonToClass[TDC_CLASS_MENU_BUTTONS] =
{
	TDC_CLASS_UNDEFINED,
	TDC_CLASS_GRUNT_LIGHT,
	TDC_CLASS_GRUNT_NORMAL,
	TDC_CLASS_GRUNT_HEAVY
};

// background music
#define TDC_CLASS_MENU_MUSIC "music.class_menu"

// hoverup sounds for each class
static const char *g_aHoverupSounds[TDC_CLASS_MENU_BUTTONS] =
{
	NULL,
	"music.class_menu_01",
	"music.class_menu_02",
	"music.class_menu_03",
};

int GetIndexForClass( int iClass )
{
	int iIndex = 0;

	for ( int i = 0; i < TDC_CLASS_MENU_BUTTONS; i++ )
	{
		if ( iRemapButtonToClass[i] == iClass )
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCClassMenu::CTDCClassMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_CLASS )
{
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

											// initialize dialog
	SetTitle( "", true );

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	SetMoveable( false );
	SetSizeable( false );

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	m_iClassMenuKey = BUTTON_CODE_INVALID;
	m_iCurrentButtonIndex = TDC_CLASS_GRUNT_NORMAL;

	memset( m_pClassModels, 0, sizeof( m_pClassModels ) );
	memset( m_pClassButtons, 0, sizeof( m_pClassButtons ) );

	// Make buttons for all normal classes.
	for ( int i = TDC_FIRST_NORMAL_CLASS; i <= TDC_LAST_NORMAL_CLASS; i++ )
	{
		char modelPanelName[128] = "model_";
		char buttonPanelName[128] = "button_";
		V_strcat_safe(modelPanelName, g_aPlayerClassNames_NonLocalized[i]);
		V_strcat_safe(buttonPanelName, g_aPlayerClassNames_NonLocalized[i]);
		m_pClassModels[i] = new CTDCPlayerModelPanel( this, modelPanelName );
		m_pClassButtons[i] = new CExButton( this, buttonPanelName, "" );
	}

	m_pCancelButton = new CExButton( this, "CancelButton", "" );
	m_pLoadoutButton = new CExButton( this, "EditLoadoutButton", "" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ClassSelection.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::PerformLayout()
{
	BaseClass::PerformLayout();

#ifndef _X360
	m_pCountLabel = dynamic_cast<CExLabel *>( FindChildByName( "CountLabel" ) );

	if ( m_pCountLabel )
	{
		m_pCountLabel->SetVisible( false );
		m_pCountLabel->SizeToContents();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCClassMenu::GetCurrentClass( void )
{
	// Default to normal grunt.
	int iClass = TDC_CLASS_GRUNT_NORMAL;
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( pLocalPlayer && pLocalPlayer->GetPlayerClass()->GetClassIndex() != TDC_CLASS_UNDEFINED )
	{
		iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();
	}

	return iClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::SetData( KeyValues *data )
{
	SetTeam( data->GetInt( "team" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::ShowPanel( bool bShow )
{
	if ( bShow == IsVisible() )
		return;

	if ( bShow )
	{
		engine->CheckPoint( "ClassMenu" );

		Activate();
		SetMouseInputEnabled( true );

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, TDC_CLASS_MENU_MUSIC );

		m_iClassMenuKey = gameuifuncs->GetButtonCodeForBind( "changeclass" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		// Set button images.
		for ( int i = TDC_FIRST_NORMAL_CLASS; i <= TDC_LAST_NORMAL_CLASS; i++ )
		{
			m_pClassButtons[i]->SetShouldPaint(true);
			m_pClassButtons[i]->SetArmedSound( NULL );
			m_pClassModels[i]->SetToPlayerClass( i );
			m_pClassModels[i]->SetTeam( m_iTeamNum );
			m_pClassModels[i]->LoadItems();
			m_pClassModels[i]->UseCvarsForTintColor( true );
		}

		// Show our current class
		SelectClass( GetCurrentClass() );
	}
	else
	{
		C_BaseEntity::StopSound( SOUND_FROM_UI_PANEL, TDC_CLASS_MENU_MUSIC );

		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( ( m_iClassMenuKey != BUTTON_CODE_INVALID && m_iClassMenuKey == code ) ||
		code == KEY_XBUTTON_BACK ||
		code == KEY_XBUTTON_B )
	{
		C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pLocalPlayer && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() != TDC_CLASS_UNDEFINED ) )
		{
			ShowPanel( false );
		}
	}
	else if ( code >= KEY_1 && code <= KEY_9 )
	{
		int iButton = ( code - KEY_1 + 1 );
		OnCommand( VarArgs( "select %d", iRemapButtonToClass[iButton] ) );
	}
	else if ( code == KEY_SPACE || code == KEY_XBUTTON_A || code == KEY_XBUTTON_RTRIGGER )
	{
		ipanel()->SendMessage( GetFocusNavGroup().GetDefaultButton(), new KeyValues( "PressButton" ), GetVPanel() );
	}
	else if ( code == KEY_XBUTTON_RIGHT || code == KEY_XSTICK1_RIGHT )
	{
		int iButton = m_iCurrentButtonIndex;
		int loopCheck = 0;

		do
		{
			loopCheck++;
			iButton++;
			iButton = ( iButton % TDC_CLASS_MENU_BUTTONS );
		} while ( ( m_pClassButtons[iRemapButtonToClass[iButton]] == NULL ) && ( loopCheck < TDC_CLASS_MENU_BUTTONS ) );

		CExButton *pButton = m_pClassButtons[iRemapButtonToClass[iButton]];
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if ( code == KEY_XBUTTON_LEFT || code == KEY_XSTICK1_LEFT )
	{
		int iButton = m_iCurrentButtonIndex;
		int loopCheck = 0;

		do
		{
			loopCheck++;
			iButton--;
			if ( iButton <= 0 )
			{
				iButton = TDC_CLASS_GRUNT_NORMAL;
			}
		} while ( ( m_pClassButtons[iRemapButtonToClass[iButton]] == NULL ) && ( loopCheck < TDC_CLASS_MENU_BUTTONS ) );

		CExButton *pButton = m_pClassButtons[iRemapButtonToClass[iButton]];
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if ( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class tips up
		//PostMessage( m_pTipsPanel, new KeyValues( "MoveScrollBarDirect", "delta", 1 ) );
	}
	else if ( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class tips down
		//PostMessage( m_pTipsPanel, new KeyValues( "MoveScrollBarDirect", "delta", -1 ) );
	}
	else if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		GetTDCViewPort()->ShowScoreboard( true, code );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	// Check which button is armed and switch to class appropriately.
	for ( int i = TDC_FIRST_NORMAL_CLASS; i < TDC_CLASS_MENU_BUTTONS; i++ )
	{
		CExButton *pButton = m_pClassButtons[i];

		if ( pButton && pButton->IsArmed() )
		{
			// Play the selection sound. Not putting it in SelectClass since we don't want to play sounds when the menu is opened.
			C_BaseEntity::StopSound( SOUND_FROM_UI_PANEL, g_aHoverupSounds[iRemapButtonToClass[m_iCurrentButtonIndex]] );

			C_RecipientFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, g_aHoverupSounds[i] );

			SelectClass( i );
			break;
		}
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::Update()
{
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( ( pLocalPlayer && pLocalPlayer->GetPlayerClass()->GetClassIndex() != TDC_CLASS_UNDEFINED ) )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", true );
		}
#else
		m_pCancelButton->SetVisible( true );
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", false );
		}
#else
		m_pCancelButton->SetVisible( false );
#endif
	}
}

//-----------------------------------------------------------------------------
// Draw nothing
//-----------------------------------------------------------------------------
void CTDCClassMenu::PaintBackground( void )
{
}

//-----------------------------------------------------------------------------
// Do things that should be done often, eg number of players in the 
// selected class
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnTick( void )
{
	//When a player changes teams, their class and team values don't get here 
	//necessarily before the command to update the class menu. This leads to the cancel button 
	//being visible and people cancelling before they have a class. check for class == TF_CLASS_UNDEFINED and if so
	//hide the cancel button

	if ( !IsVisible() )
		return;

#ifndef _X360
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( pLocalPlayer && pLocalPlayer->GetPlayerClass()->GetClassIndex() == TDC_CLASS_UNDEFINED )
	{
		m_pCancelButton->SetVisible( false );
	}

#endif

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnClose()
{
	ShowPanel( false );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_HEALTH;
	}

	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	m_KeyRepeat.Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnCommand( const char *command )
{
	if ( V_strnicmp( command, "select ", 7 ) == 0 )
	{
		int iClass = atoi( command + 7 );
		if ( iClass < TDC_CLASS_COUNT_ALL )
		{
			engine->ClientCmd( VarArgs( "joinclass %s", g_aPlayerClassNames_NonLocalized[iClass] ) );
		}
	}
	else if ( V_stricmp( command, "vguicancel" ) != 0 )
	{
		engine->ClientCmd( command );
	}

	Close();

	gViewPortInterface->ShowBackGround( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClassMenu::SetTeam( int iTeam )
{
	// If we're in Arena mode then class menu may be opened while we're in spec. In that case, show RED menu.
	if ( iTeam < FIRST_GAME_TEAM )
	{
		m_iTeamNum = TDC_TEAM_RED;
	}
	else
	{
		m_iTeamNum = iTeam;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show the model and tips for this class.
//-----------------------------------------------------------------------------
void CTDCClassMenu::SelectClass( int iClass )
{
	m_iCurrentButtonIndex = GetIndexForClass( iClass );

	// Select the button for this class and unselect all other ones.
	for ( int i = TDC_FIRST_NORMAL_CLASS; i < TDC_CLASS_MENU_BUTTONS; i++ )
	{
		CExButton *pButton = m_pClassButtons[i];
		if ( pButton )
		{
			pButton->SetSelected( i == iClass );

			if ( i == iClass )
			{
				pButton->SetAsDefaultButton( 1 );
			}
		}
	}

	// Hide the loadout button if Random button is selected.
	m_pLoadoutButton->SetVisible( iClass <= TDC_LAST_NORMAL_CLASS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnShowToTeam( int iTeam )
{
	SetTeam( iTeam );
	gViewPortInterface->ShowPanel( this, true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCClassMenu::OnLoadoutChanged( void )
{
	if ( IsVisible() )
	{
		// Reselect the current class, so the model's items are updated.
		SelectClass( iRemapButtonToClass[m_iCurrentButtonIndex] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Console command to select a class
//-----------------------------------------------------------------------------
void CTDCClassMenu::Join_Class( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		OnCommand( VarArgs( "joinclass %s", args.Arg( 1 ) ) );
	}
}
