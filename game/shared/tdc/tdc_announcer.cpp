//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tdc_announcer.h"
#include "tdc_gamerules.h"

#ifdef CLIENT_DLL
#include <engine/IEngineSound.h>
#include "hud_macros.h"
#endif

const char *CTDCAnnouncer::m_aAnnouncerSounds[TDC_ANNOUNCERTYPE_COUNT][TDC_ANNOUNCER_MESSAGE_COUNT] =
{
	// VOX
	{
		"Announcer_VOX.Stalemate",
		"Announcer_VOX.SuddenDeath",
		"Announcer_VOX.YourTeamWon",
		"Announcer_VOX.YourTeamLost",
		"Announcer_VOX.Overtime",
		"Announcer_VOX.RoundBegins60Seconds",
		"Announcer_VOX.RoundBegins30Seconds",
		"Announcer_VOX.RoundBegins10Seconds",
		"Announcer_VOX.RoundBegins5Seconds",
		"Announcer_VOX.RoundBegins4Seconds",
		"Announcer_VOX.RoundBegins3Seconds",
		"Announcer_VOX.RoundBegins2Seconds",
		"Announcer_VOX.RoundBegins1Seconds",
		"Announcer_VOX.RoundEnds5minutes",
		"Announcer_VOX.RoundEnds60seconds",
		"Announcer_VOX.RoundEnds30seconds",
		"Announcer_VOX.RoundEnds10seconds",
		"Announcer_VOX.RoundEnds5seconds",
		"Announcer_VOX.RoundEnds4seconds",
		"Announcer_VOX.RoundEnds3seconds",
		"Announcer_VOX.RoundEnds2seconds",
		"Announcer_VOX.RoundEnds1seconds",
		"Announcer_VOX.TimeAdded",
		"Announcer_VOX.TimeAwardedForTeam",
		"Announcer_VOX.TimeAddedForEnemy",
		"Announcer_VOX.CaptureFlag_EnemyStolen",
		"Announcer_VOX.CaptureFlag_EnemyDropped",
		"Announcer_VOX.CaptureFlag_EnemyCaptured",
		"Announcer_VOX.CaptureFlag_EnemyReturned",
		"Announcer_VOX.CaptureFlag_TeamStolen",
		"Announcer_VOX.CaptureFlag_TeamDropped",
		"Announcer_VOX.CaptureFlag_TeamCaptured",
		"Announcer_VOX.CaptureFlag_TeamReturned",
		"Announcer_VOX.AttackDefend_EnemyStolen",
		"Announcer_VOX.AttackDefend_EnemyDropped",
		"Announcer_VOX.AttackDefend_EnemyCaptured",
		"Announcer_VOX.AttackDefend_EnemyReturned",
		"Announcer_VOX.AttackDefend_TeamStolen",
		"Announcer_VOX.AttackDefend_TeamDropped",
		"Announcer_VOX.AttackDefend_TeamCaptured",
		"Announcer_VOX.AttackDefend_TeamReturned",
		"Announcer_VOX.Invade_EnemyStolen",
		"Announcer_VOX.Invade_EnemyDropped",
		"Announcer_VOX.Invade_EnemyCaptured",
		"Announcer_VOX.Invade_TeamStolen",
		"Announcer_VOX.Invade_TeamDropped",
		"Announcer_VOX.Invade_TeamCaptured",
		"Announcer_VOX.Invade_FlagReturned",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"Announcer_VOX.ControlPointContested",
		"Announcer_VOX.ControlPointContested_Neutral",
		"Announcer_VOX.Cart.WarningAttacker",
		"Announcer_VOX.Cart.WarningDefender",
		"Announcer_VOX.Cart.FinalWarningAttacker",
		"Announcer_VOX.Cart.FinalWarningDefender",
		"Announcer_VOX.AM_RoundStartRandom",
		"Announcer_VOX.AM_FirstBloodRandom",
		"Announcer_VOX.AM_FirstBloodFast",
		"Announcer_VOX.AM_FirstBloodFinally",
		"Announcer_VOX.AM_CapEnabledRandom",
		"Announcer_VOX.AM_FlawlessVictory01",
		"Announcer_VOX.AM_FlawlessVictoryRandom",
		"Announcer_VOX.AM_FlawlessDefeatRandom",
		"Announcer_VOX.AM_TeamScrambleRandom",
		"Announcer_VOX.AM_LastManAlive",
		"Announcer_VOX.SecurityAlert",
		"",
		"Announcer_VOX.DM_CritIncoming",
		"Announcer_VOX.DM_ShieldIncoming",
		"Announcer_VOX.DM_HasteIncoming",
		"Announcer_VOX.DM_BerserkIncoming",
		"Announcer_VOX.DM_CloakIncoming",
		"Announcer_VOX.DM_MegahealthIncoming",
		"Announcer_VOX.DM_DisplacerIncoming",
		"Announcer_VOX.DM_CritSpawn",
		"Announcer_VOX.DM_ShieldSpawn",
		"Announcer_VOX.DM_HasteSpawn",
		"Announcer_VOX.DM_BerserkSpawn",
		"Announcer_VOX.DM_CloakSpawn",
		"Announcer_VOX.DM_MegahealthSpawn",
		"Announcer_VOX.DM_DisplacerSpawn",
		"Announcer_VOX.DM_CritTeamPickup",
		"Announcer_VOX.DM_ShieldTeamPickup",
		"Announcer_VOX.DM_HasteTeamPickup",
		"Announcer_VOX.DM_BerserkTeamPickup",
		"Announcer_VOX.DM_CloakTeamPickup",
		"Announcer_VOX.DM_MegahealthTeamPickup",
		"Announcer_VOX.DM_DisplacerTeamPickup",
		"Announcer_VOX.DM_CritEnemyPickup",
		"Announcer_VOX.DM_ShieldEnemyPickup",
		"Announcer_VOX.DM_HasteEnemyPickup",
		"Announcer_VOX.DM_BerserkEnemyPickup",
		"Announcer_VOX.DM_CloakEnemyPickup",
		"Announcer_VOX.DM_MegahealthEnemyPickup",
		"Announcer_VOX.DM_DisplacerEnemyPickup",
		"Announcer_VOX.LeadTaken",
		"Announcer_VOX.LeadLost",
		"Announcer_VOX.LeadTied",
		"Announcer_VOX.HumansWin",
		"Announcer_VOX.ZombiesWin",
		"Announcer_VOX.PD_CaptureZoneActive",
		"Announcer_VOX.PD_CaptureZoneInactive",
		"Announcer_VOX.PD_TeamInZone",
		"Announcer_VOX.PD_EnemyInZone",
	},
};

CTDCAnnouncer g_TFAnnouncer( "Announcer" );

#ifdef CLIENT_DLL
static void __MsgFunc_AnnouncerSpeak( bf_read &msg )
{
	int iMessage = msg.ReadShort();
	g_TFAnnouncer.Speak( iMessage );
}
#endif

CTDCAnnouncer::CTDCAnnouncer( const char *pszName ) : CAutoGameSystem( pszName )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCAnnouncer::Init( void )
{
#ifdef CLIENT_DLL
	HOOK_MESSAGE( AnnouncerSpeak );

#ifdef STAGING_ONLY
	// DRM check.
	QAngle angles;
	engine->GetViewAngles( angles );
	if ( angles == vec3_angle )
	{
		return false;
	}
#endif

#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCAnnouncer::LevelInitPreEntity( void )
{
#ifdef GAME_DLL
	for ( int i = 0; i < TDC_ANNOUNCERTYPE_COUNT; i++ )
	{
		for ( int j = 0; j < TDC_ANNOUNCER_MESSAGE_COUNT; j++ )
		{
			const char *pszSound = m_aAnnouncerSounds[i][j];
			if ( !pszSound || !pszSound[0] )
				continue;

			CBaseEntity::PrecacheScriptSound( pszSound );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTDCAnnouncer::GetSoundForMessage( int iMessage )
{
	int iType = TDC_ANNOUNCERTYPE_VOX;
	return m_aAnnouncerSounds[iType][iMessage];
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCAnnouncer::Speak( IRecipientFilter &filter, int iMessage )
{
	UserMessageBegin( filter, "AnnouncerSpeak" );
	WRITE_SHORT( iMessage );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCAnnouncer::Speak( int iMessage )
{
	CReliableBroadcastRecipientFilter filter;
	Speak( filter, iMessage );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCAnnouncer::Speak( CBasePlayer *pPlayer, int iMessage )
{
	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();
	Speak( filter, iMessage );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCAnnouncer::Speak( int iTeam, int iMessage )
{
	CTeamRecipientFilter filter( iTeam, true );
	Speak( filter, iMessage );
}

#else

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCAnnouncer::Speak( int iMessage )
{
	const char *pszSound = GetSoundForMessage( iMessage );
	if ( !pszSound || !pszSound[0] )
		return;

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSound );
}

#endif
