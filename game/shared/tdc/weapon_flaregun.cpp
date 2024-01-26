//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_flaregun.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#include "tdc_projectile_flare.h"
#else
#include "c_tdc_player.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( WeaponFlareGun, weapon_flaregun );

acttable_t CWeaponFlareGun::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_FLAREGUN,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_FLAREGUN,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_FLAREGUN,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_FLAREGUN,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_FLAREGUN,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_FLAREGUN,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_FLAREGUN,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_FLAREGUN,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_FLAREGUN,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_FLAREGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_FLAREGUN,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_FLAREGUN,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_FLAREGUN,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponFlareGun );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponFlareGun::FireProjectile( void )
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
	bool bCritical = IsCurrentAttackACrit();
	float flRadius = GetTDCWpnData().m_flDamageRadius;

	CProjectile_Flare::Create( vecSrc, vecVelocity, flDamage, flRadius, bCritical, pPlayer, this );

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
