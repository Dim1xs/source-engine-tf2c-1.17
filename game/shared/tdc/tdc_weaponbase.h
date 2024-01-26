//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//	CTDCWeaponBase
//	|
//	|--> CTDCWeaponBaseMelee
//	|		|
//	|		|--> CTDCWeaponCrowbar
//	|		|--> CTDCWeaponKnife
//	|		|--> CTDCWeaponMedikit
//	|		|--> CTDCWeaponWrench
//	|
//	|--> CTDCWeaponBaseGrenade
//	|		|
//	|		|--> CTDCWeapon
//	|		|--> CTDCWeapon
//	|
//	|--> CTDCWeaponBaseGun
//
//=============================================================================
#ifndef TDC_WEAPONBASE_H
#define TDC_WEAPONBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_playeranimstate.h"
#include "tdc_weapon_parse.h"
#include "npcevent.h"

// Client specific.
#if defined( CLIENT_DLL )
#define CTDCWeaponBase C_TDCWeaponBase
#include "tdc_fx_muzzleflash.h"
#endif

#define WEAPON_RANDOM_RANGE 10000
#define MAX_TRACER_NAME		128

CTDCWeaponInfo *GetTDCWeaponInfo( ETDCWeaponID iWeapon );

// Temp macro that falls through to base class. Replace with IMPLEMENT_ACTTABLE as needed.
#define IMPLEMENT_TEMP_ACTTABLE(className) \
	acttable_t *className::ActivityList( int &iActivityCount ) { \
	if ( GetTDCWpnData().m_bUseNewActTable ) { iActivityCount = ARRAYSIZE(m_acttable); return m_acttable; } \
	else { return BaseClass::ActivityList( iActivityCount ); } }

class CTDCPlayer;

void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, ITraceFilter *pFilter );

// Reloading singly.
enum
{
	TDC_RELOAD_START = 0,
	TDC_RELOADING,
	TDC_RELOADING_CONTINUE,
	TDC_RELOAD_FINISH
};

// structure to encapsulate state of head bob
struct BobState_t
{
	BobState_t() 
	{ 
		m_flBobTime = 0; 
		m_flLastBobTime = 0;
		m_flLastSpeed = 0;
		m_flVerticalBob = 0;
		m_flLateralBob = 0;
	}

	float m_flBobTime;
	float m_flLastBobTime;
	float m_flLastSpeed;
	float m_flVerticalBob;
	float m_flLateralBob;
};

#ifdef CLIENT_DLL
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState );
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState );
#endif

//=============================================================================
//
// Base TF Weapon Class
//
class CTDCWeaponBase : public CBaseCombatWeapon, public IHasOwner
{
	DECLARE_CLASS( CTDCWeaponBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Setup.
	CTDCWeaponBase();
	virtual ~CTDCWeaponBase();

	virtual void Spawn();
	virtual void Precache();
	virtual bool IsPredicted() const			{ return true; }

	// Weapon Data.
	CTDCWeaponInfo const	&GetTDCWpnData() const;
	virtual ETDCWeaponID GetWeaponID( void ) const;
	bool IsWeapon( ETDCWeaponID iWeapon ) const;
	virtual float GetFireRate( void );
	virtual int	GetDamageType() const { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual ETDCDmgCustom GetCustomDamageType() const { return TDC_DMG_CUSTOM_NONE; }

	// View model.
	virtual void SetViewModel();
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual const char *GetViewModelPerClass(int iViewModel, int iClassIdx ) const;
	virtual void UpdateViewModel( void );
	CBaseViewModel *GetPlayerViewModel( bool bObserverOK = false ) const;

	// World model.
	virtual const char *GetWorldModel( void ) const;

	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool IsHolstered( void ) { return ( m_iState != WEAPON_IS_ACTIVE ); }
	virtual bool Deploy( void );
	virtual void GiveTo( CBaseEntity *pEntity );
	virtual void Equip( CBaseCombatCharacter *pOwner );
#ifdef GAME_DLL
	virtual void UnEquip( void );
#endif
	bool IsViewModelFlipped( void );

	// Attacks.
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	void			CalcIsAttackCritical( void );
	virtual bool	CalcIsAttackCriticalHelper( void );
	bool			IsCurrentAttackACrit( void ) { return m_bCurrentAttackIsCrit; }
	virtual bool	CanHeadshot( void );
	virtual bool	CanCritHeadshot( void ) { return true; }

	// Ammo.
	virtual int	GetMaxClip1( void ) const;
	virtual int	GetDefaultClip1( void ) const;
	virtual int GetMaxAmmo( void );
	virtual int GetInitialAmmo( void );
	virtual bool HasPrimaryAmmoToFire( void );

	// Reloads.
	virtual bool Reload( void );
	virtual	void CheckReload( void );
	virtual void PerformReloadRefill( bool bDropRemainingMag = false, bool bSingle = false, bool bReloadPrimary = true, bool bReloadSecondary = true );
	virtual void FinishReload( void );
	virtual void AbortReload( void );
	virtual bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	virtual bool CanAutoReload( void ) { return true; }
	virtual bool ReloadOrSwitchWeapons( void );
	bool IsReloading( void ) { return m_iReloadMode != TDC_RELOAD_START; }

	virtual bool CanFidget( void );
	virtual void Fidget( void );

	virtual bool CanDrop( void ) { return false; }

	// Sound.
	void PlayReloadSound( void );

	// Activities.
	virtual void ItemBusyFrame( void );
	virtual void ItemPostFrame( void );
	virtual void ItemHolsterFrame( void );

	virtual void SetWeaponVisible( bool visible );
	virtual bool WeaponShouldBeVisible( void );

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttablePrimary[];
	static acttable_t m_acttableSecondary[];
	static acttable_t m_acttableMelee[];
	static acttable_t m_acttableBuilding[];
	static acttable_t m_acttablePDA[];
	static acttable_t m_acttableItem1[];
	static acttable_t m_acttableItem2[];
	static acttable_t m_acttableMeleeAllClass[];
	static acttable_t m_acttableSecondary2[];
	static acttable_t m_acttablePrimary2[];

	// Utility.
	CBasePlayer *GetPlayerOwner() const;
	CTDCPlayer *GetTDCPlayerOwner() const;

	virtual CBaseEntity *GetOwnerViaInterface( void ) { return GetOwner(); }

#ifdef CLIENT_DLL
	bool			UsingViewModel( void );
	C_BaseAnimating	*GetWeaponForEffect();
#endif

	bool CanAttack( void ) const;

	virtual bool	SendWeaponAnim( int iActivity );
	virtual void	WeaponIdle( void );

	bool				HasActivity( Activity activity );
	virtual Activity	GetDrawActivity( void );
	virtual Activity	GetFidgetActivity( void );

	virtual void	WeaponReset( void );
	virtual void	WeaponRegenerate( void ) {}

	// Muzzleflashes
	virtual const char *GetMuzzleFlashParticleEffect( void );

	virtual const char	*GetTracerType( void );
	virtual void		MakeTracer( const Vector &vecTracerSrc, const trace_t &tr );
	virtual int			GetTracerAttachment( void );
	virtual void		GetTracerOrigin( Vector &vecPos );

	float				GetLastFireTime( void ) { return m_flLastFireTime; }
	virtual bool		CanFireAccurateShot( int nBulletsPerShot );
	virtual const Vector2D	*GetSpreadPattern( int &iNumBullets );

	virtual bool		HasEffectMeter( void ) { return false; }
	void				StartEffectBarRegen( void );
	void				EffectBarRegenFinished( void );
	void				CheckEffectBarRegen( void );
	virtual float		GetEffectBarProgress( void );
	virtual const char	*GetEffectLabelText( void ) { return ""; }
	virtual int			GetCount( void ) { return -1; }

	void				OnControlStunned( void );

// Server specific.
#if !defined( CLIENT_DLL )

	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	// Spawning.
	virtual void FallInit( void );
	virtual void CheckRespawn( void );

	// Ammo.
	virtual const Vector& GetBulletSpread( void );

// Client specific.
#else

	virtual void	CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt );
	virtual int		InternalDrawModel( int flags );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual bool	ShouldDraw( void );
	virtual ShadowType_t	ShadowCastType( void );
	virtual int		CalcOverrideModelIndex() OVERRIDE;

	virtual bool	ShouldPredict();
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual int		GetWorldModelIndex( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	Redraw( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	virtual int		GetSkin( void );
	BobState_t		*GetBobState();
	virtual bool	AllowViewModelOffset( void );
	virtual float	ViewModelOffsetScale( void );

	virtual bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void EjectBrass( C_BaseAnimating *pEntity, int iAttachment = -1 );
	virtual void DumpBrass( C_BaseAnimating *pEntity, int iAttachment = -1 );
	void EjectMagazine( C_BaseAnimating *pEntity, const Vector &vecForce );
	void UnhideMagazine( C_BaseAnimating *pEntity );
	virtual CStudioHdr *OnNewModel( void );
	virtual void ThirdPersonSwitch( bool bThirdperson );

	// Model muzzleflashes
	CHandle<C_MuzzleFlashModel>		m_hMuzzleFlashModel;

#endif

protected:
	// Reloads.
	void UpdateReloadTimers( bool bStart );
	void SetReloadTimer( float flReloadTime, float flRefillTime = 0.0f );
	bool ReloadSingly( void );
	virtual void OnReloadSinglyUpdate( void ) {}

	virtual float InternalGetEffectBarRechargeTime( void ) { return 0.0f; }

protected:

	CNetworkVar( int, m_iWeaponMode );
	CNetworkVar( int, m_iReloadMode );
	bool			m_bInAttack;
	bool			m_bInAttack2;
	bool			m_bCurrentAttackIsCrit;

	int				m_iReloadStartClipAmount;

	CNetworkVar( float, m_flCritTime );
	CNetworkVar( float,	m_flLastCritCheckTime );
	int				m_iLastCritCheckFrame;

	char			m_szTracerName[MAX_TRACER_NAME];

	CNetworkVar( bool, m_bResetParity );

	CNetworkVar( float, m_flNextFidgetTime );
	bool			m_bInFidget;

#ifdef CLIENT_DLL
	bool m_bOldResetParity;

	KeyValues *m_pModelKeyValues;
	KeyValues *m_pModelWeaponData;

	int m_iMuzzleAttachment;
	int m_iBrassAttachment;
	int m_iMagBodygroup;
	int m_iMagAttachment;
#endif

	CNetworkVar( float, m_flClipRefillTime );
	CNetworkVar( bool, m_bReloadedThroughAnimEvent );
	CNetworkVar( float, m_flLastFireTime );
	CNetworkVar( float, m_flEffectBarRegenTime );

	float		m_flGivenTime;

private:
	CTDCWeaponBase( const CTDCWeaponBase & );
};

class CTraceFilterIgnorePlayers : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterIgnorePlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );
};

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	int m_iIgnoreTeam;
};

#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )			\
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
															\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
															\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
															\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	PRECACHE_WEAPON_REGISTER( entityname );

#endif // TDC_WEAPONBASE_H
