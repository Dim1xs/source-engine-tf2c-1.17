//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_stengun.h"

CREATE_SIMPLE_WEAPON_TABLE( WeaponStenGun, weapon_stengun );

acttable_t CWeaponStenGun::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_STEN,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_STEN,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_STEN,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_STEN,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_STEN,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_STEN,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_STEN,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_STEN,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_STEN,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_STEN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_STEN,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_STEN,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_STEN,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_STEN,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_STEN,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_STEN,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_STEN,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponStenGun );
