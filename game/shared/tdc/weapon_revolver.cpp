//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_revolver.h"

CREATE_SIMPLE_WEAPON_TABLE( WeaponRevolver, weapon_revolver );

// TEMP!!! Copy-paste of Secondary 2 act table.
acttable_t CWeaponRevolver::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_REVOLVER,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_REVOLVER,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_REVOLVER,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_REVOLVER,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_REVOLVER,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_REVOLVER,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_REVOLVER,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_REVOLVER,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_REVOLVER,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_REVOLVER,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_REVOLVER,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_REVOLVER,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_REVOLVER,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_REVOLVER,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_REVOLVER,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_REVOLVER,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_REVOLVER,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponRevolver );
