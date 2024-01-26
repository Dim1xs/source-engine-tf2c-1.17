//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tdc_hud_spectator.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include "c_tdc_player.h"
#include "tdc_gamerules.h"
#include "gamevars_shared.h"

using namespace vgui;

const char *GetMapDisplayName( const char *mapName );

DECLARE_HUDELEMENT_DEPTH( CTDCSpectatorGUI, 95 ); // Show it above sniper scope.

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCSpectatorGUI::CTDCSpectatorGUI( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSpectator" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flNextTipChangeTime = 0;
	m_iTipClass = TDC_CLASS_UNDEFINED;

	m_pReinforcementsLabel = new Label( this, "ReinforcementsLabel", "" );
	m_pClassOrTeamLabel = new Label( this, "ClassOrTeamLabel", "" );
	m_pRespawnLabel = new Label( this, "RespawnLabel", "" );
	m_pSwitchCamModeKeyLabel = new Label( this, "SwitchCamModeKeyLabel", "" );
	m_pCycleTargetFwdKeyLabel = new Label( this, "CycleTargetFwdKeyLabel", "" );
	m_pCycleTargetRevKeyLabel = new Label( this, "CycleTargetRevKeyLabel", "" );
	m_pMapLabel = new Label( this, "MapLabel", "" );

	ivgui()->AddTickSignal( GetVPanel() );

	// remove ourselves from the global group so the scoreboard doesn't hide us
	UnregisterForRenderGroup( "global" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCSpectatorGUI::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/Spectator.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCSpectatorGUI::ShouldDraw( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	if ( pPlayer->GetObserverMode() <= OBS_MODE_FREEZECAM )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCSpectatorGUI::OnTick()
{
	if ( !IsVisible() )
		return;

	UpdateReinforcements();
	UpdateKeyLabels();
	UpdateRespawnLabel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCSpectatorGUI::UpdateRespawnLabel( void )
{
	if ( !m_pRespawnLabel )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( pPlayer )
	{
		static wchar_t wzFinal[512] = L"";
		float flTimeToRespawn = pPlayer->m_Shared.GetTimeUntilRespawn();
		if ( flTimeToRespawn > 0 )
		{
			char szSecs[6];
			V_sprintf_safe( szSecs, "%f", flTimeToRespawn );
			wchar_t wSecs[4];
			g_pVGuiLocalize->ConvertANSIToUnicode( szSecs, wSecs, sizeof( wSecs ) );
			g_pVGuiLocalize->ConstructString( wzFinal, sizeof( wzFinal ), g_pVGuiLocalize->Find( "#TDC_Spectator_Respawn_Countdown" ), 1, wSecs );
		}
		else
		{
			UTIL_ReplaceKeyBindings( g_pVGuiLocalize->Find( "#TDC_Spectator_Respawn" ), 0, wzFinal, sizeof( wzFinal ) );
		}
		m_pRespawnLabel->SetText( wzFinal );
		m_pRespawnLabel->SetVisible( pPlayer->m_Shared.IsWaitingToRespawn() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCSpectatorGUI::UpdateReinforcements( void )
{
	if ( !m_pReinforcementsLabel )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer || pPlayer->IsHLTV() ||
		( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM ) ||
		( pPlayer->m_Shared.GetState() != TDC_STATE_OBSERVER && pPlayer->m_Shared.GetState() != TDC_STATE_DYING ) ||
		( pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM ) )
	{
		m_pReinforcementsLabel->SetVisible( false );
		return;
	}

	wchar_t wLabel[128];

	if ( TDCGameRules()->InStalemate() )
	{
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#game_respawntime_stalemate" ), 0 );
	}
	else if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		// a team has won the round
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#game_respawntime_next_round" ), 0 );
	}
	else
	{
		//TODO: Remove wave code.
		float flNextRespawn = 0.0f;
		if ( !flNextRespawn )
		{
			m_pReinforcementsLabel->SetVisible( false );
			return;
		}

		int iRespawnWait = ( flNextRespawn - gpGlobals->curtime );
		if ( iRespawnWait <= 0 )
		{
			g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#game_respawntime_now" ), 0 );
		}
		else if ( iRespawnWait <= 1.0 )
		{
			g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#game_respawntime_in_sec" ), 0 );
		}
		else
		{
			char szSecs[6];
			V_sprintf_safe( szSecs, "%d", iRespawnWait );
			wchar_t wSecs[4];
			g_pVGuiLocalize->ConvertANSIToUnicode( szSecs, wSecs, sizeof( wSecs ) );
			g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#game_respawntime_in_secs" ), 1, wSecs );
		}
	}

	m_pReinforcementsLabel->SetVisible( true );
	m_pReinforcementsLabel->SetText( wLabel, true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCSpectatorGUI::UpdateKeyLabels( void )
{
	// get the desired player class
	int iClass = TDC_CLASS_UNDEFINED;
	bool bIsHLTV = engine->IsHLTV();

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( pPlayer )
	{
		iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	}

	// if it's time to change the tip, or the player has changed desired class, update the tip
	if ( ( gpGlobals->curtime >= m_flNextTipChangeTime ) || ( iClass != m_iTipClass ) )
	{
		if ( bIsHLTV )
		{
			const wchar_t *wzTip = g_pVGuiLocalize->Find( "#Tip_HLTV" );

			if ( wzTip )
			{
				SetDialogVariable( "tip", wzTip );
			}
		}
		else
		{
			SetDialogVariable( "tip", "" );
		}

		m_flNextTipChangeTime = gpGlobals->curtime + 10.0f;
		m_iTipClass = iClass;
	}

	if ( m_pClassOrTeamLabel )
	{
		C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
		if ( pPlayer )
		{
			static wchar_t wzFinal[512] = L"";
			const wchar_t *wzTemp = NULL;

			bool bVisible = true;
			if ( bIsHLTV )
			{
				wzTemp = g_pVGuiLocalize->Find( "#TDC_Spectator_AutoDirector" );
			}
			else if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				wzTemp = g_pVGuiLocalize->Find( "#TDC_Spectator_ChangeTeam" );
			}
			else
			{
				bVisible = false;
			}

			if ( wzTemp && bVisible )
			{
				UTIL_ReplaceKeyBindings( wzTemp, 0, wzFinal, sizeof( wzFinal ) );
				m_pClassOrTeamLabel->SetText( wzFinal );
			}

			m_pClassOrTeamLabel->SetVisible( bVisible );
		}
	}

	if ( m_pSwitchCamModeKeyLabel )
	{
		if ( ( pPlayer && pPlayer->GetTeamNumber() > TEAM_SPECTATOR ) && ( ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE ) || mp_fadetoblack.GetBool() ) )
		{
			if ( m_pSwitchCamModeKeyLabel->IsVisible() )
			{
				m_pSwitchCamModeKeyLabel->SetVisible( false );

				Label *pLabel = dynamic_cast<Label *>( FindChildByName( "SwitchCamModeLabel" ) );
				if ( pLabel )
				{
					pLabel->SetVisible( false );
				}
			}
		}
		else
		{
			if ( !m_pSwitchCamModeKeyLabel->IsVisible() )
			{
				m_pSwitchCamModeKeyLabel->SetVisible( true );

				Label *pLabel = dynamic_cast<Label *>( FindChildByName( "SwitchCamModeLabel" ) );
				if ( pLabel )
				{
					pLabel->SetVisible( true );
				}
			}

			wchar_t wLabel[256] = L"";
			const wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TDC_Spectator_SwitchCamModeKey" );
			UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
			m_pSwitchCamModeKeyLabel->SetText( wLabel );
		}
	}

	if ( m_pCycleTargetFwdKeyLabel )
	{
		if ( ( pPlayer && pPlayer->GetTeamNumber() > TEAM_SPECTATOR ) && ( mp_fadetoblack.GetBool() || ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE ) ) )
		{
			if ( m_pCycleTargetFwdKeyLabel->IsVisible() )
			{
				m_pCycleTargetFwdKeyLabel->SetVisible( false );

				Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetFwdLabel" ) );
				if ( pLabel )
				{
					pLabel->SetVisible( false );
				}
			}
		}
		else
		{
			if ( !m_pCycleTargetFwdKeyLabel->IsVisible() )
			{
				m_pCycleTargetFwdKeyLabel->SetVisible( true );

				Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetFwdLabel" ) );
				if ( pLabel )
				{
					pLabel->SetVisible( true );
				}
			}

			wchar_t wLabel[256] = L"";
			const wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TDC_Spectator_CycleTargetFwdKey" );
			UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
			m_pCycleTargetFwdKeyLabel->SetText( wLabel );
		}
	}

	if ( m_pCycleTargetRevKeyLabel )
	{
		if ( ( pPlayer && pPlayer->GetTeamNumber() > TEAM_SPECTATOR ) && ( mp_fadetoblack.GetBool() || ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE ) ) )
		{
			if ( m_pCycleTargetRevKeyLabel->IsVisible() )
			{
				m_pCycleTargetRevKeyLabel->SetVisible( false );

				Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetRevLabel" ) );
				if ( pLabel )
				{
					pLabel->SetVisible( false );
				}
			}
		}
		else
		{
			if ( !m_pCycleTargetRevKeyLabel->IsVisible() )
			{
				m_pCycleTargetRevKeyLabel->SetVisible( true );

				Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetRevLabel" ) );
				if ( pLabel )
				{
					pLabel->SetVisible( true );
				}
			}

			wchar_t wLabel[256] = L"";
			const wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TDC_Spectator_CycleTargetRevKey" );
			UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
			m_pCycleTargetRevKeyLabel->SetText( wLabel );
		}
	}

	if ( m_pMapLabel )
	{
		wchar_t wMapName[16];
		wchar_t wLabel[256];
		char szMapName[16];

		char tempname[128];
		Q_FileBase( engine->GetLevelName(), tempname, sizeof( tempname ) );
		Q_strlower( tempname );

		if ( IsX360() )
		{
			char *pExt = Q_stristr( tempname, ".360" );
			if ( pExt )
			{
				*pExt = '\0';
			}
		}

		V_strcpy_safe( szMapName, GetMapDisplayName( tempname ) );

		g_pVGuiLocalize->ConvertANSIToUnicode( szMapName, wMapName, sizeof( wMapName ) );
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#Spec_Map" ), 1, wMapName );

		m_pMapLabel->SetText( wLabel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CTDCSpectatorGUI::SetVisible( bool bVisible )
{
	if ( bVisible != IsVisible() )
	{
		if ( bVisible )
		{
			m_flNextTipChangeTime = 0;	// force a new tip immediately
			UpdateKeyLabels();
		}
	}

	BaseClass::SetVisible( bVisible );
}
