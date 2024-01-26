//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//
#ifndef TDC_GAMESTATS_H
#define TDC_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "gamestats.h"
#include "tdc_gamestats_shared.h"

class CTDCPlayer;

//=============================================================================
//
// TF Game Stats Class
//
class CTDCGameStats : public CBaseGameStats, public CAutoGameSystemPerFrame
{
public:

	// Constructor/Destructor.
	CTDCGameStats( void );
	~CTDCGameStats( void );

	virtual void Clear( void );

	// Stats enable.
	virtual bool StatTrackingEnabledForMod( void );
	virtual bool ShouldTrackStandardStats( void ) { return false; } 
	virtual bool AutoUpload_OnLevelShutdown( void ) { return true; }
	virtual bool AutoSave_OnShutdown( void ) { return false; }
	virtual bool AutoUpload_OnShutdown( void ) { return false; }

	virtual bool LoadFromFile( void );

	// Buffers.
	virtual void AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer );
	virtual void LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer );

	virtual bool Init();

	// Events.
	virtual void Event_LevelInit( void );
	virtual void Event_LevelShutdown( float flElapsed );
	virtual void Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info );
	void Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime, bool bWasSuddenDeathWin );
	void Event_PlayerConnected( CBasePlayer *pPlayer );
	void Event_PlayerDisconnected( CBasePlayer *pPlayer );
	void Event_PlayerChangedClass( CTDCPlayer *pPlayer );
	void Event_PlayerSpawned( CTDCPlayer *pPlayer );
	void Event_PlayerForceRespawn( CTDCPlayer *pPlayer );
	void Event_PlayerLeachedHealth( CTDCPlayer *pPlayer, bool bDispenserHeal, float amount );
	void Event_PlayerHealedOther( CTDCPlayer *pPlayer, float amount );
	void Event_AssistKill( CTDCPlayer *pPlayer, CBaseEntity *pVictim );
	void Event_PlayerInvulnerable( CTDCPlayer *pPlayer );
	void Event_Headshot( CTDCPlayer *pKiller );
	void Event_Backstab( CTDCPlayer *pKiller );
	void Event_PlayerUsedTeleport( CTDCPlayer *pTeleportOwner, CTDCPlayer *pTeleportingPlayer );
	void Event_PlayerFiredWeapon( CTDCPlayer *pPlayer, bool bCritical );
	void Event_PlayerDamage( CBasePlayer *pPlayer, const CTakeDamageInfo &info, int iDamageTaken );
	void Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info );
	void Event_PlayerSuicide( CBasePlayer *pPlayer );
	void Event_PlayerCapturedPoint( CTDCPlayer *pPlayer );
	void Event_PlayerDefendedPoint( CTDCPlayer *pPlayer );
	void Event_PlayerDominatedOther( CTDCPlayer *pAttacker );
	void Event_PlayerRevenge( CTDCPlayer *pAttacker );
	void Event_MaxSentryKills( CTDCPlayer *pAttacker, int iMaxKills );
	void Event_PlayerAwardBonusPoints( CTDCPlayer *pPlayer, CBaseEntity *pAwarder, int iAmount );
	void Event_GameEnd( void );
	void Event_PlayerScoresEscortPoints( CTDCPlayer *pPlayer, int iAmount );
	void Event_PlayerMovedToSpectators( CTDCPlayer *pPlayer );

	virtual void FrameUpdatePostEntityThink();

	void AccumulateGameData();
	void ClearCurrentGameData();
	bool ShouldSendToClient( TFStatType_t statType );
	void GetVoteData( const char *pszVoteIssue, int iNumOptions, CUtlStringList &optionList );

	// Utilities.
	TDC_Gamestats_LevelStats_t	*GetCurrentMap( void )			{ return m_reportedStats.m_pCurrentGame; }

	struct PlayerStats_t *		FindPlayerStats( CBasePlayer *pPlayer );
	void						ResetPlayerStats( CTDCPlayer *pPlayer );
	void						ResetKillHistory( CTDCPlayer *pPlayer, bool bTeammatesOnly = false );
	void						ResetRoundStats();
protected:	
	void						IncrementStat( CTDCPlayer *pPlayer, TFStatType_t statType, int iValue );
	void						SendStatsToPlayer( CTDCPlayer *pPlayer, int iMsgType );
	void						AccumulateAndResetPerLifeStats( CTDCPlayer *pPlayer );
	void						TrackKillStats( CBasePlayer *pAttacker, CBasePlayer *pVictim );

protected:

public:
	TFReportedStats_t			m_reportedStats;		// Stats which are uploaded from TF server to Steam
protected:

	PlayerStats_t				m_aPlayerStats[MAX_PLAYERS+1];	// List of stats for each player for current life - reset after each death or class change
};

extern CTDCGameStats CTDC_GameStats;

#endif // TDC_GAMESTATS_H
