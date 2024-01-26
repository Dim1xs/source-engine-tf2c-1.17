//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "weapon_nailgun.h"
#include "tdc_projectile_nail.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#else
#include "c_tdc_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( WeaponNailGun, weapon_nailgun );

acttable_t CWeaponNailGun::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_NAILGUN,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_NAILGUN,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_NAILGUN,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_NAILGUN,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_NAILGUN,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_NAILGUN,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_NAILGUN,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_NAILGUN,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_NAILGUN,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_NAILGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_NAILGUN,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_NAILGUN,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_NAILGUN,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_NAILGUN,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_NAILGUN,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_NAILGUN,		false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_NAILGUN,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponNailGun );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponNailGun::FireProjectile( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Play shoot sound.
	PlayWeaponShootSound();

	// Send animations.
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	Vector vecSrc;
	Vector vecDir;
	GetProjectileFireSetup( pPlayer, Vector( 16, 6, -8 ), vecSrc, vecDir );

	Vector vecVelocity = vecDir * GetProjectileSpeed();
	float flDamage = GetProjectileDamage();
	bool bCritical = IsCurrentAttackACrit();

	CProjectile_Syringe::Create( vecSrc, vecVelocity, TDC_NAIL_NORMAL, flDamage, bCritical, pPlayer, this );

	// Subtract ammo
	int iAmmoCost = GetAmmoPerShot();
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 -= iAmmoCost;
	}
	else
	{
		pPlayer->RemoveAmmo( iAmmoCost, m_iPrimaryAmmoType );
	}

	// Do visual effects.
	DoMuzzleFlash();
	AddViewKick();
}
