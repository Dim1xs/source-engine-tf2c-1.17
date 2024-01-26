//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "func_flag_alert.h"
#include "tdc_player.h"
#include "tdc_announcer.h"
#include "tdc_team.h"

BEGIN_DATADESC( CFuncFlagAlertZone )
	DEFINE_KEYFIELD( m_bPlaySound, FIELD_BOOLEAN, "playsound" ),
	DEFINE_OUTPUT( m_OnTriggeredByTeam1, "OnTriggeredByTeam1" ),
	DEFINE_OUTPUT( m_OnTriggeredByTeam2, "OnTriggeredByTeam2" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_flag_alert, CFuncFlagAlertZone );

CFuncFlagAlertZone::CFuncFlagAlertZone()
{
	m_bPlaySound = true;
	memset( m_flNextAlertTime, 0, sizeof( m_flNextAlertTime ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFlagAlertZone::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFlagAlertZone::StartTouch( CBaseEntity *pOther )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( pOther );
	if ( !pPlayer )
		return;

	if ( pPlayer->GetTeamNumber() != GetTeamNumber() && pPlayer->HasTheFlag() && gpGlobals->curtime > m_flNextAlertTime[pPlayer->GetTeamNumber()] )
	{
		if ( GetTeamNumber() == TEAM_UNASSIGNED )
		{
			// Announce alert to everyone but the flag carrier's team.
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				if ( i == pPlayer->GetTeamNumber() )
					continue;

				CTeamRecipientFilter filter( i, true );
				g_TFAnnouncer.Speak( filter, TDC_ANNOUNCER_SECURITYALERT );
			}
		}
		else
		{
			// Announce alert to my team.
			g_TFAnnouncer.Speak( GetTeamNumber(), TDC_ANNOUNCER_SECURITYALERT );
		}

		// Fire output.
		switch ( pPlayer->GetTeamNumber() )
		{
		case TDC_TEAM_RED:
			m_OnTriggeredByTeam1.FireOutput( this, this );
			break;
		case TDC_TEAM_BLUE:
			m_OnTriggeredByTeam2.FireOutput( this, this );
			break;
		}

		// Repeat in 10 seconds.
		m_flNextAlertTime[pPlayer->GetTeamNumber()] = gpGlobals->curtime + 10.0f;
	}
}
