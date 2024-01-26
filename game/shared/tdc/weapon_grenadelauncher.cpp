//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_grenadelauncher.h"

#ifdef GAME_DLL
#include "tdc_projectile_pipebomb.h"
#include "tdc_player.h"
#else
#include "c_tdc_player.h"
#include "physpropclientside.h"
#include "gamestringpool.h"
#endif


//=============================================================================
//
// Weapon Grenade Launcher tables.
//
CREATE_SIMPLE_WEAPON_TABLE( WeaponGrenadeLauncher, weapon_grenadelauncher );

acttable_t CWeaponGrenadeLauncher::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,  ACT_MP_STAND_GRENADELAUNCHER,      false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_GRENADELAUNCHER,     false },
	{ ACT_MP_RUN,         ACT_MP_RUN_GRENADELAUNCHER,        false },
	{ ACT_MP_AIRWALK,     ACT_MP_AIRWALK_GRENADELAUNCHER,    false },
	{ ACT_MP_CROUCHWALK,  ACT_MP_CROUCHWALK_GRENADELAUNCHER, false },
	{ ACT_MP_JUMP_START,  ACT_MP_JUMP_START_GRENADELAUNCHER, false },
	{ ACT_MP_JUMP_FLOAT,  ACT_MP_JUMP_FLOAT_GRENADELAUNCHER, false },
	{ ACT_MP_JUMP_LAND,   ACT_MP_JUMP_LAND_GRENADELAUNCHER,  false },
	{ ACT_MP_SWIM,        ACT_MP_SWIM_GRENADELAUNCHER,       false },
	{ ACT_MP_SLIDE,		  ACT_MP_SLIDE_GRENADELAUNCHER,		 false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,   ACT_MP_ATTACK_STAND_GRENADELAUNCHER,   false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,  ACT_MP_ATTACK_CROUCH_GRENADELAUNCHER,  false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,    ACT_MP_ATTACK_SWIM_GRENADELAUNCHER,    false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_GRENADELAUNCHER, false },
	{ ACT_MP_ATTACK_SLIDE_PRIMARYFIRE,   ACT_MP_ATTACK_SLIDE_GRENADELAUNCHER,   false },

	{ ACT_MP_RELOAD_STAND,        ACT_MP_RELOAD_STAND_GRENADELAUNCHER,        false },
	{ ACT_MP_RELOAD_STAND_LOOP,   ACT_MP_RELOAD_STAND_GRENADELAUNCHER_LOOP,   false },
	{ ACT_MP_RELOAD_STAND_END,    ACT_MP_RELOAD_STAND_GRENADELAUNCHER_END,    false },
	{ ACT_MP_RELOAD_CROUCH,       ACT_MP_RELOAD_CROUCH_GRENADELAUNCHER,       false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,  ACT_MP_RELOAD_CROUCH_GRENADELAUNCHER_LOOP,  false },
	{ ACT_MP_RELOAD_CROUCH_END,   ACT_MP_RELOAD_CROUCH_GRENADELAUNCHER_END,   false },
	{ ACT_MP_RELOAD_SWIM,         ACT_MP_RELOAD_SWIM_GRENADELAUNCHER,         false },
	{ ACT_MP_RELOAD_SWIM_LOOP,    ACT_MP_RELOAD_SWIM_GRENADELAUNCHER_LOOP,    false },
	{ ACT_MP_RELOAD_SWIM_END,     ACT_MP_RELOAD_SWIM_GRENADELAUNCHER_END,     false },
	{ ACT_MP_RELOAD_AIRWALK,      ACT_MP_RELOAD_AIRWALK_GRENADELAUNCHER,      false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_GRENADELAUNCHER_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END,  ACT_MP_RELOAD_AIRWALK_GRENADELAUNCHER_END,  false },
	{ ACT_MP_RELOAD_SLIDE,		  ACT_MP_RELOAD_SLIDE_GRENADELAUNCHER,		  false },
	{ ACT_MP_RELOAD_SLIDE_LOOP,   ACT_MP_RELOAD_SLIDE_GRENADELAUNCHER_LOOP,   false },
	{ ACT_MP_RELOAD_SLIDE_END,	  ACT_MP_RELOAD_SLIDE_GRENADELAUNCHER_END,    false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponGrenadeLauncher );

CWeaponGrenadeLauncher::CWeaponGrenadeLauncher()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::FireProjectile( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Play shoot sound.
	PlayWeaponShootSound();

	// Send animations.
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	// Server only - create the pipe.
#ifdef GAME_DLL

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecSrc = pPlayer->EyePosition() + vecForward * 16.0f + vecUp * -8.0f;
	Vector vecVelocity = vecForward * GetProjectileSpeed() + vecUp * 200.0f;
	AngularImpulse angVelocity( 600, RandomInt( -1200, 1200 ), 0 );
	float flDamage = GetProjectileDamage();
	float flRadius = GetTDCWpnData().m_flDamageRadius;
	bool bCritical = IsCurrentAttackACrit();
	float flTimer = GetTDCWpnData().m_flPrimerTime;

	CProjectile_Pipebomb::Create( vecSrc, pPlayer->EyeAngles(),
		vecVelocity, angVelocity,
		flDamage, flRadius, bCritical, flTimer,
		pPlayer, this );

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


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrenadeLauncher::EjectBrass( C_BaseAnimating *pEntity, int iAttachment /*= -1*/ )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	const char *pszModel = GetTDCWpnData().m_szBrassModel;
	if ( !pszModel[0] )
		return;

	if ( iAttachment <= 0 )
	{
		iAttachment = m_iBrassAttachment;
	}

	Vector vecOrigin, vecPlayerVelocity;
	QAngle vecAngles;
	GetWeaponForEffect()->GetAttachment( iAttachment, vecOrigin, vecAngles );
	pPlayer->EstimateAbsVelocity( vecPlayerVelocity );

	Vector vForward, vRight, vUp;
	AngleVectors( vecAngles, &vForward, &vRight, &vUp );

	QAngle vecShellAngles;
	VectorAngles( -vUp, vecShellAngles );

	Vector vecVelocity = RandomFloat( 130, 180 ) * vForward +
		RandomFloat( -30, 30 ) * vRight +
		RandomFloat( -30, 30 ) * vUp +
		vecPlayerVelocity;

	C_PhysPropClientside *pShell = C_PhysPropClientside::CreateNew();
	if ( !pShell )
		return;

	pShell->SetModelName( AllocPooledString( pszModel ) );
	pShell->SetLocalOrigin( vecOrigin );
	pShell->SetLocalAngles( vecShellAngles );
	pShell->SetOwnerEntity( pPlayer );
	pShell->SetPhysicsMode( PHYSICS_MULTIPLAYER_CLIENTSIDE );
	pShell->SetCreateTime( gpGlobals->curtime );

	if ( !pShell->Initialize() )
	{
		pShell->Release();
		return;
	}

	pShell->m_iHealth = 1;
	pShell->SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	pShell->StartFadeOut( RandomFloat( 3.0f, 6.0f ) );

	IPhysicsObject *pPhysicsObject = pShell->VPhysicsGetObject();

	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &vecVelocity, &vec3_origin );
	}
}
#endif

