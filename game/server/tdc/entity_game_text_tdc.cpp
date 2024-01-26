//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "tdc_gamerules.h"
#include "tdc_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTDCHudNotify : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTDCHudNotify, CLogicalEntity );
	DECLARE_DATADESC();

	void InputDisplay( inputdata_t &inputdata );
	void Display( CBaseEntity *pActivator );

private:
	string_t m_iszMessage;
	string_t m_iszIcon;
	int m_iRecipientTeam;
	int m_iBackgroundTeam;
};

LINK_ENTITY_TO_CLASS( game_text_tdc, CTDCHudNotify );

BEGIN_DATADESC( CTDCHudNotify )

DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
DEFINE_KEYFIELD( m_iszIcon, FIELD_STRING, "icon" ),
DEFINE_KEYFIELD( m_iRecipientTeam, FIELD_INTEGER, "display_to_team" ),
DEFINE_KEYFIELD( m_iBackgroundTeam, FIELD_INTEGER, "background" ),

// Inputs
DEFINE_INPUTFUNC( FIELD_VOID, "Display", InputDisplay ),

END_DATADESC()


void CTDCHudNotify::InputDisplay( inputdata_t &inputdata )
{
	Display( inputdata.pActivator );
}

void CTDCHudNotify::Display( CBaseEntity *pActivator )
{
	CTeamRecipientFilter filter( m_iRecipientTeam );

	TDCGameRules()->SendHudNotification( filter, STRING( m_iszMessage ), STRING( m_iszIcon ), m_iBackgroundTeam );
}
