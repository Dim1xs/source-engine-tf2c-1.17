//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTDC Regenerate Zone.
//
//=============================================================================//

#include "cbase.h"
#include "tdc_player.h"
#include "tdc_item.h"
#include "tdc_team.h"
#include "func_regenerate.h"
#include "tdc_gamerules.h"
#include "eventqueue.h"

LINK_ENTITY_TO_CLASS( func_restock, CRestockZone );

#define TDC_REGENERATE_SOUND				"Regenerate.Touch"
#define TDC_REGENERATE_NEXT_USE_TIME		3.0f

//=============================================================================
//
// CTDC Regenerate Zone tables.
//

BEGIN_DATADESC( CRestockZone )
	DEFINE_FIELD( m_hAssociatedModel, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszAssociatedModel, FIELD_STRING, "associatedmodel" ),
	DEFINE_KEYFIELD( m_bRestoreHealth, FIELD_BOOLEAN, "RestoreHealth" ),
	DEFINE_KEYFIELD( m_bRestoreAmmo, FIELD_BOOLEAN, "RestoreAmmo" ),

	// Functions.
	DEFINE_ENTITYFUNC( Touch ),
END_DATADESC();

//=============================================================================
//
// CTDC Regenerate Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRestockZone::CRestockZone()
{
	m_bDisabled = false;
	m_bRestoreHealth = true;
	m_bRestoreAmmo = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CRestockZone::Spawn( void )
{
	Precache();
	InitTrigger();
	SetTouch( &CRestockZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CRestockZone::Precache( void )
{
	PrecacheScriptSound( TDC_REGENERATE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the entity
//-----------------------------------------------------------------------------
void CRestockZone::Activate( void )
{
	BaseClass::Activate();

	if ( m_iszAssociatedModel != NULL_STRING )
	{
		CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, STRING(m_iszAssociatedModel) );
		if ( !pEnt )
		{
			Warning("%s(%s) unable to find associated model named '%s'.\n", GetClassname(), GetDebugName(), STRING(m_iszAssociatedModel) );
		}
		else
		{
			m_hAssociatedModel = dynamic_cast<CDynamicProp*>(pEnt);
			if ( !m_hAssociatedModel )
			{
				Warning("%s(%s) tried to use associated model named '%s', but it isn't a dynamic prop.\n", GetClassname(), GetDebugName(), STRING(m_iszAssociatedModel) );
			}
		}	
	}
	else
	{
		Warning("%s(%s) has no associated model.\n", GetClassname(), GetDebugName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRestockZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( pOther );
		if ( pPlayer )
		{
			if ( pPlayer->GetNextRegenTime() > gpGlobals->curtime )
				return;

			int iTeam = GetTeamNumber();

			if ( TDCGameRules()->State_Get() != GR_STATE_TEAM_WIN )
			{
				if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
					return;
			}
			else
			{
				// no health for the losing team, but all zones work for the winning team
				if ( TDCGameRules()->GetWinningTeam() != pPlayer->GetTeamNumber() )
					return;
			}

			if ( TDCGameRules()->InStalemate() )
				return;

			Regenerate( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRestockZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRestockZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CRestockZone::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRestockZone::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRestockZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRestockZone::Regenerate( CTDCPlayer *pPlayer )
{
	pPlayer->Restock( m_bRestoreHealth, m_bRestoreAmmo );
	pPlayer->SetNextRegenTime( gpGlobals->curtime + TDC_REGENERATE_NEXT_USE_TIME );

	CSingleUserRecipientFilter filter( pPlayer );
	EmitSound( filter, pPlayer->entindex(), TDC_REGENERATE_SOUND );

	if ( m_hAssociatedModel )
	{
		variant_t tmpVar;
		tmpVar.SetString( MAKE_STRING( "open" ) );
		m_hAssociatedModel->AcceptInput( "SetAnimation", this, this, tmpVar, 0 );

		tmpVar.SetString( MAKE_STRING( "close" ) );
		g_EventQueue.AddEvent( m_hAssociatedModel, "SetAnimation", tmpVar, TDC_REGENERATE_NEXT_USE_TIME - 1.0, this, this );
	}
}
