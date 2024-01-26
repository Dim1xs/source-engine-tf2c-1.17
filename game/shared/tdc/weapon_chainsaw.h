//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_WEAPON_CHAINSAW_H
#define TDC_WEAPON_CHAINSAW_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CWeaponChainsaw C_WeaponChainsaw
#endif

//=============================================================================

class CWeaponChainsaw : public CTDCWeaponBaseMelee
{
public:

	DECLARE_CLASS( CWeaponChainsaw, CTDCWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponChainsaw();

	virtual ETDCWeaponID	GetWeaponID( void ) const { return WEAPON_CHAINSAW; }

	enum ChansawState_t
	{
		SAW_STATE_OFF = 0,
		SAW_STATE_IDLE,
		SAW_STATE_STARTFIRING,
		SAW_STATE_FIRING,
	};

	enum ChainsawImpact_t
	{
		SAW_IMPACT_NONE = 0,
		SAW_IMPACT_WORLD,
		SAW_IMPACT_FLESH,
	};

	virtual void		WeaponReset( void );
	virtual void		ItemPostFrame( void );
	virtual bool		HasPrimaryAmmo();
	virtual bool		CanBeSelected();
	void				ChainsawCut( void );
	void				Pounce( void );
	virtual bool		SendWeaponAnim( int iActivity );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void		DoImpactEffect( trace_t &tr, int nDamageType );
	virtual bool		CalcIsAttackCriticalHelper( void );
	virtual float		GetMeleeDamage( CBaseEntity *pTarget, ETDCDmgCustom &iCustomDamage );
	virtual float		GetSwingRange( void );

	virtual bool		HasEffectMeter( void ) { return true; }
	virtual float		GetEffectBarProgress( void );
	virtual const char	*GetEffectLabelText( void ) { return "#TDC_Lunge"; }

	virtual void	OnEntityHit(CBaseEntity *pEntity);

#ifdef GAME_DLL
	virtual int			UpdateTransmitState( void );
	virtual void		Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#else
	virtual void		UpdateOnRemove( void );
	virtual void		OnPreDataChanged( DataUpdateType_t type );
	virtual void		OnDataChanged( DataUpdateType_t type );
	virtual CStudioHdr	*OnNewModel( void );
	virtual void		Simulate( void );
	virtual bool		OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual int		GetSkin( void );

	void				WeaponSoundUpdate( bool bEngineStarted = false );
	void				CutSoundUpdate( void );
#endif

private:
	CWeaponChainsaw( const CWeaponChainsaw & ) {}

	void				WindUp( void );
	void				WindDown( void );

	CNetworkVar( ChansawState_t, m_iWeaponState );
	CNetworkVar( bool, m_bCritShot );
	CNetworkVar( float, m_flStateUpdateTime );
	CNetworkVar( float, m_flPounceTime );
	CNetworkVar( int, m_iNumHits );

	bool m_bBackstab;

#ifdef CLIENT_DLL
	int				m_iOldWeaponState;

	CSoundPatch		*m_pSoundCur;				// the weapon sound currently being played
	int				m_iSoundCur;			// the enum value of the weapon sound currently being played

	int				m_iImpactType;
	CSoundPatch		*m_pImpactSound;
	int				m_iImpactSound;

	int				m_iImpactMaterial;
	CNewParticleEffect	*m_pImpactEffect;

	int				m_iChainPoseParameter;
#endif
};

#endif // TDC_WEAPON_CHAINSAW_H
