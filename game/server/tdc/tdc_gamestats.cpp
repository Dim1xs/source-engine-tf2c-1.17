//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tdc_gamerules.h"
#include "tdc_gamestats.h"
#include "usermessages.h"
#include "player_resource.h"
#include "tdc_team.h"
#include "hl2orange.spa.h"

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar tdc_stats_track( "tdc_stats_track", "1", FCVAR_NONE, "Turn on//off tf stats tracking." );
static ConVar tdc_stats_verbose( "tdc_stats_verbose", "0", FCVAR_NONE, "Turn on//off verbose logging of stats." );

extern ConVar tdc_nemesis_relationships;

CTDCGameStats CTDC_GameStats;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTDCGameStats::CTDCGameStats()
{
	gamestats = this;
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :  - 
//-----------------------------------------------------------------------------
CTDCGameStats::~CTDCGameStats()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Clear out game stats
// Input  :  - 
//-----------------------------------------------------------------------------
void CTDCGameStats::Clear( void )
{
	m_reportedStats.Clear();
	Q_memset( m_aPlayerStats, 0, sizeof( m_aPlayerStats ) );
	CBaseGameStats::Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCGameStats::StatTrackingEnabledForMod( void )
{
	return tdc_stats_track.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Loads previously saved game stats from file
//-----------------------------------------------------------------------------
bool CTDCGameStats::LoadFromFile( void )
{
	// We deliberately don't load from previous file.  That's data we've already
	// reported, and for TF stats we don't want to re-accumulate data, just
	// keep sending fresh stuff to server.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::AppendCustomDataToSaveBuffer( CUtlBuffer &SaveBuffer )
{
	m_reportedStats.AppendCustomDataToSaveBuffer( SaveBuffer );
	// clear stats since we've now reported these
	m_reportedStats.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::LoadCustomDataFromBuffer( CUtlBuffer &LoadBuffer )
{
	m_reportedStats.LoadCustomDataFromBuffer( LoadBuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCGameStats::Init( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_LevelInit( void )
{
	CBaseGameStats::Event_LevelInit();

	ClearCurrentGameData();

	// Get the host ip and port.
	int nIPAddr = 0;
	short nPort = 0;
	ConVar *hostip = cvar->FindVar( "hostip" );
	if ( hostip )
	{
		nIPAddr = hostip->GetInt();
	}			

	ConVar *hostport = cvar->FindVar( "hostport" );
	if ( hostport )
	{
		nPort = hostport->GetInt();
	}			

	m_reportedStats.m_pCurrentGame->Init( STRING( gpGlobals->mapname ), nIPAddr, nPort, gpGlobals->curtime );

	TDC_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	map->Init( STRING( gpGlobals->mapname ), nIPAddr, nPort, gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_LevelShutdown( float flElapsed )
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		flElapsed = gpGlobals->curtime - m_reportedStats.m_pCurrentGame->m_flRoundStartTime;
		m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flElapsed;
	}

	// add current game data in to data for this level
	AccumulateGameData();

	CBaseGameStats::Event_LevelShutdown( flElapsed );
}

//-----------------------------------------------------------------------------
// Purpose: Resets all stats for this player
//-----------------------------------------------------------------------------
void CTDCGameStats::ResetPlayerStats( CTDCPlayer *pPlayer )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	// reset the stats on this player
	stats.Reset();
	// reset the matrix of who killed whom with respect to this player
	ResetKillHistory( pPlayer );
	// let the client know to reset its stats
	SendStatsToPlayer( pPlayer, STATMSG_RESET );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the kill history for this player
//-----------------------------------------------------------------------------
void CTDCGameStats::ResetKillHistory( CTDCPlayer *pPlayer, bool bTeammatesOnly /*= false*/ )
{
	int iPlayerIndex = pPlayer->entindex();

	// for every other player, set all all the kills with respect to this player to 0
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{
		if ( bTeammatesOnly )
		{
			CBasePlayer *pOther = UTIL_PlayerByIndex( i );

			if ( pOther && pPlayer->IsEnemy( pOther ) )
				continue;
		}

		PlayerStats_t &statsOther = m_aPlayerStats[i];
		statsOther.statsKills.iNumKilled[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledBy[iPlayerIndex] = 0;
		statsOther.statsKills.iNumKilledByUnanswered[iPlayerIndex] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets per-round stats for all players
//-----------------------------------------------------------------------------
void CTDCGameStats::ResetRoundStats()
{
	for ( int i = 0; i < ARRAYSIZE( m_aPlayerStats ); i++ )
	{		
		m_aPlayerStats[i].statsCurrentRound.Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increments specified stat for specified player by specified amount
//-----------------------------------------------------------------------------
void CTDCGameStats::IncrementStat( CTDCPlayer *pPlayer, TFStatType_t statType, int iValue )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
	stats.statsCurrentLife.m_iStat[statType] += iValue;
	stats.statsCurrentRound.m_iStat[statType] += iValue;
	stats.statsAccumulated.m_iStat[statType] += iValue;

	// if this stat should get sent to client, mark it as dirty
	if ( ShouldSendToClient( statType ) )
	{
		stats.iStatsChangedBits |= 1 << ( statType - TFSTAT_FIRST );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::SendStatsToPlayer( CTDCPlayer *pPlayer, int iMsgType )
{
	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	int iSendBits = stats.iStatsChangedBits;
	switch ( iMsgType )
	{
	case STATMSG_PLAYERDEATH:
	case STATMSG_PLAYERRESPAWN:
	case STATMSG_GAMEEND:
		// Calc player score from this life.
		AccumulateAndResetPerLifeStats( pPlayer );
		iSendBits = stats.iStatsChangedBits;
		break;
	case STATMSG_RESET:
		// this is a reset message, no need to send any stat values with it
		iSendBits = 0;
		break;
	case STATMSG_UPDATE:
		// if nothing changed, no need to send a message
		if ( iSendBits == 0 )
			return;
		break;
	case STATMSG_PLAYERSPAWN:
		// do a full update at player spawn
		for ( int i = TFSTAT_FIRST; i < TFSTAT_MAX; i++ )
		{
			iSendBits |= ( 1 << ( i - TFSTAT_FIRST ) );
		}
		break;
	default:
		Assert( false );
	}

	int iStat = TFSTAT_FIRST;
	CSingleUserRecipientFilter filter( pPlayer );
	UserMessageBegin( filter, "PlayerStatsUpdate" );
	WRITE_BYTE( pPlayer->GetPlayerClass()->GetClassIndex() );		// write the class
	WRITE_BYTE( iMsgType );											// type of message
	WRITE_LONG( iSendBits );										// write the bit mask of which stats follow in the message

	// write all the stats specified in the bit mask
	while ( iSendBits > 0 )
	{
		if ( iSendBits & 1 )
		{
			WRITE_LONG( stats.statsAccumulated.m_iStat[iStat] );
		}
		iSendBits >>= 1;
		iStat ++;
	}
	MessageEnd();

	stats.iStatsChangedBits = 0;
	stats.m_flTimeLastSend = gpGlobals->curtime;

	if ( iMsgType == STATMSG_PLAYERDEATH || iMsgType == STATMSG_GAMEEND )
	{
		// max sentry kills is different from other stats, it is a max value and can span player lives.  Reset it to zero so 
		// it doesn't get re-reported in the next life unless the sentry stays alive and gets more kills.
		Event_MaxSentryKills( pPlayer, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::AccumulateAndResetPerLifeStats( CTDCPlayer *pPlayer )
{
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];

	// add score from previous life and reset current life stats
	int iScore = TDCGameRules()->CalcPlayerScore( &stats.statsCurrentLife );
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iScore += iScore;
	}
	stats.statsCurrentRound.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsAccumulated.m_iStat[TFSTAT_POINTSSCORED] += iScore;
	stats.statsCurrentLife.Reset();

	if ( iScore != 0 )
	{
		stats.iStatsChangedBits |= 1 << ( TFSTAT_POINTSSCORED - TFSTAT_FIRST );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerConnected( CBasePlayer *pPlayer )
{
	ResetPlayerStats( ToTDCPlayer( pPlayer ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerDisconnected( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );
	if ( !pTFPlayer )
		return;

	ResetPlayerStats( pTFPlayer );

	if ( pPlayer->IsAlive() )
	{
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerChangedClass( CTDCPlayer *pPlayer )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerSpawned( CTDCPlayer *pPlayer )
{	
	// if player is spawning as a member of valid team, increase the spawn count for his class
	int iTeam = pPlayer->GetTeamNumber();
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( TEAM_UNASSIGNED != iTeam && TEAM_SPECTATOR != iTeam )
	{
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iSpawns++;
		}
	}

	TDC_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( !map )
		return;

	// calculate peak player count on each team
	for ( iTeam = FIRST_GAME_TEAM; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		CTDCTeam *pTeam = GetGlobalTFTeam( iTeam );

		int iPlayerCount = pTeam->GetNumPlayers();
		if ( iPlayerCount > map->m_iPeakPlayerCount[iTeam] )
		{
			map->m_iPeakPlayerCount[iTeam] = iPlayerCount;
		}
	}

	if ( iClass >= TDC_FIRST_NORMAL_CLASS && iClass < TDC_CLASS_COUNT_ALL )
	{
		SendStatsToPlayer( pPlayer, STATMSG_PLAYERSPAWN );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void CTDCGameStats::Event_PlayerForceRespawn( CTDCPlayer *pPlayer )
{
	if ( pPlayer->IsAlive() && !TDCGameRules()->PrevRoundWasWaitingForPlayers() )
	{		
		// send stats to player
		SendStatsToPlayer( pPlayer, STATMSG_PLAYERRESPAWN );

		// if player is alive before respawn, add time from this life to class stat
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pPlayer->GetSpawnTime() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerLeachedHealth( CTDCPlayer *pPlayer, bool bDispenserHeal, float amount ) 
{
	IncrementStat( pPlayer, TFSTAT_HEALTHLEACHED, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerHealedOther( CTDCPlayer *pPlayer, float amount ) 
{
	IncrementStat( pPlayer, TFSTAT_HEALING, (int) amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_AssistKill( CTDCPlayer *pAttacker, CBaseEntity *pVictim )
{
	// increment player's stat
	IncrementStat( pAttacker, TFSTAT_KILLASSISTS, 1 );
	// increment reported class stats
	int iClass = pAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iAssists++;
	}

	if ( pVictim->IsPlayer() )
	{
		// keep track of how many times every player kills every other player
		CTDCPlayer *pPlayerVictim = ToTDCPlayer( pVictim );
		TrackKillStats( pAttacker, pPlayerVictim );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerInvulnerable( CTDCPlayer *pPlayer ) 
{
	IncrementStat( pPlayer, TFSTAT_INVULNS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_Headshot( CTDCPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_HEADSHOTS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_Backstab( CTDCPlayer *pKiller )
{
	IncrementStat( pKiller, TFSTAT_BACKSTABS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerUsedTeleport( CTDCPlayer *pTeleportOwner, CTDCPlayer *pTeleportingPlayer )
{
	// We don't count the builder's teleports
	if ( pTeleportOwner != pTeleportingPlayer )
	{
		IncrementStat( pTeleportOwner, TFSTAT_TELEPORTS, 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerFiredWeapon( CTDCPlayer *pPlayer, bool bCritical ) 
{
	// If normal gameplay state, track weapon stats. 
	if ( TDCGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		CTDCWeaponBase *pTFWeapon = pPlayer->GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			// record shots fired in reported per-weapon stats
			ETDCWeaponID iWeaponID = pTFWeapon->GetWeaponID();

			if ( m_reportedStats.m_pCurrentGame != NULL )
			{
				TDC_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[iWeaponID];
				pWeaponStats->iShotsFired++;
				if ( bCritical )
				{
					pWeaponStats->iCritShotsFired++;
				}
			}

			pPlayer->OnMyWeaponFired( pTFWeapon );
		}
	}

	IncrementStat( pPlayer, TFSTAT_SHOTS_FIRED, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info, int iDamageTaken )
{
	CTDCPlayer *pTarget = ToTDCPlayer( pBasePlayer );
	CTDCPlayer *pAttacker = ToTDCPlayer( info.GetAttacker() );

	if ( !pAttacker )
		return;

	// don't count damage to yourself
	if ( pTarget == pAttacker )
		return;
	
	IncrementStat( pAttacker, TFSTAT_DAMAGE, iDamageTaken );

	TDC_Gamestats_LevelStats_t::PlayerDamageLump_t damage;
	Vector killerOrg = vec3_origin;

	// set the location where the target was hit
	const Vector &org = pTarget->GetAbsOrigin();
	damage.nTargetPosition[ 0 ] = static_cast<int>( org.x );
	damage.nTargetPosition[ 1 ] = static_cast<int>( org.y );
	damage.nTargetPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	if ( pAttacker )
	{
		damage.iAttackClass = pAttacker->GetPlayerClass()->GetClassIndex();
		killerOrg = pAttacker->GetAbsOrigin();
	}
	else
	{
		damage.iAttackClass = TDC_CLASS_UNDEFINED;
		killerOrg = org;
	}

	// find the weapon the killer used
	damage.iWeapon = (short)GetWeaponFromDamage( info );

	// If normal gameplay state, track weapon stats. 
	if ( ( TDCGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( damage.iWeapon != WEAPON_NONE  ) )
	{
		// record hits & damage in reported per-weapon stats
		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			TDC_Gamestats_WeaponStats_t *pWeaponStats = &m_reportedStats.m_pCurrentGame->m_aWeaponStats[damage.iWeapon];
			pWeaponStats->iHits++;
			pWeaponStats->iTotalDamage += iDamageTaken;

			// Try and figure out where the damage is coming from
			Vector vecDamageOrigin = info.GetReportedPosition();
			// If we didn't get an origin to use, try using the attacker's origin
			if ( vecDamageOrigin == vec3_origin )
			{
				vecDamageOrigin = killerOrg;
			}

			if ( vecDamageOrigin != vec3_origin )
			{
				pWeaponStats->iHitsWithKnownDistance++;
				int iDistance = (int) vecDamageOrigin.DistTo( pBasePlayer->GetAbsOrigin() );
//				Msg( "Damage distance: %d\n", iDistance );
				pWeaponStats->iTotalDistance += iDistance;
			}
		}
	}

	Assert( damage.iAttackClass != TDC_CLASS_UNDEFINED );

	// record the time the damage occurred
	damage.fTime = gpGlobals->curtime;

	// store the attacker's position
	damage.nAttackerPosition[ 0 ] = static_cast<int>( killerOrg.x );
	damage.nAttackerPosition[ 1 ] = static_cast<int>( killerOrg.y );
	damage.nAttackerPosition[ 2 ] = static_cast<int>( killerOrg.z );

	// set the class of the target
	damage.iTargetClass = pTarget->GetPlayerClass()->GetClassIndex();

	Assert( damage.iTargetClass != TDC_CLASS_UNDEFINED );

	// record the damage done
	damage.iDamage = info.GetDamage();

	// record if it was a crit
	damage.iCrit = ( ( info.GetDamageType() & DMG_CRITICAL ) != 0 );

	// record if it was a kill
	damage.iKill = ( pTarget->GetHealth() <= 0 );

	// add it to the list of damages
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aPlayerDamage.AddToTail( damage );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	// This also gets called when the victim is a building.  That gets tracked separately as building destruction, don't count it here
	if ( !pVictim->IsPlayer() )
		return;

	CTDCPlayer *pPlayerAttacker = static_cast< CTDCPlayer * >( pAttacker );
	
	IncrementStat( pPlayerAttacker, TFSTAT_KILLS, 1 );

	// keep track of how many times every player kills every other player
	CTDCPlayer *pPlayerVictim = ToTDCPlayer( pVictim );
	TrackKillStats( pAttacker, pPlayerVictim );
	
	int iClass = pPlayerAttacker->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iKills++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerSuicide( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFVictim = static_cast<CTDCPlayer *>( pPlayer );

	IncrementStat( pTFVictim, TFSTAT_SUICIDES, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_RoundEnd( int iWinningTeam, bool bFullRound, float flRoundTime, bool bWasSuddenDeathWin )
{
	TDC_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	Assert( map );
	if ( !map )
		return;

	m_reportedStats.m_pCurrentGame->m_Header.m_iTotalTime += (int) flRoundTime;
	m_reportedStats.m_pCurrentGame->m_flRoundStartTime = gpGlobals->curtime;

	// only record full rounds, not mini-rounds
	if ( !bFullRound )
		return;

	map->m_Header.m_iRoundsPlayed++;
	switch ( iWinningTeam )
	{
	case TDC_TEAM_RED:
		map->m_Header.m_iRedWins++;
		if ( bWasSuddenDeathWin )
		{
			map->m_Header.m_iRedSuddenDeathWins++;
		}
		break;
	case TDC_TEAM_BLUE:
		map->m_Header.m_iBlueWins++;
		if ( bWasSuddenDeathWin )
		{
			map->m_Header.m_iBlueSuddenDeathWins++;
		}
		break;
	case TEAM_UNASSIGNED:
		map->m_Header.m_iStalemates++;
		break;
	default:
		Assert( false );
		break;
	}

	// add current game data in to data for this level
	AccumulateGameData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerCapturedPoint( CTDCPlayer *pPlayer )
{
	// increment player stats
	IncrementStat( pPlayer, TFSTAT_CAPTURES, 1 );
	// increment reported stats
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( m_reportedStats.m_pCurrentGame != NULL )
	{
		m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iCaptures++;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerDefendedPoint( CTDCPlayer *pPlayer )
{
	IncrementStat( pPlayer, TFSTAT_DEFENSES, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerDominatedOther( CTDCPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_DOMINATIONS, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerRevenge( CTDCPlayer *pAttacker )
{
	IncrementStat( pAttacker, TFSTAT_REVENGE, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_MaxSentryKills( CTDCPlayer *pAttacker, int iMaxKills )
{
	// Max sentry kills is a little different from other stats, it is the most kills from
	// any single sentry the player builds during his lifetime.  It does not increase monotonically
	// so this is a little different than the other stat code.
	PlayerStats_t &stats = m_aPlayerStats[pAttacker->entindex()];
	int iCur = stats.statsCurrentRound.m_iStat[TFSTAT_MAXSENTRYKILLS];
	if ( iCur != iMaxKills )
	{
		stats.statsCurrentRound.m_iStat[TFSTAT_MAXSENTRYKILLS] = iMaxKills;
		stats.iStatsChangedBits |= ( 1 << ( TFSTAT_MAXSENTRYKILLS - TFSTAT_FIRST ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	Assert( pPlayer );
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	IncrementStat( pTFPlayer, TFSTAT_DEATHS, 1 );

	TDC_Gamestats_LevelStats_t::PlayerDeathsLump_t death;
	Vector killerOrg;

	// set the location where the target died
	const Vector &org = pPlayer->GetAbsOrigin();
	death.nPosition[ 0 ] = static_cast<int>( org.x );
	death.nPosition[ 1 ] = static_cast<int>( org.y );
	death.nPosition[ 2 ] = static_cast<int>( org.z );

	// set the class of the attacker
	CTDCPlayer *pScorer = ToTDCPlayer( info.GetAttacker() );

	if ( pScorer )
	{
		death.iAttackClass = pScorer->GetPlayerClass()->GetClassIndex();
		killerOrg = pScorer->GetAbsOrigin();
	}
	else
	{
		death.iAttackClass = TDC_CLASS_UNDEFINED;
		killerOrg = org;

		// Environmental death.
		IncrementStat( pTFPlayer, TFSTAT_ENV_DEATHS, 1 );
	}

	SendStatsToPlayer( pTFPlayer, STATMSG_PLAYERDEATH );

	// set the class of the target
	death.iTargetClass = pTFPlayer->GetPlayerClass()->GetClassIndex();

	// find the weapon the killer used
	death.iWeapon = (short)GetWeaponFromDamage( info );

	// calculate the distance to the killer
	death.iDistance = static_cast<unsigned short>( ( killerOrg - org ).Length() );

	// add it to the list of deaths
	TDC_Gamestats_LevelStats_t *map = m_reportedStats.m_pCurrentGame;
	if ( map )
	{
		map->m_aPlayerDeaths.AddToTail( death );
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();

		if ( m_reportedStats.m_pCurrentGame != NULL )
		{
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iDeaths++;
			m_reportedStats.m_pCurrentGame->m_aClassStats[iClass].iTotalTime += (int) ( gpGlobals->curtime - pTFPlayer->GetSpawnTime() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerAwardBonusPoints( CTDCPlayer *pPlayer, CBaseEntity *pAwarder, int iAmount )
{
	IncrementStat( pPlayer, TFSTAT_BONUS, iAmount );

#if 0
	if ( pAwarder )
	{
		CSingleUserRecipientFilter filter( pPlayer );
		filter.MakeReliable();

		UserMessageBegin( filter, "PlayerBonusPoints" );
			WRITE_BYTE( iAmount );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pAwarder->entindex() );
		MessageEnd();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_GameEnd( void )
{
	// Calculate score and send out stats to everyone.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsAlive() )
		{
			AccumulateAndResetPerLifeStats( pPlayer );
			SendStatsToPlayer( pPlayer, STATMSG_GAMEEND );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerScoresEscortPoints( CTDCPlayer *pPlayer, int iAmount )
{
	IncrementStat( pPlayer, TFSTAT_CAPTURES, iAmount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCGameStats::Event_PlayerMovedToSpectators( CTDCPlayer *pPlayer )
{
	if ( pPlayer && pPlayer->IsAlive() )
	{
		// Send out their stats.
		SendStatsToPlayer( pPlayer, STATMSG_GAMEEND );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Per-frame handler
//-----------------------------------------------------------------------------
void CTDCGameStats::FrameUpdatePostEntityThink()
{
	// see if any players have stat changes we need to send
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
		{
			PlayerStats_t &stats = m_aPlayerStats[pPlayer->entindex()];
			// if there are any updated stats for this player and we haven't sent a stat update for this player in the last second,
			// send one now.
			if ( ( stats.iStatsChangedBits > 0 ) && ( gpGlobals->curtime >= stats.m_flTimeLastSend + 1.0f ) )
			{
				SendStatsToPlayer( pPlayer, STATMSG_UPDATE );
			}						
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds data from current game into accumulated data for this level.
//-----------------------------------------------------------------------------
void CTDCGameStats::AccumulateGameData()
{
	// find or add a bucket for this level
	TDC_Gamestats_LevelStats_t *map = m_reportedStats.FindOrAddMapStats( STRING( gpGlobals->mapname ) );
	// get current game data
	TDC_Gamestats_LevelStats_t *game = m_reportedStats.m_pCurrentGame;
	if ( !map || !game )
		return;
	
	// Sanity-check that this looks like real game play -- must have minimum # of players on both teams,
	// minimum time and some damage to players must have occurred
	if ( ( game->m_iPeakPlayerCount[TDC_TEAM_RED] >= 3  ) && ( game->m_iPeakPlayerCount[TDC_TEAM_BLUE] >= 3 ) &&
		( game->m_Header.m_iTotalTime >= 4 * 60 ) && ( game->m_aPlayerDamage.Count() > 0 ) ) 
	{
		// if this looks like real game play, add it to stats
		map->Accumulate( game );
	}
		
	ClearCurrentGameData();
}

//-----------------------------------------------------------------------------
// Purpose: Clears data for current game
//-----------------------------------------------------------------------------
void CTDCGameStats::ClearCurrentGameData()
{
	if ( m_reportedStats.m_pCurrentGame )
	{
		delete m_reportedStats.m_pCurrentGame;
	}
	m_reportedStats.m_pCurrentGame = new TDC_Gamestats_LevelStats_t;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this stat should be sent to the client
//-----------------------------------------------------------------------------
bool CTDCGameStats::ShouldSendToClient( TFStatType_t statType )
{
	switch ( statType )
	{
	// don't need to send these
	case TFSTAT_SHOTS_HIT:
	case TFSTAT_SHOTS_FIRED:
	case TFSTAT_SUICIDES:
	case TFSTAT_ENV_DEATHS:
		return false;
	default:
		return true;
	}
}

struct MapNameAndPlaytime_t
{
	const char *mapname;
	int timeplayed;
};

static int SortMapPlaytime( const MapNameAndPlaytime_t *map1, const MapNameAndPlaytime_t *map2 )
{
	return ( map1->timeplayed - map2->timeplayed );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCGameStats::GetVoteData( const char *pszVoteIssue, int iNumOptions, CUtlStringList &optionList )
{
	if ( V_strcmp( pszVoteIssue, "NextLevel" ) == 0 )
	{
		// Always add the next map in the cycle.
		char szNextMap[MAX_MAP_NAME];
		TDCGameRules()->GetNextLevelName( szNextMap, MAX_MAP_NAME );
		optionList.CopyAndAddToTail( szNextMap );
		iNumOptions--;

		if ( !g_pStringTableServerMapCycle )
			return;

		// Load the map list from the cycle.
		int iStringIndex = g_pStringTableServerMapCycle->FindStringIndex( "ServerMapCycle" );
		if ( iStringIndex == INVALID_STRING_INDEX )
			return;

		int nLength = 0;
		const char *pszMapCycle = (const char *)g_pStringTableServerMapCycle->GetStringUserData( iStringIndex, &nLength );
		if ( !pszMapCycle || !pszMapCycle[0] )
			return;

		CUtlStringList mapList;
		V_SplitString( pszMapCycle, "\n", mapList );

		CUtlVector<MapNameAndPlaytime_t> mapPlaytimeList;

		for ( const char *pszMapName : mapList )
		{
			// Skip the current map.
			if ( V_strcmp( pszMapName, STRING( gpGlobals->mapname ) ) == 0 )
				continue;

			// Skip the next map as we've already included it.
			if ( V_strcmp( pszMapName, szNextMap ) == 0 )
				continue;

			MapNameAndPlaytime_t &mapPlaytime = mapPlaytimeList[mapPlaytimeList.AddToTail()];
			mapPlaytime.mapname = pszMapName;

			// See if we have stats on this map.
			int idx = m_reportedStats.m_dictMapStats.Find( pszMapName );
			if ( idx != m_reportedStats.m_dictMapStats.InvalidIndex() )
			{
				mapPlaytime.timeplayed = m_reportedStats.m_dictMapStats[idx].m_Header.m_iTotalTime;
			}
			else
			{
				mapPlaytime.timeplayed = 0;
			}
		}

		// Sort them by playtime in ascending order and add randomness for equal maps.
		mapPlaytimeList.Shuffle();
		mapPlaytimeList.Sort( SortMapPlaytime );

		// Add the desired number of maps to the options list.
		for ( int i = 0; i < mapPlaytimeList.Count() && i < iNumOptions; i++ )
		{
			optionList.CopyAndAddToTail( mapPlaytimeList[i].mapname );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the stats of who has killed whom
//-----------------------------------------------------------------------------
void CTDCGameStats::TrackKillStats( CBasePlayer *pAttacker, CBasePlayer *pVictim )
{
	int iPlayerIndexAttacker = pAttacker->entindex();
	int iPlayerIndexVictim = pVictim->entindex();

	PlayerStats_t &statsAttacker = m_aPlayerStats[iPlayerIndexAttacker];
	PlayerStats_t &statsVictim = m_aPlayerStats[iPlayerIndexVictim];

	statsVictim.statsKills.iNumKilledBy[iPlayerIndexAttacker]++;
	statsVictim.statsKills.iNumKilledByUnanswered[iPlayerIndexAttacker]++;
	statsAttacker.statsKills.iNumKilled[iPlayerIndexVictim]++;
	statsAttacker.statsKills.iNumKilledByUnanswered[iPlayerIndexVictim] = 0;

}

struct PlayerStats_t *CTDCGameStats::FindPlayerStats( CBasePlayer *pPlayer )
{
	return &m_aPlayerStats[pPlayer->entindex()];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void CC_ListDeaths( const CCommand &args )
{
	TDC_Gamestats_LevelStats_t *map = CTDC_GameStats.m_reportedStats.m_pCurrentGame;
	if ( !map )
		return;

	for( int i = 0; i < map->m_aPlayerDeaths.Count(); i++ )
	{
		Msg( "%s killed %s with %s at (%d,%d,%d), distance %d\n",
            g_aPlayerClassNames_NonLocalized[ map->m_aPlayerDeaths[ i ].iAttackClass ],
			g_aPlayerClassNames_NonLocalized[map->m_aPlayerDeaths[i].iTargetClass],
			WeaponIdToAlias( (ETDCWeaponID)map->m_aPlayerDeaths[ i ].iWeapon ), 
			map->m_aPlayerDeaths[ i ].nPosition[ 0 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 1 ],
			map->m_aPlayerDeaths[ i ].nPosition[ 2 ],
			map->m_aPlayerDeaths[ i ].iDistance );
	}

	Msg( "\n---------------------------------\n\n" );

	for( int i = 0; i < map->m_aPlayerDamage.Count(); i++ )
	{
		Msg( "%.2f : %s at (%d,%d,%d) caused %d damage to %s with %s at (%d,%d,%d)%s%s\n",
			map->m_aPlayerDamage[ i ].fTime,
			g_aPlayerClassNames_NonLocalized[map->m_aPlayerDamage[i].iAttackClass],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 0 ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 1 ],
			map->m_aPlayerDamage[ i ].nAttackerPosition[ 2 ],
			map->m_aPlayerDamage[ i ].iDamage,
			g_aPlayerClassNames_NonLocalized[map->m_aPlayerDamage[i].iTargetClass],
			WeaponIdToAlias( (ETDCWeaponID)map->m_aPlayerDamage[ i ].iWeapon ), 
			map->m_aPlayerDamage[ i ].nTargetPosition[ 0 ],
			map->m_aPlayerDamage[ i ].nTargetPosition[ 1 ],
			map->m_aPlayerDamage[ i ].nTargetPosition[ 2 ],
			map->m_aPlayerDamage[ i ].iCrit ? ", CRIT!" : "",
			map->m_aPlayerDamage[ i ].iKill ? ", KILL" : ""	);
	}

	Msg( "\n---------------------------------\n\n" );
	Msg( "listed %d deaths\n", map->m_aPlayerDeaths.Count() );
	Msg( "listed %d damages\n\n", map->m_aPlayerDamage.Count() );
}

static ConCommand listDeaths("listdeaths", CC_ListDeaths, "lists player deaths", FCVAR_DEVELOPMENTONLY );
