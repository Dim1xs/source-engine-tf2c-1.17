//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#ifndef TDC_PLAYER_H
#define TDC_PLAYER_H
#pragma once

#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "tdc_playeranimstate.h"
#include "tdc_shareddefs.h"
#include "tdc_player_shared.h"
#include "tdc_playerclass.h"

class CTDCPlayer;
class CTDCTeam;
class CTDCItem;
class CTDCWeaponBase;
class CCaptureFlag;
class CTDCPlayerEquip;
class CTDCWearable;

//=============================================================================
//
// Player State Information
//
class CPlayerStateInfo
{
public:

	int				m_nPlayerState;
	const char		*m_pStateName;

	// Enter/Leave state.
	void ( CTDCPlayer::*pfnEnterState )();	
	void ( CTDCPlayer::*pfnLeaveState )();

	// Think (called every frame).
	void ( CTDCPlayer::*pfnThink )();
};

struct DamagerHistory_t
{
	DamagerHistory_t()
	{
		Reset();
	}
	void Reset()
	{
		hDamager = NULL;
		flTimeDamage = 0;
		hWeapon = NULL;
	}
	EHANDLE hDamager;
	float	flTimeDamage;
	EHANDLE hWeapon;
};
#define MAX_DAMAGER_HISTORY 2

//=============================================================================
//
// TF Player
//
class CTDCPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CTDCPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTDCPlayer();
	~CTDCPlayer();

	friend class CTDCGameMovement;

	// Creation/Destruction.
	static CTDCPlayer	*CreatePlayer( const char *className, edict_t *ed );

	virtual void		Spawn();
	virtual int			ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void		ForceRespawn();
	virtual CBaseEntity	*EntSelectSpawnPoint( void );
	virtual void		InitialSpawn();
	virtual void		Precache();
	virtual bool		IsReadyToPlay( void );
	virtual bool		IsReadyToSpawn( void );
	virtual bool		ShouldGainInstantSpawn( void );
	virtual float		GetCurSpeed(void) { return GetAbsVelocity().Length2D(); }
	virtual void		ResetPerRoundStats( void );
	virtual void		ResetScores( void );
	void				RemoveNemesisRelationships( bool bTeammatesOnly = false );
	virtual void		PlayerUse( void );

	void				CreateViewModel( int iViewModel = 0 );
	CBaseViewModel		*GetOffHandViewModel();
	void				SendOffHandViewModelActivity( Activity activity );

	virtual void		ImpulseCommands( void );
	virtual void		CheatImpulseCommands( int iImpulse );

	virtual void		CommitSuicide( bool bExplode = false, bool bForce = false );

	// Combats
	virtual void		TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);
	virtual int			TakeHealth( float flHealth, int bitsDamageType );
	virtual	void		Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	void				CleanupOnDeath( bool bDropItems = false );
	virtual bool		Event_Gibbed( const CTakeDamageInfo &info );
	virtual bool		BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	virtual void		PlayerDeathThink( void );

	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void				ApplyPushFromDamage( const CTakeDamageInfo &info, const Vector &vecDir );
	void				SetBlastJumpState( int iJumpType, bool bPlaySound );
	void				ClearBlastJumpState( void );
	int					GetBlastJumpFlags( void ) { return m_nBlastJumpFlags; }
	void				AddDamagerToHistory( EHANDLE hDamager, CTDCWeaponBase *pWeapon = NULL );
	void				ClearDamagerHistory();
	DamagerHistory_t	&GetDamagerHistory( int i ) { return m_DamagerHistory[i]; }
	virtual void		DamageEffect(float flDamage, int fDamageType);
	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	CTDCWeaponBase		*GetActiveTFWeapon( void ) const;
	bool				IsActiveTFWeapon( ETDCWeaponID iWeaponID ) const;

	CBaseEntity			*GiveNamedItem( const char *pszName, int iSubType = 0, int iAmmo = TDC_GIVEAMMO_NONE );

	void				NoteWeaponFired( CTDCWeaponBase *pWeapon );

	virtual void		OnMyWeaponFired( CBaseCombatWeapon *weapon );

	bool				HasItem( void ) const;					// Currently can have only one item at a time.
	void				SetItem( CTDCItem *pItem );
	CTDCItem				*GetItem( void ) const;
	CCaptureFlag		*GetTheFlag( void ) const;

	void				Restock( bool bHealth, bool bAmmo );
	float				GetNextRegenTime( void ){ return m_flNextRegenerateTime; }
	void				SetNextRegenTime( float flTime ){ m_flNextRegenerateTime = flTime; }
	void				SetRegenerating( bool bRegenerate ) { m_bRegenerating = bRegenerate; }

	virtual	void		RemoveAllItems( bool removeSuit );
	virtual void		RemoveAllWeapons( void );
	void				RemoveAllWearables( void );

	void				DropFlag( void );

	// Class.
	CTDCPlayerClass			*GetPlayerClass( void ) 				{ return &m_PlayerClass; }
	const CTDCPlayerClass	*GetPlayerClass( void ) const			{ return &m_PlayerClass; }
	int					m_iDesiredPlayerClass;

	// Team.
	void				ForceChangeTeam( int iTeamNum, bool bForceRespawn = false );
	virtual void		ChangeTeam( int iTeamNum, bool bAutoTeam = false, bool bSilent = false );

	// mp_fadetoblack
	void				HandleFadeToBlack( void );

	// Flashlight controls for SFM - JasonM
	virtual int			FlashlightIsOn( void );
	virtual void		FlashlightTurnOn( void );
	virtual void		FlashlightTurnOff( void );

	void				SprintPower_Update( void );

	// Think.
	virtual void		PreThink();
	virtual void		PostThink();
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );

	virtual void		ItemPostFrame();
	virtual void		HandleAnimEvent( animevent_t *pEvent );
	virtual void		Weapon_FrameUpdate( void );
	virtual void		Weapon_HandleAnimEvent( animevent_t *pEvent );
	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	virtual void		UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual void		OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume );

	// Utility.
	void				UpdateModel( void );
	void				UpdateSkin( int iTeam );

	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource ammosource );
	virtual void		RemoveAmmo( int iCount, int iAmmoIndex );
	virtual int			GetAmmoCount( int iAmmoIndex ) const;
	int					GetMaxAmmo( int iAmmoIndex, bool bAddMissingClip = false ) const;
	bool				HasInfiniteAmmo( void ) const;

	bool				CanAttack( void );

	// This passes the event to the client's and server's CPlayerAnimState.
	void				DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0 );

	virtual bool		ClientCommand( const CCommand &args );

	int					BuildObservableEntityList( void );
	virtual int			GetNextObserverSearchStartPoint( bool bReverse ); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual CBaseEntity *FindNextObserverTarget( bool bReverse );
	virtual bool		IsValidObserverTarget( CBaseEntity * target ); // true, if player is allowed to see this target
	virtual bool		SetObserverTarget( CBaseEntity * target );
	virtual bool		ModeWantsSpectatorGUI( int iMode ) { return ( iMode != OBS_MODE_FREEZECAM && iMode != OBS_MODE_DEATHCAM ); }
	void				FindInitialObserverTarget( void );
	CBaseEntity		    *FindNearestObservableTarget( Vector vecOrigin, float flMaxDist );
	virtual void		ValidateCurrentObserverTarget( void );

	virtual unsigned int PlayerSolidMask( bool brushOnly = false ) const;

	void CheckUncoveringSpies( CTDCPlayer *pTouchedPlayer );
	virtual void StartTouch( CBaseEntity* pOther );
	void Touch( CBaseEntity *pOther );

	void TeamFortress_SetSpeed();

	void TeamFortress_ClientDisconnected();
	void RemoveAllOwnedEntitiesFromWorld( bool bSilent = true );
	void RemoveOwnedProjectiles( void );
		
	void SetAnimation( PLAYER_ANIM playerAnim );

	bool IsPlayerClass( int iClass ) const;
	bool IsNormalClass( void ) const;
	bool IsZombie( void ) const;
	bool IsVIP(void) const;

	void PlayFlinch( const CTakeDamageInfo &info );

	float PlayCritReceivedSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );

	// TF doesn't want the explosion ringing sound
	virtual void			OnDamagedByExplosion( const CTakeDamageInfo &info ) { return; }

	void	OnBurnOther( CTDCPlayer *pTFPlayerVictim );

	CTDCTeam *GetTDCTeam( void ) const;

	void TeleportEffect( int iTeam );
	bool IsAllowedToPickUpFlag( void ) const;
	bool HasTheFlag( void ) const;

	// Death & Ragdolls.
	virtual void CreateRagdollEntity( void );
	void CreateRagdollEntity( int nFlags, float flInvisLevel, ETDCDmgCustom iDamageCustom );
	void DestroyRagdoll( void );
	void StopRagdollDeathAnim( void );
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
	virtual bool ShouldGib( const CTakeDamageInfo &info );

	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );

	// Dropping Ammo
	void DropHealthPack( void );
	bool DropWeapon( CTDCWeaponBase *pWeapon, bool bKilled = false, bool bDissolve = false );
	void DropPowerups( ETDCPowerupDropStyle dropStyle );

	float GetLastDamageTime( void ) { return m_flLastDamageTime; }

	bool ShouldAutoRezoom( void ) { return m_bAutoRezoom; }
	void SetAutoRezoom( bool bAutoRezoom ) { m_bAutoRezoom = bAutoRezoom; }

	bool ShouldFlipViewModel( void ) { return m_bFlipViewModel; }
	void SetFlipViewModel( bool bFlip ) { m_bFlipViewModel = bFlip; }

	float GetViewModelFOV( void ) { return m_flViewModelFOV; }
	void SetViewModelFOV( float flVal ) { m_flViewModelFOV = flVal; }

	const Vector &GetViewModelOffset( void ) { return m_vecViewModelOffset; }
	void SetViewModelOffset( const Vector &vecOffset ) { m_vecViewModelOffset = vecOffset; }

	bool ShouldHoldToZoom( void ) { return m_bHoldZoom; }
	void SetHoldToZoom( bool bEnable ) { m_bHoldZoom = bEnable; }

	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );

	virtual bool CanHearAndReadChatFrom( CBasePlayer *pPlayer );
	virtual bool CanBeAutobalanced( void );

	Vector 	GetClassEyeHeight( void );

	void	UpdateExpression( void );
	void	ClearExpression( void );

	virtual IResponseSystem *GetResponseSystem();
	virtual bool			SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	virtual bool CanSpeakVoiceCommand( void );
	virtual bool ShouldShowVoiceSubtitleToEnemy( void );
	virtual void NoteSpokeVoiceCommand( const char *pszScenePlayed );
	void	SpeakWeaponFire( int iCustomConcept = MP_CONCEPT_NONE );
	void	ClearWeaponFireScene( void );

	virtual int DrawDebugTextOverlays( void );

	virtual int	CalculateTeamBalanceScore( void );
	void CalculateTeamScrambleScore( void );
	float GetTeamScrambleScore( void ) { return m_flTeamScrambleScore; }

	bool ShouldAnnouceAchievement( void );

	void JumpSound( void );
	virtual bool IsDeflectable( void ) { return true; }

	int GetNumberOfDominations( void ) { return m_iDominations; }
	void UpdateDominationsCount( void );

	bool GetClientConVarBoolValue( const char *pszValue );
	int GetClientConVarIntValue( const char *pszValue );
	float GetClientConVarFloatValue( const char *pszValue );
	void UpdatePlayerColor( void );
	static void ClampPlayerColor( Vector &vecColor );

	void UpdatePlayerSkinTone( void );

	void	UpdateAnimState( void ) { m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] ); }

	bool	IsEnemy( const CBaseEntity *pEntity ) const;

	CTDCPlayer *GetObservedPlayer( bool bFirstPerson );
	virtual Vector Weapon_ShootPosition( void );
	void SetEyeAngles( const QAngle &angles );

	float GetSpectatorSwitchTime( void ) { return m_flSpectatorTime; }

	void AddMoneyPack( void ) { m_nMoneyPacks++; }
	int GetNumMoneyPacks( void ) { return m_nMoneyPacks; }
	void DrainMoneyPack( void ) { m_nMoneyPacks--; }

	bool IsPlayerDev( void ) { return m_bIsPlayerADev; }

	bool CanPickUpWeapon( CBaseEntity *pItem ) { return !m_bJustPickedWeapon; }
	void OnPickedUpWeapon( CBaseEntity *pItem );

	void AddWearable( CTDCWearable *pWearable );
	void RemoveWearable( CTDCWearable *pWearable );

	int GetItemPreset( int iClass, ETDCWearableSlot iSlot ) { return m_ItemPreset[iClass][iSlot]; }
	void SetItemPreset( int iClass, ETDCWearableSlot iSlot, int iItemID ) { m_ItemPreset[iClass][iSlot] = iItemID; }

	// Entity inputs
	void	InputIgnitePlayer( inputdata_t &inputdata );
	void	InputExtinguishPlayer( inputdata_t &inputdata );
	void	InputSpeakResponseConcept( inputdata_t &inputdata );

	float HealthFractionBuffed() const;

public:
	CNetworkVector( m_vecPlayerColor );

	CNetworkVector( m_vecPlayerSkinTone );

	CTDCPlayerShared m_Shared;

	float				m_flNextNameChangeTime;
	float				m_flNextTimeCheck;		// Next time the player can execute a "timeleft" command

	int					StateGet( void ) const;

	float				GetSpawnTime() { return m_flSpawnTime; }
	float				RespawnRequiresAction() { return m_bRespawnRequiresAction; }

	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual void		Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity );

	void				GiveDefaultItems();
	void				SetEquipEntity( CTDCPlayerEquip *pEquipEnt );
	void				ManageRegularWeapons( TDCPlayerClassData_t *pData );
	ETDCWeaponID			GetLoadoutWeapon( int iSlot );
	void				RecalculatePlayerBodygroups( void );
	bool				CanWearCosmetics( void );

	// Taunts.
	bool				IsAllowedToTaunt( void );
	void				Taunt( void );
	void				StopTaunt( void );
	void				PlayTauntScene( const char *pszScene );
	void				OnTauntSucceeded( const char *pszScene );
	bool				IsTaunting( void ) { return m_Shared.InCond( TDC_COND_TAUNTING ); }

	virtual float		PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
	virtual bool		StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	virtual	bool		ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	void				SetDeathFlags( int iDeathFlags ) { m_iDeathFlags = iDeathFlags; }
	int					GetDeathFlags() { return m_iDeathFlags; }
	
	void				CheckForIdle( void );
	void				PickWelcomeObserverPoint();

	void				StopRandomExpressions( void ) { m_flNextRandomExpressionTime = -1; }
	void				StartRandomExpressions( void ) { m_flNextRandomExpressionTime = gpGlobals->curtime; }

	virtual bool		WantsLagCompensationOnEntity( const CBasePlayer	*pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

	CTDCWeaponBase		*GetTDCWeapon( int iWeapon ) const { return static_cast<CTDCWeaponBase *>( GetWeapon( iWeapon ) ); }
	CTDCWeaponBase		*GetTDCWeaponBySlot( int iBucket ) const { return static_cast<CTDCWeaponBase *>( Weapon_GetSlot( iBucket ) ); }
	CTDCWeaponBase		*Weapon_OwnsThisID( ETDCWeaponID iWeaponID ) const;

	bool				CalculateAmmoPackPositionAndAngles( CTDCWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles );

	bool				SelectFurthestSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot, bool bTelefrag = true );

	float				GetTimeSinceLastInjuryByAnyEnemyTeam() const;

	bool				IsArenaSpectator() const { return m_bArenaSpectator; }

	bool				IsBasicBot() const  { return m_bIsBasicBot; }
	void				SetAsBasicBot()     { m_bIsBasicBot = true; }

private:
	// Creation/Destruction.
	void				InitClass( void );
	bool				SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot );
	void				PrecachePlayerModels( void );

	// Think.
	void				TFPlayerThink();
	void				SlideThink();
	void				RegenThink();
	void				UpdateTimers( void );

	// Taunt.
	EHANDLE				m_hTauntScene;
	bool				m_bInitTaunt;

	// Client commands.
public:
	int					GetAutoTeam( void );
	
	void				HandleCommand_JoinTeam( const char *pTeamName );
	void				HandleCommand_JoinTeam_Duel( const char *pTeamName );
	void				HandleCommand_JoinTeam_Infected( const char *pTeamName );
	void				HandleCommand_JoinTeam_NoMenus( const char *pTeamName );
	void				HandleCommand_JoinTeam_NoKill( const char *pTeamName );
	int					GetSpawnClass( void );
	void				ShowTeamMenu( bool bShow = true );

	// Bots.
	friend void			Bot_Think( CTDCPlayer *pBot );

private:
	// Physics.
	void				PhysObjectSleep();
	void				PhysObjectWake();


	bool				m_bSprintEnabled;		// Used to disable sprint temporarily
	bool				m_bIsAutoSprinting;		// A proxy for holding down the sprint key.
	float				m_fAutoSprintMinTime;	// Minimum time to maintain autosprint regardless of player speed. 

	CNetworkVar( float, m_flSprintPower );
	CNetworkVar( float, m_flSprintPowerLastCheckTime );
	CNetworkVar( float, m_flSprintRegenStartTime );

	// Ammo pack.
	void DroppedWeaponCleanUp( void );

	// State.
	CPlayerStateInfo	*StateLookupInfo( int nState );
	void				StateEnter( int nState );
	void				StateLeave( void );
	void				StateTransition( int nState );
	void				StateEnterWELCOME( void );
	void				StateThinkWELCOME( void );
	void				StateEnterPICKINGTEAM( void );
	void				StateEnterACTIVE( void );
	void				StateEnterOBSERVER( void );
	void				StateThinkOBSERVER( void );
	void				StateEnterDYING( void );
	void				StateThinkDYING( void );

	virtual bool		SetObserverMode(int mode);
	virtual void		AttemptToExitFreezeCam( void );

	bool				PlayGesture( const char *pGestureName );
	bool				PlaySpecificSequence( const char *pSequenceName );

	bool				GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes );

private:
	// Networked.
	CNetworkVar( bool, m_bSaveMeParity );
	CNetworkQAngle( m_angEyeAngles );					// Copied from EyeAngles() so we can send it to the client.
	CNetworkVar( int, m_iSpawnCounter );
	CNetworkVar( float, m_flLastDamageTime );
	CNetworkVar( bool, m_bArenaSpectator );

	// Items.
	CNetworkHandle( CTDCItem, m_hItem );

	// Combat.
	CNetworkVar( int, m_nActiveWpnClip );
	CNetworkVar( int, m_nActiveWpnAmmo );

	CNetworkVar( bool, m_bTyping );
	CNetworkVar( int, m_nMoneyPacks );
	CNetworkVar( bool, m_bWasHoldingJump );
	CNetworkVar( float, m_flHeadshotFadeTime );
	CNetworkVar( bool, m_bIsPlayerADev );

	// TFBot stuff (mostly MvM related, basically just stubs)
	//CNetworkVar( bool, m_bIsMiniBoss );
	CNetworkVar( bool, m_bIsABot );
	//CNetworkVar( int, m_nBotSkill );

	// Non-networked.
	bool				m_bAbortFreezeCam;
	bool				m_bRegenerating;
	float				m_flNextRegenerateTime;

	// Ragdolls.
	Vector				m_vecTotalBulletForce;

	// State.
	CPlayerStateInfo	*m_pStateInfo;

	CTDCPlayerClass		m_PlayerClass;

	CTDCPlayerAnimState	*m_PlayerAnimState;

	float				m_flNextPainSoundTime;
	int					m_LastDamageType;
	int					m_iDeathFlags;				// TDC_DEATH_* flags with additional death info

	bool				m_bPlayedFreezeCamSound;

	float				m_flSpawnTime;
	bool				m_bRespawnRequiresAction;

	float				m_flLastAction;
	bool				m_bIsIdle;

	CUtlVector<EHANDLE>	m_hObservableEntities;
	DamagerHistory_t	m_DamagerHistory[MAX_DAMAGER_HISTORY];	// history of who has damaged this player

	// Background expressions
	string_t			m_iszExpressionScene;
	EHANDLE				m_hExpressionSceneEnt;
	float				m_flNextRandomExpressionTime;
	EHANDLE				m_hWeaponFireSceneEnt;
	float				m_flNextVoiceCommandTime;
	float				m_flNextSpeakWeaponFire;

	// ConVar settingss.
	bool				m_bAutoRezoom;	// does the player want to re-zoom after each shot for sniper rifles
	CNetworkVar( bool, m_bFlipViewModel );
	CNetworkVar( float, m_flViewModelFOV );
	CNetworkVector( m_vecViewModelOffset );
	bool				m_bHoldZoom;

	int					m_nBlastJumpFlags;
	bool				m_bBlastLaunched;
	bool				m_bJumpEffect;

	int					m_iDominations;

	float				m_flWaterExitTime;

	CountdownTimer		m_ctNavCombatUpdate;

	// TFBot stuff
	bool				m_bIsBasicBot; // true if this player is a bot (a basic bot, not a TFBot)

	CHandle<CTDCPlayerEquip>	m_hEquipEnt;
	bool				m_bJustPickedWeapon;
	int					m_ItemPreset[TDC_CLASS_COUNT_ALL][TDC_WEARABLE_COUNT];
	CNetworkArray( CHandle<CTDCWearable>, m_hWearables, TDC_WEARABLE_COUNT );

	float				m_flTeamScrambleScore;

	float				m_flSpectatorTime;

	COutputEvent		m_OnDeath;

public:
	bool				PlayerHasPowerplay( void );
};

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert an entity into a tf player.
//   Input: pEntity - the entity to convert into a player
//-----------------------------------------------------------------------------
inline CTDCPlayer *ToTDCPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<CTDCPlayer *>( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Const-qualified version of ToTDCPlayer.
//-----------------------------------------------------------------------------
inline const CTDCPlayer *ToTDCPlayer( const CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<const CTDCPlayer *>( pEntity );
}

inline int CTDCPlayer::StateGet( void ) const
{
	return m_Shared.m_nPlayerState;
}

#endif	// TDC_PLAYER_H
