//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

#include "tdc_hud_condstatus.h"
#include "tdc_hud_freezepanel.h"
#include "tdc_gamerules.h"

using namespace vgui;

DECLARE_HUDELEMENT( CTDCHudCondStatus );

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
CTDCPowerupPanel::CTDCPowerupPanel( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pProgressBar = new CTDCProgressBar( this, "TimePanelProgressBar" );
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CTDCPowerupPanel::ApplySchemeSettings( IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/PowerupPanel.res" );

	if ( m_pProgressBar )
	{
		for ( int i = 0; g_aPowerups[i].cond != TDC_COND_LAST; i++ )
		{
			ETDCCond nCond = g_aPowerups[i].cond;
			if ( nCond == m_nCond )
			{
				m_pProgressBar->SetIcon( g_aPowerups[i].hud_icon );
			}
		}
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CTDCPowerupPanel::UpdateStatus( void )
{
	// Update remaining power-up time.
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( pPlayer )
	{
		m_flDuration = pPlayer->m_Shared.GetConditionDuration( m_nCond );
		m_flInitDuration = Max( m_flDuration, m_flInitDuration );
	}

	if ( m_pProgressBar )
	{
		float flPerc = ( m_flDuration != PERMANENT_CONDITION ) ? ( m_flInitDuration - m_flDuration ) / m_flInitDuration : 0.0f;
		m_pProgressBar->SetPercentage( flPerc );
	}
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CTDCPowerupPanel::SetData( ETDCCond cond, float dur, float initdur )
{
	m_nCond = cond;
	m_flDuration = dur;
	m_flInitDuration = initdur;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudCondStatus::CTDCHudCondStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudCondStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	for ( int i = 0; g_aPowerups[i].cond != TDC_COND_LAST; i++ )
	{
		CTDCPowerupPanel *pPowerup = new CTDCPowerupPanel( this, "PowerupPanel" );
		pPowerup->SetData( g_aPowerups[i].cond, 0.0f, 0.0f );
		m_pPowerups.AddToTail( pPowerup );
	}

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCHudCondStatus::~CTDCHudCondStatus()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCHudCondStatus::ShouldDraw( void )
{
	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudCondStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudCondStatus.res" );
}

void CTDCHudCondStatus::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

static int DurationSort( CTDCPowerupPanel* const *a, CTDCPowerupPanel* const *b )
{
	return ( ( *a )->m_flDuration < ( *b )->m_flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudCondStatus::PerformLayout()
{
	BaseClass::PerformLayout();

	int count = m_pPowerups.Count();

	int totalWide = 0;
	for ( int i = 0; i < count; i++ )
	{
		m_pPowerups[i]->SetPos( totalWide, 0 );

		// Skip inactive powerups.
		if ( m_pPowerups[i]->IsVisible() )
		{
			totalWide += m_pPowerups[i]->GetWide();
		}
	}

	SetWide( totalWide );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudCondStatus::OnTick( void )
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( pPlayer )
	{
		for ( int i = 0; i < m_pPowerups.Count(); i++ )
		{
			m_pPowerups[i]->UpdateStatus();
		}
	}

	bool bUpdateLayout = false;

	for ( int i = 0; i < m_pPowerups.Count(); i++ )
	{
		// Show indicators for active power-ups.
		bool bWasVisible = m_pPowerups[i]->IsVisible();
		bool bVisible = m_pPowerups[i]->m_flDuration != 0.0f;

		if ( bVisible != bWasVisible )
		{
			m_pPowerups[i]->SetVisible( bVisible );
			bUpdateLayout = true;
		}
	}

	if ( bUpdateLayout )
		InvalidateLayout( true );
}
