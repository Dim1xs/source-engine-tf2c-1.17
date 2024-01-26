//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_shotgun.h"

#ifdef CLIENT_DLL
#include "functionproxy.h"
#include "c_tdc_player.h"
#endif

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( WeaponShotgun, weapon_shotgun )

acttable_t CWeaponShotgun::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,  ACT_MP_STAND_SHOTGUN,      false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_SHOTGUN,     false },
	{ ACT_MP_RUN,         ACT_MP_RUN_SHOTGUN,        false },
	{ ACT_MP_AIRWALK,     ACT_MP_AIRWALK_SHOTGUN,    false },
	{ ACT_MP_CROUCHWALK,  ACT_MP_CROUCHWALK_SHOTGUN, false },
	{ ACT_MP_JUMP_START,  ACT_MP_JUMP_START_SHOTGUN, false },
	{ ACT_MP_JUMP_FLOAT,  ACT_MP_JUMP_FLOAT_SHOTGUN, false },
	{ ACT_MP_JUMP_LAND,   ACT_MP_JUMP_LAND_SHOTGUN,  false },
	{ ACT_MP_SWIM,        ACT_MP_SWIM_SHOTGUN,       false },
	{ ACT_MP_SLIDE,		  ACT_MP_SLIDE_SHOTGUN,		 false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,   ACT_MP_ATTACK_STAND_SHOTGUN,   false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,  ACT_MP_ATTACK_CROUCH_SHOTGUN,  false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,    ACT_MP_ATTACK_SWIM_SHOTGUN,    false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_SHOTGUN, false },
	{ ACT_MP_ATTACK_SLIDE_PRIMARYFIRE,   ACT_MP_ATTACK_SLIDE_SHOTGUN,	false },

	{ ACT_MP_RELOAD_STAND,        ACT_MP_RELOAD_STAND_SHOTGUN,        false },
	{ ACT_MP_RELOAD_STAND_LOOP,   ACT_MP_RELOAD_STAND_SHOTGUN_LOOP,   false },
	{ ACT_MP_RELOAD_STAND_END,    ACT_MP_RELOAD_STAND_SHOTGUN_END,    false },
	{ ACT_MP_RELOAD_CROUCH,       ACT_MP_RELOAD_CROUCH_SHOTGUN,       false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,  ACT_MP_RELOAD_CROUCH_SHOTGUN_LOOP,  false },
	{ ACT_MP_RELOAD_CROUCH_END,   ACT_MP_RELOAD_CROUCH_SHOTGUN_END,   false },
	{ ACT_MP_RELOAD_SWIM,         ACT_MP_RELOAD_SWIM_SHOTGUN,         false },
	{ ACT_MP_RELOAD_SWIM_LOOP,    ACT_MP_RELOAD_SWIM_SHOTGUN_LOOP,    false },
	{ ACT_MP_RELOAD_SWIM_END,     ACT_MP_RELOAD_SWIM_SHOTGUN_END,     false },
	{ ACT_MP_RELOAD_AIRWALK,      ACT_MP_RELOAD_AIRWALK_SHOTGUN,      false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_SHOTGUN_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END,  ACT_MP_RELOAD_AIRWALK_SHOTGUN_END,  false },
	{ ACT_MP_RELOAD_SLIDE,		  ACT_MP_RELOAD_SLIDE_SHOTGUN,		  false },
	{ ACT_MP_RELOAD_SLIDE_LOOP,	  ACT_MP_RELOAD_SLIDE_SHOTGUN_LOOP,	  false },
	{ ACT_MP_RELOAD_SLIDE_END,    ACT_MP_RELOAD_SLIDE_SHOTGUN_END,	  false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponShotgun );

//=============================================================================
//
// Weapon Shotgun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CWeaponShotgun::CWeaponShotgun()
{
	m_bReloadsSingly = true;
}
