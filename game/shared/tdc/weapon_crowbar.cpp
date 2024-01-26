//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_crowbar.h"
#include "tdc_lagcompensation.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tdc_player.h"
// Client specific.
#else
#include "c_tdc_player.h"
#endif

//=============================================================================
//
// Weapon Crowbar tables.
//
//CREATE_SIMPLE_WEAPON_TABLE( WeaponCrowbar, weapon_crowbar )

CREATE_SIMPLE_WEAPON_TABLE( WeaponCrowbar, weapon_crowbar)
CREATE_SIMPLE_WEAPON_TABLE( WeaponTireIron, weapon_tireiron )
CREATE_SIMPLE_WEAPON_TABLE( WeaponLeadPipe, weapon_leadpipe )
CREATE_SIMPLE_WEAPON_TABLE( WeaponUmbrella, weapon_umbrella )

acttable_t CWeaponCrowbar::m_acttable[] =
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

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_CROWBAR,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_CROWBAR,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_CROWBAR,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_CROWBAR,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponCrowbar );
