//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "tdc_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"
#include "tdc_viewmodel.h"

#ifdef CLIENT_DLL
#include "c_tdc_player.h"
#else
#include "tdc_player.h"
#endif

#define TDC_RUN_SPEED			320.0f
#define TDC_WALK_SPEED			75.0f
#define TDC_CROUCHWALK_SPEED		110.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CTDCPlayerAnimState* CreateTFPlayerAnimState( CTDCPlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = TDC_RUN_SPEED;
	movementData.m_flWalkSpeed = TDC_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CTDCPlayerAnimState *pRet = new CTDCPlayerAnimState( pPlayer, movementData );

	// Specific TF player initialization.
	pRet->InitTF( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTDCPlayerAnimState::CTDCPlayerAnimState()
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CTDCPlayerAnimState::CTDCPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pTFPlayer = NULL;

	// Don't initialize TF specific variables here. Init them in InitTF()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTDCPlayerAnimState::~CTDCPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Team Fortress specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTDCPlayerAnimState::InitTF( CTDCPlayer *pPlayer )
{
	m_pTFPlayer = pPlayer;
	m_bInAirWalk = false;
	m_flHoldDeployedPoseUntilTime = 0.0f;
	m_flTauntAnimTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerAnimState::ClearAnimationState( void )
{
	m_bInAirWalk = false;
	m_flLastAimTurnTime = 0.0f;

	BaseClass::ClearAnimationState();
}

acttable_t m_acttableLoserState[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_LOSERSTATE,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_LOSERSTATE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_LOSERSTATE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_LOSERSTATE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_LOSERSTATE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_LOSERSTATE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_LOSERSTATE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_LOSERSTATE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_LOSERSTATE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_LOSERSTATE,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_LOSERSTATE,				false },
	{ ACT_MP_DOUBLEJUMP,		ACT_MP_DOUBLEJUMP_LOSERSTATE,		false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_LOSERSTATE, false },
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CTDCPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	if ( GetTDCPlayer()->m_Shared.IsLoser() )
	{
		int actCount = ARRAYSIZE( m_acttableLoserState );
		for ( int i = 0; i < actCount; i++ )
		{
			const acttable_t& act = m_acttableLoserState[i];
			if ( actDesired == act.baseAct)
				return (Activity)act.weaponAct;
		}
	}

	CBaseCombatWeapon *pWeapon = GetTDCPlayer()->GetActiveWeapon();
	if ( pWeapon )
	{
		translateActivity = pWeapon->ActivityOverride( translateActivity, nullptr );
	}

	return translateActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Get the TF player.
	CTDCPlayer *pTFPlayer = GetTDCPlayer();
	if ( !pTFPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pTFPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		if ( m_pTFPlayer->m_Shared.IsMovementLocked() )
		{
			// Keep feet directed forward.
			m_bForceAimYaw = true;
		}

		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch( pStudioHdr );

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw( pStudioHdr );
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		GetBasePlayer()->SetPlaybackRate( 1.0f );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CTDCPlayerAnimState::CalcMainActivity( void )
{
	CheckStunAnimation();
	return BaseClass::CalcMainActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CTDCPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			CTDCPlayer *pPlayer = GetTDCPlayer();
			if ( !pPlayer )
				return;

			// Heavy weapons primary fire.
			if ( pPlayer->IsActiveTFWeapon( WEAPON_LEVERRIFLE ) && pPlayer->m_Shared.InCond( TDC_COND_ZOOMED ) )
			{
				// Weapon primary fire, zoomed in
				if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED );
				}

				iGestureActivity = ACT_VM_PRIMARYATTACK;

				// Hold our deployed pose for a few seconds
				m_flHoldDeployedPoseUntilTime = gpGlobals->curtime + 2.0;
			}
			else
			{
				// Weapon primary fire.
				if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );
				}

				iGestureActivity = ACT_VM_PRIMARYATTACK;
			}

			break;
		}

	case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		{
			if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );
			}
			break;
		}
	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
			// Weapon secondary fire.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
			}

			iGestureActivity = ACT_VM_PRIMARYATTACK;
			break;
		}
	case PLAYERANIMEVENT_ATTACK_PRE:
		{
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING ) 
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
			}
			else
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
				iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false );

			break;
		}
	case PLAYERANIMEVENT_ATTACK_POST:
		{
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING ) 
			{
				// Weapon post-fire. Used for minigun winddown in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_POSTFIRE;
			}
			else
			{
				// Weapon post-fire. Used for minigun winddown.
				iGestureActivity = ACT_MP_ATTACK_STAND_POSTFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity );

			break;
		}

	case PLAYERANIMEVENT_RELOAD:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_LOOP );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_RELOAD_END:
		{
			// Weapon reload.
			if ( m_bInAirWalk )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_AIRWALK_END );
			}
			else
			{
				BaseClass::DoAnimationEvent( event, nData );
			}
			break;
		}
	case PLAYERANIMEVENT_DOUBLEJUMP:
		{
			// Check to see if we are jumping!
			if ( !m_bJumping )
			{
				m_bJumping = true;
				m_bFirstJumpFrame = true;
				m_flJumpStartTime = gpGlobals->curtime;
				RestartMainSequence();
			}

			// Force the air walk off.
			m_bInAirWalk = false;

			// Player the air dash gesture.
			if (GetBasePlayer()->GetFlags() & FL_DUCKING)
			{
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP_CROUCH );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_DOUBLEJUMP );
			}
			break;
		}
	case PLAYERANIMEVENT_STUN_BEGIN:
		{
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_STUN_BEGIN );
			break;
		}
	case PLAYERANIMEVENT_STUN_MIDDLE:
		{
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_STUN_MIDDLE );
			break;
		}
	case PLAYERANIMEVENT_STUN_END:
		{
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_STUN_END );
			break;
		}
	default:
		{
			BaseClass::DoAnimationEvent( event, nData );
			break;
		}
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID && GetBasePlayer() != C_BasePlayer::GetLocalPlayer() )
	{
		CBaseCombatWeapon *pWeapon = GetTDCPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->SendWeaponAnim( iGestureActivity );
			pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
		}
	}
#endif
}

void CTDCPlayerAnimState::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	BaseClass::RestartGesture( iGestureSlot, iGestureActivity, bAutoKill );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CTDCPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );

	if ( bInWater )
	{
		if ( m_pTFPlayer->IsActiveTFWeapon( WEAPON_LEVERRIFLE ) )
		{
			if ( m_pTFPlayer->m_Shared.InCond( TDC_COND_ZOOMED ) )
			{
				idealActivity = ACT_MP_SWIM_DEPLOYED;
			}
		}
	}

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerAnimState::PlayFlinchGesture( Activity iActivity )
{
	BaseClass::PlayFlinchGesture( iActivity );

	CTDCViewModel *vm = dynamic_cast<CTDCViewModel*>( GetTDCPlayer()->GetViewModel() );
	if ( vm )
	{
		vm->RestartGesture( GESTURE_SLOT_FLINCH, ACT_MP_GESTURE_FLINCH );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	float flSpeed = GetOuterXYSpeed();

	// If we move, cancel the deployed anim hold
	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		m_flHoldDeployedPoseUntilTime = 0.0;
	}

	if ( m_pTFPlayer->m_Shared.IsLoser() )
	{
		return BaseClass::HandleMoving( idealActivity );
	}

	if ( m_pTFPlayer->m_Shared.InCond( TDC_COND_AIMING ) ) 
	{
		if ( flSpeed > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_DEPLOYED;
		}
		else
		{
			idealActivity = ACT_MP_DEPLOYED_IDLE;
		}
	}
	else if ( m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
	{
		// Unless we move, hold the deployed pose for a number of seconds after being deployed
		idealActivity = ACT_MP_DEPLOYED_IDLE;
	}
	else 
	{
		return BaseClass::HandleMoving( idealActivity );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	if (GetBasePlayer()->GetFlags() & FL_DUCKING)
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED || m_pTFPlayer->m_Shared.IsLoser() )
		{
			idealActivity = ACT_MP_CROUCH_IDLE;		
			if ( m_pTFPlayer->m_Shared.InCond( TDC_COND_AIMING ) || m_flHoldDeployedPoseUntilTime > gpGlobals->curtime )
			{
				idealActivity = ACT_MP_CROUCH_DEPLOYED_IDLE;
			}
		}
		else
		{
			idealActivity = ACT_MP_CROUCHWALK;		

			if ( m_pTFPlayer->m_Shared.InCond( TDC_COND_AIMING ) )
			{
				idealActivity = ACT_MP_CROUCH_DEPLOYED;
			}
		}

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
bool CTDCPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );
		
	// Handle air walking before handling jumping - air walking supersedes jump
	bool bOnGround = ( GetBasePlayer()->GetFlags() & FL_ONGROUND ) != 0;
	bool bInWater = ( GetBasePlayer()->GetWaterLevel() >= WL_Waist );

	CTDCViewModel *vm = dynamic_cast<CTDCViewModel*>( GetTDCPlayer()->GetViewModel() );

	if ( vecVelocity.z > 300.0f || m_bInAirWalk )
	{
		// Check to see if we were in an airwalk and now we are basically on the ground.
		if ( bOnGround && m_bInAirWalk )
		{				
			m_bInAirWalk = false;
			RestartMainSequence();
			RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );	
			if ( vm )
			{
				vm->RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );
			}
		}
		else if ( bInWater )
		{
			// Turn off air walking and reset the animation.
			m_bInAirWalk = false;
			RestartMainSequence();
			if ( vm )
			{
				vm->StopGestureSlot( GESTURE_SLOT_JUMP );
			}
		}
		else if ( !bOnGround )
		{
			// In an air walk.
			idealActivity = ACT_MP_AIRWALK;
			m_bInAirWalk = true;
		}
	}
	// Jumping.
	else if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
			if ( vm )
			{
				vm->RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_START );
			}
		}

		// Reset if we hit water and start swimming.
		if ( bInWater )
		{
			m_bJumping = false;
			RestartMainSequence();
			if ( vm )
			{
				vm->StopGestureSlot( GESTURE_SLOT_JUMP );
			}
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( bOnGround )
			{
				m_bJumping = false;
				RestartMainSequence();
				RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );

				if ( vm )
				{
					vm->RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );
				}
			}
		}

		// if we're still jumping
		if ( m_bJumping )
		{
			if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
			{
				idealActivity = ACT_MP_JUMP_FLOAT;
			}
			else
			{
				idealActivity = ACT_MP_JUMP_START;
			}
			if ( vm )
			{
				if ( !vm->IsGestureSlotPlaying( GESTURE_SLOT_JUMP, ACT_MP_JUMP_START ) && 
					!vm->IsGestureSlotPlaying( GESTURE_SLOT_JUMP, ACT_MP_JUMP_FLOAT ) )
				{
					vm->RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_FLOAT );
				}
			}
		}
	}

	if ( m_bJumping || m_bInAirWalk )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerAnimState::CheckStunAnimation( void )
{
	if ( m_pTFPlayer->m_Shared.InCond( TDC_COND_STUNNED ) )
	{
		int iStunPhase = m_pTFPlayer->m_Shared.GetStunPhase();

		if ( iStunPhase == STUN_PHASE_NONE )
		{
			// Play stun start animation.
			int iSequence = SelectWeightedSequence( ACT_MP_STUN_BEGIN );
			m_flTauntAnimTime = gpGlobals->curtime + m_pTFPlayer->SequenceDuration( iSequence );
			m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_BEGIN );

			m_pTFPlayer->m_Shared.SetStunPhase( STUN_PHASE_LOOP );
		}
		else if ( iStunPhase == STUN_PHASE_LOOP )
		{
			if ( gpGlobals->curtime < m_pTFPlayer->m_Shared.GetStunExpireTime() )
			{
				if ( gpGlobals->curtime >= m_flTauntAnimTime )
				{
					// Continue looping animation.
					int iSequence = SelectWeightedSequence( ACT_MP_STUN_MIDDLE );
					m_flTauntAnimTime = gpGlobals->curtime + m_pTFPlayer->SequenceDuration( iSequence );
					m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_MIDDLE );
				}
			}
			else
			{
				// Play finishing animation.
				int iSequence = SelectWeightedSequence( ACT_MP_STUN_END );
				m_pTFPlayer->m_Shared.SetStunExpireTime( gpGlobals->curtime + m_pTFPlayer->SequenceDuration( iSequence ) );
				m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_END );

				m_pTFPlayer->m_Shared.SetStunPhase( STUN_PHASE_END );
			}
		}
	}
	else
	{
		if ( m_pTFPlayer->m_Shared.GetStunPhase() == STUN_PHASE_LOOP )
		{
			// Clear it up.
			m_pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_STUN_END );
			m_pTFPlayer->m_Shared.SetStunPhase( STUN_PHASE_NONE );
		}
	}
}
