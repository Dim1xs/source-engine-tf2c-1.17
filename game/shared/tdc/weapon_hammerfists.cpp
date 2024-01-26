//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_hammerfists.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#else
#include "c_tdc_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( WeaponHammerFists, weapon_hammerfists );

acttable_t CWeaponHammerFists::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_HAMMERFISTS,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_HAMMERFISTS,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_HAMMERFISTS,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_HAMMERFISTS,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_HAMMERFISTS,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_HAMMERFISTS,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_HAMMERFISTS,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_HAMMERFISTS,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_HAMMERFISTS,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_HAMMERFISTS,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_HAMMERFISTS,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_HAMMERFISTS,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_HAMMERFISTS,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponHammerFists );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponHammerFists::CanHolster( void ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
	{
		return false;
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponHammerFists::ForceWeaponSwitch( void ) const
{
	if ( !CanHolster() )
		return true;

	return BaseClass::ForceWeaponSwitch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CWeaponHammerFists::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	// reversed for 360 because the primary attack is on the right side of the controller
	if ( IsX360() )
	{
		m_iWeaponMode = TDC_WEAPON_SECONDARY_MODE;
	}
	else
	{
		m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	}

	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CWeaponHammerFists::SecondaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	if ( IsX360() )
	{
		m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	}
	else
	{
		m_iWeaponMode = TDC_WEAPON_SECONDARY_MODE;
	}

	Punch();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CWeaponHammerFists::Punch( void )
{
	// Get the current player.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Swing the weapon.
	Swing( pPlayer );

	m_flNextSecondaryAttack = m_flNextPrimaryAttack;

#if !defined( CLIENT_DLL ) 
	pPlayer->NoteWeaponFired( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CWeaponHammerFists::SendPlayerAnimEvent( CTDCPlayer *pPlayer )
{
	if ( IsCurrentAttackACrit() )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHammerFists::DoViewModelAnimation( void )
{
	Activity act;

	if ( IsCurrentAttackACrit() )
	{
		act = ACT_VM_SWINGHARD;
	}
	else
	{
		act = ( m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITLEFT : ACT_VM_HITRIGHT;
	}

	SendWeaponAnim( act );
}
