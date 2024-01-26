//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TDC_GAMERULES_H
#define TDC_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplay_gamerules.h"
#include "tdc_round_timer.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "GameEventListener.h"
#include "tdc_gamestats_shared.h"

#ifdef CLIENT_DLL
#include "c_tdc_player.h"
#else
#include "tdc_player.h"
#endif

#ifdef CLIENT_DLL

#define CTDCGameRules C_TDCGameRules
#define CTDCGameRulesProxy C_TDCGameRulesProxy

#else

class CHealthKit;

extern ConVar mp_stalemate_timelimit;

#endif

extern ConVar tdc_avoidteammates_pushaway;

class CTDCGameRules;

//-----------------------------------------------------------------------------
// Round states
//-----------------------------------------------------------------------------
enum gamerules_roundstate_t
{
	// initialize the game, create teams
	GR_STATE_INIT = 0,

	//Before players have joined the game. Periodically checks to see if enough players are ready
	//to start a game. Also reverts to this when there are no active players
	GR_STATE_PREGAME,

	//The game is about to start, wait a bit and spawn everyone
	GR_STATE_STARTGAME,

	//All players are respawned, frozen in place
	GR_STATE_PREROUND,

	//Round is on, playing normally
	GR_STATE_RND_RUNNING,

	//Someone has won the round
	GR_STATE_TEAM_WIN,

	//Noone has won, manually restart the game, reset scores
	GR_STATE_RESTART,

	//Noone has won, restart the game
	GR_STATE_STALEMATE,

	//Game is over, showing the scoreboard etc
	GR_STATE_GAME_OVER,

	//Game is in a bonus state, transitioned to after a round ends
	GR_STATE_BONUS,

	//Game is awaiting the next wave/round of a multi round experience
	GR_STATE_BETWEEN_RNDS,

	GR_NUM_ROUND_STATES
};

enum {
	WINREASON_NONE = 0,
	WINREASON_ALL_POINTS_CAPTURED,
	WINREASON_OPPONENTS_DEAD,
	WINREASON_FLAG_CAPTURE_LIMIT,
	WINREASON_DEFEND_UNTIL_TIME_LIMIT,
	WINREASON_STALEMATE,
	WINREASON_TIMELIMIT,
	WINREASON_WINLIMIT,
	WINREASON_WINDIFFLIMIT,
	WINREASON_RD_REACTOR_CAPTURED,
	WINREASON_RD_CORES_COLLECTED,
	WINREASON_RD_REACTOR_RETURNED,
	// TDC
	WINREASON_VIP_ESCAPED,
	WINREASON_ROUNDSCORELIMIT,
	WINREASON_DUEL_FRAGLIMIT,
	WINREASON_DUEL_OPPONENTLEFT,
	WINREASON_DUEL_BOTHPLAYERSLEFT,
	
	WINREASON_COUNT
};

enum stalemate_reasons_t
{
	STALEMATE_JOIN_MID,
	STALEMATE_TIMER,
	STALEMATE_SERVER_TIMELIMIT,

	NUM_STALEMATE_REASONS,
};

//-----------------------------------------------------------------------------
// Purpose: Per-state data
//-----------------------------------------------------------------------------
class CGameRulesRoundStateInfo
{
public:
	gamerules_roundstate_t	m_iRoundState;
	const char				*m_pStateName;

	void ( CTDCGameRules::*pfnEnterState )( );	// Init and deinit the state.
	void ( CTDCGameRules::*pfnLeaveState )( );
	void ( CTDCGameRules::*pfnThink )( );	// Do a PreThink() in this state.
};

#ifdef GAME_DLL
class CTDCRadiusDamageInfo
{
public:
	CTDCRadiusDamageInfo();

	bool ApplyToEntity( CBaseEntity *pEntity ) const;

public:
	CTakeDamageInfo m_DmgInfo;
	Vector m_vecSrc;
	float m_flRadius;
	float m_flSelfDamageRadius;
	int m_iClassIgnore;
	CBaseEntity *m_pEntityIgnore;
	ETDCWeaponID m_iWeaponID;
};

struct KillingWeaponData_t
{
	KillingWeaponData_t()
	{
		szWeaponName[0] = '\0';
		szWeaponLogName[0] = '\0';
		iWeaponID = WEAPON_NONE;
	}

	char szWeaponName[128];
	char szWeaponLogName[128];
	ETDCWeaponID iWeaponID;
};
#endif

class CTDCGameRulesProxy : public CGameRulesProxy, public CGameEventListener
{
public:
	DECLARE_CLASS( CTDCGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	CTDCGameRulesProxy();
	~CTDCGameRulesProxy();

	friend class CTDCGameRules;

	virtual void FireGameEvent( IGameEvent *event );

	void	InputRoundActivate( inputdata_t &data );
	void	InputSetStalemateOnTimelimit( inputdata_t &inputdata );
	void	InputSetRedTeamGoalString( inputdata_t &inputdata );
	void	InputSetBlueTeamGoalString( inputdata_t &inputdata );
	void	InputSetRequiredObserverTarget( inputdata_t &inputdata );
	void	InputAddRedTeamScore( inputdata_t &inputdata );
	void	InputAddBlueTeamScore( inputdata_t &inputdata );

	void	InputPlayVO( inputdata_t &inputdata );
	void	InputPlayVORed( inputdata_t &inputdata );
	void	InputPlayVOBlue( inputdata_t &inputdata );

	void	InputHandleMapEvent( inputdata_t &inputdata );
	void	InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata );

	virtual void Activate();
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	ETDCGameType GetGameTypeFromString( const char *pszName );
	bool IsGameTypeValid( ETDCGameType iType );
	void OnGameTypeStart( ETDCGameType iType );

private:
	bool m_bEnabledModes[TDC_GAMETYPE_COUNT];
	ETDCGameType m_iDefaultType;

	COutputEvent m_OnStartMode[TDC_GAMETYPE_COUNT];
	COutputEvent m_OnWonByTeam1;
	COutputEvent m_OnWonByTeam2;
	COutputEvent m_Team1PlayersChanged;
	COutputEvent m_Team2PlayersChanged;
	COutputEvent m_OnStateEnterBetweenRounds;
	COutputEvent m_OnStateEnterPreRound;
	COutputEvent m_OnStateExitPreRound;
	COutputEvent m_OnStateEnterRoundRunning;

#endif

	//----------------------------------------------------------------------------------
	// Client specific
#ifdef CLIENT_DLL
public:
	void			OnPreDataChanged( DataUpdateType_t updateType );
	void			OnDataChanged( DataUpdateType_t updateType );
	virtual void	FireGameEvent( IGameEvent *event ) {}
#endif // CLIENT_DLL
};

struct PlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int	iTotalScore;	// total points scored across all rounds
	int	iKills;
	int iDeaths;
};

struct ArenaPlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int iDamage;
	int iHealing;
	int iLifeTime;
	int iKills;
};

#define MAX_TEAMGOAL_STRING		256

class CTDCGameRules : public CTeamplayRules, public CGameEventListener
{
public:
	DECLARE_CLASS( CTDCGameRules, CTeamplayRules );
	DECLARE_NETWORKCLASS_NOBASE();

	CTDCGameRules();

	friend class CTDCGameRulesProxy;

	enum
	{
		HALLOWEEN_SCENARIO_DOOMSDAY
	};

	// Damage Queries.
	bool			Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	bool			Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	bool			Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP:
	int				Damage_GetTimeBased( void );
	int				Damage_GetShowOnHud( void );
	int				Damage_GetShouldNotBleed( void );

	float			GetLastRoundStateChangeTime( void ) const { return m_flLastRoundStateChangeTime; }
	float			m_flLastRoundStateChangeTime;

	// Data accessors
	inline gamerules_roundstate_t State_Get( void ) { return m_iRoundState; }
	bool			IsInWaitingForPlayers( void ) { return m_bInWaitingForPlayers; }
	bool			InRoundRestart( void ) { return State_Get() == GR_STATE_PREROUND; }
	bool			InStalemate( void ) { return State_Get() == GR_STATE_STALEMATE; }
	bool			RoundHasBeenWon( void ) { return State_Get() == GR_STATE_TEAM_WIN; }
	float			GetNextRespawnWave( int iTeam, CBasePlayer *pPlayer );

	// Return false if players aren't allowed to cap points at this time (i.e. in WaitingForPlayers)
	bool			PointsMayBeCaptured( void );

	void			LevelInitPostEntity( void );
	bool			ShouldRespawnQuickly( CBasePlayer *pPlayer ) { return false; }

	int				GetWinningTeam( void ) { return m_iWinningTeam; }
	int				GetWinReason() { return m_iWinReason; }
	bool			InOvertime( void ) { return m_bInOvertime; }
	bool			InSetup( void ) { return m_bInSetup; }

	bool			SwitchedTeamsThisRound( void ) { return m_bSwitchedTeamsThisRound; }
	bool			ShouldBalanceTeams( void );
	bool			WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam );
	bool			AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam );
	bool			IsInTournamentMode( void );
	bool			IsInPreMatch( void ) { return ( IsInTournamentMode() && IsInWaitingForPlayers() ); }
	bool			IsWaitingForTeams( void ) { return m_bAwaitingReadyRestart; }
	bool			IsInStopWatch( void ) { return m_bStopWatch; }
	void			SetInStopWatch( bool bState ) { m_bStopWatch = bState; }
	void			StopWatchModeThink( void ) { }

	bool IsTeamReady( int iTeamNumber )
	{
		return m_bTeamReady[iTeamNumber];
	}

	bool IsPlayerReady( int iIndex )
	{
		return m_bPlayerReady[iIndex];
	}

	void			HandleTeamScoreModify( int iTeam, int iScore ) {  }

	float			GetRoundRestartTime( void ) { return m_flRestartRoundTime; }
	int				GetBonusRoundTime( bool bFinal = false );

	void			SetAllowBetweenRounds( bool bValue ) { m_bAllowBetweenRounds = bValue; }
	bool			HaveCheatsBeenEnabledDuringLevel( void ) { return m_bCheatsEnabledDuringLevel; }

	static int		CalcPlayerScore( RoundStats_t *pRoundStats );

	const unsigned char *GetEncryptionKey( void ) { return ( unsigned char * )"E2NcUkG2"; }

#ifdef GAME_DLL
public:
	bool			TimerMayExpire( void );
	void			HandleSwitchTeams( void );
	void			HandleScrambleTeams( void );

	// Override this to prevent removal of game specific entities that need to persist
	bool			RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	bool			ShouldCreateEntity( const char *pszClassName );

	void			RunPlayerConditionThink( void );
	void			FrameUpdatePostEntityThink();

	// Called when a new round is being initialized
	void			SetupOnRoundStart( void );

	// Called when a new round is off and running
	void			SetupOnRoundRunning( void );

	void			PreRound_End( void );

	// Called before a new round is started (so the previous round can end)
	void			PreviousRoundEnd( void );

	// Send the team scores down to the client
	void			SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	void			SendWinPanelInfo( void );

	// Called when a round has entered stalemate mode (timer has run out)
	void			SetupOnStalemateStart( void );
	void			SetupOnStalemateEnd( void );
	void			SetSetup( bool bSetup );

	bool			ShouldGoToBonusRound( void ) { return false; }
	void			SetupOnBonusStart( void ) { return; }
	void			SetupOnBonusEnd( void ) { return; }
	void			BonusStateThink( void ) { return; }

	void			BetweenRounds_Start( void ) { return; }
	void			BetweenRounds_End( void ) { return; }
	void			BetweenRounds_Think( void ) { return; }

	bool			PrevRoundWasWaitingForPlayers() { return m_bPrevRoundWasWaitingForPlayers; }

	bool			CheckNextLevelCvar( bool bAllowEnd = true );

	bool			IsValveMap( void ) { return false; }

	void			RestartTournament( void );

	bool			TournamentModeCanEndWithTimelimit( void ) { return true; }

public:
	void			State_Transition( gamerules_roundstate_t newState );

	void			RespawnPlayers( bool bForceRespawn, bool bTeam = false, int iTeam = TEAM_UNASSIGNED );
	void			UpdateRespawnWaves( int iTeam );

	void			SetWinningTeam( int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false ); //OVERRIDE; // SanyaSho: wtf
	void			SetStalemate( int iReason, bool bForceMapReset = true, bool bSwitchTeams = false );

	float			GetWaitingForPlayersTime( void ) { return mp_waitingforplayers_time.GetFloat(); }
	void			ShouldResetScores( bool bResetTeam, bool bResetPlayer ){ m_bResetTeamScores = bResetTeam; m_bResetPlayerScores = bResetPlayer; }
	void			ShouldResetRoundsPlayed( bool bResetRoundsPlayed ){ m_bResetRoundsPlayed = bResetRoundsPlayed; }

	void			SetStalemateOnTimelimit( bool bStalemate ) { m_bAllowStalemateAtTimelimit = bStalemate; }

	bool			IsGameUnderTimeLimit( void );

	CTeamRoundTimer *GetActiveRoundTimer( void );

	void			HandleTimeLimitChange( void );

	void SetTeamReadyState( bool bState, int iTeam )
	{
		m_bTeamReady.Set( iTeam, bState );
	}

	void SetPlayerReadyState( int iIndex, bool bState )
	{
		m_bPlayerReady.Set( iIndex, bState );
	}
	void			ResetPlayerAndTeamReadyState( void );

	bool			PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext );

	void			BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags = 0 );
	int				GetRoundsPlayed( void ) { return m_nRoundsPlayed; }

	bool			ShouldSkipAutoScramble( void );

	bool			ShouldWaitToStartRecording( void ){ return IsInWaitingForPlayers(); }

	void			SetOvertime( bool bOvertime );

	void			Activate();

	bool			AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

protected:
	void			CheckChatText( CBasePlayer *pPlayer, char *pText );
	void			CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg );

	void			SetInWaitingForPlayers( bool bWaitingForPlayers );
	void			CheckWaitingForPlayers( void );
	bool			AllowWaitingForPlayers( void ) { return true; }
	void			CheckRestartRound( void );
	bool			CheckTimeLimit( bool bAllowEnd = true );
	bool			CheckWinLimit( bool bAllowEnd = true );
	bool			CheckMaxRounds( bool bAllowEnd = true );
	int				GetWinsRemaining( void );
	int				GetRoundsRemaining( void );

	void			CheckReadyRestart( void );

	bool	CanChangelevelBecauseOfTimeLimit( void );
	bool	CanGoToStalemate( void );

	// State machine handling
	void State_Enter( gamerules_roundstate_t newState );	// Initialize the new state.
	void State_Leave();										// Cleanup the previous state.
	void State_Think();										// Update the current state.
	static CGameRulesRoundStateInfo* State_LookupInfo( gamerules_roundstate_t state );	// Find the state info for the specified state.

	// State Functions
	void State_Enter_INIT( void );
	void State_Think_INIT( void );

	void State_Enter_PREGAME( void );
	void State_Think_PREGAME( void );

	void State_Enter_STARTGAME( void );
	void State_Think_STARTGAME( void );

	void State_Enter_PREROUND( void );
	void State_Leave_PREROUND( void );
	void State_Think_PREROUND( void );

	void State_Enter_RND_RUNNING( void );
	void State_Think_RND_RUNNING( void );

	void State_Enter_TEAM_WIN( void );
	void State_Think_TEAM_WIN( void );

	void State_Enter_RESTART( void );
	void State_Think_RESTART( void );

	void State_Enter_STALEMATE( void );
	void State_Think_STALEMATE( void );
	void State_Leave_STALEMATE( void );

	void State_Enter_BONUS( void );
	void State_Think_BONUS( void );
	void State_Leave_BONUS( void );

	void State_Enter_BETWEEN_RNDS( void );
	void State_Leave_BETWEEN_RNDS( void );
	void State_Think_BETWEEN_RNDS( void );

	// mp_scrambleteams_auto
	void ResetTeamsRoundWinTracking( void );

protected:
	void			InitTeams( void );

	int				CountActivePlayers( void );

	void			RoundRespawn( void );
	void			CleanUpMap( void );
	void			CheckRespawns( void );
	float			GetRespawnTime( CTDCPlayer* pPlayer );
	bool			HasRespawnWaitPeriodPassed( CTDCPlayer* pPlayer );
	void			BalanceTeams( bool bRequireSwitcheesToBeDead );
	void			ResetScores( void );
	void			ResetMapTime( void );

	void			PlayStartRoundVoice( void );
	void			PlayWinSong( int team );
	void			PlayStalemateSong( void );
	void			PlaySuddenDeathSong( void );

	void			RespawnTeam( int iTeam ) { RespawnPlayers( false, true, iTeam ); }

	void			InternalHandleTeamWin( int iWinningTeam );

	bool			MapHasActiveTimer( void );
	void			CreateTimeLimitTimer( void );

	float			GetLastMajorEventTime( void ) OVERRIDE{ return m_flLastTeamWin; }

	static int		PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 );
	static int		ArenaPlayerRoundScoreSortFunc( const ArenaPlayerRoundScore_t *pRoundScore1, const ArenaPlayerRoundScore_t *pRoundScore2 );

#endif // GAME_DLL

public:
	// Collision and Damage rules.
	bool			ShouldCollide( int collisionGroup0, int collisionGroup1 );

	int				GetTimeLeft( void );

	// Get the view vectors for this mod.
	const CViewVectors *GetViewVectors() const;

	void			FireGameEvent( IGameEvent *event );

	const char		*GetGameTypeName( void ) { return g_aGameTypeInfo[m_nGameType].localized_name; }
	int				GetGameType( void ) { return m_nGameType.Get(); }
	GameTypeInfo_t	*GetGameTypeInfo( void ) { return &g_aGameTypeInfo[m_nGameType.Get()]; }

	bool			FlagsMayBeCapped( void );

	const char		*GetTeamGoalString( int iTeam );

	bool			IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );
	bool			AllowThirdPersonCamera( void );
	CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );

	bool			IsTeamplay( void ) { return m_bTeamPlay; }
	bool			IsRoundBasedMode( void ) { return g_aGameTypeInfo[m_nGameType].roundbased; }
	bool			IsInDuelMode( void ) { return m_nGameType == TDC_GAMETYPE_DUEL; }
	bool			IsBloodMoney( void ) { return ( m_nGameType == TDC_GAMETYPE_BM || m_nGameType == TDC_GAMETYPE_TBM ); }
	bool			IsInfectionMode( void ) { return m_nGameType == TDC_GAMETYPE_INFECTION; }
	bool			IsInstagib( void ) { return m_bInstagib; }
	bool			IsVIPMode( void ) { return m_nGameType == TDC_GAMETYPE_VIP; }
	bool			ShouldWaitForPlayersInPregame( void ) { return IsInDuelMode() || IsInfectionMode() || IsVIPMode(); }

	CTDCPlayer		*GetVIP( void ) { return m_hVIPPlayer; }
	void			ResetVIP( void ) { m_hVIPPlayer = NULL; }

	//Training Mode
	bool			IsInTraining( void ) { return false; }
	bool			IsInItemTestingMode( void ) { return false; }

	CTeamRoundTimer *GetWaitingForPlayersTimer( void ) { return m_hWaitingForPlayersTimer; }
	CTeamRoundTimer *GetStalemateTimer( void ) { return m_hStalemateTimer; }
	CTeamRoundTimer *GetTimeLimitTimer( void ) { return m_hTimeLimitTimer; }
	CTeamRoundTimer *GetRoundTimer( void ) { return m_hRoundTimer; }

	int				GetScoreLimit( void );

#ifdef CLIENT_DLL

	void			SetRoundState( int iRoundState );
	void			OnPreDataChanged( DataUpdateType_t updateType );
	void			OnDataChanged( DataUpdateType_t updateType );
	void			HandleOvertimeBegin();
	void			GetTeamGlowColor( int nTeam, float &r, float &g, float &b );

	bool			ShouldShowTeamGoal( void );

	const char		*GetVideoFileForMap( bool bWithExtension = true );

	bool			AllowWeatherParticles( void );

#else

	~CTDCGameRules();

	void LevelShutdown();

	bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	void Think();

	void GoToIntermission( void );
	bool CheckScoreLimit( bool bAllowEnd, bool &bTied );
	int GetFragsRemaining( void );

	bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );

	// Spawing rules.
	CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers );

	virtual int ItemShouldRespawn( CItem *pItem );
	float FlItemRespawnTime( CItem *pItem );
	Vector VecItemRespawnSpot( CItem *pItem );
	QAngle VecItemRespawnAngles( CItem *pItem );
	bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

	const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void ClientSettingsChanged( CBasePlayer *pPlayer );
	void GetTaggedConVarList( KeyValues *pCvarTagList );
	void ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );
	void ChangePlayerName( CTDCPlayer *pPlayer, const char *pszNewName );

	void OnNavMeshLoad();

	VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem );

	int GetAutoAimMode() { return AUTOAIM_NONE; }

	bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex );

	const char *GetGameDescription( void );

	// trace line rules
	float WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd, unsigned int mask, trace_t *ptr );

	// Sets up g_pPlayerResource.
	void CreateStandardEntities();

	void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );

	void CalcDominationAndRevenge( CTDCPlayer *pAttacker, CTDCPlayer *pVictim, bool bIsAssist, int *piDeathFlags );

	void GetKillingWeaponName( const CTakeDamageInfo &info, CTDCPlayer *pVictim, KillingWeaponData_t &weaponData );
	CTDCPlayer *GetAssister( CTDCPlayer *pVictim, CTDCPlayer *pScorer, const CTakeDamageInfo &info );
	CTDCPlayer *GetRecentDamager( CTDCPlayer *pVictim, int iDamager, float flMaxElapsed );

	bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	void ClientDisconnected( edict_t *pClient );

	void	RadiusDamage( CTDCRadiusDamageInfo &radiusInfo );
	void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );

	float FlPlayerFallDamage( CBasePlayer *pPlayer );

	bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

	bool UseSuicidePenalty() { return false; }

	int		GetPreviousRoundWinners( void ) { return m_iPreviousRoundWinners; }

	void	SendHudNotification( IRecipientFilter &filter, HudNotification_t iType );
	void	SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam = TEAM_UNASSIGNED );

	void			SetTeamGoalString( int iTeam, const char *pszGoal );

	void			HandleCTDCCaptureBonus( int iTeam );

	// Speaking, vcds, voice commands.
	void			InitCustomResponseRulesDicts();
	void			ShutdownCustomResponseRulesDicts();

	int				PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	void			SetKothTimer( CTeamRoundTimer *pTimer, int iTeam );

	void			ManageServerSideVoteCreation( void );
	void			StartNextMapVote( void );
	void			ScheduleNextMapVote( void );
	void			ExtendCurrentMap( void );

	CBaseEntity		*GetRequiredObserverTarget( void ) { return m_hRequiredObserverTarget.Get(); }
	void			SetRequiredObserverTarget( CBaseEntity *pEntity ) { m_hRequiredObserverTarget = pEntity; }

	void			SetPreRoundFreezeTimeEnabled( bool bEnabled ) { m_bPreRoundFreezeTime = bEnabled; }

	void			SendDeathmatchResults( void );

	bool			AssignNextDuelingPlayers( void );
	void			StartDuelRound( void );
	bool			AreNextDuelingPlayersSet( void );
	CTDCPlayer		*GetDuelOpponent( CTDCPlayer *pPlayer );
	void			SetWinningPlayer( CTDCPlayer *pPlayer, int iWinReason = WINREASON_NONE );
	CTDCPlayer		*GetWinnngPlayer( void ) { return m_hWinningPlayer.Get(); }

	void			StartInfectedRound( void );
	bool			Infected_RunLogic( void );

	void			StartVIPRound( void );
	bool			VIP_RunLogic( void );
	CTDCPlayer		*SelectVIP( int iTeamNum );

	int				GetAssignedHumanTeam();

private:

	int DefaultFOV( void ) { return 90; }

#endif

private:

	// Server specific
#ifdef GAME_DLL
	CGameRulesRoundStateInfo	*m_pCurStateInfo;			// Per-state data 
	float						m_flStateTransitionTime;	// Timer for round states

	float						m_flWaitingForPlayersTimeEnds;
	

	float						m_flNextPeriodicThink;
	bool						m_bChangeLevelOnRoundEnd;

	bool						m_bResetTeamScores;
	bool						m_bResetPlayerScores;
	bool						m_bResetRoundsPlayed;

	// Stalemate
	float						m_flStalemateStartTime;

	bool						m_bPrevRoundWasWaitingForPlayers;	// was the previous map reset after a waiting for players period

	bool						m_bAllowStalemateAtTimelimit;
	bool						m_bChangelevelAfterStalemate;

	float						m_flRoundStartTime;		// time the current round started
	float						m_flNewThrottledAlertTime;		// time that we can play another throttled alert

	int							m_nRoundsPlayed;

	gamerules_roundstate_t		m_prevState;

	bool						m_bPlayerReadyBefore[MAX_PLAYERS + 1];	// Test to see if a player has hit ready before

	float						m_flLastTeamWin;

	float m_flStartBalancingTeamsAt;
	float m_flNextBalanceTeamsTime;
	bool m_bPrintedUnbalanceWarning;
	float m_flFoundUnbalancedTeamsTime;

	float	m_flAutoBalanceQueueTimeEnd;
	int		m_nAutoBalanceQueuePlayerIndex;
	int		m_nAutoBalanceQueuePlayerScore;

	//----------------------------------------------------------------------------------

	CUtlVector<CHandle<CHealthKit> > m_hDisabledHealthKits;

	char	m_szMostRecentCappers[MAX_PLAYERS + 1];	// list of players who made most recent capture.  Stored as string so it can be passed in events.

	bool m_bVoteMapOnNextRound;
	bool m_bVotedForNextMap;
	int m_iMaxRoundsBonus;
	int m_iWinLimitBonus;

	float m_flNextVoteThink;

	EHANDLE m_hRequiredObserverTarget;
	bool m_bPreRoundFreezeTime;

	CHandle<CTDCPlayer> m_hWinningPlayer;
	CUtlVector<CHandle<CTDCPlayer>> m_NextDuelingPlayers;

	CHandle<CTDCPlayer> m_hLastStandingPlayer;

	KeyValues *m_pAuthData;
#endif
	// End server specific

	// End server specific
	//----------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------
	// Client specific
#ifdef CLIENT_DLL
	bool			m_bOldInWaitingForPlayers;
	bool			m_bOldInOvertime;
	bool			m_bOldInSetup;
#endif // CLIENT_DLL

private:
	CNetworkVar( gamerules_roundstate_t, m_iRoundState );
	CNetworkVar( bool, m_bInOvertime ); // Are we currently in overtime?
	CNetworkVar( bool, m_bInSetup ); // Are we currently in setup?
	CNetworkVar( bool, m_bSwitchedTeamsThisRound );
	CNetworkVar( int, m_iWinningTeam );				// Set before entering GR_STATE_TEAM_WIN
	CNetworkVar( int, m_iWinReason );
	CNetworkVar( bool, m_bInWaitingForPlayers );
	CNetworkVar( bool, m_bAwaitingReadyRestart );
	CNetworkVar( float, m_flRestartRoundTime );
	CNetworkVar( float, m_flMapResetTime );						// Time that the map was reset
	CNetworkArray( bool, m_bTeamReady, MAX_TEAMS );
	CNetworkVar( bool, m_bStopWatch );
	CNetworkArray( bool, m_bPlayerReady, MAX_PLAYERS );
	CNetworkVar( bool, m_bCheatsEnabledDuringLevel );

	//----------------------------------------------------------------------------------

private:
	CNetworkVar( ETDCGameType, m_nGameType ); // Type of game this map is (CTDC, CP)
	CNetworkString( m_pszTeamGoalStringRed, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringBlue, MAX_TEAMGOAL_STRING );
	CNetworkVar( bool, m_bTeamPlay );
	CNetworkVar( bool, m_bInstagib );
	CNetworkHandle( CTeamRoundTimer, m_hWaitingForPlayersTimer );
	CNetworkHandle( CTeamRoundTimer, m_hStalemateTimer );
	CNetworkHandle( CTeamRoundTimer, m_hTimeLimitTimer );
	CNetworkHandle( CTeamRoundTimer, m_hRoundTimer );
	CNetworkVar( int, m_iMapTimeBonus );
	CNetworkVar( int, m_iFragLimitBonus );
	CNetworkVar( float, m_flNextRespawnWaveTime );
	CNetworkHandle( CTDCPlayer, m_hVIPPlayer );

public:

	float	m_flStopWatchTotalTime;

	//----------------------------------------------------------------------------------

	int	 m_iPreviousRoundWinners;

private:
	bool	m_bAllowBetweenRounds;

#ifdef GAME_DLL // TFBot stuff
private:
	CountdownTimer m_ctBotCountUpdate;
#endif
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CTDCGameRules* TDCGameRules()
{
	return static_cast<CTDCGameRules*>( g_pGameRules );
}

#ifdef GAME_DLL
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );
#endif

#ifdef CLIENT_DLL
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
#endif

#endif // TDC_GAMERULES_H
