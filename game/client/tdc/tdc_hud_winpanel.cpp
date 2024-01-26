//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tdc_hud_winpanel.h"
#include "tdc_hud_statpanel.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tdc_playerresource.h"
#include "c_tdc_team.h"
#include "tdc_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

vgui::IImage* GetDefaultAvatarImage( int iPlayerIndex );

DECLARE_HUDELEMENT_DEPTH( CTDCWinPanel, 1 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCWinPanel::CTDCWinPanel( const char *pElementName ) : EditablePanel( NULL, "WinPanel" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bShouldBeVisible = false;
	SetAlpha( 0 );

	// This is needed for custom colors.
	SetScheme( scheme()->LoadSchemeFromFile( "Resource/ClientScheme_tf2c.res", "ClientScheme_tf2c" ) );

	m_pBGPanel = new EditablePanel( this, "WinPanelBGBorder" );
	m_pTeamScorePanel = new EditablePanel( this, "TeamScoresPanel" );
	m_pBlueBG = new EditablePanel( m_pTeamScorePanel, "BlueScoreBG" );
	m_pRedBG = new EditablePanel( m_pTeamScorePanel, "RedScoreBG" );

	m_pBlueBorder = NULL;
	m_pRedBorder = NULL;
	m_pBlackBorder = NULL;

	m_flTimeUpdateTeamScore = 0;
	m_iBlueTeamScore = 0;
	m_iRedTeamScore = 0;
	m_iScoringTeam = 0;

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWinPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWinPanel::Reset()
{
	m_bShouldBeVisible = false;
	m_flTimeUpdateTeamScore = 0.0f;
	m_iScoringTeam = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWinPanel::Init()
{
	// listen for events
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );

	m_bShouldBeVisible = false;

	CHudElement::Init();
}

void CTDCWinPanel::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	if ( state )
	{
		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWinPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "tf_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_win_panel", pEventName ) == 0 )
	{
		if ( !g_PR )
			return;

		int iWinningTeam = event->GetInt( "winning_team" );
		int iWinReason = event->GetInt( "winreason" );
		int iFlagCapLimit = event->GetInt( "flagcaplimit" );

		SetDialogVariable( "WinningTeamLabel", "" );
		SetDialogVariable( "AdvancingTeamLabel", "" );
		SetDialogVariable( "WinReasonLabel", "" );
		SetDialogVariable( "DetailsLabel", "" );

		// set the appropriate background image and label text
		const char *pTeamLabel = NULL;
		const char *pTopPlayersLabel = NULL;
		const wchar_t *pLocalizedTeamName = GetLocalizedTeamName( iWinningTeam );

		switch ( iWinningTeam )
		{
		case TDC_TEAM_BLUE:
			m_pBGPanel->SetBorder( m_pBlueBorder );
			pTeamLabel = "#Winpanel_BlueWins";
			pTopPlayersLabel = "#Winpanel_BlueMVPs";
			break;
		case TDC_TEAM_RED:
			m_pBGPanel->SetBorder( m_pRedBorder );
			pTeamLabel = "#Winpanel_RedWins";
			pTopPlayersLabel = "#Winpanel_RedMVPs";
			break;
		case TEAM_UNASSIGNED:	// stalemate
			m_pBGPanel->SetBorder( m_pBlackBorder );
			pTeamLabel = "#Winpanel_Stalemate";
			pTopPlayersLabel = "#Winpanel_TopPlayers";
			break;
		default:
			Assert( false );
			break;
		}

		if ( TDCGameRules()->IsInfectionMode() )
		{
			pTopPlayersLabel = "#Winpanel_TopPlayers";
		}

		SetDialogVariable( "WinningTeamLabel", g_pVGuiLocalize->Find( pTeamLabel ) );
		SetDialogVariable( "TopPlayersLabel", g_pVGuiLocalize->Find( pTopPlayersLabel ) );

		wchar_t wzWinReason[256] = L"";
		switch ( iWinReason )
		{
		case WINREASON_ALL_POINTS_CAPTURED:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_AllPointsCaptured" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_FLAG_CAPTURE_LIMIT:
		{
			wchar_t wzFlagCaptureLimit[16];
			V_swprintf_safe( wzFlagCaptureLimit, L"%i", iFlagCapLimit );
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_FlagCaptureLimit" ), 2,
				pLocalizedTeamName, wzFlagCaptureLimit );
		}
		break;
		case WINREASON_OPPONENTS_DEAD:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_OpponentsDead" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_DEFEND_UNTIL_TIME_LIMIT:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_DefendedUntilTimeLimit" ), 1, pLocalizedTeamName );
			break;
		case WINREASON_STALEMATE:
			g_pVGuiLocalize->ConstructString( wzWinReason, sizeof( wzWinReason ), g_pVGuiLocalize->Find( "#Winreason_Stalemate" ), 0 );
			break;
		default:
			Assert( false );
			break;
		}
		SetDialogVariable( "WinReasonLabel", wzWinReason );

		if ( iWinReason == WINREASON_ALL_POINTS_CAPTURED || iWinReason == WINREASON_FLAG_CAPTURE_LIMIT )
		{
			// if this was a full round that ended with point capture or flag capture, show the winning cappers
			const char *pCappers = event->GetString( "cappers" );
			int iCappers = Q_strlen( pCappers );
			if ( iCappers > 0 )
			{
				char szPlayerNames[256] = "";
				wchar_t wzPlayerNames[256] = L"";
				wchar_t wzCapMsg[512] = L"";
				for ( int i = 0; i < iCappers; i++ )
				{
					V_strcat_safe( szPlayerNames, g_PR->GetPlayerName( (int)pCappers[i] ) );
					if ( i < iCappers - 1 )
					{
						V_strcat_safe( szPlayerNames, ", " );
					}
				}
				g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerNames, wzPlayerNames, sizeof( wzPlayerNames ) );

				g_pVGuiLocalize->ConstructString( wzCapMsg, sizeof( wzCapMsg ), g_pVGuiLocalize->Find( "#Winpanel_WinningCapture" ), 1, wzPlayerNames );
				SetDialogVariable( "DetailsLabel", wzCapMsg );
			}
		}

		// get the current & previous team scores
		m_iBlueTeamScore = event->GetInt( "blue_score", 0 );
		m_iRedTeamScore = event->GetInt( "red_score", 0 );
		m_iScoringTeam = iWinningTeam;

		m_pBlueBG->SetBorder( m_pBlueBorder );
		m_pRedBG->SetBorder( m_pRedBorder );

		m_pTeamScorePanel->SetDialogVariable( "blueteamscore", GetTeamScore( GetLeftTeam(), true ) );
		m_pTeamScorePanel->SetDialogVariable( "redteamscore", GetTeamScore( GetRightTeam(), true ) );

		m_pTeamScorePanel->SetDialogVariable( "blueteamname", GetLocalizedTeamName( GetLeftTeam() ) );
		m_pTeamScorePanel->SetDialogVariable( "redteamname", GetLocalizedTeamName( GetRightTeam() ) );

		if ( m_iScoringTeam != TEAM_UNASSIGNED )
		{
			// if the new scores are different, set ourselves to update the scoreboard to the new values after a short delay, so players
			// see the scores tick up
			m_flTimeUpdateTeamScore = gpGlobals->curtime + 2.0f;
		}

		m_pTeamScorePanel->SetVisible( true );

		if ( !g_TDC_PR )
			return;

		// look for the top 3 players sent in the event
		for ( int i = 1; i <= 3; i++ )
		{
			bool bShow = false;
			char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "";
			// get player index and round points from the event
			V_sprintf_safe( szPlayerIndexVal, "player_%d", i );
			V_sprintf_safe( szPlayerScoreVal, "player_%d_points", i );
			int iPlayerIndex = event->GetInt( szPlayerIndexVal, 0 );
			int iRoundScore = event->GetInt( szPlayerScoreVal, 0 );
			// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
			if ( iRoundScore > 0 )
				bShow = true;

#if !defined( _X360 )
			CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( CFmtStr( "Player%dAvatar", i ) ) );

			if ( pPlayerAvatar )
			{
				pPlayerAvatar->ClearAvatar();
				pPlayerAvatar->SetShouldScaleImage( true );
				pPlayerAvatar->SetShouldDrawFriendIcon( false );

				if ( bShow )
				{
					pPlayerAvatar->SetDefaultAvatar( GetDefaultAvatarImage( iPlayerIndex ) );
					pPlayerAvatar->SetPlayer( iPlayerIndex );
				}
				pPlayerAvatar->SetVisible( bShow );
			}
#endif
			vgui::Label *pPlayerName = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dName", i ) ) );
			vgui::Label *pPlayerClass = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dClass", i ) ) );
			vgui::Label *pPlayerScore = dynamic_cast<Label *>( FindChildByName( CFmtStr( "Player%dScore", i ) ) );

			if ( !pPlayerName || !pPlayerClass || !pPlayerScore )
				return;

			if ( bShow )
			{
				// set the player labels to team color
				Color clr = g_PR->GetTeamColor( g_PR->GetTeam( iPlayerIndex ) );
				pPlayerName->SetFgColor( clr );
				pPlayerClass->SetFgColor( clr );
				pPlayerScore->SetFgColor( clr );

				// set label contents
				pPlayerName->SetText( g_PR->GetPlayerName( iPlayerIndex ) );
				//pPlayerClass->SetText( g_aPlayerClassNames[g_TDC_PR->GetPlayerClass( iPlayerIndex )] );
				pPlayerScore->SetText( CFmtStr( "%d", iRoundScore ) );
			}

			// show or hide labels for this player position
			pPlayerName->SetVisible( bShow );
			//pPlayerClass->SetVisible( bShow );
			pPlayerClass->SetVisible( false );
			pPlayerScore->SetVisible( bShow );
		}

		m_bShouldBeVisible = true;

		MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTDCWinPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/WinPanel.res" );

	m_pBlueBorder = pScheme->GetBorder( "TFFatLineBorderBlueBG" );
	m_pRedBorder = pScheme->GetBorder( "TFFatLineBorderRedBG" );
	m_pBlackBorder = pScheme->GetBorder( "TFFatLineBorder" );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTDCWinPanel::ShouldDraw()
{
	if ( !m_bShouldBeVisible )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: panel think method
//-----------------------------------------------------------------------------
void CTDCWinPanel::OnThink()
{
	// if we've scheduled ourselves to update the team scores, handle it now
	if ( m_flTimeUpdateTeamScore > 0 && gpGlobals->curtime >= m_flTimeUpdateTeamScore )
	{
		// play a sound
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );

		// update the team scores
		m_pTeamScorePanel->SetDialogVariable( "blueteamscore", GetTeamScore( GetLeftTeam(), false ) );
		m_pTeamScorePanel->SetDialogVariable( "redteamscore", GetTeamScore( GetRightTeam(), false ) );

		m_flTimeUpdateTeamScore = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCWinPanel::GetLeftTeam( void )
{
	return TDC_TEAM_BLUE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCWinPanel::GetRightTeam( void )
{
	return TDC_TEAM_RED;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCWinPanel::GetTeamScore( int iTeam, bool bPrevious )
{
	int iScore = 0;

	switch ( iTeam )
	{
	case TDC_TEAM_RED:
		iScore = m_iRedTeamScore;
		break;
	case TDC_TEAM_BLUE:
		iScore = m_iBlueTeamScore;
		break;
	}

	// If this is the winning team then their previous score is 1 point lower.
	if ( bPrevious && iTeam == m_iScoringTeam )
		iScore--;

	return iScore;
}
