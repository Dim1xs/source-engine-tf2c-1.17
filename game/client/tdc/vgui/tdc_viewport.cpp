//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client DLL VGUI2 Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "input.h"
#include "tdc_viewport.h"
#include "c_tdc_player.h"
#include "tdc_gamerules.h"
#include "voice_status.h"
#include "tdc_clientscoreboard.h"
#include "tdc_mapinfomenu.h"
#include "tdc_teammenu.h"
#include "tdc_classmenu.h"
#include "tdc_intromenu.h"
#include "tdc_hud_notification_panel.h"

/*
CON_COMMAND( spec_help, "Show spectator help screen")
{
	if ( gViewPortInterface )
		gViewPortInterface->ShowPanel( PANEL_INFO, true );
}

CON_COMMAND( spec_menu, "Activates spectator menu")
{
	bool bShowIt = true;

	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();

	if ( pPlayer && !pPlayer->IsObserver() )
		return;

	if ( args.ArgC() == 2 )
	{
		bShowIt = atoi( args[ 1 ] ) == 1;
	}

	if ( gViewPortInterface )
		gViewPortInterface->ShowPanel( PANEL_SPECMENU, bShowIt );
}
*/

CON_COMMAND( showmapinfo, "Show map info panel" )
{
	if ( !gViewPortInterface )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	// don't let the player open the team menu themselves until they're a spectator or they're on a regular team and have picked a class
	if ( pPlayer )
	{
		if ( ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR ) || 
		     ( ( pPlayer->GetTeamNumber() != TEAM_UNASSIGNED ) && ( pPlayer->GetPlayerClass()->GetClassIndex() != TDC_CLASS_UNDEFINED ) ) )
		{
			// close all the other panels that could be open
			gViewPortInterface->ShowPanel( PANEL_TEAM, false );
			gViewPortInterface->ShowPanel( PANEL_CLASS, false );
			gViewPortInterface->ShowPanel( PANEL_INTRO, false );
			gViewPortInterface->ShowPanel( PANEL_DEATHMATCHTEAMSELECT, false );

			gViewPortInterface->ShowPanel( PANEL_MAPINFO, true );
		}
	}
}

TFViewport::TFViewport()
{
}

//-----------------------------------------------------------------------------
// Purpose: called when the VGUI subsystem starts up
//			Creates the sub panels and initialises them
//-----------------------------------------------------------------------------
void TFViewport::Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager2 * pGameEventManager )
{
	BaseClass::Start( pGameUIFuncs, pGameEventManager );
}

void TFViewport::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	gHUD.InitColors( pScheme );

	SetPaintBackgroundEnabled( false );

	// Precache some font characters for the 360
 	if ( IsX360() )
 	{
 		wchar_t *pAllChars = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.!:";
 		wchar_t *pNumbers = L"0123456789";
 
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "ScoreboardTeamName" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "ScoreboardMedium" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "ScoreboardSmall" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "ScoreboardVerySmall" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "TFFontMedium" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "TFFontSmall" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "HudFontMedium" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "HudFontMediumSmallSecondary" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "HudFontSmall" ), pAllChars );
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "DefaultSmall" ), pAllChars );
 
 		vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "ScoreboardTeamScore" ), pNumbers );
 	}
}

//
// This is the main function of the viewport. Right here is where we create our class menu, 
// team menu, and anything else that we want to turn on and off in the UI.
//
IViewPortPanel* TFViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	// overwrite MOD specific panel creation

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName ) == 0 )
	{
		newpanel = new CTDCClientScoreBoardDialog( this, szPanelName );
	}
	else if ( Q_strcmp( PANEL_MAPINFO, szPanelName ) == 0 )
	{
		newpanel = new CTDCMapInfoMenu( this );
	}
	else if ( Q_strcmp( PANEL_TEAM, szPanelName ) == 0 )
	{
		CTDCTeamMenu *pTeamMenu = new CTDCTeamMenu( this, szPanelName );	
		pTeamMenu->CreateTeamButtons();
		newpanel = pTeamMenu;
	}
	else if ( Q_strcmp( PANEL_DEATHMATCHTEAMSELECT, szPanelName ) == 0 )
	{
		CTDCTeamMenu *pTeamMenu = new CTDCDeathmatchTeamMenu( this, szPanelName );
		pTeamMenu->CreateTeamButtons();
		newpanel = pTeamMenu;
	}
	else if ( Q_strcmp( PANEL_CLASS, szPanelName ) == 0 )
	{
		newpanel = new CTDCClassMenu( this );	
	}
	else if ( Q_strcmp( PANEL_INTRO, szPanelName ) == 0 )
	{
		newpanel = new CTDCIntroMenu( this );
	}
	else
	{
		// create a generic base panel, don't add twice
		newpanel = BaseClass::CreatePanelByName( szPanelName );
	}

	return newpanel; 
}

void TFViewport::CreateDefaultPanels( void )
{
	AddNewPanel( CreatePanelByName( PANEL_SCOREBOARD ), "PANEL_SCOREBOARD" );
	AddNewPanel( CreatePanelByName( PANEL_MAPINFO ), "PANEL_MAPINFO" );
	AddNewPanel( CreatePanelByName( PANEL_TEAM ), "PANEL_TEAM" );
	AddNewPanel( CreatePanelByName( PANEL_CLASS ), "PANEL_CLASS" );
	AddNewPanel( CreatePanelByName( PANEL_INTRO ), "PANEL_INTRO" );
	AddNewPanel( CreatePanelByName( PANEL_DEATHMATCHTEAMSELECT ), "PANEL_DEATHMATCHTEAMSELECT" );
}

int TFViewport::GetDeathMessageStartHeight( void )
{
	int y = YRES(2);

	if ( IsX360() )
	{
		y = YRES(36);
	}

	return y;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for handling multiple scoreboard.
//-----------------------------------------------------------------------------
const char *TFViewport::GetModeSpecificScoreboardName( void )
{
	return PANEL_SCOREBOARD;
}

//-----------------------------------------------------------------------------
// Purpose: Use this when you need to open the scoreboard unless you want a specific scoreboard panel.
//-----------------------------------------------------------------------------
void TFViewport::ShowScoreboard( bool bState, int nPollHideCode /*= BUTTON_CODE_INVALID*/ )
{
	const char *pszScoreboard = GetModeSpecificScoreboardName();
	ShowPanel( pszScoreboard, bState );

	if ( bState && nPollHideCode != BUTTON_CODE_INVALID )
	{
		// The scoreboard was opened by another dialog so we need to send the close button code.
		// See CTDCClientScoreBoardDialog::OnThink for the explanation.
		PostMessageToPanel( pszScoreboard, new KeyValues( "PollHideCode", "code", nPollHideCode ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void TFViewport::ShowTeamMenu( bool bState )
{
	if ( !TDCGameRules() )
		return;

	if ( !TDCGameRules()->IsTeamplay() || TDCGameRules()->IsInfectionMode() )
	{
		ShowPanel( PANEL_DEATHMATCHTEAMSELECT, bState );
	}
	else
	{
		ShowPanel( PANEL_TEAM, bState );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFViewport::CC_ChangeTeam( const CCommand &args )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	if ( engine->IsHLTV() )
		return;

	// don't let the player open the team menu themselves until they're on a team
	if ( pPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
		return;

	// Losers can't change team during bonus time.
	if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
		pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM &&
		pPlayer->GetTeamNumber() != TDCGameRules()->GetWinningTeam() )
		return;

	ShowTeamMenu( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void TFViewport::ShowClassMenu( bool bState )
{
	if ( bState )
	{
		// Need to set the class menu to the proper team when showing it.
		PostMessageToPanel( PANEL_CLASS, new KeyValues( "ShowToTeam", "iTeam", GetLocalPlayerTeam() ) );
	}
	else
	{
		ShowPanel( PANEL_CLASS, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFViewport::CC_ChangeClass( const CCommand &args )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	if ( engine->IsHLTV() )
		return;

	if ( pPlayer->CanShowClassMenu() )
	{
		ShowClassMenu( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFViewport::OnScreenSizeChanged( int iOldWide, int iOldTall )
{
	BaseClass::OnScreenSizeChanged( iOldWide, iOldTall );

	// we've changed resolution, let's try to figure out if we need to show any of our menus
	if ( !gViewPortInterface )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( pPlayer )
	{
		// are we on a team yet?
		if ( pPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
		{
			engine->ClientCmd( "show_motd" );
		}
	}
}
