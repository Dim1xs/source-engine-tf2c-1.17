//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Client side C_TDCTeam class
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tdc_team.h"
#include "tdc_shareddefs.h"
#include <vgui_controls/Controls.h>
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TDCTeam, DT_TDCTeam, CTDCTeam )
	RecvPropInt( RECVINFO( m_iRoundScore ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDCTeam::C_TDCTeam()
{
	m_wszLocalizedTeamName[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDCTeam::~C_TDCTeam()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCTeam::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		UpdateTeamName();
		SetNextClientThink( gpGlobals->curtime + 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCTeam::ClientThink( void )
{
	UpdateTeamName();
	SetNextClientThink( gpGlobals->curtime + 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCTeam::UpdateTeamName( void )
{
	// TODO: Add tournament mode team name handling here.
	const wchar_t *pwszLocalized = NULL;

	switch ( GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		pwszLocalized = g_pVGuiLocalize->Find( "#TDC_RedTeam_Name" );
		break;
	case TDC_TEAM_BLUE:
		pwszLocalized = g_pVGuiLocalize->Find( "#TDC_BlueTeam_Name" );
		break;
	case TEAM_SPECTATOR:
		pwszLocalized = g_pVGuiLocalize->Find( "#TDC_Spectators" );
		break;
	}

	if ( pwszLocalized )
	{
		V_wcscpy_safe( m_wszLocalizedTeamName, pwszLocalized );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( g_aTeamNames[GetTeamNumber()], m_wszLocalizedTeamName, sizeof( m_wszLocalizedTeamName ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the C_TDCTeam for the specified team number
//-----------------------------------------------------------------------------
C_TDCTeam *GetGlobalTFTeam( int iTeamNumber )
{
	return assert_cast<C_TDCTeam *>( GetGlobalTeam( iTeamNumber ) );
}

const wchar_t *GetLocalizedTeamName( int iTeamNumber )
{
	C_TDCTeam *pTeam = GetGlobalTFTeam( iTeamNumber );
	if ( pTeam )
	{
		return pTeam->GetTeamName();
	}

	return L"";
}
