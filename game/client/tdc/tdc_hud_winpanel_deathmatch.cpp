//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tdc_hud_winpanel_deathmatch.h"
#include "iclientmode.h"
#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include "vgui_avatarimage.h"
#include "tdc_controls.h"
#include "c_tdc_playerresource.h"
#include "tdc_gamerules.h"
#include "tdc_merc_customizations.h"
#include "animation.h"
#include "tdc_viewport.h"
#include <engine/IEngineSound.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CTDCWinPanelDeathmatch );

CTDCWinPanelDeathmatch::CTDCWinPanelDeathmatch( const char *pszName ) : CHudElement( pszName ), BaseClass( NULL, "WinPanelDeathmatch" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	for ( int i = 0; i < 3; i++ )
	{
		m_pPlayerNames[i] = new CExLabel( this, VarArgs( "Player%dName", i + 1 ), "" );
		m_pPlayerKills[i] = new CExLabel( this, VarArgs( "Player%dKills", i + 1 ), "" );
		m_pPlayerDeaths[i] = new CExLabel( this, VarArgs( "Player%dDeaths", i + 1 ), "" );
		m_pPlayerModels[i] = new CTDCPlayerModelPanel( this, VarArgs( "Player%dModel", i + 1 ) );
	}

	m_flShowAt = -1.0f;
	m_bHiddenScoreboard = false;
	m_iWinAnimation = 0;

	ListenForGameEvent( "deathmatch_results" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWinPanelDeathmatch::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/WinPanelDeathmatch.res" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWinPanelDeathmatch::ShouldDraw( void )
{
	if ( m_flShowAt != -1.0f && gpGlobals->curtime >= m_flShowAt )
	{
		// Time to show ourselves, hide the scoreboard.
		if ( !m_bHiddenScoreboard )
		{
			GetTDCViewPort()->ShowScoreboard( false );
			m_bHiddenScoreboard = true;

			// Play the win animation for the winning player.
			if ( m_pPlayerModels[0] && m_pPlayerModels[0]->IsVisible() )
			{
				WinAnim_t *pAnimData = g_TDCPlayerItems.GetAnimationById( m_iWinAnimation );
				if ( pAnimData )
				{
					m_pPlayerModels[0]->SetSequence( m_pPlayerModels[0]->LookupSequence( pAnimData->sequence ), true );
				}
			}
		}
	}
	else
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWinPanelDeathmatch::LevelInit( void )
{
	m_flShowAt = -1.0f;
	m_bHiddenScoreboard = false;
	m_iWinAnimation = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWinPanelDeathmatch::FireGameEvent( IGameEvent *event )
{
	bool bPlayerFirst = false;

	if ( !g_TDC_PR )
		return;

	// look for the top 3 players sent in the event
	for ( int i = 0; i < 3; i++ )
	{
		bool bShow = false;
		char szPlayerIndexVal[64] = "", szPlayerScoreVal[64] = "", szPlayerKillsVal[64] = "", szPlayerDeathsVal[64] = "";
		// get player index and round points from the event
		V_sprintf_safe( szPlayerIndexVal, "player_%d", i + 1 );
		V_sprintf_safe( szPlayerScoreVal, "player_%d_points", i + 1 );
		V_sprintf_safe( szPlayerKillsVal, "player_%d_kills", i + 1 );
		V_sprintf_safe( szPlayerDeathsVal, "player_%d_deaths", i + 1 );
		int iPlayerIndex = event->GetInt( szPlayerIndexVal, 0 );
		int iRoundScore = event->GetInt( szPlayerScoreVal, 0 );
		int iPlayerKills = event->GetInt( szPlayerKillsVal, 0 );
		int iPlayerDeaths = event->GetInt( szPlayerDeathsVal, 0 );

		// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
		if ( iRoundScore > 0 )
			bShow = true;

		CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( VarArgs( "Player%dAvatar", i + 1 ) ) );

		if ( pPlayerAvatar )
		{
			pPlayerAvatar->ClearAvatar();
			if ( bShow )
			{
				pPlayerAvatar->SetPlayer( GetSteamIDForPlayerIndex( iPlayerIndex ), k_EAvatarSize32x32 );
				pPlayerAvatar->SetAvatarSize( 32, 32 );
			}
			pPlayerAvatar->SetVisible( bShow );
		}

		if ( bShow )
		{
			// set the player labels to team color
			Color clr = g_TDC_PR->GetPlayerColor( iPlayerIndex );
			m_pPlayerNames[i]->SetFgColor( clr );
			m_pPlayerKills[i]->SetFgColor( clr );
			m_pPlayerDeaths[i]->SetFgColor( clr );

			// set label contents
			m_pPlayerNames[i]->SetText( g_PR->GetPlayerName( iPlayerIndex ) );

			wchar_t wszKills[16], wszKillsLabel[64];
			wchar_t wszDeaths[16], wszDeathsLabel[64];
			V_swprintf_safe( wszKills, L"%d", iPlayerKills );
			V_swprintf_safe( wszDeaths, L"%d", iPlayerDeaths );

			g_pVGuiLocalize->ConstructString( wszKillsLabel, sizeof( wszKillsLabel ), g_pVGuiLocalize->Find( "#TDC_Winpanel_Deathmatch_Kills" ), 1, wszKills );
			g_pVGuiLocalize->ConstructString( wszDeathsLabel, sizeof( wszDeathsLabel ), g_pVGuiLocalize->Find( "#TDC_Winpanel_Deathmatch_Deaths" ), 1, wszDeaths );

			m_pPlayerKills[i]->SetText( wszKillsLabel );
			m_pPlayerDeaths[i]->SetText( wszDeathsLabel );

			if ( i == 0 && iPlayerIndex == GetLocalPlayerIndex() )
				bPlayerFirst = true;

			m_pPlayerModels[i]->SetToPlayerClass( TDC_CLASS_GRUNT_NORMAL );
			m_pPlayerModels[i]->SetTeam( TEAM_UNASSIGNED );
			m_pPlayerModels[i]->ClearCarriedItems();
			m_pPlayerModels[i]->LoadWearablesFromPlayer( iPlayerIndex );
			m_pPlayerModels[i]->SetModelTintColor( g_TDC_PR->GetPlayerColorVector( iPlayerIndex ) );
			m_pPlayerModels[i]->SetModelSkinToneColor( g_TDC_PR->GetPlayerSkinTone( iPlayerIndex ) );

			// Remember win animation to play later.
			if ( i == 0 )
			{
				m_iWinAnimation = g_TDC_PR->GetWinAnimation( iPlayerIndex );
			}
		}

		// show or hide labels for this player position
		m_pPlayerNames[i]->SetVisible( bShow );
		m_pPlayerKills[i]->SetVisible( bShow );
		m_pPlayerDeaths[i]->SetVisible( bShow );
		m_pPlayerModels[i]->SetVisible( bShow );
	}

	bool bPlayWinMusic = ( bPlayerFirst || GetLocalPlayerTeam() < FIRST_GAME_TEAM );

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, bPlayWinMusic ? "music.dm_winpanel_first" : "music.dm_winpanel" );

	m_flShowAt = gpGlobals->curtime + 4.5f;
}
