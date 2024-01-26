//=============================================================================//
//
// Purpose: Totally redundant CP unlock countdown used in Arena.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_hud_arena_capturepoint.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include "tdc_hud_freezepanel.h"
#include "tdc_gamerules.h"

using namespace vgui;


DECLARE_HUDELEMENT( CHudArenaCapPointCountdown );

CHudArenaCapPointCountdown::CHudArenaCapPointCountdown( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaCapPointCountdown" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCapCountdownLabel = new CExLabel( this, "CapCountdownLabel", "" );

	m_bFire5SecRemain = true;
	m_bFire4SecRemain = true;
	m_bFire3SecRemain = true;
	m_bFire2SecRemain = true;
	m_bFire1SecRemain = true;
	m_bFire0SecRemain = true;

	ivgui()->AddTickSignal( GetVPanel() );
	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaCapPointCountdown::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudArenaCapPointCountdown.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaCapPointCountdown::ShouldDraw( void )
{
	if ( IsInFreezeCam() )
		return false;

	if ( !TFGameRules() )
		return false;

	if ( TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->State_Get() != GR_STATE_STALEMATE || TFGameRules()->GetCapEnableTime() == 0.0f )
		return false;

	int iTimeLeft = ceil( TFGameRules()->GetCapEnableTime() - gpGlobals->curtime );
	if ( iTimeLeft > 5 || iTimeLeft <= 0 )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaCapPointCountdown::OnTick( void )
{
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	if ( !TFGameRules() )
		return;

	if ( TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->State_Get() != GR_STATE_STALEMATE || TFGameRules()->GetCapEnableTime() == 0.0f )
	{
		return;
	}

	int iTimeLeft = ceil( TFGameRules()->GetCapEnableTime() - gpGlobals->curtime );
	if ( iTimeLeft > 5 || iTimeLeft <= 0 )
	{
		if ( iTimeLeft <= 0 && m_bFire0SecRemain )
		{
			m_bFire0SecRemain = false;
			pLocalPlayer->EmitSound( "Announcer.AM_CapEnabledRandom" );
		}

		return;
	}

	SetDialogVariable( "capturetime", iTimeLeft );

	if ( iTimeLeft <= 5 && m_bFire5SecRemain )
	{
		m_bFire5SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins5Seconds" );
	}
	else if ( iTimeLeft <= 4 && m_bFire4SecRemain )
	{
		m_bFire4SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins4Seconds" );
	}
	else if ( iTimeLeft <= 3 && m_bFire3SecRemain )
	{
		m_bFire3SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins3Seconds" );
	}
	else if ( iTimeLeft <= 2 && m_bFire2SecRemain )
	{
		m_bFire2SecRemain = false;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins2Seconds" );
	}
	else if ( iTimeLeft <= 1 && m_bFire1SecRemain )
	{
		m_bFire1SecRemain = false;
		m_bFire0SecRemain = true;
		pLocalPlayer->EmitSound( "Announcer.RoundBegins1Seconds" );
	}
}
