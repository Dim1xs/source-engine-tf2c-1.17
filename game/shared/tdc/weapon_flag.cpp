//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_flag.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tdc_player.h"
// Server specific.
#else
#include "tdc_player.h"
#endif

//=============================================================================
//
// Weapon Flag tables.
//
CREATE_SIMPLE_WEAPON_TABLE( WeaponFlag, weapon_flag );

acttable_t CWeaponFlag::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_FLAG,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_FLAG,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_FLAG,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_FLAG,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_FLAG,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_FLAG,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_FLAG,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_FLAG,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_FLAG,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_FLAG,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_FLAG,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_FLAG,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_FLAG,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponFlag );

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CWeaponFlag::CWeaponFlag()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFlag::CanHolster( void ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	if ( pPlayer && pPlayer->GetTheFlag() && !m_bDropping )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFlag::SecondaryAttack( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	m_bDropping = true;
	pPlayer->SwitchToNextBestWeapon( NULL );

#ifdef GAME_DLL
	pPlayer->DropFlag();
#endif
	m_bDropping = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CWeaponFlag::GetViewModel( int iViewModel /*= 0*/ ) const
{
	// Get the skin of the carried flag.
	CCaptureFlag *pFlag = GetOwnerFlag();
	if ( pFlag )
	{
		return pFlag->GetViewModel();
	}

	return BaseClass::GetViewModel( iViewModel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureFlag *CWeaponFlag::GetOwnerFlag( void ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return NULL;

	return pPlayer->GetTheFlag();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CWeaponFlag::GetSkin( void )
{
	// Get the skin of the carried flag.
	CCaptureFlag *pFlag = GetOwnerFlag();
	if ( !pFlag )
		return 0;

	switch ( pFlag->GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		return 0;
	case TDC_TEAM_BLUE:
		return 1;
	default:
		return 2;
	}
}
#endif
