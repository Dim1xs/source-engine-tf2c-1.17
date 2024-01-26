//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "weapon_pistol.h"

//=============================================================================
//
// Weapon Pistol tables.
//
CREATE_SIMPLE_WEAPON_TABLE( WeaponPistol, weapon_pistol );

acttable_t CWeaponPistol::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_PISTOL,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_PISTOL,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_PISTOL,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_PISTOL,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_PISTOL,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_PISTOL,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_PISTOL,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_PISTOL,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PISTOL,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PISTOL,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PISTOL,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PISTOL,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PISTOL,		false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PISTOL,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PISTOL,		false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PISTOL,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponPistol );
