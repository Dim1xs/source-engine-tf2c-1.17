//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tdc_powerup_custom.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================

BEGIN_DATADESC( CTDCPowerupCustom )
	DEFINE_KEYFIELD( m_iPowerupCondition, FIELD_INTEGER, "PowerupCondition" ),
	DEFINE_KEYFIELD( m_flEffectDuration, FIELD_FLOAT, "EffectDuration" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_powerup_custom, CTDCPowerupCustom );

//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor 
//-----------------------------------------------------------------------------
CTDCPowerupCustom::CTDCPowerupCustom()
{	
	m_iPowerupCondition = TDC_COND_POWERUP_CRITDAMAGE;
}
