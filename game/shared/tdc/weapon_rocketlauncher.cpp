//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "weapon_rocketlauncher.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#include "tdc_projectile_rocket.h"
#else
#include "c_tdc_player.h"
#endif

//=============================================================================
//
// Weapon Rocket Launcher tables.
//
CREATE_SIMPLE_WEAPON_TABLE( WeaponRocketLauncher, weapon_rocketlauncher );

acttable_t CWeaponRocketLauncher::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,  ACT_MP_STAND_ROCKETLAUNCHER,      false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_ROCKETLAUNCHER,     false },
	{ ACT_MP_RUN,         ACT_MP_RUN_ROCKETLAUNCHER,        false },
	{ ACT_MP_AIRWALK,     ACT_MP_AIRWALK_ROCKETLAUNCHER,    false },
	{ ACT_MP_CROUCHWALK,  ACT_MP_CROUCHWALK_ROCKETLAUNCHER, false },
	{ ACT_MP_JUMP_START,  ACT_MP_JUMP_START_ROCKETLAUNCHER, false },
	{ ACT_MP_JUMP_FLOAT,  ACT_MP_JUMP_FLOAT_ROCKETLAUNCHER, false },
	{ ACT_MP_JUMP_LAND,   ACT_MP_JUMP_LAND_ROCKETLAUNCHER,  false },
	{ ACT_MP_SWIM,        ACT_MP_SWIM_ROCKETLAUNCHER,       false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,   ACT_MP_ATTACK_STAND_ROCKETLAUNCHER,   false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,  ACT_MP_ATTACK_CROUCH_ROCKETLAUNCHER,  false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,    ACT_MP_ATTACK_SWIM_ROCKETLAUNCHER,    false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_ROCKETLAUNCHER, false },

	{ ACT_MP_RELOAD_STAND,        ACT_MP_RELOAD_STAND_ROCKETLAUNCHER,        false },
	{ ACT_MP_RELOAD_STAND_LOOP,   ACT_MP_RELOAD_STAND_ROCKETLAUNCHER_LOOP,   false },
	{ ACT_MP_RELOAD_STAND_END,    ACT_MP_RELOAD_STAND_ROCKETLAUNCHER_END,    false },
	{ ACT_MP_RELOAD_CROUCH,       ACT_MP_RELOAD_CROUCH_ROCKETLAUNCHER,       false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,  ACT_MP_RELOAD_CROUCH_ROCKETLAUNCHER_LOOP,  false },
	{ ACT_MP_RELOAD_CROUCH_END,   ACT_MP_RELOAD_CROUCH_ROCKETLAUNCHER_END,   false },
	{ ACT_MP_RELOAD_SWIM,         ACT_MP_RELOAD_SWIM_ROCKETLAUNCHER,         false },
	{ ACT_MP_RELOAD_SWIM_LOOP,    ACT_MP_RELOAD_SWIM_ROCKETLAUNCHER_LOOP,    false },
	{ ACT_MP_RELOAD_SWIM_END,     ACT_MP_RELOAD_SWIM_ROCKETLAUNCHER_END,     false },
	{ ACT_MP_RELOAD_AIRWALK,      ACT_MP_RELOAD_AIRWALK_ROCKETLAUNCHER,      false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_ROCKETLAUNCHER_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END,  ACT_MP_RELOAD_AIRWALK_ROCKETLAUNCHER_END,  false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponRocketLauncher );

CWeaponRocketLauncher::CWeaponRocketLauncher()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRocketLauncher::FireProjectile( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Play shoot sound.
	PlayWeaponShootSound();

	// Send animations.
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	// Server only - create the rocket.
#ifdef GAME_DLL

	Vector vecSrc;
	Vector vecDir;
	GetProjectileFireSetup( pPlayer, Vector( 23.5f, 0.0f, -3.0f ), vecSrc, vecDir, false );

	Vector vecVelocity = vecDir * GetProjectileSpeed();
	float flDamage = GetProjectileDamage();
	float flRadius = GetTDCWpnData().m_flDamageRadius;
	bool bCritical = IsCurrentAttackACrit();

	CProjectile_Rocket::Create( vecSrc, vecVelocity, flDamage, flRadius, bCritical, pPlayer, this );

#endif

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
