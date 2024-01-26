//=============================================================================
//
// Purpose: RC Bomb
//
//=============================================================================
#ifndef TDC_WEAPON_REMOTEBOMB_H
#define TDC_WEAPON_REMOTEBOMB_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_gun.h"
#include "tdc_projectile_remotebomb.h"

#ifdef CLIENT_DLL
#define CWeaponRemoteBomb C_WeaponRemoteBomb
#endif

class CWeaponRemoteBomb : public CTDCWeaponBaseGun
{
public:
	DECLARE_CLASS( CWeaponRemoteBomb, CTDCWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	static acttable_t m_acttable_alt[];

	CWeaponRemoteBomb();

	ETDCWeaponID GetWeaponID( void ) const { return WEAPON_REMOTEBOMB; }

	enum EBombLaunch
	{
		BOMB_NONE,
		BOMB_THROW,
		BOMB_LOB,
		BOMB_ROLL,
	};

	enum EBombScreen
	{
		BOMBSCREEN_OFF,
		BOMBSCREEN_ACTIVE,
		BOMBSCREEN_DETONATED_ENEMY,
		BOMBSCREEN_DETONATED,
	};

	virtual bool HasPrimaryAmmo( void );
	virtual bool CanBeSelected( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void WeaponReset( void );
	virtual void ItemPostFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual bool SendWeaponAnim( int iActivity );
	virtual void WeaponIdle( void );
	virtual void UpdateOnRemove( void );

	void PrepareThrow( bool bAlternateThrow );
	void LaunchBomb( EBombLaunch iMode );
	void DetonateBomb( void );
	void VerifyBombPosition( CTDCPlayer *pPlayer, Vector &vecPos );
	void ChangeScreenState( EBombScreen iState, float flOffTime = 0.0f );

#ifdef GAME_DLL
	virtual void DeathNotice( CBaseEntity *pVictim );
#else
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual CStudioHdr *OnNewModel( void );

	void UpdateBombBodygroups( void );
#endif

private:
	bool m_bInDetonation;

	CNetworkVar( float, m_flTimeToThrow );
	CNetworkVar( float, m_flLiveTime );
	CNetworkVar( bool, m_bAlternateThrow );
	CNetworkVar( EBombScreen, m_iScreenState );
	CNetworkVar( float, m_flScreenOffTime );

#ifdef GAME_DLL
	CHandle<CProjectile_RemoteBomb> m_hBomb;
#else
	int m_iBombBodygroup;
	int m_iRemoteBodygroup;
	int m_iScreenBodygroup;

	int m_OldWeaponMode;
	int m_iOldScreenState;
#endif
};

#endif // TDC_WEAPON_REMOTEBOMB_H
