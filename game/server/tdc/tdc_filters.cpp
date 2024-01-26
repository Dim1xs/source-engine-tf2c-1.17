//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "filters.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Team Fortress Team Filter
//
class CFilterTFTeam : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterTFTeam, CBaseFilter );

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );
};

LINK_ENTITY_TO_CLASS( filter_player_team, CFilterTFTeam );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFilterTFTeam::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	// is the entity we're asking about on the winning 
	// team during the bonus time? (winners pass all filters)
	if (  TDCGameRules() &&
		( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && 
		( TDCGameRules()->GetWinningTeam() == pEntity->GetTeamNumber() ) )
	{
		// this should open all doors for the winners
		if ( m_bNegated )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return ( pEntity->GetTeamNumber() == GetTeamNumber() );
}

class FilterDamagedByWeaponInSlot : public CBaseFilter
{
public:
	DECLARE_CLASS( FilterDamagedByWeaponInSlot, CBaseFilter );
	DECLARE_DATADESC();

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		return true;
	}

	bool PassesDamageFilterImpl( const CTakeDamageInfo &info )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( info.GetAttacker() );
		if ( !pPlayer )
			return false;

		CTDCWeaponBase *pWeapon = dynamic_cast<CTDCWeaponBase *>( info.GetWeapon() );
		if ( !pWeapon )
			return false;

		return ( pWeapon->GetSlot() == m_iSlot );
	}

private:
	int m_iSlot;
};

BEGIN_DATADESC( FilterDamagedByWeaponInSlot )
DEFINE_KEYFIELD( m_iSlot, FIELD_INTEGER, "weaponSlot" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_damage_weaponslot, FilterDamagedByWeaponInSlot );

//=============================================================================
//
// Class filter
//
class CFilterTDCClass : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterTDCClass, CBaseFilter );
	DECLARE_DATADESC();

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

private:
	int	m_iAllowedClass;
};

BEGIN_DATADESC( CFilterTDCClass )
DEFINE_KEYFIELD( m_iAllowedClass, FIELD_INTEGER, "tdcclass" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_player_class, CFilterTDCClass );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFilterTDCClass::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );
	if ( !pPlayer )
		return false;

	return pPlayer->IsPlayerClass( m_iAllowedClass );
}
