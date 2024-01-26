//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "../EventLog.h"
#include "team.h"
#include "tdc_gamerules.h"
#include "tdc_team.h"
#include "KeyValues.h"

class CTDCEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	bool PrintEvent( IGameEvent *event )	// override virtual function
	{
		if ( !PrintTFEvent( event ) ) // allow TF to override logging
		{
			return BaseClass::PrintEvent( event );
		}
		else
		{
			return true;
		}
	}

	bool Init()
	{
		BaseClass::Init();

		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "tf_game_over" );
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "teamplay_round_stalemate" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_game_over" );

		return true;
	}

protected:

	bool PrintTFEvent( IGameEvent *event )	// print Mod specific logs
	{
		const char *eventName = event->GetName();

		if ( !Q_strncmp( eventName, "server_", strlen( "server_" ) ) )
		{
			return false; // ignore server_ messages
		}

		if ( !Q_strncmp( eventName, "player_death", Q_strlen( "player_death" ) ) )
		{
			const int userid = event->GetInt( "userid" );
			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
			if ( !pPlayer )
			{
				return false;
			}

			const int attackerid = event->GetInt( "attacker" );
			const char *weapon = event->GetString( "weapon" );
			auto iCustomDamage = (ETDCDmgCustom)event->GetInt( "customkill" );
			CBasePlayer *pAttacker = UTIL_PlayerByUserId( attackerid );

			if ( pPlayer == pAttacker )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (attacker_position \"%d %d %d\")\n",
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName(),
					weapon,
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
			}
			else if ( pAttacker )
			{
				const char *pszCustom = NULL;

				switch ( iCustomDamage )
				{
				case TDC_DMG_CUSTOM_HEADSHOT:
					pszCustom = "headshot";
					break;
				case TDC_DMG_CUSTOM_BACKSTAB:
					pszCustom = "backstab";
					break;

				default:
					break;
				}

				if ( pszCustom )
				{
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\" (customkill \"%s\") (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",
						pAttacker->GetPlayerName(),
						attackerid,
						pAttacker->GetNetworkIDString(),
						pAttacker->GetTeam()->GetName(),
						pPlayer->GetPlayerName(),
						userid,
						pPlayer->GetNetworkIDString(),
						pPlayer->GetTeam()->GetName(),
						weapon,
						pszCustom,
						(int)pAttacker->GetAbsOrigin().x,
						(int)pAttacker->GetAbsOrigin().y,
						(int)pAttacker->GetAbsOrigin().z,
						(int)pPlayer->GetAbsOrigin().x,
						(int)pPlayer->GetAbsOrigin().y,
						(int)pPlayer->GetAbsOrigin().z );
				}
				else
				{
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",
						pAttacker->GetPlayerName(),
						attackerid,
						pAttacker->GetNetworkIDString(),
						pAttacker->GetTeam()->GetName(),
						pPlayer->GetPlayerName(),
						userid,
						pPlayer->GetNetworkIDString(),
						pPlayer->GetTeam()->GetName(),
						weapon,
						(int)pAttacker->GetAbsOrigin().x,
						(int)pAttacker->GetAbsOrigin().y,
						(int)pAttacker->GetAbsOrigin().z,
						(int)pPlayer->GetAbsOrigin().x,
						(int)pPlayer->GetAbsOrigin().y,
						(int)pPlayer->GetAbsOrigin().z );
				}
			}
			else
			{
				// killed by the world
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"world\" (attacker_position \"%d %d %d\")\n",
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName(),
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
			}

			// Assist kill
			int assistid = event->GetInt( "assister" );
			CBasePlayer *pAssister = UTIL_PlayerByUserId( assistid );

			if ( pAssister )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"kill assist\" against \"%s<%i><%s><%s>\" (assister_position \"%d %d %d\") (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",
					pAssister->GetPlayerName(),
					assistid,
					pAssister->GetNetworkIDString(),
					pAssister->GetTeam()->GetName(),
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName(),
					(int)pAssister->GetAbsOrigin().x,
					(int)pAssister->GetAbsOrigin().y,
					(int)pAssister->GetAbsOrigin().z,
					(int)pAttacker->GetAbsOrigin().x,
					(int)pAttacker->GetAbsOrigin().y,
					(int)pAttacker->GetAbsOrigin().z,
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
			}

			// Domination and Revenge
			// pAttacker //int attackerid = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			// pPlayer //int userid = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			// pAssister // assistid

			if ( event->GetInt( "dominated" ) > 0 && pAttacker )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"domination\" against \"%s<%i><%s><%s>\"\n",
					pAttacker->GetPlayerName(),
					attackerid,
					pAttacker->GetNetworkIDString(),
					pAttacker->GetTeam()->GetName(),
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName()
					);
			}
			if ( event->GetInt( "assister_dominated" ) > 0 && pAssister )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"domination\" against \"%s<%i><%s><%s>\" (assist \"1\")\n",
					pAssister->GetPlayerName(),
					assistid,
					pAssister->GetNetworkIDString(),
					pAssister->GetTeam()->GetName(),
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName()
					);
			}
			if ( event->GetInt( "revenge" ) > 0 && pAttacker )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"revenge\" against \"%s<%i><%s><%s>\"\n",
					pAttacker->GetPlayerName(),
					attackerid,
					pAttacker->GetNetworkIDString(),
					pAttacker->GetTeam()->GetName(),
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName()
					);
			}
			if ( event->GetInt( "assister_revenge" ) > 0 && pAssister )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"revenge\" against \"%s<%i><%s><%s>\" (assist \"1\")\n",
					pAssister->GetPlayerName(),
					assistid,
					pAssister->GetNetworkIDString(),
					pAssister->GetTeam()->GetName(),
					pPlayer->GetPlayerName(),
					userid,
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName()
					);
			}

			return true;
		}
		else if ( FStrEq( eventName, "tf_game_over" ) || FStrEq( eventName, "teamplay_game_over" ) )
		{
			UTIL_LogPrintf( "World triggered \"Game_Over\" reason \"%s\"\n", event->GetString( "reason" ) );

			if ( TDCGameRules()->IsTeamplay() )
			{
				for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
				{
					CTeam *pTeam = GetGlobalTeam( i );
					UTIL_LogPrintf( "Team \"%s\" final score \"%d\" with \"%d\" players\n", pTeam->GetName(), pTeam->GetScore(), pTeam->GetNumPlayers() );
				}
			}

			return true;
		}
		else if ( FStrEq( eventName, "teamplay_flag_event" ) )
		{
			int playerindex = event->GetInt( "player" );

			CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerindex );
			if ( !pPlayer )
			{
				return false;
			}

			char *pszEvent = "unknown";	// picked up, dropped, defended, captured
			auto iEventType = (ETDCFlagEventTypes)event->GetInt( "eventtype" );
			bool bPlainLogEntry = true;

			switch ( iEventType )
			{
			case TDC_FLAGEVENT_PICKUP:
				pszEvent = "picked up";
				break;
			case TDC_FLAGEVENT_CAPTURE:
				pszEvent = "captured";

				if ( TDCGameRules()->GetScoreLimit() > 0 )
				{
					bPlainLogEntry = false;
				}
				break;
			case TDC_FLAGEVENT_DEFEND:
				pszEvent = "defended";
				break;
			case TDC_FLAGEVENT_DROPPED:
				pszEvent = "dropped";
				break;
			}

			if ( bPlainLogEntry )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"flagevent\" (event \"%s\") (position \"%d %d %d\")\n",
					pPlayer->GetPlayerName(),
					pPlayer->GetUserID(),
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName(),
					pszEvent,
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
			}
			else
			{
				CTDCTeam *pTeam = GetGlobalTFTeam( pPlayer->GetTeamNumber() );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"flagevent\" (event \"%s\") (team_caps \"%d\") (caps_per_round \"%d\") (position \"%d %d %d\")\n",
					pPlayer->GetPlayerName(),
					pPlayer->GetUserID(),
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName(),
					pszEvent,
					pTeam->GetRoundScore(),
					TDCGameRules()->GetScoreLimit(),
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
			}

			return true;
		}
		else if ( FStrEq( eventName, "teamplay_round_stalemate" ) )
		{
			int iReason = event->GetInt( "reason" );
			if ( iReason == STALEMATE_TIMER )
			{
				UTIL_LogPrintf( "World triggered \"Round_SuddenDeath\" reason \"round timelimit reached\"\n" );
			}
			else if ( iReason == STALEMATE_SERVER_TIMELIMIT )
			{
				UTIL_LogPrintf( "World triggered \"Round_SuddenDeath\" reason \"server timelimit reached\"\n" );
			}
			else
			{
				UTIL_LogPrintf( "World triggered \"Round_SuddenDeath\"\n" );
			}

			return true;
		}
		else if ( FStrEq( eventName, "teamplay_round_win" ) )
		{
			bool bShowScores = true;
			int iTeam = event->GetInt( "team" );
			bool bFullRound = event->GetBool( "full_round" );
			if ( iTeam == TEAM_UNASSIGNED )
			{
				UTIL_LogPrintf( "World triggered \"Round_Stalemate\"\n" );
			}
			else
			{
				const char *pszWinner = g_aTeamNames[iTeam];

				if ( bFullRound )
				{
					UTIL_LogPrintf( "World triggered \"Round_Win\" (winner \"%s\")\n", pszWinner );
					UTIL_LogPrintf( "World triggered \"Round_Length\" (seconds \"%0.2f\")\n", event->GetFloat( "round_time" ) );

					bShowScores = true;
				}
			}

			if ( bShowScores )
			{
				for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
				{
					CTeam *pTeam = GetGlobalTeam( i );
					UTIL_LogPrintf( "Team \"%s\" current score \"%d\" with \"%d\" players\n", pTeam->GetName(), pTeam->GetScore(), pTeam->GetNumPlayers() );
				}
			}
		}

		return false;
	}
};

CTDCEventLog g_TFEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_TFEventLog;
}

