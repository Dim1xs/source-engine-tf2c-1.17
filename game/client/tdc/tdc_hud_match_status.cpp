//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tdc_hud_match_status.h"
#include "iclientmode.h"
#include "tdc_round_timer.h"
#include "c_tdc_objective_resource.h"

using namespace vgui;

DECLARE_HUDELEMENT( CTDCHudMatchStatus );

CTDCHudMatchStatus::CTDCHudMatchStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMatchStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTimePanel = new CTDCHudTimeStatus( this, "ObjectiveStatusTimePanel" );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudMatchStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudMatchStatus.res" );

	// Hide everything but the timer.
	for ( int i = 0; i < GetChildCount(); i++ )
	{
		Panel *pChild = GetChild( i );
		if ( pChild && pChild != m_pTimePanel )
		{
			pChild->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudMatchStatus::Reset( void )
{
	m_pTimePanel->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudMatchStatus::OnThink( void )
{
	if ( !ObjectiveResource() )
		return;

	// check for an active timer and turn the time panel on or off if we need to
	// Don't draw in freezecam, or when the game's not running
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	bool bDisplayTimer = !( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );
	if ( bDisplayTimer )
	{
		// is the time panel still pointing at an active timer?
		int iCurrentTimer = m_pTimePanel->GetTimerIndex();
		CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer *>( ClientEntityList().GetEnt( iCurrentTimer ) );

		if ( pTimer && !pTimer->IsDormant() && pTimer->ShowInHud() && pTimer == ObjectiveResource()->GetTimerToShowInHUD() )
		{
			// the current timer is fine, make sure the panel is visible
			bDisplayTimer = true;
		}
		else
		{
			// check for a different timer
			pTimer = ObjectiveResource()->GetTimerToShowInHUD();
			bDisplayTimer = ( pTimer && !pTimer->IsDormant() && pTimer->ShowInHud() );

			if ( bDisplayTimer )
				m_pTimePanel->SetTimerIndex( pTimer->entindex() );
		}
	}

	m_pTimePanel->SetVisible( bDisplayTimer );
}
