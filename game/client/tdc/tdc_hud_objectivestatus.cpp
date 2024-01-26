//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=====================================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "tdc_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tdc_player.h"
#include "c_team.h"
#include "c_tdc_team.h"
#include "c_tdc_objective_resource.h"
#include "tdc_hud_flagstatus.h"
#include "tdc_hud_objectivestatus.h"
#include "tdc_hud_deathmatchstatus.h"
#include "tdc_gamerules.h"

using namespace vgui;

extern ConVar tdc_coloredhud;

DECLARE_BUILD_FACTORY( CTDCProgressBar );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCProgressBar::CTDCProgressBar( vgui::Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{
	m_flPercent = 0.0f;

	SetIcon( "hud/objectives_timepanel_progressbar" );
}

void CTDCProgressBar::SetIcon( const char* szIcon )
{
	m_iTexture = vgui::surface()->DrawGetTextureId( szIcon );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();
	}

	vgui::surface()->DrawSetTextureFile( m_iTexture, szIcon, true, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProgressBar::Paint()
{
	int wide, tall;
	GetSize( wide, tall );

	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv11( uv1, uv1 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );
	Vector2D uv12( uv1, uv2 );

	vgui::Vertex_t verts[4];
	verts[0].Init( Vector2D( 0, 0 ), uv11 );
	verts[1].Init( Vector2D( wide, 0 ), uv21 );
	verts[2].Init( Vector2D( wide, tall ), uv22 );
	verts[3].Init( Vector2D( 0, tall ), uv12 );

	// first, just draw the whole thing inactive.
	vgui::surface()->DrawSetTexture( m_iTexture );
	vgui::surface()->DrawSetColor( m_clrInActive );
	vgui::surface()->DrawTexturedPolygon( 4, verts );

	// now, let's calculate the "active" part of the progress bar
	if ( m_flPercent < m_flPercentWarning )
	{
		vgui::surface()->DrawSetColor( m_clrActive );
	}
	else
	{
		vgui::surface()->DrawSetColor( m_clrWarning );
	}

	// we're going to do this using quadrants
	//  -------------------------
	//  |           |           |
	//  |           |           |
	//  |     4     |     1     |
	//  |           |           |
	//  |           |           |
	//  -------------------------
	//  |           |           |
	//  |           |           |
	//  |     3     |     2     |
	//  |           |           |
	//  |           |           |
	//  -------------------------

	float flCompleteCircle = ( 2.0f * M_PI_F );
	float fl90degrees = flCompleteCircle / 4.0f;

	float flEndAngle = flCompleteCircle * ( 1.0f - m_flPercent ); // count DOWN (counter-clockwise)
	//float flEndAngle = flCompleteCircle * m_flPercent; // count UP (clockwise)

	float flHalfWide = (float)wide / 2.0f;
	float flHalfTall = (float)tall / 2.0f;

	if ( flEndAngle >= fl90degrees * 3.0f ) // >= 270 degrees
	{
		// draw the first and second quadrants
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 1.0f );
		uv12.Init( 0.5, 1.0f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, tall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, tall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the third quadrant
		uv11.Init( 0.0f, 0.5f );
		uv21.Init( 0.5f, 0.5f );
		uv22.Init( 0.5f, 1.0f );
		uv12.Init( 0.0f, 1.0f );

		verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
		verts[1].Init( Vector2D( flHalfWide, flHalfTall ), uv21 );
		verts[2].Init( Vector2D( flHalfWide, tall ), uv22 );
		verts[3].Init( Vector2D( 0.0f, tall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial fourth quadrant
		if ( flEndAngle > fl90degrees * 3.5f ) // > 315 degrees
		{
			uv11.Init( 0.0f, 0.0f );
			uv21.Init( 0.5f - ( tan( fl90degrees * 4.0f - flEndAngle ) * 0.5 ), 0.0f );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.0f, 0.5f );

			verts[0].Init( Vector2D( 0.0f, 0.0f ), uv11 );
			verts[1].Init( Vector2D( flHalfWide - ( tan( fl90degrees * 4.0f - flEndAngle ) * flHalfTall ), 0.0f ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 315 degrees
		{
			uv11.Init( 0.0f, 0.5f );
			uv21.Init( 0.0f, 0.5f - ( tan( flEndAngle - fl90degrees * 3.0f ) * 0.5 ) );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.0f, 0.5f );

			verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( 0.0f, flHalfTall - ( tan( flEndAngle - fl90degrees * 3.0f ) * flHalfWide ) ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else if ( flEndAngle >= fl90degrees * 2.0f ) // >= 180 degrees
	{
		// draw the first and second quadrants
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 1.0f );
		uv12.Init( 0.5, 1.0f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, tall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, tall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial third quadrant
		if ( flEndAngle > fl90degrees * 2.5f ) // > 225 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 0.5f, 1.0f );
			uv22.Init( 0.0f, 1.0f );
			uv12.Init( 0.0f, 0.5f + ( tan( fl90degrees * 3.0f - flEndAngle ) * 0.5 ) );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
			verts[2].Init( Vector2D( 0.0f, tall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall + ( tan( fl90degrees * 3.0f - flEndAngle ) * flHalfWide ) ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 225 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 0.5f, 1.0f );
			uv22.Init( 0.5f - ( tan( flEndAngle - fl90degrees * 2.0f ) * 0.5 ), 1.0f );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
			verts[2].Init( Vector2D( flHalfWide - ( tan( flEndAngle - fl90degrees * 2.0f ) * flHalfTall ), tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else if ( flEndAngle >= fl90degrees ) // >= 90 degrees
	{
		// draw the first quadrant
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 0.5f );
		uv12.Init( 0.5f, 0.5f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, flHalfTall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial second quadrant
		if ( flEndAngle > fl90degrees * 1.5f ) // > 135 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 1.0f, 0.5f );
			uv22.Init( 1.0f, 1.0f );
			uv12.Init( 0.5f + ( tan( fl90degrees * 2.0f - flEndAngle ) * 0.5f ), 1.0f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( wide, tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide + ( tan( fl90degrees * 2.0f - flEndAngle ) * flHalfTall ), tall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 135 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 1.0f, 0.5f );
			uv22.Init( 1.0f, 0.5f + ( tan( flEndAngle - fl90degrees ) * 0.5f ) );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall + ( tan( flEndAngle - fl90degrees ) * flHalfWide ) ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else // > 0 degrees
	{
		if ( flEndAngle > fl90degrees / 2.0f ) // > 45 degrees
		{
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 1.0f, 0.0f );
			uv22.Init( 1.0f, 0.5f - ( tan( fl90degrees - flEndAngle ) * 0.5 ) );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall - ( tan( fl90degrees - flEndAngle ) * flHalfWide ) ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 45 degrees
		{
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 0.5 + ( tan( flEndAngle ) * 0.5 ), 0.0f );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.5f, 0.0f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( flHalfWide + ( tan( flEndAngle ) * flHalfTall ), 0.0f ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, 0.0f ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudTimeStatus::CTDCHudTimeStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_iTeamIndex = -1;
	m_pTimeValue = new CExLabel( this, "TimePanelValue", "" );
	m_pProgressBar = NULL;
	m_pOvertimeLabel = NULL;
	m_pOvertimeBG = NULL;
	m_pSuddenDeathLabel = NULL;
	m_pSuddenDeathBG = NULL;
	m_pWaitingForPlayersBG = NULL;
	m_pWaitingForPlayersLabel = NULL;
	m_pSetupLabel = NULL;
	m_pSetupBG = NULL;
	m_pTimePanelBG = NULL;

	m_flNextThink = 0.0f;
	m_iTimerIndex = 0;
	m_bOvertime = false;

	m_iTimerDeltaHead = 0;
	for ( int i = 0; i < NUM_TIMER_DELTA_ITEMS; i++ )
	{
		m_TimerDeltaItems[i].m_flDieTime = 0.0f;
	}

	ListenForGameEvent( "teamplay_update_timer" );
	ListenForGameEvent( "teamplay_timer_time_added" );
	ListenForGameEvent( "localplayer_changeteam" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_update_timer" ) )
	{
		SetExtraTimePanels();
	}
	else if ( !Q_strcmp( eventName, "teamplay_timer_time_added" ) )
	{
		int iIndex = event->GetInt( "timer", -1 );
		int nSeconds = event->GetInt( "seconds_added", 0 );

		SetTimeAdded( iIndex, nSeconds );
	}
	else if ( !Q_strcmp( eventName, "localplayer_changeteam" ) )
	{
		SetTeamBackground();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::SetTeamBackground( void )
{
	if ( !TDCGameRules() )
		return;

	if ( m_pTimePanelBG )
	{
		const char *pszImage = "../hud/objectives_timepanel_black_bg";

		if ( TDCGameRules()->IsTeamplay() )
		{
			int iTeamNumber = GetLocalPlayerTeam();

			if ( m_iTeamIndex > -1 )
				iTeamNumber = m_iTeamIndex;

			switch ( iTeamNumber )
			{
			case TDC_TEAM_RED:
				pszImage = "../hud/objectives_timepanel_red_bg";
				break;

			case TDC_TEAM_BLUE:
				pszImage = "../hud/objectives_timepanel_blue_bg";
				break;

			default:
				pszImage = "../hud/objectives_timepanel_black_bg";
				break;
			}
		}
		else
		{
			if ( tdc_coloredhud.GetBool() )
			{
				pszImage = "../hud/objectives_timepanel_custom_bg";
			}
		}

		m_pTimePanelBG->SetImage( pszImage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::SetTimeAdded( int iIndex, int nSeconds )
{
	if ( m_iTimerIndex != iIndex ) // make sure this is the timer we're displaying in the HUD
		return;

	if ( nSeconds != 0 )
	{
		// create a delta item that floats off the top
		timer_delta_t *pNewDeltaItem = &m_TimerDeltaItems[m_iTimerDeltaHead];

		m_iTimerDeltaHead++;
		m_iTimerDeltaHead %= NUM_TIMER_DELTA_ITEMS;

		pNewDeltaItem->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
		pNewDeltaItem->m_nAmount = nSeconds;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::CheckClockLabelLength( CExLabel *pLabel, CTDCImagePanel *pBG )
{
	if ( !pLabel || !pBG )
		return;

	int textWide, textTall;
	pLabel->GetContentSize( textWide, textTall );

	// make sure our string isn't longer than the label it's in
	if ( textWide > pLabel->GetWide() )
	{
		int xStart, yStart, wideStart, tallStart;
		pLabel->GetBounds( xStart, yStart, wideStart, tallStart );

		int newXPos = xStart + ( wideStart / 2.0f ) - ( textWide / 2.0f );
		pLabel->SetBounds( newXPos, yStart, textWide, tallStart );
	}

	// turn off the background if our text label is wider than it is
	if ( pLabel->GetWide() > pBG->GetWide() )
	{
		pBG->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::SetExtraTimePanels()
{
	if ( !TDCGameRules() )
		return;

	CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer*>( ClientEntityList().GetEnt( m_iTimerIndex ) );
	if ( !pTimer )
		return;

	if ( m_pSetupLabel )
	{
		bool bInSetup = TDCGameRules()->InSetup();

		if ( m_pSetupBG )
			m_pSetupBG->SetVisible( bInSetup );

		m_pSetupLabel->SetVisible( bInSetup );
	}

	// Set the Sudden Death panels to be visible
	if ( m_pSuddenDeathLabel )
	{
		bool bInSD = TDCGameRules()->InStalemate();

		if ( m_pSuddenDeathBG )
			m_pSuddenDeathBG->SetVisible( bInSD );

		m_pSuddenDeathLabel->SetVisible( bInSD );
	}

	if ( m_pOvertimeLabel )
	{
		m_bOvertime = TDCGameRules()->InOvertime();

		if ( m_bOvertime )
		{
			if ( m_pOvertimeBG && !m_pOvertimeBG->IsVisible() )
			{
				m_pOvertimeLabel->SetAlpha( 0 );
				m_pOvertimeBG->SetAlpha( 0 );

				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "OvertimeShow" );

				// need to turn off the SuddenDeath images if they're on
				if ( m_pSuddenDeathBG )
					m_pSuddenDeathBG->SetVisible( false );

				m_pSuddenDeathLabel->SetVisible( false );
			}

			if ( m_pOvertimeBG )
				m_pOvertimeBG->SetVisible( true );

			m_pOvertimeLabel->SetVisible( true );

			CheckClockLabelLength( m_pOvertimeLabel, m_pOvertimeBG );
		}
		else
		{
			if ( m_pOvertimeBG )
				m_pOvertimeBG->SetVisible( false );

			m_pOvertimeLabel->SetVisible( false );
		}
	}

	if ( m_pWaitingForPlayersLabel )
	{
		bool bInWaitingForPlayers = TDCGameRules()->IsInWaitingForPlayers();

		m_pWaitingForPlayersLabel->SetVisible( bInWaitingForPlayers );

		if ( m_pWaitingForPlayersBG )
			m_pWaitingForPlayersBG->SetVisible( bInWaitingForPlayers );

		if ( bInWaitingForPlayers )
		{
			// can't be waiting for players *AND* in setup at the same time

			if ( m_pSetupLabel )
				m_pSetupLabel->SetVisible( false );

			if ( m_pSetupBG )
				m_pSetupBG->SetVisible( false );

			CheckClockLabelLength( m_pWaitingForPlayersLabel, m_pWaitingForPlayersBG );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_iTimerIndex = 0;

	m_iTimerDeltaHead = 0;
	for ( int i = 0; i < NUM_TIMER_DELTA_ITEMS; i++ )
	{
		m_TimerDeltaItems[i].m_flDieTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveTimePanel.res" );

	//m_pTimeValue = dynamic_cast<CExLabel *>(FindChildByName("TimePanelValue"));
	m_pProgressBar = dynamic_cast<CTDCProgressBar *>( FindChildByName( "TimePanelProgressBar" ) );

	m_pOvertimeLabel = dynamic_cast<CExLabel *>( FindChildByName( "OvertimeLabel" ) );
	m_pOvertimeBG = dynamic_cast<CTDCImagePanel *>( FindChildByName( "OvertimeBG" ) );

	m_pSuddenDeathLabel = dynamic_cast<CExLabel *>( FindChildByName( "SuddenDeathLabel" ) );
	m_pSuddenDeathBG = dynamic_cast<CTDCImagePanel *>( FindChildByName( "SuddenDeathBG" ) );

	m_pWaitingForPlayersLabel = dynamic_cast<CExLabel *>( FindChildByName( "WaitingForPlayersLabel" ) );
	m_pWaitingForPlayersBG = dynamic_cast<CTDCImagePanel *>( FindChildByName( "WaitingForPlayersBG" ) );

	m_pSetupLabel = dynamic_cast<CExLabel *>( FindChildByName( "SetupLabel" ) );
	m_pSetupBG = dynamic_cast<CTDCImagePanel *>( FindChildByName( "SetupBG" ) );

	m_pTimePanelBG = dynamic_cast<ScalableImagePanel *>( FindChildByName( "TimePanelBG" ) );

	m_flNextThink = 0.0f;
	m_iTimerIndex = 0;

	SetTeamBackground();
	SetExtraTimePanels();

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer*>( ClientEntityList().GetEnt( m_iTimerIndex ) );
		// get the time remaining (in seconds)
		if ( pTimer )
		{
			int nTotalTime = pTimer->GetTimerMaxLength();
			int nTimeRemaining = (int)ceil( pTimer->GetTimeRemaining() );

			if ( !pTimer->ShowTimeRemaining() )
			{
				nTimeRemaining = nTotalTime - nTimeRemaining;
			}

			if ( m_pTimeValue && m_pTimeValue->IsVisible() )
			{
				// set our label
				int nMinutes = 0;
				int nSeconds = 0;
				char temp[256];

				if ( nTimeRemaining <= 0 )
				{
					nMinutes = 0;
					nSeconds = 0;
				}
				else
				{
					nMinutes = nTimeRemaining / 60;
					nSeconds = nTimeRemaining % 60;
				}

				V_sprintf_safe( temp, "%d:%02d", nMinutes, nSeconds );
				m_pTimeValue->SetText( temp );
			}

			// let the progress bar know the percentage of time that's passed ( 0.0 -> 1.0 )
			if ( m_pProgressBar && m_pProgressBar->IsVisible() )
			{
				m_pProgressBar->SetPercentage( ( (float)nTotalTime - nTimeRemaining ) / (float)nTotalTime );
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}

	int iTimerYPos = 0;

	SetPos( GetXPos(), iTimerYPos );
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CTDCHudTimeStatus::Paint( void )
{
	BaseClass::Paint();

	for ( int i = 0; i < NUM_TIMER_DELTA_ITEMS; i++ )
	{
		// update all the valid delta items
		if ( m_TimerDeltaItems[i].m_flDieTime > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			// color is determined by the delta - green for positive, red for negative

			Color c = ( m_TimerDeltaItems[i].m_nAmount > 0 ) ? m_DeltaPositiveColor : m_DeltaNegativeColor;

			float flLifetimePercent = ( m_TimerDeltaItems[i].m_flDieTime - gpGlobals->curtime ) / m_flDeltaLifetime;

			// fade out after half our lifetime
			if ( flLifetimePercent < 0.5 )
			{
				c[3] = (int)( 255.0f * ( flLifetimePercent / 0.5 ) );
			}

			float flHeight = ( m_flDeltaItemStartPos - m_flDeltaItemEndPos );
			float flYPos = m_flDeltaItemEndPos + flLifetimePercent * flHeight;

			vgui::surface()->DrawSetTextFont( m_hDeltaItemFont );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawSetTextPos( m_flDeltaItemX, (int)flYPos );

			wchar_t wBuf[20];
			int nMinutes, nSeconds;
			int nClockTime = ( m_TimerDeltaItems[i].m_nAmount > 0 ) ? m_TimerDeltaItems[i].m_nAmount : ( m_TimerDeltaItems[i].m_nAmount * -1 );
			nMinutes = nClockTime / 60;
			nSeconds = nClockTime % 60;

			if ( m_TimerDeltaItems[i].m_nAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d:%02d", nMinutes, nSeconds );
			}
			else
			{
				V_swprintf_safe( wBuf, L"-%d:%02d", nMinutes, nSeconds );
			}

			vgui::surface()->DrawPrintText( wBuf, wcslen( wBuf ), FONT_DRAW_NONADDITIVE );
		}
	}
}


DECLARE_HUDELEMENT( CTDCHudObjectiveStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudObjectiveStatus::CTDCHudObjectiveStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudObjectiveStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pFlagPanel = new CTDCHudFlagObjectives( this, "ObjectiveStatusFlagPanel" );
	m_pDMPanel = new CTDCHudDeathMatchObjectives( this, "ObjectiveStatusDeathMatchPanel" );
	m_pTDMPanel = new CTDCHudTeamDeathMatchObjectives( this, "ObjectiveStatusTeamDeathmatchPanel" );
	m_pBloodMoneyPanel = new CHudBloodMoney( this, "ObjectiveStatusBloodMoney" );
	m_pTeamBloodMoneyPanel = new CHudTeamBloodMoney( this, "ObjectiveStatusTeamBloodMoney" );

	SetHiddenBits( 0 );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudObjectiveStatus::ApplySchemeSettings( IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/HudObjectiveStatus.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudObjectiveStatus::Reset()
{
	m_pFlagPanel->Reset();
	m_pDMPanel->Reset();
	m_pBloodMoneyPanel->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudObjectiveStatus::OnThink( void )
{
	SetVisiblePanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudObjectiveStatus::SetVisiblePanels( void )
{
	if ( !TDCGameRules() )
		return;

	TurnOffPanels();

	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_FFA:
	case TDC_GAMETYPE_DUEL:
		m_pDMPanel->SetVisible( true );
		break;
	case TDC_GAMETYPE_TDM:
		m_pTDMPanel->SetVisible( true );
		break;
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_ATTACK_DEFEND:
	case TDC_GAMETYPE_INVADE:
		m_pFlagPanel->SetVisible( true );
		break;
	case TDC_GAMETYPE_BM:
		m_pBloodMoneyPanel->SetVisible( true );
		break;
	case TDC_GAMETYPE_TBM:
		m_pTeamBloodMoneyPanel->SetVisible( true );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudObjectiveStatus::TurnOffPanels()
{
	m_pFlagPanel->SetVisible( false );
	m_pDMPanel->SetVisible( false );
	m_pTDMPanel->SetVisible( false );
	m_pBloodMoneyPanel->SetVisible( false );
	m_pTeamBloodMoneyPanel->SetVisible( false );
}
