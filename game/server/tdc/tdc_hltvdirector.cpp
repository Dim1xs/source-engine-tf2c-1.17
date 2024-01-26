//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "hltvdirector.h"

class CTDCHLTVDirector : public CHLTVDirector
{
public:
	DECLARE_CLASS( CTDCHLTVDirector, CHLTVDirector );

	const char** GetModEvents();
	void SetHLTVServer( IHLTVServer *hltv );
	void CreateShotFromEvent( CHLTVGameEvent *event);

	virtual char	*GetFixedCameraEntityName( void ) { return "info_observer_point"; }
};

void CTDCHLTVDirector::SetHLTVServer( IHLTVServer *hltv )
{
	BaseClass::SetHLTVServer( hltv );

	if ( m_pHLTVServer )
	{
		// mod specific events the director uses to find interesting shots
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "ctf_flag_captured" );
	}
}

void CTDCHLTVDirector::CreateShotFromEvent( CHLTVGameEvent *event ) 
{
	// show event at least for 2 more seconds after it occured
	const char *name = event->m_Event->GetName();

	if ( !Q_strcmp( "ctf_flag_captured", name ) )
	{
		CBasePlayer *capper = UTIL_PlayerByUserId( event->m_Event->GetInt("capper") );
		if ( capper )
		{
			StartChaseCameraShot( capper->entindex(), 0, 96, 20, 0, false );
		}
	}
	else if ( !Q_strcmp( "teamplay_flag_event", name ) )
	{
		StartChaseCameraShot( event->m_Event->GetInt("player"), 0, 96, 20, 0, false );
	}
	else
	{

		// let baseclass create a shot
		BaseClass::CreateShotFromEvent( event );
	}
}

const char** CTDCHLTVDirector::GetModEvents()
{
	// game events relayed to spectator clients
	static const char *s_modevents[] =
	{
		"game_newmap",
		"hltv_status",
		"hltv_chat",
		"player_connect",
		"player_disconnect",
		"player_changeclass",
		"player_team",
		"player_info",
		"player_death",
		"player_chat",
		"player_spawn",
		"round_start",
		"round_end",
		"server_cvar",
		"server_spawn",
				
		// additional TF events:
		"controlpoint_starttouch",
		"controlpoint_endtouch",
		"teamplay_capture_broken",
		"ctf_flag_captured",
		"teamplay_broadcast_audio",
		"teamplay_capture_blocked",
		"teamplay_flag_event",
		"teamplay_game_over",
		"teamplay_point_captured",
		"teamplay_round_stalemate",
		"teamplay_round_start",
		"teamplay_round_win",
		"teamplay_timer_time_added",
		"teamplay_update_timer",
		"teamplay_win_panel",
		"tf_game_over",
		"object_destroyed",
			
		NULL
	};

	return s_modevents;
}

static CTDCHLTVDirector s_HLTVDirector;	// singleton

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CHLTVDirector, IHLTVDirector, INTERFACEVERSION_HLTVDIRECTOR, s_HLTVDirector );

CHLTVDirector* HLTVDirector()
{
	return &s_HLTVDirector;
}

IGameSystem* HLTVDirectorSystem()
{
	return &s_HLTVDirector;
}