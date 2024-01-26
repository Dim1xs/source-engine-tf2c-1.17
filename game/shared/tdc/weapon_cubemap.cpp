//=============================================================================
//
// Purpose: "Weapon" for testing cubemaps
//
//=============================================================================
#include "cbase.h"
#include "weapon_cubemap.h"

CREATE_SIMPLE_WEAPON_TABLE( WeaponCubemap, weapon_cubemap );

acttable_t CWeaponCubemap::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_CROWBAR,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_CROWBAR,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_CROWBAR,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_CROWBAR,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_CROWBAR,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_CROWBAR,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_CROWBAR,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_CROWBAR,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_CROWBAR,		false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponCubemap );
