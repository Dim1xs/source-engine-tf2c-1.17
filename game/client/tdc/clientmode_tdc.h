//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_CLIENTMODE_H
#define TDC_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "tdc_viewport.h"
#include "GameUI/IGameUI.h"

class CTDCClientScoreBoardDialog;
class CTDCFreezePanel;

class ClientModeTFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual void	OverrideView( CViewSetup *pSetup );

//	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual bool	DoPostScreenSpaceEffects( const CViewSetup *pSetup );

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

	virtual float	GetViewModelFOV( void );
	virtual bool	ShouldDrawViewModel();

	int				GetDeathMessageStartHeight( void );

	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	PostRenderVGui();

	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *cmd );

	virtual int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual int		HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	void			UpdateSteamScreenshotsHooking( void );
	void			OnScreenshotRequested( ScreenshotRequested_t *info );
	void			OnScreenshotReady( ScreenshotReady_t *info );

	void			OnLobbyCreated( LobbyCreated_t *pCallResult, bool iofailure );
	void			OnLobbyEnter( LobbyEnter_t *pCallResult, bool iofailure );
	void			OnLobbyJoinRequested( GameLobbyJoinRequested_t *pInfo );
	void			OnLobbyChatUpdate( LobbyChatUpdate_t *pInfo );
	void			OnLobbyChatMsg( LobbyChatMsg_t *pInfo );

	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_create", CreateLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_leave", LeaveLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_players", ListLobbyPlayers, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "lobby_invite", InviteLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "connect_lobby", ConnectLobby, "", 0 );
	CON_COMMAND_MEMBER_F( ClientModeTFNormal, "say_lobby", SayLobby, "", 0 );
	
private:
	
	//	void	UpdateSpectatorMode( void );

private:
	CTDCFreezePanel		*m_pFreezePanel;
	IGameUI			*m_pGameUI;

	CTDCClientScoreBoardDialog *m_pScoreboard;

	CSteamID m_LobbyID;

	CCallback<ClientModeTFNormal, ScreenshotRequested_t> m_sScreenshotRequestedCallback;
	CCallback<ClientModeTFNormal, ScreenshotReady_t> m_sScreenshotReadyCallback;

	CCallResult<ClientModeTFNormal, LobbyCreated_t> m_LobbyCreateResult;
	CCallResult<ClientModeTFNormal, LobbyEnter_t> m_LobbyEnterResult;
	CCallback<ClientModeTFNormal, GameLobbyJoinRequested_t> m_LobbyJoinCallback;
	CCallback<ClientModeTFNormal, LobbyChatUpdate_t> m_LobyChatUpdateCallback;
	CCallback<ClientModeTFNormal, LobbyChatMsg_t> m_LobbyChatMsgCallback;
};


extern IClientMode *GetClientModeNormal();
extern ClientModeTFNormal* GetClientModeTFNormal();

#endif // TDC_CLIENTMODE_H
