//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "clientmode_tdc.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/ISurface.h"
#include "vgui/IPanel.h"
#include "GameUI/IGameUI.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "buymenu.h"
#include "filesystem.h"
#include "vgui/IVGui.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <KeyValues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "c_tdc_player.h"
#include "ienginevgui.h"
#include "in_buttons.h"
#include "voice_status.h"
#include "tdc_clientscoreboard.h"
#include "tdc_hud_freezepanel.h"
#include "clienteffectprecachesystem.h"
#include "glow_outline_effect.h"
#include "cam_thirdperson.h"
#include "tdc_gamerules.h"
#include "tdc_hud_chat.h"
#include "c_tdc_team.h"
#include "c_tdc_playerresource.h"
#include "hud_macros.h"
#include "iviewrender.h"
#include "tdc_mainmenu.h"
#include <tier0/icommandline.h>
#include "clientsteamcontext.h"

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "90", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 90, true, MAX_FOV );

void SteamScreenshotsCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( GetClientModeTFNormal() )
	{
		GetClientModeTFNormal()->UpdateSteamScreenshotsHooking();
	}
}
ConVar cl_steamscreenshots( "cl_steamscreenshots", "1", FCVAR_ARCHIVE, "Enable/disable saving screenshots to Steam", SteamScreenshotsCallback );

void HUDMinModeChangedCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	engine->ExecuteClientCmd( "hud_reloadscheme" );
}
ConVar cl_hud_minmode( "cl_hud_minmode", "0", FCVAR_ARCHIVE, "Set to 1 to turn on the advanced minimalist HUD mode.", HUDMinModeChangedCallBack );
ConVar tdc_coloredhud( "tdc_coloredhud", "1", FCVAR_ARCHIVE, "Set to 1 to enable colored HUD in FFA Deathmatch.", HUDMinModeChangedCallBack );

extern ConVar tdc_scoreboard_mouse_mode;

IClientMode *g_pClientMode = NULL;
// --------------------------------------------------------------------------------- //
// CTDCModeManager.
// --------------------------------------------------------------------------------- //

class CTDCModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CTDCModeManager g_ModeManager;
IVModeManager *modemanager = (IVModeManager *)&g_ModeManager;

CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffectsGlow )
CLIENTEFFECT_MATERIAL( "dev/glow_blur_x" )
CLIENTEFFECT_MATERIAL( "dev/glow_blur_y" )
CLIENTEFFECT_MATERIAL( "dev/glow_color" )
CLIENTEFFECT_MATERIAL( "dev/glow_downsample" )
CLIENTEFFECT_MATERIAL( "dev/halo_add_to_screen" )
CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90 )

// --------------------------------------------------------------------------------- //
// CTDCModeManager implementation.
// --------------------------------------------------------------------------------- //

void CTDCModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();
	GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

void CTDCModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );

	ConVarRef voice_steal( "voice_steal" );

	if ( voice_steal.IsValid() )
	{
		voice_steal.SetValue( 1 );
	}

	g_ThirdPersonManager.Init();
}

void CTDCModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeTFNormal::ClientModeTFNormal() :
m_sScreenshotRequestedCallback( this, &ClientModeTFNormal::OnScreenshotRequested ),
m_sScreenshotReadyCallback( this, &ClientModeTFNormal::OnScreenshotReady ),
m_LobbyJoinCallback( this, &ClientModeTFNormal::OnLobbyJoinRequested ),
m_LobyChatUpdateCallback( this, &ClientModeTFNormal::OnLobbyChatUpdate ),
m_LobbyChatMsgCallback( this, &ClientModeTFNormal::OnLobbyChatMsg )
{
	m_pGameUI = NULL;
	m_pFreezePanel = NULL;

	m_pScoreboard = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeTFNormal::~ClientModeTFNormal()
{
}

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUI( "GameUI" );
extern uint64 groupcheckmask;
extern uint64 groupcheck;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Init()
{
#ifdef STAGING_ONLY && 0
	bool bAuthorized = false;

	if ( ClientSteamContext().BLoggedOn() )
	{
		int numGroups = steamapicontext->SteamFriends()->GetClanCount();

		for ( int i = 0; i < numGroups; i++ )
		{
			CSteamID clanID = steamapicontext->SteamFriends()->GetClanByIndex( i );
			Assert( clanID.IsValid() );
			if ( ( clanID.ConvertToUint64() ^ groupcheckmask ) == groupcheck )
			{
				bAuthorized = true;
				break;
			}
		}
	}

	if ( bAuthorized )
	{
		QAngle ang( QAngle( 0, 0, 180 ) );
		engine->SetViewAngles( ang );
	}
#endif

	m_pFreezePanel = GET_HUDELEMENT( CTDCFreezePanel );
	Assert( m_pFreezePanel );

	CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
	if ( gameUIFactory )
	{
		m_pGameUI = (IGameUI *)gameUIFactory( GAMEUI_INTERFACE_VERSION, NULL );
		if ( m_pGameUI )
		{
#if 0
			// Create the loading screen panel.
			CTDCLoadingScreen *pPanel = new CTDCLoadingScreen();
			pPanel->InvalidateLayout( false, true );
			pPanel->SetVisible( false );
			pPanel->MakePopup( false );
			m_pGameUI->SetLoadingBackgroundDialog( pPanel->GetVPanel() );
#endif

			// Create the new main menu.
			if ( CommandLine()->CheckParm( "-nonewmenu" ) == NULL )
			{
				// Disable normal BG music played in GameUI.
				CommandLine()->AppendParm( "-nostartupsound", NULL );
				CTDCMainMenu *pMenu = new CTDCMainMenu();
				m_pGameUI->SetMainMenuOverride( pMenu->GetVPanel() );
			}
		}
	}

	ListenForGameEvent( "teamplay_teambalanced_player" );
	ListenForGameEvent( "duel_start" );
	ListenForGameEvent( "duel_end" );
	ListenForGameEvent( "game_maploaded" );

	// Move Steam notications to the upper left corner.
	if ( steamapicontext->SteamUtils() )
	{
		steamapicontext->SteamUtils()->SetOverlayNotificationPosition( k_EPositionTopLeft );
	}

	UpdateSteamScreenshotsHooking();

	BaseClass::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Shutdown()
{
	if ( m_LobbyID.IsValid() )
	{
		steamapicontext->SteamMatchmaking()->LeaveLobby( m_LobbyID );
	}
}

void ClientModeTFNormal::InitViewport()
{
	InitPlayerClasses();

	// Parse weapon data.
	for ( int i = WEAPON_NONE + 1; i < WEAPON_COUNT; i++ )
	{
		WEAPON_FILE_INFO_HANDLE hWeapon;
		ReadWeaponDataFromFileForSlot( filesystem, WeaponIdToClassname( (ETDCWeaponID)i ), &hWeapon, ( unsigned char * )"E2NcUkG2" );
	}

	m_pViewport = new TFViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

ClientModeTFNormal g_ClientModeNormal;

IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}


ClientModeTFNormal* GetClientModeTFNormal()
{
	return assert_cast<ClientModeTFNormal*>( GetClientModeNormal() );
}

//-----------------------------------------------------------------------------
// Purpose: Fixes some bugs from base class.
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OverrideView( CViewSetup *pSetup )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	// Let the player override the view.
	pPlayer->OverrideView( pSetup );

	if ( ::input->CAM_IsThirdPerson() )
	{
		int iObserverMode = pPlayer->GetObserverMode();
		if ( iObserverMode == OBS_MODE_NONE || iObserverMode == OBS_MODE_IN_EYE )
		{
			QAngle camAngles;
			if ( pPlayer->InThirdPersonShoulder() && !pPlayer->m_Shared.IsMovementLocked() )
			{
				VectorCopy( pSetup->angles, camAngles );
			}
			else
			{
				const Vector& cam_ofs = g_ThirdPersonManager.GetCameraOffsetAngles();
				camAngles[PITCH] = cam_ofs[PITCH];
				camAngles[YAW] = cam_ofs[YAW];
				camAngles[ROLL] = 0;

				// Override angles from third person camera
				VectorCopy( camAngles, pSetup->angles );
			}

			Vector camForward, camRight, camUp, cam_ofs_distance;

			// get the forward vector
			AngleVectors( camAngles, &camForward, &camRight, &camUp );

			cam_ofs_distance = g_ThirdPersonManager.GetDesiredCameraOffset();
			cam_ofs_distance *= g_ThirdPersonManager.GetDistanceFraction();

			VectorMA( pSetup->origin, -cam_ofs_distance[DIST_FORWARD], camForward, pSetup->origin );
			VectorMA( pSetup->origin, cam_ofs_distance[DIST_RIGHT], camRight, pSetup->origin );
			VectorMA( pSetup->origin, cam_ofs_distance[DIST_UP], camUp, pSetup->origin );
		}
	}
	else if ( ::input->CAM_IsOrthographic() )
	{
		pSetup->m_bOrtho = true;
		float w, h;
		::input->CAM_OrthographicSize( w, h );
		w *= 0.5f;
		h *= 0.5f;
		pSetup->m_OrthoLeft = -w;
		pSetup->m_OrthoTop = -h;
		pSetup->m_OrthoRight = w;
		pSetup->m_OrthoBottom = h;
	}
}

extern ConVar v_viewmodel_fov;
float ClientModeTFNormal::GetViewModelFOV( void )
{
#if 0
	C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( !pPlayer )
		return v_viewmodel_fov.GetFloat();

	return pPlayer->GetViewModelFOV();
#else
	return v_viewmodel_fov.GetFloat();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::ShouldDrawViewModel()
{
	C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( !pPlayer || pPlayer->m_Shared.InCond( TDC_COND_ZOOMED ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	if ( !IsInFreezeCam() )
		g_GlowObjectManager.RenderGlowEffects( pSetup, 0 );

	return BaseClass::DoPostScreenSpaceEffects( pSetup );
}

int ClientModeTFNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}


bool IsInCommentaryMode( void );
bool PlayerNameNotSetYet( const char *pszName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( !eventname || !eventname[0] )
		return;

	if ( V_strcmp( "player_changename", eventname ) == 0 )
	{
		CHudChat *pChat = GetTDCChatHud();
		if ( !pChat )
			return;

		const char *pszOldName = event->GetString( "oldname" );
		if ( PlayerNameNotSetYet( pszOldName ) )
			return;

		int iPlayerIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

		wchar_t wszOldName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszOldName, wszOldName, sizeof( wszOldName ) );

		wchar_t wszNewName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString( "newname" ), wszNewName, sizeof( wszNewName ) );

		wchar_t wszLocalized[128];
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_Name_Change" ), 2, wszOldName, wszNewName );

		char szLocalized[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_NAMECHANGE, "%s", szLocalized );
	}
	else if ( V_strcmp( "player_team", eventname ) == 0 )
	{
		// Using our own strings here.
		CHudChat *pChat = GetTDCChatHud();
		if ( !pChat )
			return;

		if ( event->GetBool( "silent" ) || event->GetBool( "disconnect" ) || IsInCommentaryMode() )
			return;

		int iPlayerIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		int iTeam = event->GetInt( "team" );
		bool bAutoTeamed = event->GetBool( "autoteam" );

		const char *pszName = event->GetString( "name" );
		if ( PlayerNameNotSetYet( pszName ) )
			return;

		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

		wchar_t wszTeam[64];
		V_wcscpy_safe( wszTeam, GetLocalizedTeamName( iTeam ) );

		// Client isn't going to catch up on team change so we have to set the color manually here.
		Color col = pChat->GetTeamColor( iTeam );

		wchar_t wszLocalized[128];

		if ( TDCGameRules()->IsTeamplay() )
		{
			if ( bAutoTeamed )
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_Joined_AutoTeam" ), 2, wszPlayerName, wszTeam );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_Joined_Team" ), 2, wszPlayerName, wszTeam );
			}
		}
		else
		{
			if ( iTeam >= FIRST_GAME_TEAM )
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_Joined_Deathmatch" ), 1, wszPlayerName );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_Joined_Deathmatch_Spectator" ), 1, wszPlayerName );
			}

			if ( g_TDC_PR )
			{
				col = g_TDC_PR->GetPlayerColor( iPlayerIndex );
			}
		}

		char szLocalized[100];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->SetCustomColor( col );
		pChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_TEAMCHANGE, "%s", szLocalized );

		C_BasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );

		if ( pPlayer && pPlayer->IsLocalPlayer() )
		{
			// that's me
			pPlayer->TeamChange( iTeam );
		}
	}
	else if ( V_strcmp( "teamplay_teambalanced_player", eventname ) == 0 )
	{
		CHudChat *pChat = GetTDCChatHud();
		if ( !pChat || !g_PR )
			return;

		int iPlayerIndex = event->GetInt( "player" );
		const char *pszName = g_PR->GetPlayerName( iPlayerIndex );
		int iTeam = event->GetInt( "team" );

		// Get the names of player and team.
		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

		wchar_t wszTeam[64];
		V_wcscpy_safe( wszTeam, GetLocalizedTeamName( iTeam ) );

		wchar_t wszLocalized[128];
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_AutoBalanced" ), 2, wszPlayerName, wszTeam );

		char szLocalized[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		// Set the color to team's color.
		pChat->SetCustomColor( pChat->GetTeamColor( iTeam ) );
		pChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_TEAMCHANGE, "%s", szLocalized );
	}
	else if ( V_strcmp( "duel_start", eventname ) == 0 )
	{
		CHudChat *pChat = GetTDCChatHud();
		if ( !pChat || !g_PR )
			return;

		int iPlayer1 = event->GetInt( "player_1" );
		int iPlayer2 = event->GetInt( "player_2" );

		wchar_t wszPlayerName1[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName2[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayer1 ), wszPlayerName1, sizeof( wszPlayerName1 ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayer2 ), wszPlayerName2, sizeof( wszPlayerName2 ) );

		wchar_t wszLocalized[128];
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_DuelStart" ), 2, wszPlayerName1, wszPlayerName2 );

		char szLocalized[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->ChatPrintf( 0, CHAT_FILTER_NONE, "%s", szLocalized );
	}
	else if ( V_strcmp( "duel_end", eventname ) == 0 )
	{
		CHudChat *pChat = GetTDCChatHud();
		if ( !pChat || !g_PR )
			return;

		int iWinner = event->GetInt( "winner" );

		wchar_t wszLocalized[128];
		if ( iWinner )
		{
			wchar_t wszWinnerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iWinner ), wszWinnerName, sizeof( wszWinnerName ) );

			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_DuelEnd" ), 1, wszWinnerName );
		}
		else
		{
			V_wcscpy_safe( wszLocalized, g_pVGuiLocalize->Find( "#TDC_DuelEnd_Draw" ) );
		}

		char szLocalized[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->ChatPrintf( iWinner, CHAT_FILTER_NONE, "%s", szLocalized );

		int iPlayer1 = event->GetInt( "next_player_1" );
		int iPlayer2 = event->GetInt( "next_player_2" );
		if ( !iPlayer1 || !iPlayer2 )
			return;

		wchar_t wszPlayerName1[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName2[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayer1 ), wszPlayerName1, sizeof( wszPlayerName1 ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayer2 ), wszPlayerName2, sizeof( wszPlayerName2 ) );

		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#TDC_NextDuel" ), 2, wszPlayerName1, wszPlayerName2 );
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->ChatPrintf( 0, CHAT_FILTER_NONE, "%s", szLocalized );
	}
	else if ( V_strcmp( "player_disconnect", eventname ) == 0 )
	{
		CHudChat *pChat = GetTDCChatHud();
		if ( !pChat )
			return;

		if ( IsInCommentaryMode() )
			return;

		const char *pszName = event->GetString( "name" );

		if ( PlayerNameNotSetYet( pszName ) )
			return;

		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszName, wszPlayerName, sizeof( wszPlayerName ) );

		wchar_t wszReason[64];
		const char *pszReason = event->GetString( "reason" );
		if ( pszReason && ( pszReason[0] == '#' ) && g_pVGuiLocalize->Find( pszReason ) )
		{
			V_wcscpy_safe( wszReason, g_pVGuiLocalize->Find( pszReason ) );
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszReason, wszReason, sizeof( wszReason ) );
		}

		wchar_t wszLocalized[100];
		if ( IsPC() )
		{
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_left_game" ), 2, wszPlayerName, wszReason );
		}
		else
		{
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_left_game" ), 1, wszPlayerName );
		}

		char szLocalized[100];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

		pChat->ChatPrintf( 0, CHAT_FILTER_JOINLEAVE, "%s", szLocalized );
	}
	else if ( V_strcmp( "game_maploaded", eventname ) == 0 )
	{
		// Remember which scoreboard should be used on this map.
		m_pScoreboard = (CTDCClientScoreBoardDialog *)GetTDCViewPort()->FindPanelByName( GetTDCViewPort()->GetModeSpecificScoreboardName() );
	}
	else
	{
		BaseClass::FireGameEvent( event );
	}
}


void ClientModeTFNormal::PostRenderVGui()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
	return BaseClass::CreateMove( flInputSampleTime, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: See if hud elements want key input. Return 0 if the key is swallowed
//-----------------------------------------------------------------------------
int	ClientModeTFNormal::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( tdc_scoreboard_mouse_mode.GetInt() == 2 )
	{
		if ( m_pScoreboard && !m_pScoreboard->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pFreezePanel )
	{
		m_pFreezePanel->HudElementKeyInput( down, keynum, pszCurrentBinding );
	}

	return BaseClass::HudElementKeyInput( down, keynum, pszCurrentBinding );
}

//-----------------------------------------------------------------------------
// Purpose: See if spectator input occurred. Return 0 if the key is swallowed.
//-----------------------------------------------------------------------------
int ClientModeTFNormal::HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( tdc_scoreboard_mouse_mode.GetInt() == 2 )
	{
		// Scoreboard allows enabling mouse input with right click so don't steal input from it.
		if ( m_pScoreboard && m_pScoreboard->IsVisible() )
		{
			return 1;
		}
	}

	if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+use" ) == 0 )
	{
		engine->ClientCmd( "spec_mode" );
		return 0;
	}

	if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+jump" ) == 0 )
	{
		return 1;
	}

	return BaseClass::HandleSpectatorKeyInput( down, keynum, pszCurrentBinding );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::UpdateSteamScreenshotsHooking( void )
{
	if ( steamapicontext->SteamScreenshots() )
	{
		ConVarRef cl_savescreenshotstosteam( "cl_savescreenshotstosteam" );
		if ( cl_savescreenshotstosteam.IsValid() )
		{
			cl_savescreenshotstosteam.SetValue( cl_steamscreenshots.GetBool() );
			steamapicontext->SteamScreenshots()->HookScreenshots( cl_steamscreenshots.GetBool() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnScreenshotRequested( ScreenshotRequested_t *info )
{
	// Fake screenshot button press.
	HudElementKeyInput( 0, BUTTON_CODE_INVALID, "screenshot" );
	engine->ClientCmd( "screenshot" );
}

const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnScreenshotReady( ScreenshotReady_t *info )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( pPlayer && !enginevgui->IsGameUIVisible() && !vgui::surface()->IsCursorVisible() && info->m_eResult == k_EResultOK )
	{
		// Set map name.
		char szMapName[MAX_MAP_NAME];
		V_FileBase( engine->GetLevelName(), szMapName, sizeof( szMapName ) );
		steamapicontext->SteamScreenshots()->SetLocation( info->m_hLocal, GetMapDisplayName( szMapName ) );

		// Now tag users.
		CTDCFreezePanel *pFreezePanel = GET_HUDELEMENT( CTDCFreezePanel );
		if ( pFreezePanel && pFreezePanel->IsVisible() )
		{
			for ( CSteamID steamID : pFreezePanel->GetUsersInScreenshot() )
			{
				steamapicontext->SteamScreenshots()->TagUser( info->m_hLocal, steamID );
			}
		}
		else
		{
			CUtlVector<CSteamID> userList;
			pPlayer->CollectVisibleSteamUsers( userList );

			for ( CSteamID steamID : userList )
			{
				steamapicontext->SteamScreenshots()->TagUser( info->m_hLocal, steamID );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::CreateLobby( const CCommand &args )
{
	if ( !steamapicontext->SteamMatchmaking() )
		return;

	if ( !m_LobbyID.IsValid() )
	{
		ELobbyType type = args.ArgC() >= 2 ? k_ELobbyTypePrivate : k_ELobbyTypeFriendsOnly;
		SteamAPICall_t hCall = steamapicontext->SteamMatchmaking()->CreateLobby( type, 24 );
		m_LobbyCreateResult.Set( hCall, this, &ClientModeTFNormal::OnLobbyCreated );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::LeaveLobby( const CCommand &args )
{
	if ( m_LobbyID.IsValid() )
	{
		steamapicontext->SteamMatchmaking()->LeaveLobby( m_LobbyID );
		m_LobbyID.Clear();
		Msg( "You left the lobby.\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::InviteLobby( const CCommand &arg )
{
	if ( m_LobbyID.IsValid() )
	{
		steamapicontext->SteamFriends()->ActivateGameOverlayInviteDialog( m_LobbyID );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::ConnectLobby( const CCommand &args )
{
	if ( args.ArgC() < 2 )
		return;

	if ( !steamapicontext->SteamMatchmaking() )
		return;

	CSteamID lobbyID( V_atoui64( args[1] ) );
	SteamAPICall_t hCall = steamapicontext->SteamMatchmaking()->JoinLobby( lobbyID );
	m_LobbyEnterResult.Set( hCall, this, &ClientModeTFNormal::OnLobbyEnter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::ListLobbyPlayers( const CCommand &args )
{
	if ( !m_LobbyID.IsValid() )
		return;

	char szUsers[1024] = "Players: ";

	int count = steamapicontext->SteamMatchmaking()->GetNumLobbyMembers( m_LobbyID );
	for ( int i = 0; i < count; i++ )
	{
		if ( i != 0 )
		{
			V_strcat_safe( szUsers, ", " );
		}

		CSteamID steamID = steamapicontext->SteamMatchmaking()->GetLobbyMemberByIndex( m_LobbyID, i );
		const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( steamID );
		V_strcat_safe( szUsers, pszName );
	}

	Msg( "%s\n", szUsers );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::SayLobby( const CCommand &args )
{
	if ( !m_LobbyID.IsValid() || args.ArgC() < 2 )
		return;

	steamapicontext->SteamMatchmaking()->SendLobbyChatMsg( m_LobbyID, args[1], strlen( args[1] ) + 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnLobbyCreated( LobbyCreated_t *pCallResult, bool iofailure )
{
	if ( pCallResult->m_eResult == k_EResultOK )
	{
		m_LobbyID.SetFromUint64( pCallResult->m_ulSteamIDLobby );
		Msg( "Created a lobby.\n" );
	}
	else
	{
		Msg( "Failed to create a lobby.\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnLobbyEnter( LobbyEnter_t *pCallResult, bool iofailure )
{
	if ( pCallResult->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess )
	{
		m_LobbyID.SetFromUint64( pCallResult->m_ulSteamIDLobby );
		Msg( "Entered a lobby with %d users.\n", steamapicontext->SteamMatchmaking()->GetNumLobbyMembers( m_LobbyID ) );
	}
	else
	{
		Msg( "Failed to join the lobby.\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnLobbyJoinRequested( GameLobbyJoinRequested_t *pInfo )
{
	SteamAPICall_t hCall = steamapicontext->SteamMatchmaking()->JoinLobby( pInfo->m_steamIDLobby );
	m_LobbyEnterResult.Set( hCall, this, &ClientModeTFNormal::OnLobbyEnter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnLobbyChatUpdate( LobbyChatUpdate_t *pInfo )
{
	CSteamID steamID( pInfo->m_ulSteamIDUserChanged );

	if ( pInfo->m_rgfChatMemberStateChange == k_EChatMemberStateChangeEntered )
	{
		Msg( "%s joined the lobby.\n", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ) );
	}
	else
	{
		Msg( "%s left the lobby.\n", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OnLobbyChatMsg( LobbyChatMsg_t *pInfo )
{
	char szMessage[4096];
	CSteamID steamID;
	EChatEntryType type;
	steamapicontext->SteamMatchmaking()->GetLobbyChatEntry( m_LobbyID, pInfo->m_iChatID, &steamID, szMessage, sizeof( szMessage ), &type );

	Msg( "%s: %s\n", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ), szMessage );
}

//-----------------------------------------------------------------------------
// Purpose: Sends an event once all map entities have been spawned.
//-----------------------------------------------------------------------------
class CPostEntityHudInitSystem : public CAutoGameSystem
{
public:
	CPostEntityHudInitSystem( const char *name ) : CAutoGameSystem( name )
	{
	}

	virtual void LevelInitPostEntity( void )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "game_maploaded" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
} g_PostEntityHudInit( "PostEntityHudInit" );

ConVar tdc_hud_drawdummy( "tdc_hud_drawummy", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Dummy HUD element for testing RES files.
//-----------------------------------------------------------------------------
class CHudDummy : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudDummy, vgui::EditablePanel );

	CHudDummy( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDummy" )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
	}

	virtual bool ShouldDraw( void ) { return tdc_hud_drawdummy.GetBool(); }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		LoadControlSettings( "resource/UI/HudDummy.res" );
	}
};

DECLARE_HUDELEMENT( CHudDummy );
