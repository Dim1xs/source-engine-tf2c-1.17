//=============================================================================
//
// Purpose:
//
//=============================================================================
#include "cbase.h"
#include "weapon_huntingshotgun.h"

CREATE_SIMPLE_WEAPON_TABLE( WeaponHuntingShotgun, weapon_huntingshotgun );

acttable_t CWeaponHuntingShotgun::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,  ACT_MP_STAND_HUNTINGSHOTGUN,      false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_HUNTINGSHOTGUN,     false },
	{ ACT_MP_RUN,         ACT_MP_RUN_HUNTINGSHOTGUN,        false },
	{ ACT_MP_AIRWALK,     ACT_MP_AIRWALK_HUNTINGSHOTGUN,    false },
	{ ACT_MP_CROUCHWALK,  ACT_MP_CROUCHWALK_HUNTINGSHOTGUN, false },
	{ ACT_MP_JUMP_START,  ACT_MP_JUMP_START_HUNTINGSHOTGUN, false },
	{ ACT_MP_JUMP_FLOAT,  ACT_MP_JUMP_FLOAT_HUNTINGSHOTGUN, false },
	{ ACT_MP_JUMP_LAND,   ACT_MP_JUMP_LAND_HUNTINGSHOTGUN,  false },
	{ ACT_MP_SWIM,        ACT_MP_SWIM_HUNTINGSHOTGUN,       false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,   ACT_MP_ATTACK_STAND_HUNTINGSHOTGUN,   false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,  ACT_MP_ATTACK_CROUCH_HUNTINGSHOTGUN,  false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,    ACT_MP_ATTACK_SWIM_HUNTINGSHOTGUN,    false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_HUNTINGSHOTGUN, false },

	{ ACT_MP_RELOAD_STAND,        ACT_MP_RELOAD_STAND_HUNTINGSHOTGUN,        false },
	{ ACT_MP_RELOAD_STAND_LOOP,   ACT_MP_RELOAD_STAND_HUNTINGSHOTGUN_LOOP,   false },
	{ ACT_MP_RELOAD_STAND_END,    ACT_MP_RELOAD_STAND_HUNTINGSHOTGUN_END,    false },
	{ ACT_MP_RELOAD_CROUCH,       ACT_MP_RELOAD_CROUCH_HUNTINGSHOTGUN,       false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,  ACT_MP_RELOAD_CROUCH_HUNTINGSHOTGUN_LOOP,  false },
	{ ACT_MP_RELOAD_CROUCH_END,   ACT_MP_RELOAD_CROUCH_HUNTINGSHOTGUN_END,   false },
	{ ACT_MP_RELOAD_SWIM,         ACT_MP_RELOAD_SWIM_HUNTINGSHOTGUN,         false },
	{ ACT_MP_RELOAD_SWIM_LOOP,    ACT_MP_RELOAD_SWIM_HUNTINGSHOTGUN_LOOP,    false },
	{ ACT_MP_RELOAD_SWIM_END,     ACT_MP_RELOAD_SWIM_HUNTINGSHOTGUN_END,     false },
	{ ACT_MP_RELOAD_AIRWALK,      ACT_MP_RELOAD_AIRWALK_HUNTINGSHOTGUN,      false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_HUNTINGSHOTGUN_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END,  ACT_MP_RELOAD_AIRWALK_HUNTINGSHOTGUN_END,  false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponHuntingShotgun );

CWeaponHuntingShotgun::CWeaponHuntingShotgun()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spread pattern for tf_use_fixed_weaponspreads.
//-----------------------------------------------------------------------------
const Vector2D *CWeaponHuntingShotgun::GetSpreadPattern( int &iNumBullets )
{
	static Vector2D vecFixedHuntingShotgunSpread[] =
	{
		Vector2D( 0.0, 0.5 ),
		Vector2D( -0.25, 0.325 ),
		Vector2D( 0.25, 0.325 ),
		Vector2D( -0.5, 0.2 ),
		Vector2D( 0.0, 0.2 ),
		Vector2D( 0.0, 0.2 ),
		Vector2D( 0.5, 0.2 ),
		Vector2D( -0.25, 0.0 ),
		Vector2D( 0.25, 0.0 ),
		Vector2D( -0.5, -0.2 ),
		Vector2D( 0.0, -0.2 ),
		Vector2D( 0.0, -0.2 ),
		Vector2D( 0.5, -0.2 ),
		Vector2D( -0.25, -0.325 ),
		Vector2D( 0.25, -0.325 ),
		Vector2D( 0.0, -0.5 ),
	};

	iNumBullets = ARRAYSIZE( vecFixedHuntingShotgunSpread );
	return vecFixedHuntingShotgunSpread;
}
