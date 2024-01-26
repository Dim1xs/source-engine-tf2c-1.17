//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tdc_imagepanel.h"
#include "c_tdc_player.h"
#include "tdc_gamerules.h"
#include "functionproxy.h"

using namespace vgui;

extern ConVar tdc_coloredhud;

DECLARE_BUILD_FACTORY( CTDCImagePanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCImagePanel::CTDCImagePanel( Panel *parent, const char *name ) : ScalableImagePanel( parent, name )
{
	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		m_szTeamBG[i][0] = '\0';
	}

	m_bAlwaysColored = false;

	UpdateBGTeam();

	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "server_spawn" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCImagePanel::ApplySettings( KeyValues *inResourceData )
{
	for ( int i = 0; i < TDC_TEAM_COUNT; i++ )
	{
		V_strcpy_safe( m_szTeamBG[i], inResourceData->GetString( VarArgs( "teambg_%d", i ), "" ) );
	}

	m_bAlwaysColored = inResourceData->GetBool( "alwaysColored" );

	BaseClass::ApplySettings( inResourceData );

	UpdateBGImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCImagePanel::UpdateBGImage( void )
{
	if ( m_iBGTeam >= 0 && m_iBGTeam < TDC_TEAM_COUNT )
	{
		if ( m_szTeamBG[m_iBGTeam][0] != '\0' )
		{
			SetImage( m_szTeamBG[m_iBGTeam] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCImagePanel::SetBGImage( int iTeamNum )
{
	m_iBGTeam = iTeamNum;
	UpdateBGImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCImagePanel::UpdateBGTeam( void )
{
	// Show special BG in FFA.
	if ( TDCGameRules() && !TDCGameRules()->IsTeamplay() )
	{
		m_iBGTeam = tdc_coloredhud.GetBool() || m_bAlwaysColored ? TEAM_UNASSIGNED : TEAM_SPECTATOR;
	}
	else
	{
		m_iBGTeam = GetLocalPlayerTeam();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCImagePanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		UpdateBGTeam();
		UpdateBGImage();
	}
	else if ( FStrEq( "server_spawn", event->GetName() ) )
	{
		m_iBGTeam = TEAM_SPECTATOR;
		UpdateBGImage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTDCImagePanel::GetDrawColor( void )
{
	Color tempColor = GetFgColor();
	tempColor[3] = GetAlpha();

	return tempColor;
}

//-----------------------------------------------------------------------------
// Purpose: Same as PlayerTintColor but gets color of the local player. Should be used on HUD panels.
//-----------------------------------------------------------------------------
class CLocalPlayerTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pPlayer )
		{
			m_pResult->SetVecValue( pPlayer->m_vecPlayerColor.Base(), 3 );
		}
		else
		{
			m_pResult->SetVecValue( 0, 0, 0 );
		}
	}
};

EXPOSE_INTERFACE( CLocalPlayerTintColor, IMaterialProxy, "LocalPlayerTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );
