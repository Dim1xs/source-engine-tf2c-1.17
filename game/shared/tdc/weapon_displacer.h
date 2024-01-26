//=============================================================================
//
// Purpose: TELEMAX DISPLACER CANNON
//
//=============================================================================
#ifndef TDC_WEAPON_DISPLACER_H
#define TDC_WEAPON_DISPLACER_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CWeaponDisplacer C_WeaponDisplacer
#endif

class CWeaponDisplacer : public CTDCWeaponBaseGun, public TAutoList<CWeaponDisplacer>
{
public:
	DECLARE_CLASS( CWeaponDisplacer, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponDisplacer();

	virtual ETDCWeaponID GetWeaponID( void ) const { return WEAPON_DISPLACER; }

	virtual void	Precache( void );
	virtual void	WeaponReset( void );
	virtual bool	Deploy( void );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	Equip( CBaseCombatCharacter *pOwner );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	CanPerformSecondaryAttack( void ) const;
	virtual void	FireProjectile( void );
	virtual bool	SendWeaponAnim( int iActivity );
	virtual void	UpdateOnRemove( void );

	virtual bool	HasEffectMeter( void ) { return true; }
	virtual float	GetEffectBarProgress( void );
	virtual const char	*GetEffectLabelText( void ) { return "#TDC_TeleportBar"; }

	bool			IsChargingAnyShot( void ) const;
	void			FinishTeleport( void );
	void			ShowHolsterModel( bool bShow );

#ifdef GAME_DLL
	CBaseEntity		*GetTeleportSpot( void ) { return m_hTeleportSpot.Get(); }
	virtual int		UpdateTransmitState( void );
#else
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	void			CreateWarmupEffect( bool bSecondary );
	void			ManageIdleEffect( void );
	void			UpdateIdleSound( void );
#endif

private:
	CNetworkVar( float, m_flBlastTime );
	CNetworkVar( float, m_flTeleportTime );
	CNetworkVar( float, m_flTeleportRechargeTime );
	CNetworkVar( bool, m_bLockedOn );
	CNetworkVar( bool, m_bIdle );
	CNetworkHandle( CBaseEntity, m_hHolsterModel );

#ifdef GAME_DLL
	EHANDLE m_hTeleportSpot;
#else
	bool m_bWasChargingBlast;
	bool m_bWasChargingTeleport;

	CNewParticleEffect *m_pIdleEffect;
	EHANDLE m_hEffectHost;
	CSoundPatch *m_pIdleSound;
#endif
};

#ifdef CLIENT_DLL
#define CWeaponHolster C_WeaponHolster
#endif

class CWeaponHolster : public CBaseAnimating
{
public:
	DECLARE_CLASS( CWeaponHolster, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CWeaponHolster();

#ifdef GAME_DLL
	virtual int	UpdateTransmitState( void );
	virtual int ShouldTransmit( CCheckTransmitInfo *pInfo );
#else
	virtual bool ShouldPredict( void )
	{
		if ( GetOwnerEntity() && GetOwnerEntity() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

	virtual bool ShouldDraw( void );
	virtual ShadowType_t ShadowCastType( void );
#endif
};

#endif // TDC_WEAPON_DISPLACER_H
