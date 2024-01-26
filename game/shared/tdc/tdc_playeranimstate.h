//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_PLAYERANIMSTATE_H
#define TDC_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_TDCPlayer;
#define CTDCPlayer C_TDCPlayer
#else
class CTDCPlayer;
#endif

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CTDCPlayerAnimState : public CMultiPlayerAnimState
{
public:

	DECLARE_CLASS( CTDCPlayerAnimState, CMultiPlayerAnimState );

	CTDCPlayerAnimState();
	CTDCPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CTDCPlayerAnimState();

	void InitTF( CTDCPlayer *pPlayer );
	CTDCPlayer *GetTDCPlayer( void )							{ return m_pTFPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );
	virtual Activity CalcMainActivity( void );

	virtual void	DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual void	RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill = true );

	virtual bool	HandleMoving( Activity &idealActivity );
	virtual bool	HandleJumping( Activity &idealActivity );
	virtual bool	HandleDucking( Activity &idealActivity );
	virtual bool	HandleSwimming( Activity &idealActivity );

	virtual void PlayFlinchGesture( Activity iActivity );

	void			CheckStunAnimation( void );

private:

	CTDCPlayer   *m_pTFPlayer;
	bool		m_bInAirWalk;

	float		m_flHoldDeployedPoseUntilTime;
	float		m_flTauntAnimTime;
};

CTDCPlayerAnimState *CreateTFPlayerAnimState( CTDCPlayer *pPlayer );

#endif // TDC_PLAYERANIMSTATE_H
