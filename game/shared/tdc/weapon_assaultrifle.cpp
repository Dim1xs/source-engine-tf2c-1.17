//=============================================================================
//
// Purpose: Burst Rifle
//
//=============================================================================
#include "cbase.h"
#include "weapon_assaultrifle.h"
#include "in_buttons.h"

//=============================================================================
//
// Weapon Assault Rifle tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAssaultRifle, DT_TDCAssaultRifle );
BEGIN_NETWORK_TABLE( CWeaponAssaultRifle, DT_TDCAssaultRifle )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iBurstSize ) ),
#else
	SendPropInt( SENDINFO( m_iBurstSize ), 8, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAssaultRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iBurstSize, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_assaultrifle, CWeaponAssaultRifle );
PRECACHE_WEAPON_REGISTER( weapon_assaultrifle );

acttable_t CWeaponAssaultRifle::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_ASSAULTRIFLE,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_ASSAULTRIFLE,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_ASSAULTRIFLE,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_ASSAULTRIFLE,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_ASSAULTRIFLE,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_ASSAULTRIFLE,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_ASSAULTRIFLE,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_ASSAULTRIFLE,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_ASSAULTRIFLE,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_ASSAULTRIFLE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_ASSAULTRIFLE,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_ASSAULTRIFLE,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_ASSAULTRIFLE,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_ASSAULTRIFLE,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_ASSAULTRIFLE,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_ASSAULTRIFLE,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_ASSAULTRIFLE,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponAssaultRifle );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponAssaultRifle::ItemPostFrame( void )
{
	CBasePlayer *pOwner = GetPlayerOwner();
	if ( pOwner )
	{
		if ( m_iBurstSize > 0 )
		{
			// Fake the fire button.
			pOwner->m_nButtons |= IN_ATTACK;
		}
	}

	int iOldBurstSize = m_iBurstSize;

	BaseClass::ItemPostFrame();

	if ( m_iBurstSize > 0 )
	{
		// Stop burst if we run out of ammo.
		if ( !HasPrimaryAmmoToFire() )
		{
			m_iBurstSize = 0;
		}
	}
	else if ( iOldBurstSize > 0 )
	{
		// Delay the next burst.
		m_flNextPrimaryAttack = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flBurstDelay;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponAssaultRifle::FireProjectile()
{
	if ( m_iBurstSize == 0 )
	{
		// Start the burst.
		m_iBurstSize = GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_nBurstSize;
	}

	if ( m_iBurstSize > 0 )
	{
		m_iBurstSize--;
	}

	BaseClass::FireProjectile();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponAssaultRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// Stop the burst.
	m_iBurstSize = 0;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponAssaultRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();

	// Stop the burst.
	m_iBurstSize = 0;
}
