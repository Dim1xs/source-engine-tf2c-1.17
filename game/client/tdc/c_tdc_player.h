//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef C_TDC_PLAYER_H
#define C_TDC_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_playeranimstate.h"
#include "c_baseplayer.h"
#include "tdc_shareddefs.h"
#include "baseparticleentity.h"
#include "tdc_player_shared.h"
#include "c_tdc_playerclass.h"
#include "entity_capture_flag.h"
#include "props_shared.h"
#include "hintsystem.h"
#include "c_playerattachedmodel.h"
#include "iinput.h"

class C_MuzzleFlashModel;
class C_BaseObject;
class C_TDCTeam;
class C_TDCRagdoll;
class C_TDCWearable;

extern ConVar cl_autorezoom;
extern ConVar cl_flipviewmodels;
extern ConVar tdc_zoom_hold;

extern ConVar tdc_merc_color_r;
extern ConVar tdc_merc_color_g;
extern ConVar tdc_merc_color_b;
extern ConVar tdc_merc_winanim;

extern ConVar tdc_merc_skintone;
extern ConVar tdc_merc_color;

//=============================================================================
//
// Ragdoll
//
// ----------------------------------------------------------------------------- //
// Client ragdoll entity.
// ----------------------------------------------------------------------------- //
class C_TDCRagdoll : public C_BaseFlex
{
public:

	DECLARE_CLASS( C_TDCRagdoll, C_BaseFlex );
	DECLARE_CLIENTCLASS();
	
	C_TDCRagdoll();
	~C_TDCRagdoll();

	virtual void UpdateOnRemove( void );
	virtual void OnDataChanged( DataUpdateType_t type );

	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );

	void ClientThink( void );
	void StartFadeOut( float fDelay );
	void EndFadeOut();

	C_TDCPlayer *GetOwningPlayer( void );

	bool IsRagdollVisible();
	float GetBurnStartTime() { return m_flBurnEffectStartTime; }

	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual float FrameAdvance( float flInterval = 0.0f );

	bool IsPlayingDeathAnim( void ) { return m_bPlayingDeathAnim; }

	Vector	m_vecPlayerColor;
	Vector	m_vecPlayerSkinTone;
private:
	
	C_TDCRagdoll( const C_TDCRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );

	void CreateTFRagdoll( void );
	void CreateTFGibs( void );

private:

	Vector  m_vecRagdollVelocity;
	Vector  m_vecRagdollOrigin;
	float	m_fDeathTime;
	bool	m_bFadingOut;
	int		m_nRagdollFlags;
	ETDCDmgCustom m_iDamageCustom;
	int		m_iTeam;
	int		m_iClass;
	float	m_flBurnEffectStartTime;	// start time of burning, or 0 if not burning
	bool	m_bPlayingDeathAnim;
	bool	m_bFinishedDeathAnim;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TDCPlayer : public C_BasePlayer
{
public:

	DECLARE_CLASS( C_TDCPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_TDCPlayer();
	~C_TDCPlayer();

	friend class CTDCGameMovement;

	static C_TDCPlayer* GetLocalTDCPlayer();

	virtual void UpdateOnRemove( void );

	virtual bool IsNextBot( void ) const { return m_bIsABot; }
	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();
	virtual void SetDormant( bool bDormant );
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ProcessMuzzleFlashEvent();
	virtual void ValidateModelIndex( void );

	virtual Vector GetObserverCamOrigin( void );
	virtual int DrawModel( int flags );
	virtual int	InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd );

	virtual bool IsAllowedToSwitchWeapons( void );

	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual void PhysicsSimulate( void );
	virtual void ClientThink();

	// Deal with recording
	virtual void GetToolRecordingState( KeyValues *msg );

	CTDCWeaponBase *GetActiveTFWeapon( void ) const;
	bool		 IsActiveTFWeapon( ETDCWeaponID iWeaponID ) const;

	virtual void Simulate( void );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	bool CanAttack( void );

	C_TDCPlayerClass *GetPlayerClass( void )				{ return &m_PlayerClass; }
	const C_TDCPlayerClass *GetPlayerClass( void ) const	{ return &m_PlayerClass; }
	bool CanShowClassMenu( void ) const;
	bool IsPlayerClass( int iClass ) const;
	bool IsNormalClass( void ) const;
	bool IsZombie( void ) const;
	virtual int GetMaxHealth( void ) const;

	virtual int GetRenderTeamNumber( void );

	bool IsWeaponLowered( void );

	void	AvoidPlayers( CUserCmd *pCmd );

	// Get the ID target entity index. The ID target is the player that is behind our crosshairs, used to
	// display the player's name.
	void UpdateIDTarget();
	int GetIDTarget() const;
	void SetForcedIDTarget( int iTarget );
	void GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes, bool &bShowingAmmo );

	void SetAnimation( PLAYER_ANIM playerAnim );

	virtual float GetMinFOV() const;

	virtual const QAngle& EyeAngles();

	virtual void ComputeFxBlend( void );

	// Taunts/VCDs
	virtual bool	StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget );
	virtual void	CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	bool			StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	void			TurnOnTauntCam( void );
	void			TurnOffTauntCam( void );
	void			TauntCamInterpolation( void );
	bool			InTauntCam( void ) { return m_bWasTaunting; }
	bool			InThirdPersonShoulder( void );
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	virtual void	InitPhonemeMappings();

	// Gibs.
	void InitPlayerGibs( void );
	void CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning = false );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	float GetPercentInvisible( void );
	float GetEffectiveInvisibilityLevel( void );	// takes viewer into account

	virtual void AddDecal( const Vector& rayStart, const Vector& rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

	virtual void CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual Vector GetChaseCamViewOffset( CBaseEntity *target );

	void ClientPlayerRespawn( void );

	void RecalculateViewmodelBodygroups( void );
	
	virtual bool	IsOverridingViewmodel( void );
	virtual int		DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags );

	void			UpdateRecentlyTeleportedEffect( void );

	void			InitializePoseParams( void );
	void			UpdateLookAt( void );

	bool			IsEnemyPlayer( void );
	void			ShowNemesisIcon( bool bShow );

	bool			IsSprinting(void){
		return true;
	}

	CUtlVector<EHANDLE>		*GetSpawnedGibs( void ) { return &m_hSpawnedGibs; }

	Vector 			GetClassEyeHeight( void );

	bool			ShouldAutoRezoom( void ){ return cl_autorezoom.GetBool(); }
	bool			ShouldFlipViewModel( void ) { return m_bFlipViewModel; }
	float			GetViewModelFOV( void ) { return m_flViewModelFOV; }
	const Vector	&GetViewModelOffset( void ) { return m_vecViewModelOffset; }
	bool			ShouldHoldToZoom( void ) { return tdc_zoom_hold.GetBool(); }

public:
	// Shared functions
	void			TeamFortress_SetSpeed();
	bool			HasItem( void ) const;					// Currently can have only one item at a time.
	void			SetItem( C_TDCItem *pItem );
	C_TDCItem		*GetItem( void ) const;
	bool			IsAllowedToPickUpFlag( void ) const;
	bool			HasTheFlag( void ) const;
	C_CaptureFlag	*GetTheFlag( void ) const;

	virtual void	ItemPostFrame( void );

	void			SetOffHandWeapon( CTDCWeaponBase *pWeapon );
	void			HolsterOffHandWeapon( void );

	virtual int GetSkin();

	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );
	virtual	bool		Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 );

	CTDCWeaponBase		*GetTDCWeapon( int iWeapon ) const { return static_cast<CTDCWeaponBase *>( GetWeapon( iWeapon ) ); }
	CTDCWeaponBase		*GetTDCWeaponBySlot( int iBucket ) const { return static_cast<CTDCWeaponBase *>( Weapon_GetSlot( iBucket ) ); }
	CTDCWeaponBase		*Weapon_OwnsThisID( ETDCWeaponID iWeaponID ) const;
	virtual bool		Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon );
	virtual CBaseCombatWeapon *Weapon_GetSlot( int slot ) const;

	virtual void		UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual void		OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume );

	C_TDCTeam *GetTDCTeam( void ) const;

	virtual void		RemoveAmmo( int iCount, int iAmmoIndex );
	virtual int			GetAmmoCount( int iAmmoIndex ) const;
	int					GetMaxAmmo( int iAmmoIndex, bool bAddMissingClip = false ) const;
	bool				HasInfiniteAmmo( void ) const;

	bool				IsEnemy( const C_BaseEntity *pEntity ) const;

	void JumpSound( void );
	CTDCPlayer *GetObservedPlayer( bool bFirstPerson );
	virtual Vector Weapon_ShootPosition( void );
	void SetEyeAngles( const QAngle &angles );
	int GetNumMoneyPacks( void ) { return m_nMoneyPacks; }

	float HealthFractionBuffed() const;

public:
	// Ragdolls.
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual IRagdoll		*GetRepresentativeRagdoll() const;
	EHANDLE	m_hRagdoll;
	Vector m_vecRagdollVelocity;

	virtual CStudioHdr *OnNewModel( void );

	void				DisplaysHintsForTarget( C_BaseEntity *pTarget );

	// Shadows
	virtual ShadowType_t ShadowCastType( void ) ;
	virtual void GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual bool GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const;

	CMaterialReference *GetInvulnMaterialRef( void ) { return &m_InvulnerableMaterial; }
	bool ShouldShowNemesisIcon();

	virtual	IMaterial *GetHeadLabelMaterial( void );

	virtual void FireGameEvent( IGameEvent *event );

	void UpdateSpyStateChange( void );

	bool CanShowSpeechBubbles( void );
	void UpdateSpeechBubbles( void );
	void UpdateOverhealEffect( void );

	void UpdateClientSideGlow( void );
	virtual void	GetGlowEffectColor( float *r, float *g, float *b );

	float GetDesaturationAmount( void );

	void CollectVisibleSteamUsers( CUtlVector<CSteamID> &userList );

protected:

	void ResetFlexWeights( CStudioHdr *pStudioHdr );

private:

	void HandleTaunting( void );

	void OnPlayerClassChange( void );

	void InitInvulnerableMaterial( void );

	bool				m_bWasTaunting;
	float				m_flTauntOffTime;

public:

	Vector				m_vecPlayerColor;
	Vector				m_vecPlayerSkinTone;

	float				m_flSprintPower;
	float				m_flSprintPowerLastCheckTime;
	float				m_flSprintRegenStartTime;

private:

	CSoundPatch		*m_pSoundCur;				

	C_TDCPlayerClass		m_PlayerClass;

	// ID Target
	int					m_iIDEntIndex;
	int					m_iForcedIDTarget;

	CNewParticleEffect	*m_pTeleporterEffect;
	bool				m_bToolRecordingVisibility;

	int					m_iOldSpawnCounter;

	int					m_iOldObserverMode;
	EHANDLE				m_hOldObserverTarget;

	int					m_iOldHealth;

	// Look At
	/*
	int m_headYawPoseParam;
	int m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;
	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;
	*/

public:

	CTDCPlayerShared m_Shared;

// Called by shared code.
public:

	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	CTDCPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkHandle( C_TDCItem, m_hItem );

	CNetworkHandle( C_TDCWeaponBase, m_hOffHandWeapon );
	int				m_nActiveWpnClip;
	int				m_nActiveWpnAmmo;

	CNewParticleEffect *m_pNemesisEffect;

	int				m_iSpawnCounter;

	int				m_nOldWaterLevel;
	float			m_flWaterEntryTime;
	bool			m_bWaterExitEffectActive;

	CMaterialReference	m_InvulnerableMaterial;

	// Burning
	float				m_flBurnEffectStartTime;
	float				m_flBurnRenewTime;

	EHANDLE					m_hFirstGib;
	CUtlVector<EHANDLE>		m_hSpawnedGibs;

	int				m_iOldTeam;
	int				m_iOldPlayerClass;

	EHANDLE			m_hOldActiveWeapon;

	bool			m_bArenaSpectator;
	float			m_flLastDamageTime;

	bool			m_bFlipViewModel;
	float			m_flViewModelFOV;
	Vector			m_vecViewModelOffset;
	int				m_nMoneyPacks;
	bool			m_bWasHoldingJump;
	float			m_flHeadshotFadeTime;
	bool			m_bIsPlayerADev;

	bool m_bUpdateAttachedModels;

	bool			m_bTyping;
	CNewParticleEffect *m_pTypingEffect;
	CNewParticleEffect *m_pVoiceEffect;

	CNewParticleEffect *m_pOverhealEffect;

	CountdownTimer m_blinkTimer;

	// Cosmetics
	CNetworkArray( CHandle< C_TDCWearable >, m_hWearables, TDC_WEARABLE_COUNT );

private:

	// TFBot stuff (mostly MvM related, therefore unused)
	bool m_bIsABot;
	//bool m_bIsMiniBoss;
	//bool m_nBotSkill;

	// Gibs.
	CUtlVector<breakmodel_t>	m_aGibs;

	C_TDCPlayer( const C_TDCPlayer & );
};

inline C_TDCPlayer* ToTDCPlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<C_TDCPlayer *>( pEntity );
}

inline const C_TDCPlayer* ToTDCPlayer( const C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<const C_TDCPlayer *>( pEntity );
}

C_TDCPlayer *GetLocalObservedPlayer( bool bFirstPerson );

#endif // C_TDC_PLAYER_H
