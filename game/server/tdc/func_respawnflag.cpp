//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTDC Respawnflag.
//
//=============================================================================//

#include "cbase.h"
#include "tdc_player.h"
#include "tdc_item.h"
#include "entity_capture_flag.h"
#include "func_respawnflag.h"

LINK_ENTITY_TO_CLASS( func_respawnflag, CFuncRespawnFlagZone );

//=============================================================================
//
// CTDC Respawnflag tables.
//

BEGIN_DATADESC(CFuncRespawnFlagZone)
	DEFINE_FUNCTION(Touch),
END_DATADESC();

//=============================================================================
//
// CTDC Respawnflag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool PointInRespawnFlagZone( const Vector &vecFlagOrigin )
{
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassname(pEntity, "func_respawnflag")) != NULL )
	{
		CFuncRespawnFlagZone *v3 = (CFuncRespawnFlagZone *)pEntity;

		if ( !v3->IsDisabled() && v3->PointIsWithin(vecFlagOrigin) )
		{
			return true;	
		}
	}
	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncRespawnFlagZone::CFuncRespawnFlagZone()
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the entity
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Spawn(void)
{
	Precache();
	InitTrigger();
	SetTouch( &CFuncRespawnFlagZone::Touch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Activate(void)
{
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Touch( CBaseEntity *pOther )
{
	if ( !IsDisabled() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( pOther );
		if ( pPlayer )
		{
			CCaptureFlag *pFlag = pPlayer->GetTheFlag();
			if ( pFlag )
			{
				pPlayer->DropFlag();
				pFlag->Reset();
				pFlag->ResetMessage();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::InputEnable(inputdata_t &inputdata)
{
	SetDisabled(false);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::InputDisable(inputdata_t &inputdata)
{
	SetDisabled(true);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncRespawnFlagZone::IsDisabled(void)
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::InputToggle(inputdata_t &inputdata)
{
	if (m_bDisabled)
	{
		SetDisabled(false);
	}
	else
	{
		SetDisabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::SetDisabled(bool bDisabled)
{
	m_bDisabled = bDisabled;
}