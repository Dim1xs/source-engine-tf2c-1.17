//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_claws.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tdc_player.h"
// Server specific.
#else
#include "tdc_player.h"
#endif

ConVar tdc_claws_pounce_forward_speed( "tdc_claws_pounce_forward_speed", "1000", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponClaws, DT_Claws );
BEGIN_NETWORK_TABLE( CWeaponClaws, DT_Claws )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponClaws )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_claws, CWeaponClaws );
PRECACHE_WEAPON_REGISTER( weapon_claws );

acttable_t CWeaponClaws::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_CLAWS,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_CLAWS,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_CLAWS,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_CLAWS,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_CLAWS,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_CLAWS,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_CLAWS,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_CLAWS,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_CLAWS,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_CLAWS,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_CLAWS,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_CLAWS,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_CLAWS,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponClaws );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponClaws::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef GAME_DLL
	StopCharging();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CWeaponClaws::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopCharging();

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponClaws::ItemPostFrame( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer )
	{
		if ( m_flChargeBeginTime != 0.0f )
		{
			// Stop charging if player released the button.
			if ( !( pPlayer->m_nButtons & IN_ATTACK2 ) || !CanAttack() )
			{
				StopCharging();
			}
			// If player is not ducking or ducked abort the pounce charging.
			else if ( pPlayer->m_Local.m_bDucked )
			{
				if ( pPlayer->m_Local.m_bDucking )
					StopCharging();
			}
			else
			{
				if ( !pPlayer->m_Local.m_bDucking )
					StopCharging();
			}
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponClaws::PrimaryAttack( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	if ( pPlayer && m_flChargeBeginTime != 0.0f )
	{
		// Pounce if fully charged.
		if ( gpGlobals->curtime - m_flChargeBeginTime >= 1.0f && pPlayer->GetGroundEntity() != NULL )
		{
			StopCharging();
			Pounce();
		}
		return;
	}

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponClaws::SecondaryAttack( void )
{
	// Not during freeze time.
	if ( !CanAttack() )
		return;

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	// Not in the water.
	if ( pPlayer->GetWaterLevel() >= WL_Waist )
		return;

	// Not in mid-air.
	if ( pPlayer->GetGroundEntity() == NULL )
		return;

	if ( m_flChargeBeginTime != 0.0f )
		return;

	// Play the pounce charge sound.
	WeaponSound( SPECIAL2 );
	m_flChargeBeginTime = gpGlobals->curtime;
	pPlayer->m_Shared.AddCond( TDC_COND_CHARGING_POUNCE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponClaws::Pounce( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	// Play the pounce sound.
	WeaponSound( SPECIAL1 );

	// Start jump animation and player sound.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
	pPlayer->PlayStepSound( (Vector &)pPlayer->GetAbsOrigin(), pPlayer->GetSurfaceData(), 1.0, true );
	pPlayer->m_Shared.SetJumping( true );

	// In the air now.
	pPlayer->SetGroundEntity( NULL );

	// Get the jump angle.
	Vector vecForward;
	QAngle angForward = pPlayer->EyeAngles();
	angForward[PITCH] = Min( -20.0f, angForward[PITCH] );
	AngleVectors( angForward, &vecForward );

	// Update the velocity.
	Vector vecPounceVel = vecForward * tdc_claws_pounce_forward_speed.GetFloat();
	pPlayer->SetAbsVelocity( vecPounceVel );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponClaws::StopCharging( void )
{
	m_flChargeBeginTime = 0.0f;

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TDC_COND_CHARGING_POUNCE );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponClaws::GetEffectBarProgress( void )
{
	if ( m_flChargeBeginTime == 0.0f )
		return 0.0f;

	return clamp( gpGlobals->curtime - m_flChargeBeginTime, 0.0f, 1.0f );
}
