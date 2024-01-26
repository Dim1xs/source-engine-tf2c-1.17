//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tdc_teammenu.h"
#include "iclientmode.h"
#include "basemodelpanel.h"
#include <vgui_controls/AnimationController.h>
#include "IGameUIFuncs.h" // for key bindings
#include "tdc_gamerules.h"
#include "c_team.h"
#include "tdc_viewport.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTDCTeamButton );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCTeamButton::CTDCTeamButton( vgui::Panel *parent, const char *panelName ) : CExButton( parent, panelName, "" )
{
	m_szModelPanel[0] = '\0';
	m_iTeam = TEAM_UNASSIGNED;
	m_flHoverTimeToWait = -1;
	m_flHoverTime = -1;
	m_bMouseEntered = false;
	m_bTeamDisabled = false;

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	V_strcpy_safe( m_szModelPanel, inResourceData->GetString( "associated_model", "" ) );
	m_iTeam = inResourceData->GetInt( "team", TEAM_UNASSIGNED );
	m_flHoverTimeToWait = inResourceData->GetFloat( "hover", -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetDefaultColor( GetFgColor(), Color( 0, 0, 0, 0 ) );
	SetArmedColor( GetButtonFgColor(), Color( 0, 0, 0, 0 ) );
	SetDepressedColor( GetButtonFgColor(), Color( 0, 0, 0, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::SendAnimation( const char *pszAnimation )
{
	if ( !m_szModelPanel[0] )
		return;

	Panel *pParent = GetParent();
	if ( pParent )
	{
		CModelPanel *pModel = dynamic_cast<CModelPanel*>( pParent->FindChildByName( m_szModelPanel ) );
		if ( pModel )
		{
			KeyValues *kvParms = new KeyValues( "SetAnimation" );
			if ( kvParms )
			{
				kvParms->SetString( "animation", pszAnimation );
				PostMessage( pModel, kvParms );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::SetDefaultAnimation( const char *pszName )
{
	if ( !m_szModelPanel[0] )
		return;

	Panel *pParent = GetParent();
	if ( pParent )
	{
		CModelPanel *pModel = dynamic_cast<CModelPanel*>( pParent->FindChildByName( m_szModelPanel ) );
		if ( pModel )
		{
			pModel->SetDefaultAnimation( pszName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCTeamButton::IsTeamFull()
{
	return ( m_iTeam > TEAM_UNASSIGNED &&
		TDCGameRules() &&
		TDCGameRules()->WouldChangeUnbalanceTeams( m_iTeam, GetLocalPlayerTeam() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	SetMouseEnteredState( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::OnCursorExited()
{
	BaseClass::OnCursorExited();

	SetMouseEnteredState( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::SetMouseEnteredState( bool state )
{
	if ( state )
	{
		m_bMouseEntered = true;

		if ( m_flHoverTimeToWait > 0 )
		{
			m_flHoverTime = gpGlobals->curtime + m_flHoverTimeToWait;
		}
		else
		{
			m_flHoverTime = -1;
		}

		if ( m_bTeamDisabled )
		{
			SendAnimation( "enter_disabled" );
		}
		else
		{
			SendAnimation( "enter_enabled" );
		}
	}
	else
	{
		m_bMouseEntered = false;
		m_flHoverTime = -1;

		if ( m_bTeamDisabled )
		{
			SendAnimation( "exit_disabled" );
		}
		else
		{
			SendAnimation( "exit_enabled" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::OnTick()
{
	// check to see if our state has changed
	bool bDisabled = IsTeamFull();

	if ( bDisabled != m_bTeamDisabled )
	{
		m_bTeamDisabled = bDisabled;

		if ( m_bMouseEntered )
		{
			// something has changed, so reset our state
			SetMouseEnteredState( true );
		}
		else
		{
			// the mouse isn't currently over the button, but we should update the status
			if ( m_bTeamDisabled )
			{
				SendAnimation( "idle_disabled" );
			}
			else
			{
				SendAnimation( "idle_enabled" );
			}
		}
	}

	if ( ( m_flHoverTime > 0 ) && ( m_flHoverTime < gpGlobals->curtime ) )
	{
		m_flHoverTime = -1;

		if ( m_bTeamDisabled )
		{
			SendAnimation( "hover_disabled" );
		}
		else
		{
			SendAnimation( "hover_enabled" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamButton::FireActionSignal( void )
{
	// Don't fire the command if the button is disabled.
	if ( m_bTeamDisabled )
		return;

	BaseClass::FireActionSignal();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCTeamMenu::CTDCTeamMenu( IViewPort *pViewPort, const char *pName ) : BaseClass( NULL, pName )
{
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeamMenuKey = BUTTON_CODE_INVALID;

	// initialize dialog
	SetTitle( "", true );

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	SetMoveable( false );
	SetSizeable( false );

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional( true );

	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	memset( m_pTeamButtons, 0, sizeof( m_pTeamButtons ) );

	m_pSpecLabel = new CExLabel( this, "TeamMenuSpectate", "" );

	m_pHighlanderLabel = new CExLabel( this, "HighlanderLabel", "" );
	m_pHighlanderLabelShadow = new CExLabel( this, "HighlanderLabelShadow", "" );
	m_pTeamFullLabel = new CExLabel( this, "TeamsFullLabel", "" );
	m_pTeamFullLabelShadow = new CExLabel( this, "TeamsFullLabelShadow", "" );

	m_pTeamsFullArrow = new CTDCImagePanel( this, "TeamsFullArrow" );

#ifdef _X360
	m_pFooter = new CTDCFooter( this, "Footer" );
#else
	m_pCancelButton = new CExButton( this, "CancelButton", "#TDC_Cancel" );
#endif

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCTeamMenu::~CTDCTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
void CTDCTeamMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFilename() );

	SetHighlanderTeamsFullPanels( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamMenu::CreateTeamButtons( void )
{
	// Using Unassigned for Auto Team button.
	m_pTeamButtons[TEAM_UNASSIGNED] = new CTDCTeamButton( this, "teambutton2" );
	m_pTeamButtons[TEAM_SPECTATOR] = new CTDCTeamButton( this, "teambutton3" );
	m_pTeamButtons[TDC_TEAM_RED] = new CTDCTeamButton( this, "teambutton0" );
	m_pTeamButtons[TDC_TEAM_BLUE] = new CTDCTeamButton( this, "teambutton1" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamMenu::ShowPanel( bool bShow )
{
	if ( bShow == IsVisible() )
		return;

	if ( !gameuifuncs || !gViewPortInterface || !engine )
		return;

	if ( bShow )
	{
		engine->CheckPoint( "TeamMenu" );

		Activate();
		SetMouseInputEnabled( true );

		// get key bindings if shown
		m_iTeamMenuKey = gameuifuncs->GetButtonCodeForBind( "changeteam" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		// Highlight the button matching the player's team.
		int iTeam = GetLocalPlayerTeam();

		if ( iTeam < FIRST_GAME_TEAM )
		{
			iTeam = TEAM_UNASSIGNED;
		}

		if ( m_pTeamButtons[iTeam] )
		{
			if ( IsConsole() )
			{
				m_pTeamButtons[iTeam]->OnCursorEntered();
				m_pTeamButtons[iTeam]->SetDefaultAnimation( "enter_enabled" );
			}
			GetFocusNavGroup().SetCurrentFocus( m_pTeamButtons[iTeam]->GetVPanel(), m_pTeamButtons[iTeam]->GetVPanel() );
		}
	}
	else
	{
		SetHighlanderTeamsFullPanels( false );

		SetVisible( false );
		SetMouseInputEnabled( false );

		if ( IsConsole() )
		{
			// Close the door behind us
			CTDCTeamButton *pButton = dynamic_cast<CTDCTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
			if ( pButton )
			{
				pButton->OnCursorExited();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamMenu::SetHighlanderTeamsFullPanels( bool bEnabled )
{
	bool bTeamsDisabled = true;

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		C_Team *pTeam = GetGlobalTeam( i );
		if ( pTeam && pTeam->Get_Number_Players() < 9 )
		{
			bTeamsDisabled = false;
			break;
		}
	}

	m_pHighlanderLabel->SetVisible( bEnabled );
	m_pHighlanderLabelShadow->SetVisible( bEnabled );

	m_pTeamFullLabel->SetVisible( bTeamsDisabled && bEnabled );
	m_pTeamFullLabelShadow->SetVisible( bTeamsDisabled && bEnabled );

	ConVarRef mp_allowspectators( "mp_allowspectators" );
	if ( bEnabled && mp_allowspectators.IsValid() && mp_allowspectators.GetBool() && bTeamsDisabled )
	{
		m_pTeamsFullArrow->SetVisible( true );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pTeamsFullArrow, "TeamsFullArrowAnimate" );
	}
	else
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pTeamsFullArrow, "TeamsFullArrowAnimateEnd" );
		m_pTeamsFullArrow->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: called to update the menu with new information
//-----------------------------------------------------------------------------
void CTDCTeamMenu::Update( void )
{
	if ( GetLocalPlayerTeam() != TEAM_UNASSIGNED )
	{
#ifdef _X360
		m_pFooter->ShowButtonLabel( "cancel", true );
#else
		m_pCancelButton->SetVisible( true );
#endif
	}
	else
	{
#ifdef _X360
		m_pFooter->ShowButtonLabel( "cancel", false );
#else
		m_pCancelButton->SetVisible( false );
#endif
	}
}

#ifdef _X360
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamMenu::Join_Team( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		char cmd[256];
		V_sprintf_safe( cmd, "jointeam_nomenus %s", args.Arg( 1 ) );
		OnCommand( cmd );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCTeamMenu::OnKeyCodePressed( KeyCode code )
{
	if ( ( m_iTeamMenuKey != BUTTON_CODE_INVALID && m_iTeamMenuKey == code ) ||
		code == KEY_XBUTTON_BACK ||
		code == KEY_XBUTTON_B )
	{
		if ( GetLocalPlayerTeam() != TEAM_UNASSIGNED )
		{
			ShowPanel( false );
		}
	}
	else if ( code == KEY_SPACE )
	{
		engine->ClientCmd( "jointeam auto" );
		Close();
	}
	else if ( code == KEY_XBUTTON_A || code == KEY_XBUTTON_RTRIGGER )
	{
		// select the active focus
		if ( GetFocusNavGroup().GetCurrentFocus() )
		{
			ipanel()->SendMessage( GetFocusNavGroup().GetCurrentFocus()->GetVPanel(), new KeyValues( "PressButton" ), GetVPanel() );
		}
	}
	else if ( code == KEY_XBUTTON_RIGHT || code == KEY_XSTICK1_RIGHT )
	{
		CTDCTeamButton *pButton;

		pButton = dynamic_cast<CTDCTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorExited();
			GetFocusNavGroup().RequestFocusNext( pButton->GetVPanel() );
		}
		else
		{
			GetFocusNavGroup().RequestFocusNext( NULL );
		}

		pButton = dynamic_cast<CTDCTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if ( code == KEY_XBUTTON_LEFT || code == KEY_XSTICK1_LEFT )
	{
		CTDCTeamButton *pButton;

		pButton = dynamic_cast<CTDCTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorExited();
			GetFocusNavGroup().RequestFocusPrev( pButton->GetVPanel() );
		}
		else
		{
			GetFocusNavGroup().RequestFocusPrev( NULL );
		}

		pButton = dynamic_cast<CTDCTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
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
// Purpose: Called when the user picks a team
//-----------------------------------------------------------------------------
void CTDCTeamMenu::OnCommand( const char *command )
{
	if ( Q_strstr( command, "jointeam " ) || Q_strstr( command, "jointeam_nomenus " ) )
	{
		engine->ClientCmd( command );
	}

	Close();
}

//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CTDCTeamMenu::OnTick()
{
	if ( !IsVisible() || !TDCGameRules() )
		return;

	// update the number of players on each team
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		C_Team *pTeam = GetGlobalTeam( i );
		if ( !pTeam )
			continue;

		SetDialogVariable( VarArgs( "%scount", g_aTeamLowerNames[i] ), pTeam->Get_Number_Players() );
	}

	if ( m_pTeamButtons[TEAM_SPECTATOR] )
	{
		ConVarRef mp_allowspectators( "mp_allowspectators" );
		if ( mp_allowspectators.IsValid() )
		{
			if ( mp_allowspectators.GetBool() )
			{
				m_pTeamButtons[TEAM_SPECTATOR]->SetVisible( true );
				m_pSpecLabel->SetVisible( true );
			}
			else
			{
				m_pTeamButtons[TEAM_SPECTATOR]->SetVisible( false );
				m_pSpecLabel->SetVisible( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCTeamMenu::OnThink( void )
{
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
void CTDCTeamMenu::OnClose()
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
// Purpose: FFA team menu
//-----------------------------------------------------------------------------
CTDCDeathmatchTeamMenu::CTDCDeathmatchTeamMenu( IViewPort *pViewPort, const char *pName ) : BaseClass( pViewPort, pName )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCDeathmatchTeamMenu::CreateTeamButtons( void )
{
	m_pTeamButtons[TEAM_UNASSIGNED] = new CTDCTeamButton( this, "teambutton2" );
	m_pTeamButtons[TEAM_SPECTATOR] = new CTDCTeamButton( this, "teambutton3" );
}
