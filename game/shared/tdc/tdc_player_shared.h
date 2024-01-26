//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TDC_PLAYER_SHARED_H
#define TDC_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tdc_shareddefs.h"
#include "tdc_weaponbase.h"
#include "basegrenade_shared.h"

// Client specific.
#ifdef CLIENT_DLL
class C_TDCPlayer;
// Server specific.
#else
class CTDCPlayer;
#endif

//=============================================================================
//
// Tables.
//
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_TDCPlayerShared );
#else
EXTERN_SEND_TABLE( DT_TDCPlayerShared );
#endif

//=============================================================================

#define PERMANENT_CONDITION		-1

enum
{
	STUN_PHASE_NONE,
	STUN_PHASE_LOOP,
	STUN_PHASE_END,
};

//=============================================================================
//
// Shared player class.
//
class CTDCPlayerShared
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TDCPlayer;
	typedef C_TDCPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTDCPlayer;
	typedef CTDCPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTDCPlayerShared );

	// Initialization.
	CTDCPlayerShared();
	void Init( OuterClass *pOuter );

	float	GetTimeUntilRespawn( void ) { return m_flRespawnTime - gpGlobals->curtime; }
	bool	IsWaitingToRespawn( void ) { return m_bWaitingToRespawn; }
#ifdef GAME_DLL
	void	SetTimeUntilRespawn( float flRespawnTime ) { m_flRespawnTime = flRespawnTime + gpGlobals->curtime; } //Without curtime.
	void	SetRespawnTime( float flRespawnTime ) { m_flRespawnTime = flRespawnTime; } //With a pre-calculated curtime.
	void	SetWaitingToRespawn( bool bWaiting ) { m_bWaitingToRespawn = bWaiting; }
#endif

	// State (TDC_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState ) const			{ return ( m_nPlayerState == nState ); }

	// Condition (TDC_COND_*).
	void	AddCond( ETDCCond nCond, float flDuration = PERMANENT_CONDITION );
	void	RemoveCond( ETDCCond nCond );
	bool	InCond( ETDCCond nCond ) const;
	void	RemoveAllCond( void );
	bool	RemoveCondIfPresent( ETDCCond nCond );
	void	OnConditionAdded( ETDCCond nCond );
	void	OnConditionRemoved( ETDCCond nCond );
	float	GetConditionDuration( ETDCCond nCond ) const;

	bool	IsCritBoosted( void ) const;
	bool	IsInvulnerable( void ) const;
	bool	IsStealthed( void ) const;
	bool	IsHasting( void ) const;
	bool	IsMovementLocked( void ) const;
	bool	IsCarryingPowerup( void ) const;

	void	ConditionGameRulesThink( void );

	void	InvisibilityThink( void );

	int		GetMaxBuffedHealth( void ) const;
	bool	HealNegativeConds( void );
	int		GetPowerupFlags( void );

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TDCPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	SyncConditions( int nCond, int nOldCond, int nUnused, int iOffset );
#endif

#ifdef CLIENT_DLL
	void	UpdateLoopingSounds( bool bDormant );
	void	UpdateCritBoostEffect( void );
	bool	SetParticleToMercColor( CNewParticleEffect *pParticle );
#endif

	void	Burn( CTDCPlayer *pAttacker, CTDCWeaponBase *pWeapon = NULL, float flBurnTime = 10.0f );
	void	SpreadFire( CTDCPlayer *pAttacker, float flBurnTime = NULL );
	void	StunPlayer( float flDuration, CTDCPlayer *pStunner );
	void	AirblastPlayer( CTDCPlayer *pAttacker, const Vector &vecDir, float flSpeed );
	const Vector &GetAirblastPosition( void ) { return m_vecAirblastPos.Get(); }
	void	ShovePlayer( CTDCPlayer *pAttacker, const Vector &vecDir, float flSpeed );

	bool	HasDemoShieldEquipped() const { return false; }

	// Weapons.
	CTDCWeaponBase *GetActiveTFWeapon() const;

	// Utility.
	bool	IsLoser( void );

	void	FadeInvis( float flInvisFadeTime, bool bNoAttack = false );
	float	GetPercentInvisible( void );
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }

	ETDCWeaponID		GetDesiredWeaponIndex( void ) { return m_iDesiredWeaponID; }
	void	SetDesiredWeaponIndex( ETDCWeaponID iWeaponID ) { m_iDesiredWeaponID = iWeaponID; }

	ETDCDamageSourceType	GetDamageSourceType( void ) { return m_iDamageSourceType; }
#ifdef GAME_DLL
	void	SetDamageSourceType( ETDCDamageSourceType iDamageSourceType ) { m_iDamageSourceType = iDamageSourceType; }
	void	ResetDamageSourceType() { SetDamageSourceType( TDC_DMG_SOURCE_WEAPON ); }
#endif

	bool	IsJumping( void ) { return m_bJumping; }
	void	SetJumping( bool bJumping );
	int		GetAirDucks( void ) { return m_nAirDucked; }
	void	IncrementAirDucks( void );
	void	ResetAirDucks( void );

	void	DebugPrintConditions( void );

	void	SetPlayerDominated( CTDCPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTDCPlayer *pPlayer, bool bDominated );

	float	GetFlameRemoveTime( void ) { return m_flFlameRemoveTime; }

	int		GetKillstreak( void ) { return m_nStreaks.Get( 0 ); }
	void	SetKillstreak( int iKillstreak ) { m_nStreaks.Set( 0, iKillstreak ); }
	void	IncKillstreak() { m_nStreaks.Set( 0, m_nStreaks.Get( 0 ) + 1 ); }

	int		GetStunPhase( void ) { return m_iStunPhase; }
	void	SetStunPhase( int iPhase ) { m_iStunPhase = iPhase; }
	float	GetStunExpireTime( void ) { return m_flStunExpireTime; }
	void	SetStunExpireTime( float flTime ) { m_flStunExpireTime = flTime; }

	// TODO: these three stun-related functions are more-or-less just stubs for now;
	// update them if we ever implement live TF2's more complicated stun flag system
	bool	IsControlStunned() const          { return InCond(TDC_COND_STUNNED); }
	bool	IsLoserStateStunned() const       { return InCond(TDC_COND_STUNNED); }
	float	GetAmountStunned(int nFlag) const { (void)nFlag; return (InCond(TDC_COND_STUNNED) ? 1.00f : 0.00f); }

#ifdef CLIENT_DLL
	bool	ShouldShowRecentlyTeleported( void );
#endif

	int GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom );

private:

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void OnAddStealthed( void );
	void OnAddInvulnerable( void );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddTaunting( void );
	void OnAddStunned( void );
	void OnAddRagemode( void );
	void OnAddSpeedBoost( void );
	void OnAddSprint( void );

	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveTeleported( void );
	void OnRemoveTaunting( void );
	void OnRemoveStunned( void );
	void OnRemoveRagemode( void );
	void OnRemoveSpeedBoost( void );
	void OnRemoveSprint( void );

private:

	// Vars that are networked.
	CNetworkVar( float, m_flRespawnTime );
	CNetworkVar( bool, m_bWaitingToRespawn );

	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	CNetworkArray( float, m_flCondExpireTimeLeft, TDC_COND_LAST ); // Time until each condition expires

	CNetworkVar( float, m_flInvisibility );
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;

	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TDCPlayer or CTDCPlayer (client/server).

	// Burn handling
	CHandle<CTDCPlayer>		m_hBurnAttacker;
	CHandle<CTDCWeaponBase>	m_hBurnWeapon;
	float					m_flFlameBurnTime;
	CNetworkVar( float, m_flFlameRemoveTime );
	float					m_flTauntRemoveTime;

	CNetworkVar( ETDCDamageSourceType, m_iDamageSourceType );
	CNetworkVar( ETDCWeaponID, m_iDesiredWeaponID );

	float m_flNextBurningSound;

	CNetworkVar( bool, m_bJumping );
	CNetworkVar( int, m_nAirDucked );

	CNetworkArray( int, m_nStreaks, 3 );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS+1 );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS+1 );	// array of state per other player whether other players are dominating this player

	CNetworkHandle( CTDCPlayer, m_hStunner );
	CNetworkVar( float, m_flStunExpireTime );
	int m_iStunPhase;

	CNetworkVector( m_vecAirblastPos );

	CNetworkVar( float, m_flSprintStartTime );

#ifdef CLIENT_DLL
	CNewParticleEffect	*m_pBurningEffect;
	CSoundPatch			*m_pBurningSound;

	CNewParticleEffect *m_pCritEffect;
	EHANDLE m_hCritEffectHost;
	CSoundPatch *m_pCritSound;

	CNewParticleEffect *m_pSpeedEffect;

	int	m_nOldConditions;

	bool m_bWasCritBoosted;
	float m_flOldFlameRemoveTime;
#endif
};

#endif // TDC_PLAYER_SHARED_H
