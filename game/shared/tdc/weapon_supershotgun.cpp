//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_supershotgun.h"

CREATE_SIMPLE_WEAPON_TABLE( WeaponSuperShotgun, weapon_supershotgun );

CWeaponSuperShotgun::CWeaponSuperShotgun()
{
	m_bReloadsSingly = false;
}

acttable_t CWeaponSuperShotgun::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_SUPERSHOTGUN,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_SUPERSHOTGUN,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_SUPERSHOTGUN,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_SUPERSHOTGUN,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_SUPERSHOTGUN,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_SUPERSHOTGUN,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_SUPERSHOTGUN,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_SUPERSHOTGUN,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_SUPERSHOTGUN,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_SUPERSHOTGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_SUPERSHOTGUN,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_SUPERSHOTGUN,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_SUPERSHOTGUN,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_SUPERSHOTGUN,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_SUPERSHOTGUN,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_SUPERSHOTGUN,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_SUPERSHOTGUN,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponSuperShotgun );

//-----------------------------------------------------------------------------
// Purpose: Spread pattern for tf_use_fixed_weaponspreads.
//-----------------------------------------------------------------------------
const Vector2D *CWeaponSuperShotgun::GetSpreadPattern( int &iNumBullets )
{
	static Vector2D vecFixedSuperShotgunSpread[] =
	{
		Vector2D( 0.2, 0 ),
		Vector2D( 0, 0.2 ),
		Vector2D( -0.2, 0 ),
		Vector2D( 0, -0.2 ),
		Vector2D( 0.5, 0 ),
		Vector2D( 0, 0.5 ),
		Vector2D( -0.5, 0 ),
		Vector2D( 0, -0.5 ),
		Vector2D( 0.45, 0.45 ),
		Vector2D( -0.45, 0.45 ),
		Vector2D( 0.45, -0.45 ),
		Vector2D( -0.45, -0.45 ),
		Vector2D( 0.4, 0.2 ),
		Vector2D( 0.2, 0.4 ),
		Vector2D( -0.4, 0.2 ),
		Vector2D( -0.2, 0.4 ),
		Vector2D( 0.4, -0.2 ),
		Vector2D( 0.2, -0.4 ),
		Vector2D( -0.4, -0.2 ),
		Vector2D( -0.2, -0.4 ),
	};

	iNumBullets = ARRAYSIZE( vecFixedSuperShotgunSpread );
	return vecFixedSuperShotgunSpread;
}