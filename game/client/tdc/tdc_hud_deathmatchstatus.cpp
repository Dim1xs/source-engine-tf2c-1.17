//=============================================================================//
//
// Purpose: Deathmatch HUD
//
//=============================================================================//

#include "cbase.h"
#include "tdc_hud_deathmatchstatus.h"
#include "c_tdc_playerresource.h"
#include "tdc_hud_freezepanel.h"
#include "c_tdc_team.h"
#include "tdc_gamerules.h"
#include "functionproxy.h"
#include "clientsteamcontext.h"
#include "tdc_announcer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

vgui::IImage* GetDefaultAvatarImage( int iPlayerIndex );
extern ConVar fraglimit;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudDeathMatchObjectives::CTDCHudDeathMatchObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pLocalPlayerPanel = new EditablePanel( this, "LocalPlayerPanel" );
	m_pBestPlayerPanel = new EditablePanel( this, "BestPlayerPanel" );
	m_pPlayerAvatar = new CAvatarImagePanel( m_pLocalPlayerPanel, "PlayerAvatar" );
	m_pRivalAvatar = new CAvatarImagePanel( m_pBestPlayerPanel, "PlayerAvatar" );

	m_pPlayingTo = new CExLabel( this, "PlayingTo", "0" );
	m_pPlayingToBG = new CTDCImagePanel( this, "PlayingToBG" );

	m_iRivalPlayer = 0;
	m_iLeadStatus = DM_STATUS_NONE;

	ivgui()->AddTickSignal( GetVPanel(), 100 );

	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCHudDeathMatchObjectives::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	if ( GetLocalPlayerTeam() < FIRST_GAME_TEAM )
		return false;

	if ( !TDCGameRules() || TDCGameRules()->IsInWaitingForPlayers() || TDCGameRules()->State_Get() == GR_STATE_PREGAME )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudDeathMatchObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveDeathMatchPanel.res" );

	m_pPlayerAvatar->SetShouldDrawFriendIcon( false );
	m_pRivalAvatar->SetShouldDrawFriendIcon( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudDeathMatchObjectives::LevelInit( void )
{
	m_iRivalPlayer = 0;
	m_iLeadStatus = DM_STATUS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudDeathMatchObjectives::Reset()
{
	m_pPlayerAvatar->SetPlayer( ClientSteamContext().GetLocalPlayerSteamID(), k_EAvatarSize32x32 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudDeathMatchObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	m_pPlayingTo->SetVisible( bVisible );
	m_pPlayingToBG->SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudDeathMatchObjectives::OnTick( void )
{
	if ( !IsVisible() || !TDCGameRules() || !g_TDC_PR )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	// Get the local player's score.
	int iLocalIndex = pPlayer->entindex();
	int iLocalScore = g_TDC_PR->GetTotalScore( iLocalIndex );

	m_pLocalPlayerPanel->SetDialogVariable( "score", iLocalScore );

	// Figure out who's the leading player.
	int iBestIndex = 0;
	int iBestScore = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( g_TDC_PR->IsConnected( i ) && g_PR->GetTeam( i ) >= FIRST_GAME_TEAM )
		{
			int iScore = g_TDC_PR->GetTotalScore( i );

			if ( i != iLocalIndex && iScore >= iBestScore )
			{
				iBestScore = iScore;
				iBestIndex = i;
			}
		}
	}

	// Set their score and avatar.
	if ( iBestIndex )
	{
		m_pBestPlayerPanel->SetDialogVariable( "score", iBestScore );
	}
	else
	{
		m_pBestPlayerPanel->SetDialogVariable( "score", "" );
	}

	if ( iBestIndex != m_iRivalPlayer )
	{
		m_iRivalPlayer = iBestIndex;

		m_pRivalAvatar->SetPlayer( iBestIndex, k_EAvatarSize32x32 );
		m_pRivalAvatar->SetDefaultAvatar( GetDefaultAvatarImage( iBestIndex ) );
	}

	// Show the leading player on the top.
	if ( iLocalScore >= iBestScore )
	{
		m_pLocalPlayerPanel->SetZPos( 2 );
		m_pBestPlayerPanel->SetZPos( 1 );
	}
	else
	{
		m_pLocalPlayerPanel->SetZPos( 1 );
		m_pBestPlayerPanel->SetZPos( 2 );
	}

	// Update "Playing to" panel with the frag limit.
	int iFragLimit = TDCGameRules()->GetScoreLimit();

	if ( iFragLimit > 0 )
	{
		SetPlayingToLabelVisible( true );
		SetDialogVariable( "fraglimit", iFragLimit );
	}
	else
	{
		SetPlayingToLabelVisible( false );
	}

	// Check for lead announcements.
	int iStatus;

	if ( iBestIndex == 0 || pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
	{
		iStatus = DM_STATUS_NONE;
	}
	else if ( iLocalScore > iBestScore )
	{
		iStatus = DM_STATUS_LEADTAKEN;
	}
	else if ( iLocalScore == iBestScore )
	{
		iStatus = DM_STATUS_LEADTIED;
	}
	else
	{
		iStatus = DM_STATUS_LEADLOST;
	}

	if ( m_iLeadStatus != iStatus )
	{
		if ( m_iLeadStatus != DM_STATUS_NONE )
		{
			switch ( iStatus )
			{
			case DM_STATUS_LEADTAKEN:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTAKEN );
				break;
			case DM_STATUS_LEADTIED:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTIED );
				break;
			case DM_STATUS_LEADLOST:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADLOST );
				break;
			}
		}

		m_iLeadStatus = iStatus;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudDeathMatchObjectives::FireGameEvent( IGameEvent *event )
{
	m_iLeadStatus = DM_STATUS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CBestPlayerTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		if ( !g_TDC_PR )
			return;

		int iLocalIndex = GetLocalPlayerIndex();
		int iBestIndex = 0;
		int iBestScore = 0;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			if ( g_TDC_PR->IsConnected( i ) && g_PR->GetTeam( i ) >= FIRST_GAME_TEAM )
			{
				int iScore = g_TDC_PR->GetTotalScore( i );

				if ( i != iLocalIndex && iScore >= iBestScore )
				{
					iBestScore = iScore;
					iBestIndex = i;
				}
			}
		}

		if ( iBestIndex )
		{
			m_pResult->SetVecValue( g_TDC_PR->GetPlayerColorVector( iBestIndex ).Base(), 3 );
		}
		else
		{
			m_pResult->SetVecValue( 0, 0, 0 );
		}
	}
};

EXPOSE_INTERFACE( CBestPlayerTintColor, IMaterialProxy, "BestPlayerTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudTeamDeathMatchObjectives::CTDCHudTeamDeathMatchObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pRedPanel = new EditablePanel( this, "RedPanel" );
	m_pBluePanel = new EditablePanel( this, "BluePanel" );

	m_pPlayingTo = new CExLabel( this, "PlayingTo", "0" );
	m_pPlayingToBG = new CTDCImagePanel( this, "PlayingToBG" );

	m_iLeadStatus = DM_STATUS_NONE;

	ivgui()->AddTickSignal( GetVPanel(), 100 );

	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCHudTeamDeathMatchObjectives::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	if ( !TDCGameRules() || TDCGameRules()->IsInWaitingForPlayers() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTeamDeathMatchObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveTeamDeathMatchPanel.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTeamDeathMatchObjectives::LevelInit( void )
{
	m_iLeadStatus = DM_STATUS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTeamDeathMatchObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	m_pPlayingTo->SetVisible( bVisible );
	m_pPlayingToBG->SetVisible( bVisible );
}

static int TeamsScoreSort( C_TDCTeam* const *p1, C_TDCTeam* const *p2 )
{
	return ( ( *p1 )->GetRoundScore() - ( *p2 )->GetRoundScore() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTeamDeathMatchObjectives::OnTick( void )
{
	if ( !IsVisible() )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	CUtlVector<C_TDCTeam *> aTeams;
	int iLocalTeam = pPlayer->GetTeamNumber();
	int iLocalScore = -1;
	int iBestScore = -1;

	for ( int i = FIRST_GAME_TEAM; i < TDC_TEAM_COUNT; i++ )
	{
		C_TDCTeam *pTeam = GetGlobalTFTeam( i );
		if ( !pTeam )
			continue;

		EditablePanel *pPanel = GetTeamPanel( i );
		int iScore = pTeam->GetRoundScore();
		pPanel->SetDialogVariable( "score", iScore );

		if ( i == iLocalTeam )
		{
			iLocalScore = iScore;
		}
		else if ( iScore >= iBestScore )
		{
			iBestScore = iScore;
		}

		aTeams.AddToTail( pTeam );
	}

	if ( aTeams.Count() )
	{
		aTeams.Sort( TeamsScoreSort );

		// Show the leading team on the top.
		FOR_EACH_VEC( aTeams, i )
		{
			EditablePanel *pPanel = GetTeamPanel( aTeams[i]->GetTeamNumber() );
			pPanel->SetZPos( i + 1 );
		}
	}

	int iFragLimit = TDCGameRules()->GetScoreLimit();

	if ( iFragLimit > 0 )
	{
		SetPlayingToLabelVisible( true );
		SetDialogVariable( "fraglimit", iFragLimit );
	}
	else
	{
		SetPlayingToLabelVisible( false );
	}

	// Check for lead announcements.
	int iStatus;

	if ( iLocalScore == -1 || iBestScore == -1 )
	{
		iStatus = DM_STATUS_NONE;
	}
	else if ( iLocalScore > iBestScore )
	{
		iStatus = DM_STATUS_LEADTAKEN;
	}
	else if ( iLocalScore == iBestScore )
	{
		iStatus = DM_STATUS_LEADTIED;
	}
	else
	{
		iStatus = DM_STATUS_LEADLOST;
	}

	if ( m_iLeadStatus != iStatus )
	{
		if ( m_iLeadStatus != DM_STATUS_NONE )
		{
			switch ( iStatus )
			{
			case DM_STATUS_LEADTAKEN:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTAKEN );
				break;
			case DM_STATUS_LEADTIED:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTIED );
				break;
			case DM_STATUS_LEADLOST:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADLOST );
				break;
			}
		}

		m_iLeadStatus = iStatus;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTeamDeathMatchObjectives::UpdateStatus( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EditablePanel *CTDCHudTeamDeathMatchObjectives::GetTeamPanel( int iTeam )
{
	switch ( iTeam )
	{
	case TDC_TEAM_RED:
		return m_pRedPanel;
	case TDC_TEAM_BLUE:
		return m_pBluePanel;
	default:
		Assert( false );
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTeamDeathMatchObjectives::FireGameEvent( IGameEvent *event )
{
	m_iLeadStatus = DM_STATUS_NONE;
}
