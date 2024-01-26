//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/ImageList.h>
#include "vgui_avatarimage.h"
#include "tdc_clientscoreboard.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <inputsystem/iinputsystem.h>
#include "voice_status.h"
#include "gamevars_shared.h"
#include "c_tdc_playerresource.h"
#include "c_tdc_player.h"
#include "c_tdc_team.h"
#include "tdc_hud_statpanel.h"
#include "tdc_gamerules.h"

using namespace vgui;

ConVar tdc_scoreboard_mouse_mode( "tdc_scoreboard_mouse_mode", "0", FCVAR_ARCHIVE, "", true, 0, true, 2 );

extern ConVar cl_hud_playerclass_use_playermodel;

bool IsInCommentaryMode( void );
const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCClientScoreBoardDialog::CTDCClientScoreBoardDialog( IViewPort *pViewPort, const char *pszName ) : EditablePanel( NULL, pszName )
{
	m_nCloseKey = BUTTON_CODE_INVALID;

	SetProportional( true );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	MakePopup( true );

	// set the scheme before any child control is created
	SetScheme( "ClientScheme" );

	memset( m_pPlayerLists, 0, sizeof( m_pPlayerLists ) );
	memset( m_pPingLists, 0, sizeof( m_pPingLists ) );

	// Create player lists.
	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		m_pPlayerLists[i] = new SectionedListPanel( this, VarArgs( "%sPlayerList", g_aTeamNames[i] ) );
		m_pPingLists[i] = new SectionedListPanel( this, VarArgs( "%sPingList", g_aTeamNames[i] ) );
	}

	m_pPlayerLists[0]->SetName( "FFAPlayerList" );
	m_pPingLists[0]->SetName( "FFAPingList" );
	m_pPlayerLists[1]->SetVisible( false );
	m_pPingLists[1]->SetVisible( false );

	m_pLabelPlayerName = new CExLabel( this, "PlayerNameLabel", "" );
	m_pLabelMapName = new CExLabel( this, "MapName", "" );
	m_pLabelGameTypeName = new CExLabel( this, "GameTypeName", "" );
	m_pGameTypeIcon = new ImagePanel( this, "GameTypeIcon" );
	m_pServerTimeLeftValue = NULL;
	m_iImageDead = 0;
	m_iImageDominated = 0;
	m_iImageNemesis = 0;
	memset( m_iImageDominations, 0, sizeof( m_iImageDominations ) );
	memset( m_iDefaultAvatars, 0, sizeof( m_iDefaultAvatars ) );

	m_iSelectedPlayerIndex = 0;

	m_pContextMenu = new Menu( this, "contextmenu" );
	m_pContextMenu->AddCheckableMenuItem( "#TDC_ScoreBoard_Mute", "mute", this );
	m_pContextMenu->AddMenuItem( "#TDC_ScoreBoard_Kick", "kick", this );
	m_pContextMenu->AddMenuItem( "#TDC_ScoreBoard_Spectate", "spectate", this );
	m_pContextMenu->AddMenuItem( "#TDC_ScoreBoard_ShowProfile", "showprofile", this );

	m_pImageList = NULL;
	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );

	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "game_maploaded" );

	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCClientScoreBoardDialog::~CTDCClientScoreBoardDialog()
{
	delete m_pImageList;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );
	if ( TDCGameRules() )
	{
		if ( TDCGameRules()->IsTeamplay() )
		{
			AddSubKeyNamed( pConditions, "if_teamplay" );
		}
	}

	LoadControlSettings( "Resource/UI/Scoreboard.res", NULL, NULL, pConditions );
	pConditions->deleteThis();

	delete m_pImageList;
	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();

	m_iImageDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dead", true ) );
	m_iImageDominated = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dominated", true ) );
	m_iImageNemesis = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_nemesis", true ) );

	for ( int i = 0; i < TDC_SCOREBOARD_MAX_DOMINATIONS; i++ )
	{
		m_iImageDominations[i] = m_pImageList->AddImage( scheme()->GetImage( VarArgs( "../hud/leaderboard_dom%d", i + 1 ), true ) );
	}

	// resize the images to our resolution
	for ( int i = 1; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide = 13, tall = 13;
		m_pImageList->GetImage( i )->SetSize( scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(), tall ) );
	}

	CAvatarImage *pImage = new CAvatarImage();
	pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
	pImage->SetDefaultImage( scheme()->GetImage( "../vgui/avatar_default", true ) );
	m_iDefaultAvatars[0] = m_pImageList->AddImage( pImage );

	for ( int i = FIRST_GAME_TEAM; i < TDC_TEAM_COUNT; i++ )
	{
		const char *pszImage = VarArgs( "../vgui/avatar_default_%s", g_aTeamLowerNames[i] );

		CAvatarImage *pImage = new CAvatarImage();
		pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
		pImage->SetDefaultImage( scheme()->GetImage( pszImage, true ) );
		m_iDefaultAvatars[i] = m_pImageList->AddImage( pImage );
	}

	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		SetPlayerListImages( m_pPlayerLists[i] );
	}

	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetBorder( NULL );
	SetVisible( false );

	m_hTimeLeftFont = pScheme->GetFont( "ScoreboardMedium", true );
	m_hTimeLeftNotSetFont = pScheme->GetFont( "ScoreboardVerySmall", true );

	m_pServerTimeLeftValue = dynamic_cast<CExLabel *>( FindChildByName( "ServerTimeLeftValue" ) );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::ShowPanel( bool bShow )
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if ( m_pImageList == NULL )
	{
		InvalidateLayout( true, true );
	}

	// Don't show in commentary mode
	if ( IsInCommentaryMode() )
	{
		bShow = false;
	}

	if ( bShow == IsVisible() )
	{
		return;
	}

	if ( bShow )
	{
		SetVisible( true );
		MoveToFront();

		SetKeyBoardInputEnabled( false );
		SetMouseInputEnabled( tdc_scoreboard_mouse_mode.GetInt() == 1 );

		// Clear the selected item, this forces the default to the local player
		SectionedListPanel *pList = GetSelectedPlayerList();
		if ( pList )
		{
			pList->ClearSelection();
		}
	}
	else
	{
		SetVisible( false );

		m_pContextMenu->SetVisible( false );
		SetKeyBoardInputEnabled( false );
		SetMouseInputEnabled( false );
		m_nCloseKey = BUTTON_CODE_INVALID;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::OnCommand( const char* command )
{
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalPlayer )
		return;

	if ( !Q_strcmp( command, "mute" ) )
	{
		bool bMuted = GetClientVoiceMgr()->IsPlayerBlocked( m_iSelectedPlayerIndex );
		GetClientVoiceMgr()->SetPlayerBlockedState( m_iSelectedPlayerIndex, !bMuted );
	}
	else if ( !Q_strcmp( command, "kick" ) )
	{
		if ( g_TDC_PR )
		{
			int iUserID = g_TDC_PR->GetUserID( m_iSelectedPlayerIndex );

			if ( iUserID )
			{
				engine->ClientCmd( VarArgs( "callvote Kick %d", iUserID ) );
			}
		}
	}
	else if ( !Q_strcmp( command, "spectate" ) )
	{
		engine->ClientCmd( VarArgs( "spec_player %d", m_iSelectedPlayerIndex ) );
	}
	else if ( !Q_strcmp( command, "showprofile" ) )
	{
		CSteamID steamID;
		if ( g_TDC_PR && g_TDC_PR->GetSteamID( m_iSelectedPlayerIndex, &steamID ) )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "steamid", steamID );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Call every frame
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::OnThink()
{
	BaseClass::OnThink();

	// NOTE: this is necessary because of the way input works.
	// If a key down message is sent to vgui, then it will get the key up message
	// Sometimes the scoreboard is activated by other vgui menus, 
	// sometimes by console commands. In the case where it's activated by
	// other vgui menus, we lose the key up message because this panel
	// doesn't accept keyboard input. It *can't* accept keyboard input
	// because another feature of the dialog is that if it's triggered
	// from within the game, you should be able to still run around while
	// the scoreboard is up. That feature is impossible if this panel accepts input.
	// because if a vgui panel is up that accepts input, it prevents the engine from
	// receiving that input. So, I'm stuck with a polling solution.
	// 
	// Close key is set to non-invalid when something other than a keybind
	// brings the scoreboard up, and it's set to invalid as soon as the 
	// dialog becomes hidden.
	if ( m_nCloseKey != BUTTON_CODE_INVALID )
	{
		if ( !g_pInputSystem->IsButtonDown( m_nCloseKey ) )
		{
			m_nCloseKey = BUTTON_CODE_INVALID;
			gViewPortInterface->ShowPanel( this, false );
			GetClientVoiceMgr()->StopSquelchMode();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	if ( !g_TDC_PR )
		return;

	if ( g_TDC_PR->IsFakePlayer( playerIndex ) )
	{
		// Show default avatars for bots.
		kv->SetInt( "avatar", GetDefaultAvatar( playerIndex ) );
	}
	else
	{
		// Get their avatar from Steam.
		CSteamID steamIDForPlayer;
		if ( g_TDC_PR->GetSteamID( playerIndex, &steamIDForPlayer ) )
		{
			// See if we already have that avatar in our list
			int iMapIndex = m_mapAvatarsToImageList.Find( steamIDForPlayer );
			int iImageIndex;
			if ( iMapIndex == m_mapAvatarsToImageList.InvalidIndex() )
			{
				CAvatarImage *pImage = new CAvatarImage();
				pImage->SetAvatarSteamID( steamIDForPlayer );
				pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
				iImageIndex = m_pImageList->AddImage( pImage );

				m_mapAvatarsToImageList.Insert( steamIDForPlayer, iImageIndex );
			}
			else
			{
				iImageIndex = m_mapAvatarsToImageList[iMapIndex];
			}

			kv->SetInt( "avatar", iImageIndex );

			CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( iImageIndex );
			pAvIm->UpdateFriendStatus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCClientScoreBoardDialog::GetDefaultAvatar( int playerIndex )
{
	if ( !TDCGameRules()->IsTeamplay() )
		return m_iDefaultAvatars[0];

	return m_iDefaultAvatars[g_TDC_PR->GetTeam( playerIndex )];
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CTDCClientScoreBoardDialog::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( IsVisible() && tdc_scoreboard_mouse_mode.GetInt() == 2 && keynum == MOUSE_RIGHT && down && !IsMouseInputEnabled() )
	{
		SetMouseInputEnabled( true );
		return 0;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Called by vgui panels that activate the client scoreboard
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::OnPollHideCode( int code )
{
	m_nCloseKey = (ButtonCode_t)code;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::ShowContextMenu( KeyValues *data )
{
	Panel *pItem = (Panel*)data->GetPtr( "SubPanel" );
	if ( !pItem )
		return;

	SectionedListPanel *pList = (SectionedListPanel *)pItem->GetParent();
	if ( !pList )
		return;

	int iItem = data->GetInt( "itemID" );

	KeyValues *pData = pList->GetItemData( iItem );
	m_iSelectedPlayerIndex = pData->GetInt( "playerIndex", 0 );

	if ( g_PR->IsLocalPlayer( m_iSelectedPlayerIndex ) )
		return;

	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalPlayer )
		return;

	bool bMuted = GetClientVoiceMgr()->IsPlayerBlocked( m_iSelectedPlayerIndex );
	m_pContextMenu->GetMenuItem( 0 )->SetChecked( bMuted );

	// Can only votekick teammates.
	bool bCanKick = ( g_PR->GetTeam( m_iSelectedPlayerIndex ) == GetLocalPlayerTeam() );
	m_pContextMenu->GetMenuItem( 1 )->SetEnabled( bCanKick );

	// Cannot mute bots or show their profile.
	bool bIsBot = g_PR->IsFakePlayer( m_iSelectedPlayerIndex );
	m_pContextMenu->GetMenuItem( 0 )->SetEnabled( !bIsBot );
	m_pContextMenu->GetMenuItem( 3 )->SetEnabled( !bIsBot );

	bool bCanSpectate = false;

	// Only when in manual spectator mode.
	if ( pLocalPlayer->GetObserverMode() > OBS_MODE_FIXED && g_PR->IsAlive( m_iSelectedPlayerIndex ) )
	{
		if ( pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			bCanSpectate = true;
		}
		else
		{
			switch ( mp_forcecamera.GetInt() )
			{
			case OBS_ALLOW_ALL:
				bCanSpectate = true;
				break;
			case OBS_ALLOW_TEAM:
				bCanSpectate = !g_TDC_PR->IsEnemyPlayer( m_iSelectedPlayerIndex );
				break;
			case OBS_ALLOW_NONE:
				bCanSpectate = false;
				break;
			}
		}
	}

	m_pContextMenu->GetMenuItem( 2 )->SetEnabled( bCanSpectate );

	Menu::PlaceContextMenu( this, m_pContextMenu );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the scoreboard panel
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::Reset()
{
	m_flNextUpdateTime = 0.0f;

	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		InitPlayerList( m_pPlayerLists[i], m_pPingLists[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inits the player list in a list panel
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::InitPlayerList( SectionedListPanel *pPlayerList, SectionedListPanel *pPingList )
{
	pPlayerList->SetVerticalScrollbar( false );
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPlayerList->SetSectionAlwaysVisible( 0, true );
	pPlayerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	pPlayerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	pPlayerList->SetBorder( NULL );
	pPlayerList->SetDrawHeaders( false );

	// Avatars are always displayed at 32x32 regardless of resolution
	pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iAvatarWidth );

	// Stretch player name column so that it fills up all unused space.
	int iNameWidth = pPlayerList->GetWide() -
		m_iAvatarWidth -
		m_iStatusWidth -
		m_iNemesisWidth -
		m_iScoreWidth -
		m_iKillsWidth -
		m_iDeathsWidth -
		15 - // SectionedListPanel offset
		pPlayerList->GetScrollBar()->GetWide(); // Scrollbar width
	pPlayerList->AddColumnToSection( 0, "name", "#TDC_Scoreboard_Name", 0, iNameWidth );
	pPlayerList->AddColumnToSection( 0, "status", "", SectionedListPanel::COLUMN_IMAGE, m_iStatusWidth );
	pPlayerList->AddColumnToSection( 0, "nemesis", "", SectionedListPanel::COLUMN_IMAGE, m_iNemesisWidth );
	pPlayerList->AddColumnToSection( 0, "score", "#TDC_Scoreboard_Score", SectionedListPanel::COLUMN_RIGHT, m_iScoreWidth );
	pPlayerList->AddColumnToSection( 0, "kills", "#TDC_ScoreBoard_Kills", SectionedListPanel::COLUMN_RIGHT, m_iKillsWidth );
	pPlayerList->AddColumnToSection( 0, "deaths", "#TDC_ScoreBoard_Deaths", SectionedListPanel::COLUMN_RIGHT, m_iDeathsWidth );
	//pPlayerList->AddColumnToSection( 0, "ping", "#TDC_Scoreboard_Ping", SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );

	// HACK: Ping column is separate from the main list due to its different appearence. Possibly think of a better way to handle this.
	pPingList->SetVerticalScrollbar( false );
	pPingList->RemoveAll();
	pPingList->RemoveAllSections();
	pPingList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPingList->SetSectionAlwaysVisible( 0, true );
	pPingList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	pPingList->SetBgColor( Color( 0, 0, 0, 0 ) );
	pPingList->SetBorder( NULL );
	pPingList->SetDrawHeaders( false );
	pPingList->AddColumnToSection( 0, "ping", "#TDC_Scoreboard_Ping", SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );
}

//-----------------------------------------------------------------------------
// Purpose: Builds the image list to use in the player list
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::SetPlayerListImages( vgui::SectionedListPanel *pPlayerList )
{
	pPlayerList->SetImageList( m_pImageList, false );
	pPlayerList->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::Update()
{
	UpdateTeamInfo();
	UpdatePlayerList();
	UpdateSpectatorList();
	UpdateArenaWaitingToPlayList();
	MoveToCenterOfScreen();

	// Not really sure where to put this
	if ( TDCGameRules() )
	{
		if ( mp_timelimit.GetInt() > 0 )
		{
			if ( TDCGameRules()->GetTimeLeft() > 0 )
			{
				if ( m_pServerTimeLeftValue )
				{
					m_pServerTimeLeftValue->SetFont( m_hTimeLeftFont );
				}
				
				int iTimeLeft = TDCGameRules()->GetTimeLeft();

				wchar_t wszHours[5] = L"";
				wchar_t wszMinutes[3] = L"";
				wchar_t wszSeconds[3] = L"";

				if ( iTimeLeft >= 3600 )
				{
					V_swprintf_safe( wszHours, L"%d", iTimeLeft / 3600 );
					V_swprintf_safe( wszMinutes, L"%02d", ( iTimeLeft / 60 ) % 60 );
					V_swprintf_safe( wszSeconds, L"%02d", iTimeLeft % 60 );
				}
				else
				{
					V_swprintf_safe( wszMinutes, L"%d", iTimeLeft / 60 );
					V_swprintf_safe( wszSeconds, L"%02d", iTimeLeft % 60 );
				}

				wchar_t wzTimeLabelOld[256] = L"";
				wchar_t wzTimeLabelNew[256] = L"";

				if ( iTimeLeft >= 3600 )
				{
					g_pVGuiLocalize->ConstructString( wzTimeLabelOld, sizeof( wzTimeLabelOld ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeft" ), 3, wszHours, wszMinutes, wszSeconds );
					g_pVGuiLocalize->ConstructString( wzTimeLabelNew, sizeof( wzTimeLabelNew ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNew" ), 3, wszHours, wszMinutes, wszSeconds );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( wzTimeLabelOld, sizeof( wzTimeLabelOld ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNoHours" ), 2, wszMinutes, wszSeconds );
					g_pVGuiLocalize->ConstructString( wzTimeLabelNew, sizeof( wzTimeLabelNew ), g_pVGuiLocalize->Find( "#Scoreboard_TimeLeftNoHoursNew" ), 2, wszMinutes, wszSeconds );
				}

				SetDialogVariable( "servertimeleft", wzTimeLabelOld );
				SetDialogVariable( "servertime", wzTimeLabelNew );
			}
			else // Timer is set and has run out
			{
				if ( m_pServerTimeLeftValue )
				{
					m_pServerTimeLeftValue->SetFont( m_hTimeLeftNotSetFont );
				}

				SetDialogVariable( "servertimeleft", g_pVGuiLocalize->Find( "#Scoreboard_ChangeOnRoundEnd" ) );
				SetDialogVariable( "servertime", g_pVGuiLocalize->Find( "#Scoreboard_ChangeOnRoundEndNew" ) );
			}
		}
		else
		{
			if ( m_pServerTimeLeftValue )
			{
				m_pServerTimeLeftValue->SetFont( m_hTimeLeftNotSetFont );
			}

			SetDialogVariable( "servertimeleft", g_pVGuiLocalize->Find( "#Scoreboard_NoTimeLimit" ) );
			SetDialogVariable( "servertime", g_pVGuiLocalize->Find( "#Scoreboard_NoTimeLimitNew" ) );
		}
	}

	// update every second
	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

bool CTDCClientScoreBoardDialog::NeedsUpdate( void )
{
	return ( m_flNextUpdateTime < gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Updates information about teams
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::UpdateTeamInfo( void )
{
	int iTotalPlayers = 0;

	// update the team sections in the scoreboard
	for ( int teamIndex = FIRST_GAME_TEAM; teamIndex < GetNumberOfTeams(); teamIndex++ )
	{
		C_TDCTeam *team = GetGlobalTFTeam( teamIndex );
		if ( !team )
			continue;

		// update # of players on each team
		wchar_t string1[1024];
		wchar_t wNumPlayers[6];
		V_swprintf_safe( wNumPlayers, L"%i", team->Get_Number_Players() );
		if ( team->Get_Number_Players() == 1 )
		{
			g_pVGuiLocalize->ConstructString( string1, sizeof( string1 ), g_pVGuiLocalize->Find( "#TDC_ScoreBoard_Player" ), 1, wNumPlayers );
		}
		else
		{
			g_pVGuiLocalize->ConstructString( string1, sizeof( string1 ), g_pVGuiLocalize->Find( "#TDC_ScoreBoard_Players" ), 1, wNumPlayers );
		}

		iTotalPlayers += team->Get_Number_Players();

		// set # of players for team in dialog
		SetDialogVariable( VarArgs( "%steamplayercount", g_aTeamLowerNames[teamIndex] ), string1 );

		// set team score in dialog
		SetDialogVariable( VarArgs( "%steamscore", g_aTeamLowerNames[teamIndex] ), team->Get_Score() );

		// round score
		SetDialogVariable( VarArgs( "%steamroundscore", g_aTeamLowerNames[teamIndex] ), team->GetRoundScore() );

		// set team name
		SetDialogVariable( VarArgs( "%steamname", g_aTeamLowerNames[teamIndex] ), team->GetTeamName() );
	}

	SetDialogVariable( "totalplayercount", iTotalPlayers );
	SetDialogVariable( "playingto", TDCGameRules()->GetScoreLimit() );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::UpdatePlayerList( void )
{
	int iSelectedPlayerIndex = GetLocalPlayerIndex();

	// Save off which player we had selected
	SectionedListPanel *pList = GetSelectedPlayerList();

	if ( pList )
	{
		int itemID = pList->GetSelectedItem();

		if ( itemID >= 0 )
		{
			KeyValues *pInfo = pList->GetItemData( itemID );
			if ( pInfo )
			{
				iSelectedPlayerIndex = pInfo->GetInt( "playerIndex" );
			}
		}
	}

	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		m_pPlayerLists[i]->RemoveAll();
		m_pPingLists[i]->RemoveAll();
	}

	if ( !g_TDC_PR )
		return;

	for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( !g_TDC_PR->IsConnected( playerIndex ) )
			continue;

		int iTeam = g_PR->GetTeam( playerIndex );

		if ( iTeam < FIRST_GAME_TEAM )
			continue;

		if ( !TDCGameRules()->IsTeamplay() )
			iTeam = TEAM_UNASSIGNED;

		KeyValues *pKeyValues = new KeyValues( "data" );

		pKeyValues->SetInt( "playerIndex", playerIndex );
		pKeyValues->SetString( "name", g_TDC_PR->GetPlayerName( playerIndex ) );
		pKeyValues->SetInt( "score", g_TDC_PR->GetTotalScore( playerIndex ) );
		pKeyValues->SetInt( "kills", g_TDC_PR->GetPlayerScore( playerIndex ) );
		pKeyValues->SetInt( "deaths", g_TDC_PR->GetDeaths( playerIndex ) );
		pKeyValues->SetInt( "streak", g_TDC_PR->GetKillstreak( playerIndex ) );

		if ( g_TDC_PR->IsPlayerDominating( playerIndex ) )
		{
			// if local player is dominated by this player, show a nemesis icon
			pKeyValues->SetInt( "nemesis", m_iImageNemesis );
			//pKeyValues->SetString( "class", "#TDC_Nemesis" );
		}
		else if ( g_TDC_PR->IsPlayerDominated( playerIndex ) )
		{
			// if this player is dominated by the local player, show the domination icon
			pKeyValues->SetInt( "nemesis", m_iImageDominated );
			//pKeyValues->SetString( "class", "#TDC_Dominated" );
		}

		// display whether player is alive or dead (all players see this for all other players on both teams)
		pKeyValues->SetInt( "status", g_TDC_PR->IsAlive( playerIndex ) ? 0 : m_iImageDead );

		// Show number of dominations.
		int iDominations = Min( g_TDC_PR->GetNumberOfDominations( playerIndex ), TDC_SCOREBOARD_MAX_DOMINATIONS );
		pKeyValues->SetInt( "domination", iDominations > 0 ? m_iImageDominations[iDominations - 1] : 0 );
		int iPing = g_PR->GetPing( playerIndex );
		bool bIsBot = false;

		if ( iPing < 1 )
		{
			if ( g_PR->IsFakePlayer( playerIndex ) )
			{
				pKeyValues->SetString( "ping", "#TDC_Scoreboard_Bot" );
				bIsBot = true;
			}
			else
			{
				pKeyValues->SetString( "ping", "" );
			}
		}
		else
		{
			pKeyValues->SetInt( "ping", iPing );
		}

		UpdatePlayerAvatar( playerIndex, pKeyValues );

		int itemID = m_pPlayerLists[iTeam]->AddItem( 0, pKeyValues );

		Color clr;
		if ( TDCGameRules()->IsTeamplay() )
		{
			clr = g_PR->GetTeamColor( g_PR->GetTeam( playerIndex ) );
		}
		else
		{
			clr = g_TDC_PR->GetPlayerColor( playerIndex );
		}
		clr[3] = g_TDC_PR->IsAlive( playerIndex ) ? 204 : 51;

		m_pPlayerLists[iTeam]->SetItemFgColor( itemID, COLOR_WHITE );
		m_pPlayerLists[iTeam]->SetItemBgColor( itemID, clr );

		itemID = m_pPingLists[iTeam]->AddItem( 0, pKeyValues );

		if ( !bIsBot )
		{
			// Progress ping color from green to yellow to red as it gets worse.
			clr[0] = RemapValClamped( iPing, 70, 20, 190, 85 );
			clr[1] = RemapValClamped( iPing, 120, 70, 60, 190 );
			clr[2] = 60;
			clr[3] = 255;
		}
		else
		{
			clr.SetColor( 180, 189, 213, 255 );
		}

		m_pPingLists[iTeam]->SetItemFgColor( itemID, clr );
		m_pPingLists[iTeam]->SetItemBgColor( itemID, Color( 100, 100, 100, 204 ) );

		pKeyValues->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator list
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::UpdateSpectatorList( void )
{
	if ( !g_TDC_PR )
		return;

	// Spectators
	char szSpectatorList[512] = "";
	int nSpectators = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !g_TDC_PR->IsConnected( i ) )
			continue;

		if ( TDCGameRules()->IsInDuelMode() && !g_TDC_PR->IsArenaSpectator( i ) )
			continue;

		if ( g_TDC_PR->GetTeam( i ) == TEAM_SPECTATOR )
		{
			if ( nSpectators > 0 )
			{
				V_strcat_safe( szSpectatorList, ", " );
			}

			V_strcat_safe( szSpectatorList, g_TDC_PR->GetPlayerName( i ) );
			nSpectators++;
		}
	}

	wchar_t wzSpectators[512] = L"";
	if ( nSpectators > 0 )
	{
		const char *pchFormat = ( 1 == nSpectators ? "#ScoreBoard_Spectator" : "#ScoreBoard_Spectators" );

		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		V_swprintf_safe( wzSpectatorCount, L"%i", nSpectators );
		g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
		g_pVGuiLocalize->ConstructString( wzSpectators, sizeof( wzSpectators ), g_pVGuiLocalize->Find( pchFormat ), 2, wzSpectatorCount, wzSpectatorList );
	}
	SetDialogVariable( "spectators", wzSpectators );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::UpdateArenaWaitingToPlayList( void )
{
	if ( !g_TDC_PR || !TDCGameRules()->IsInDuelMode() )
	{
		SetDialogVariable( "waitingtoplay", "" );
		return;
	}

	// Spectators
	char szSpectatorList[512] = "";
	int nSpectators = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !g_TDC_PR->IsConnected( i ) )
			continue;

		if ( g_TDC_PR->GetTeam( i ) == TEAM_SPECTATOR && !g_TDC_PR->IsArenaSpectator( i ) )
		{
			if ( nSpectators > 0 )
			{
				V_strcat_safe( szSpectatorList, ", " );
			}

			V_strcat_safe( szSpectatorList, g_TDC_PR->GetPlayerName( i ) );
			nSpectators++;
		}
	}

	wchar_t wzSpectators[512] = L"";
	if ( nSpectators > 0 )
	{
		const char *pchFormat = ( 1 == nSpectators ? "#TDC_Arena_ScoreBoard_Spectator" : "#TDC_Arena_ScoreBoard_Spectators" );

		wchar_t wzSpectatorCount[16];
		wchar_t wzSpectatorList[1024];
		V_swprintf_safe( wzSpectatorCount, L"%i", nSpectators );
		g_pVGuiLocalize->ConvertANSIToUnicode( szSpectatorList, wzSpectatorList, sizeof( wzSpectatorList ) );
		g_pVGuiLocalize->ConstructString( wzSpectators, sizeof( wzSpectators ), g_pVGuiLocalize->Find( pchFormat ), 2, wzSpectatorCount, wzSpectatorList );
	}
	SetDialogVariable( "waitingtoplay", wzSpectators );
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CTDCClientScoreBoardDialog::TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData( itemID1 );
	KeyValues *it2 = list->GetItemData( itemID2 );
	Assert( it1 && it2 );

	// first compare score
	int v1 = it1->GetInt( "score" );
	int v2 = it2->GetInt( "score" );
	if ( v1 > v2 )
		return true;
	else if ( v1 < v2 )
		return false;

	// if score is the same, use player index to get deterministic sort
	int iPlayerIndex1 = it1->GetInt( "playerIndex" );
	int iPlayerIndex2 = it2->GetInt( "playerIndex" );
	return ( iPlayerIndex1 > iPlayerIndex2 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a localized string of form "1 point", "2 points", etc for specified # of points
//-----------------------------------------------------------------------------
const wchar_t *GetPointsString( int iPoints )
{
	wchar_t wzScoreVal[128];
	static wchar_t wzScore[128];
	V_swprintf_safe( wzScoreVal, L"%i", iPoints );
	if ( 1 == iPoints )
	{
		g_pVGuiLocalize->ConstructString( wzScore, sizeof( wzScore ), g_pVGuiLocalize->Find( "#TDC_ScoreBoard_Point" ), 1, wzScoreVal );
	}
	else
	{
		g_pVGuiLocalize->ConstructString( wzScore, sizeof( wzScore ), g_pVGuiLocalize->Find( "#TDC_ScoreBoard_Points" ), 1, wzScoreVal );
	}
	return wzScore;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( FStrEq( type, "server_spawn" ) )
	{
		// set server name in scoreboard
		const char *hostname = event->GetString( "hostname" );
		wchar_t wzHostName[256];
		wchar_t wzServerLabel[256];
		g_pVGuiLocalize->ConvertANSIToUnicode( hostname, wzHostName, sizeof( wzHostName ) );
		g_pVGuiLocalize->ConstructString( wzServerLabel, sizeof( wzServerLabel ), g_pVGuiLocalize->Find( "#Scoreboard_Server" ), 1, wzHostName );
		SetDialogVariable( "server", wzServerLabel );

		// Set the level name after the server spawn
		SetDialogVariable( "mapname", GetMapDisplayName( event->GetString( "mapname" ) ) );
	}
	else if ( FStrEq( type, "game_maploaded" ) )
	{
		SetDialogVariable( "gametype", g_pVGuiLocalize->Find( TDCGameRules()->GetGameTypeName() ) );

		const char *pszIcon = VarArgs( "hud/scoreboard_gametype_%s", TDCGameRules()->GetGameTypeInfo()->name );
		IMaterial *pMaterial = materials->FindMaterial( pszIcon, TEXTURE_GROUP_VGUI, false );
		if ( pMaterial->IsErrorMaterial() )
		{
			pszIcon = "hud/scoreboard_gametype_unknown";
		}

		char szIconProper[MAX_PATH];
		V_sprintf_safe( szIconProper, "../%s", pszIcon );
		m_pGameTypeIcon->SetImage( szIconProper );

		InvalidateLayout( false, true );
	}

	if ( IsVisible() )
	{
		Update();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SectionedListPanel *CTDCClientScoreBoardDialog::GetSelectedPlayerList( void )
{
	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		SectionedListPanel *pList = m_pPlayerLists[i];
		if ( pList->GetSelectedItem() >= 0 )
		{
			return pList;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Center the dialog on the screen.  (vgui has this method on
//			Frame, but we're an EditablePanel, need to roll our own.)
//-----------------------------------------------------------------------------
void CTDCClientScoreBoardDialog::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds( wx, wy, ww, wt );
	SetPos( ( ww - GetWide() ) / 2, ( wt - GetTall() ) / 2 );
}
