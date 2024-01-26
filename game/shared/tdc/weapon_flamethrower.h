//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TDC_WEAPON_FLAMETHROWER_H
#define TDC_WEAPON_FLAMETHROWER_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "particlemgr.h"
	#include "particle_util.h"
	#include "particles_simple.h"
	#include "dlight.h"

	#define CWeaponFlamethrower C_WeaponFlamethrower
#else
	#include "tdc_baseprojectile.h"
#endif

class CFlameParticle;

enum FlameThrowerState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING,
	FT_STATE_AIRBLASTING
};

//=========================================================
// Flamethrower Weapon
//=========================================================
class CWeaponFlamethrower : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponFlamethrower, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponFlamethrower();
	~CWeaponFlamethrower();

	virtual void	Precache( void );

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_FLAMETHROWER; }

	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual void	ItemHolsterFrame( void );
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();

	virtual bool	Lower( void );
	virtual void	WeaponReset( void );

	virtual void	DestroySounds( void );

	Vector			GetVisualMuzzlePos();
	Vector			GetFlameOriginPos();

	Vector			GetDeflectionSize();

	// Client specific.
#if defined( CLIENT_DLL )
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	UpdateOnRemove( void );
	virtual void	SetDormant( bool bDormant );
	virtual void	Simulate( void );
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	//	Start/stop flame sound and particle effects
	void			StartFlame();
	void			StopFlame( bool bAbrupt = false );

	void			RestartParticleEffect();	

	// constant pilot light sound
	void 			StartPilotLight();
	void 			StopPilotLight();

	// Server specific.
#else
	virtual void	DeflectEntity( CBaseEntity *pEntity, CTDCPlayer *pAttacker, Vector &vecDir );
	virtual void	DeflectPlayer( CTDCPlayer *pVictim, CTDCPlayer *pAttacker, Vector &vecDir );

	void			SetHitTarget( void );
	void			HitTargetThink( void );
	void			SimulateFlames( void );
	bool			IsSimulatingFlames( void ) { return m_bSimulatingFlames; }
#endif

private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( bool, m_bCritFire );
	CNetworkVar( float, m_flAmmoUseRemainder );
	CNetworkVar( bool, m_bHitTarget );

	float		m_flStartFiringTime;
	float		m_flNextPrimaryAttackAnim;

#if defined( CLIENT_DLL )
	int			m_iParticleWaterLevel;

	CSoundPatch	*m_pFiringStartSound;

	CSoundPatch	*m_pFiringLoop;
	bool		m_bFiringLoopCritical;

	CNewParticleEffect *m_pFlameEffect;
	EHANDLE		m_hFlameEffectHost;

	CSoundPatch *m_pPilotLightSound;

	bool		m_bOldHitTarget;
	CSoundPatch *m_pHitTargetSound;

	dlight_t	*m_pDynamicLight;
#else
	CUtlVector<CFlameParticle> m_Flames;
	bool		m_bSimulatingFlames;
	float		m_flStopHitSoundTime;
#endif

private:
	CWeaponFlamethrower( const CWeaponFlamethrower & );
};

#ifdef GAME_DLL
class CFlameParticle
{
public:
	void Init( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CWeaponFlamethrower *pWeapon );
	bool FlameThink( void );
	void CheckCollision( CBaseEntity *pOther, bool *pbHitWorld );

private:
	void OnCollide( CBaseEntity *pOther );

	Vector						m_vecOrigin;
	Vector						m_vecInitialPos;		// position the flame was fired from
	Vector						m_vecPrevPos;			// position from previous frame
	Vector						m_vecVelocity;
	Vector						m_vecBaseVelocity;		// base velocity vector of the flame (ignoring rise effect)
	Vector						m_vecAttackerVelocity;	// velocity of attacking player at time flame was fired
	Vector						m_vecMins;
	Vector						m_vecMaxs;
	float						m_flTimeRemove;			// time at which the flame should be removed
	int							m_iTeamNum;
	int							m_iDmgType;				// damage type
	float						m_flDmgAmount;			// amount of base damage
	EHANDLE						m_hOwner;
	CWeaponFlamethrower				*m_pOuter;
	CUtlVector<EHANDLE>			m_hEntitiesBurnt;		// list of entities this flame has burnt
};

#endif // GAME_DLL

#endif // TDC_WEAPON_FLAMETHROWER_H
