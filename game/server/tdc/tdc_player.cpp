//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tdc_player.h"
#include "tdc_gamerules.h"
#include "tdc_gamestats.h"
#include "KeyValues.h"
#include "viewport_panel_names.h"
#include "client.h"
#include "team.h"
#include "tdc_weaponbase.h"
#include "tdc_client.h"
#include "tdc_team.h"
#include "tdc_viewmodel.h"
#include "tdc_item.h"
#include "in_buttons.h"
#include "entity_capture_flag.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "game.h"
#include "datacache/imdlcache.h"
#include "particle_parse.h"
#include "props_shared.h"
#include "filesystem.h"
#include "toolframework_server.h"
#include "IEffects.h"
#include "func_respawnroom.h"
#include "networkstringtable_gamedll.h"
#include "sceneentity.h"
#include "fmtstr.h"
#include "weapon_leverrifle.h"
#include "triggers.h"
#include "hl2orange.spa.h"
#include "tdc_fx_blood.h"
#include "activitylist.h"
#include "steam/steam_api.h"
#include "cdll_int.h"
#include "tdc_weaponbase.h"
#include "tdc_dropped_weapon.h"
#include "baseprojectile.h"
#include "tdc_powerupbase.h"
#include "eventlist.h"
#include "tdc_fx.h"
#include "animation.h"
#include "tdc_player_resource.h"
#include "entity_healthkit.h"
#include "voice_gamemgr.h"
#include "entity_player_equip.h"
#include "weapon_displacer.h"
#include "player_command.h"
#include "engine/IEngineSound.h"
#include "choreoscene.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "nav_area.h"
#include "tdc_bloodmoney.h"
#include "weapon_flamethrower.h"
#include "tdc_wearable.h"
#include "tdc_dev_list.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool IsInCommentaryMode( void );

EHANDLE g_pLastSpawnPoints[TDC_TEAM_COUNT];

ConVar tdc_playergib( "tdc_playergib", "1", FCVAR_NOTIFY, "Allow player gibbing. 0: never, 1: normal, 2: always", true, 0, true, 2 );
ConVar tdc_max_voice_speak_delay( "tdc_max_voice_speak_delay", "1.5", FCVAR_NOTIFY, "Max time after a voice command until player can do another one" );
ConVar tdc_allow_player_use( "tdc_allow_player_use", "0", FCVAR_NOTIFY, "Allow players to execute + use while playing." );

// Team Deathmatch Classic commands
ConVar tdc_spawnprotecttime( "tdc_spawnprotecttime", "3", FCVAR_NOTIFY, "Time (in seconds) that spawn protection lasts" );
ConVar tdc_bloodmoney_maxdrop( "tdc_bloodmoney_maxdrop", "5", FCVAR_NONE, "Maximum amount of money packs a player can drop on death. 0 - unlimited." );
ConVar tdc_player_powerup_allowdrop( "tdc_player_powerup_allowdrop", "0", FCVAR_NONE, "Allow players to drop their powerups." );
ConVar tdc_player_powerup_throwforce( "tdc_player_powerup_throwforce", "500", FCVAR_DEVELOPMENTONLY, "Force when the player 'throws' their powerup." );
ConVar tdc_player_powerup_explodeforce( "tdc_player_powerup_explodeforce", "200", FCVAR_DEVELOPMENTONLY, "Force when the player 'explodes' with a powerup." );
ConVar tdc_respawn_requires_action( "tdc_respawn_requires_action", "1", FCVAR_NONE, "Players are required to perform an action (+jump) to respawn." );
ConVar tdc_spec_ffa_allowplayers( "tdc_spec_ffa_allowplayers", "1", FCVAR_NOTIFY, "Allow spectating other players when dead in FFA modes." );
ConVar tdc_player_berserk_dmgresist( "tdc_player_berserk_dmgresist", "0.15", FCVAR_DEVELOPMENTONLY, "Damage resistance scaling while using beserk." );
ConVar tdc_allow_special_classes( "tdc_allow_special_classes", "0", FCVAR_CHEAT, "Enables gamemode specific classes (VIP, ZOMBIE) in normal gameplay.");

extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar sv_maxunlag;
extern ConVar tdc_gravetalk;
extern ConVar tdc_spectalk;

extern ConVar tdc_disablefreezecam;
extern ConVar mp_scrambleteams_mode;
extern ConVar tdc_headshoteffect_mindmg;
extern ConVar tdc_headshoteffect_maxdmg;
extern ConVar tdc_headshoteffect_mintime;
extern ConVar tdc_headshoteffect_maxtime;

extern ConVar tdc_player_restrict_class;

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
	{
		m_iPlayerIndex = 0;
	}

	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropInt( SENDINFO( m_iPlayerIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	// BUGBUG:  ywb  we assume this is either 0 or an animation sequence #, but it could also be an activity, which should fit within this limit, but we're not guaranteed.
	SendPropInt( SENDINFO( m_nData ), ANIMATION_SEQUENCE_BITS ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
    Vector vecEyePos = pPlayer->EyePosition();
	CPVSFilter filter( vecEyePos );
	filter.UsePredictionRules();

	Assert( pPlayer->entindex() >= 1 && pPlayer->entindex() <= MAX_PLAYERS );
	g_TEPlayerAnimEvent.m_iPlayerIndex = pPlayer->entindex();
	g_TEPlayerAnimEvent.m_iEvent = event;
	Assert( nData < (1<<ANIMATION_SEQUENCE_BITS) );
	Assert( (1<<ANIMATION_SEQUENCE_BITS) >= ActivityList_HighestIndex() );
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//=================================================================================
//
// Ragdoll Entity
//
//=================================================================================

class CTDCRagdoll : public CBaseAnimatingOverlay
{
public:

	DECLARE_CLASS( CTDCRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	CTDCRagdoll()
	{
		m_nRagdollFlags = 0;
		m_iDamageCustom = TDC_DMG_CUSTOM_NONE;
		m_vecRagdollOrigin.Init();
		m_vecRagdollVelocity.Init();
		UseClientSideAnimation();
	}

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar( int, m_nRagdollFlags );
	CNetworkVar( ETDCDmgCustom, m_iDamageCustom );
	CNetworkVar( int, m_iTeam );
	CNetworkVar( int, m_iClass );
};

LINK_ENTITY_TO_CLASS( tdc_ragdoll, CTDCRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTDCRagdoll, DT_TDCRagdoll )
	SendPropEHandle( SENDINFO( m_hOwnerEntity ) ),
	SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecForce ), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ), 13, SPROP_ROUNDDOWN, -2048.0f, 2048.0f ),
	SendPropInt( SENDINFO( m_nForceBone ) ),
	SendPropInt( SENDINFO( m_nRagdollFlags ) ),
	SendPropInt( SENDINFO( m_iDamageCustom ) ),
	SendPropInt( SENDINFO( m_iTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClass ), 4, SPROP_UNSIGNED ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: Filters updates to a variable so that only non-local players see
// the changes.  This is so we can send a low-res origin to non-local players
// while sending a hi-res one to the local player.
// Input  : *pVarData - 
//			*pOut - 
//			objectID - 
//-----------------------------------------------------------------------------

void* SendProxy_SendNonLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient( objectID - 1 );
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalDataTable );

BEGIN_DATADESC( CTDCPlayer )
	DEFINE_INPUTFUNC( FIELD_STRING,	"SpeakResponseConcept",	InputSpeakResponseConcept ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"IgnitePlayer",	InputIgnitePlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ExtinguishPlayer",	InputExtinguishPlayer ),
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
END_DATADESC()
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// specific to the local player
BEGIN_SEND_TABLE_NOBASE( CTDCPlayer, DT_TDCLocalPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropVector( SENDINFO( m_angEyeAngles ), 32, SPROP_CHANGES_OFTEN ),

	SendPropBool( SENDINFO( m_bArenaSpectator ) ),
	SendPropInt( SENDINFO( m_nMoneyPacks ), 9, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bWasHoldingJump ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropBool( SENDINFO( m_bIsPlayerADev ) ),

END_SEND_TABLE()

// all players except the local player
BEGIN_SEND_TABLE_NOBASE( CTDCPlayer, DT_TDCNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

	SendPropInt( SENDINFO( m_nActiveWpnClip ), 9, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nActiveWpnAmmo ), 11, SPROP_CHANGES_OFTEN ),

	SendPropBool( SENDINFO( m_bTyping ) ),

END_SEND_TABLE()

//============

LINK_ENTITY_TO_CLASS( player, CTDCPlayer );
PRECACHE_REGISTER( player );

IMPLEMENT_SERVERCLASS_ST( CTDCPlayer, DT_TDCPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_nModelIndex" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// cs_playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	SendPropBool( SENDINFO( m_bIsABot ) ),

	// This will create a race condition will the local player, but the data will be the same so.....
	SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),

	SendPropFloat( SENDINFO( m_flSprintPower ) ),
	SendPropFloat( SENDINFO( m_flSprintPowerLastCheckTime ) ),
	SendPropFloat( SENDINFO( m_flSprintRegenStartTime ) ),

	SendPropEHandle( SENDINFO( m_hItem ) ),

	SendPropVector( SENDINFO( m_vecPlayerColor ), 8, 0, 0.0f, 1.0f ),

	SendPropVector( SENDINFO( m_vecPlayerSkinTone ), 8, 0, 0.0f, 1.0f ),

	// Ragdoll.
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	SendPropDataTable( SENDINFO_DT( m_PlayerClass ), &REFERENCE_SEND_TABLE( DT_TDCPlayerClassShared ) ),
	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_TDCPlayerShared ) ),

	// Data that only gets sent to the local player
	SendPropDataTable( "tflocaldata", 0, &REFERENCE_SEND_TABLE( DT_TDCLocalPlayerExclusive ), SendProxy_SendLocalDataTable ),

	// Data that gets sent to all other players
	SendPropDataTable( "tfnonlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TDCNonLocalPlayerExclusive ), SendProxy_SendNonLocalDataTable ),

	SendPropInt( SENDINFO( m_iSpawnCounter ) ),
	SendPropTime( SENDINFO( m_flLastDamageTime ) ),
	SendPropTime( SENDINFO( m_flHeadshotFadeTime ) ),
	SendPropBool( SENDINFO( m_bFlipViewModel ) ),
	SendPropFloat( SENDINFO( m_flViewModelFOV ) ),
	SendPropVector( SENDINFO( m_vecViewModelOffset ) ),
	SendPropArray3
	(
		SENDINFO_ARRAY3( m_hWearables ),
		SendPropEHandle( SENDINFO_ARRAY( m_hWearables ) )
	),

END_SEND_TABLE()



// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}

ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCPlayer::CTDCPlayer()
{
	m_PlayerAnimState = CreateTFPlayerAnimState( this );

	m_hItem = NULL;
	m_hTauntScene = NULL;

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_pStateInfo = NULL;
	m_lifeState = LIFE_DEAD; // Start "dead".
	m_flNextNameChangeTime = 0;
	m_iDesiredPlayerClass = TDC_CLASS_GRUNT_NORMAL;

	m_flNextTimeCheck = gpGlobals->curtime;
	m_flSpawnTime = 0;
	m_bRespawnRequiresAction = true;

	SetViewOffset( Vector( 0, 0, 64 ) ); // To be overriden.

	m_Shared.Init( this );

	m_bAutoRezoom = false;

	m_vecPlayerColor.Init( 1.0f, 1.0f, 1.0f );

	m_vecPlayerSkinTone.Init( 1.0f, 1.0f, 1.0f );

	SetContextThink( &CTDCPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );

	ResetScores();

	m_flLastAction = gpGlobals->curtime;

	m_bInitTaunt = false;

	m_nBlastJumpFlags = 0;
	m_bBlastLaunched = false;
	m_bJumpEffect = false;

	m_iDominations = 0;

	m_nActiveWpnClip = -1;
	m_nActiveWpnAmmo = -1;

	m_bIsPlayerADev = false;

	m_flWaterExitTime = 0.0f;

	m_bIsBasicBot = false;

	m_bJustPickedWeapon = false;
	memset( m_ItemPreset, 0, TDC_CLASS_COUNT_ALL * TDC_WEARABLE_COUNT * sizeof( int ) );

	m_flTeamScrambleScore = 0.0f;

	m_bArenaSpectator = false;
	m_flSpectatorTime = 0.0f;

	m_flViewModelFOV = 54.0f;
	m_vecViewModelOffset.Init();

	m_flSprintPower = 100.0f;
	m_flSprintPowerLastCheckTime = 0.0f;
	m_flSprintRegenStartTime = 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::TFPlayerThink()
{
	if ( m_pStateInfo && m_pStateInfo->pfnThink )
	{
		(this->*m_pStateInfo->pfnThink)();
	}

	// Time to finish the current random expression? Or time to pick a new one?
	if ( IsAlive() && m_flNextRandomExpressionTime >= 0 && gpGlobals->curtime > m_flNextRandomExpressionTime )
	{
		// Random expressions need to be cleared, because they don't loop. So if we
		// pick the same one again, we want to restart it.
		ClearExpression();
		m_iszExpressionScene = NULL_STRING;
		UpdateExpression();
	}

	// Check to see if we are in the air and taunting.  Stop if so.
	if ( GetGroundEntity() == NULL && m_Shared.InCond( TDC_COND_TAUNTING ) )
	{
		if ( m_hTauntScene.Get() )
		{
			StopScriptedScene( this, m_hTauntScene );
			m_Shared.m_flTauntRemoveTime = 0.0f;
			m_hTauntScene = NULL;
		}
	}

	if ( GetPlayerClass()->GetClassIndex() == TDC_CLASS_GRUNT_HEAVY && !GameRules()->InRoundRestart() )
	{
		SprintPower_Update();
	}

	if ( GetPlayerClass()->GetClassIndex() == TDC_CLASS_GRUNT_LIGHT && GetCurSpeed() >= MaxSpeed() && ( m_nButtons & IN_DUCK ) && GetGroundEntity() && !m_Shared.InCond( TDC_COND_SLIDE ) )
	{
		//SetFriction(0.5);
		EmitSound("Player.SlideStart");
		m_surfaceFriction = 0.1f; // This is actually 0.5
		m_Shared.AddCond( TDC_COND_SLIDE );
		SetThink( &CTDCPlayer::SlideThink );
		SetNextThink( gpGlobals->curtime );
	}

	SetContextThink( &CTDCPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );
}

void CTDCPlayer::SlideThink( void )
{
	if ( GetCurSpeed() >= 290 && ( m_nButtons & IN_DUCK ) && GetGroundEntity() && !( m_nButtons & IN_JUMP ) && !( m_nButtons & IN_BACK ) )
	{
		m_surfaceFriction = 0.1f;
		SetNextThink( gpGlobals->curtime );
	}
	else
	{
		m_Shared.RemoveCond( TDC_COND_SLIDE );
		SetNextThink( -1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::RegenThink( void )
{
	if ( IsAlive() )
	{
		if ( IsZombie() )
		{
			float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageTime();

			if ( flTimeSinceDamage >= 5.0f )
			{
				float flHealAmount = RemapValClamped( flTimeSinceDamage, 5, 10, 3, 6 );
				TakeHealth( flHealAmount, DMG_GENERIC );
			}
		}

		// Degenerate 1 HP/s if above max health.
		if ( GetHealth() > GetMaxHealth() )
		{
			m_iHealth--;
		}
	}

	SetContextThink( &CTDCPlayer::RegenThink, gpGlobals->curtime + 1.0f, "RegenThink" );
}

CTDCPlayer::~CTDCPlayer()
{
	DestroyRagdoll();
	m_PlayerAnimState->Release();
}


CTDCPlayer *CTDCPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CTDCPlayer::s_PlayerEdict = ed;
	return (CTDCPlayer*)CreateEntityByName( className );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdateTimers( void )
{
	m_Shared.InvisibilityThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PreThink()
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;
	if ( buttonsChanged & ( IN_SPEED | IN_DUCK ) || m_Shared.InCond( TDC_COND_SPRINT ) )
	{
		TeamFortress_SetSpeed();
	}

	if ( !( m_nButtons & IN_USE ) )
	{
		m_bJustPickedWeapon = false;
	}

	// Riding a vehicle?
	if ( IsInAVehicle() )
	{
		// Update timers.
		UpdateTimers();

		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();
		CheckTimeBasedDamage();

		WaterMove();

		m_vecTotalBulletForce = vec3_origin;

		CheckForIdle();
		return;
	}

	// Pass through to the base class think.
	BaseClass::PreThink();

	UpdateTimers();

#if 0
	if ( m_nButtons & IN_GRENADE1 )
	{
		TDCPlayerClassData_t *pData = m_PlayerClass.GetData();
		CTDCWeaponBase *pGrenade = Weapon_OwnsThisID( pData->m_aGrenades[0] );
		if ( pGrenade )
		{
			pGrenade->Deploy();
		}
	}
#endif

	// Reset bullet force accumulator, only lasts one frame, for ragdoll forces from multiple shots.
	m_vecTotalBulletForce = vec3_origin;

	CheckForIdle();
}

ConVar mp_idledealmethod( "mp_idledealmethod", "1", 0, "Deals with Idle Players. 1 = Sends them into Spectator mode then kicks them if they're still idle, 2 = Kicks them out of the game.", true, 0, true, 2 );
ConVar mp_idlemaxtime( "mp_idlemaxtime", "3", 0, "Maximum time a player is allowed to be idle (in minutes)." );

ConVar tdc_player_sprint_duration( "tdc_player_sprint_drain", "1.33", 0, "Sets how many seconds the player can sprint for" );
ConVar tdc_player_sprint_regen_duration( "tdc_player_sprint_regenerate", "1.33", 0, "Sets how many seconds until full regenartion");
ConVar tdc_player_sprint_regen_delay( "tdc_player_sprint_regen_delay", "0.5", 0, "Sets delay before sprint starts regenerating" );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float GetSprintPowerChange(float flStartTime, float flEndTime, float flDuration)
{
	return (flEndTime - flStartTime) / flDuration * 100.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::SprintPower_Update( void )
{
	if ( m_Shared.InCond( TDC_COND_SPRINT ) )
	{
		if ( ( m_nButtons & IN_ATTACK3 ) == false ||
			m_flSprintPower <= 0.0f ||
			IsDucked() ||
			GetWaterLevel() == WL_Eyes)
		{
			m_flSprintRegenStartTime = gpGlobals->curtime + tdc_player_sprint_regen_delay.GetFloat();

			if ( m_flSprintPower <= 0.0f)
			{
				m_flSprintPower = 0.0f;
			}

			// Reset to 0 since there's a delay before regen starts
			m_flSprintPowerLastCheckTime = 0.0f;
			m_Shared.RemoveCond(TDC_COND_SPRINT);
		}
		else
		{
			float flPowerUsed = GetSprintPowerChange( m_flSprintPowerLastCheckTime, gpGlobals->curtime, tdc_player_sprint_duration.GetFloat() );
			m_flSprintPower -= flPowerUsed;
			m_flSprintPowerLastCheckTime = gpGlobals->curtime;
		}
	}
	else
	{
		if ( ( m_nButtons & IN_ATTACK3 ) == false )
		{
			if (m_flSprintPower != 100.0f && gpGlobals->curtime > m_flSprintRegenStartTime )
			{
				if ( m_flSprintPowerLastCheckTime != 0.0f )
				{
					float flPowerGained = GetSprintPowerChange( m_flSprintPowerLastCheckTime, gpGlobals->curtime, tdc_player_sprint_regen_duration.GetFloat() );
					m_flSprintPower += flPowerGained;
				}
				m_flSprintPowerLastCheckTime = gpGlobals->curtime;

				if (m_flSprintPower >= 100.0f)
				{
					m_flSprintPower = 100.0f;
				}
			}
		}
		else if ( m_flSprintPower > 0.0f &&
			!IsDucked() &&
			GetWaterLevel() < WL_Eyes )
		{
			m_flSprintPowerLastCheckTime = gpGlobals->curtime;
			m_Shared.AddCond( TDC_COND_SPRINT );
		}
	}
}

void CTDCPlayer::CheckForIdle( void )
{
	if ( m_afButtonLast != m_nButtons )
		m_flLastAction = gpGlobals->curtime;

	if ( mp_idledealmethod.GetInt() )
	{
		if ( IsHLTV() )
			return;

		if ( IsFakeClient() )
			return;

		//Don't mess with the host on a listen server (probably one of us debugging something)
		if ( engine->IsDedicatedServer() == false && entindex() == 1 )
			return;

		if ( m_bIsIdle == false )
		{
			if ( StateGet() != TDC_STATE_ACTIVE )
				return;
		}

		float flIdleTime = mp_idlemaxtime.GetFloat() * 60;

		if ( TDCGameRules()->InStalemate() )
		{
			flIdleTime = mp_stalemate_timelimit.GetInt() * 0.5f;
		}

		if ( ( gpGlobals->curtime - m_flLastAction ) > flIdleTime )
		{
			bool bKickPlayer = false;

			ConVarRef mp_allowspectators( "mp_allowspectators" );
			if ( mp_allowspectators.IsValid() && ( mp_allowspectators.GetBool() == false ) )
			{
				// just kick the player if this server doesn't allow spectators
				bKickPlayer = true;
			}
			else if ( mp_idledealmethod.GetInt() == 1 )
			{
				//First send them into spectator mode then kick him.
				if ( m_bIsIdle == false )
				{
					ChangeTeam( TEAM_SPECTATOR, false, true );
					m_flLastAction = gpGlobals->curtime;
					m_bIsIdle = true;
					return;
				}
				else
				{
					bKickPlayer = true;
				}
			}
			else if ( mp_idledealmethod.GetInt() == 2 )
			{
				bKickPlayer = true;
			}

			if ( bKickPlayer == true )
			{
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_idle_kick", GetPlayerName() );
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", GetUserID(), "#TDC_Idle_kicked" ) );
				m_flLastAction = gpGlobals->curtime;
			}
		}
	}
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CTDCPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTDCPlayer::FlashlightTurnOn( void )
{
	if ( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTDCPlayer::FlashlightTurnOff( void )
{
	if ( IsEffectActive( EF_DIMLIGHT ) )
	{
		RemoveEffects( EF_DIMLIGHT );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PostThink()
{
	BaseClass::PostThink();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	ProcessSceneEvents();

	// Add rocket trail if we haven't already.
	if ( !m_bJumpEffect && ( m_nBlastJumpFlags & ( TDC_JUMP_ROCKET | TDC_JUMP_GRENADE ) ) && IsAlive() )
	{
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, this, "foot_L" );
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, this, "foot_R" );
		m_bJumpEffect = true;
	}

	CBaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( pWeapon && pWeapon->UsesPrimaryAmmo() )
	{
		m_nActiveWpnClip = pWeapon->Clip1();
		m_nActiveWpnAmmo = GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
	}
	else
	{
		m_nActiveWpnClip = -1;
		m_nActiveWpnAmmo = -1;
	}

	// Check if player is typing.
	m_bTyping = ( m_nButtons & IN_TYPING ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	m_touchedPhysObject = false;

	// Zero out roll on view angles, it should always be zero under normal conditions and hacking it messes up movement (speedhacks).
	ucmd->viewangles[ROLL] = 0.0f;

	// Handle FL_FROZEN.
	if ( GetFlags() & FL_FROZEN )
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
		ucmd->buttons = 0;
		ucmd->impulse = 0;
		VectorCopy( pl.v_angle, ucmd->viewangles );
	}
	else if ( m_Shared.IsMovementLocked() )
	{
		// Don't allow player to perform any actions while taunting or stunned.
		// Not preventing movement since some taunts have special movement which needs to be handled in CTDCGameMovement.
		// This is duplicated on client side in C_TDCPlayer::PhysicsSimulate.
		ucmd->buttons = 0;
		ucmd->weaponselect = 0;
		ucmd->weaponsubtype = 0;

		// Don't allow the player to turn around.
		VectorCopy( pl.v_angle, ucmd->viewangles );
	}
	
	PlayerMove()->RunCommand( this, ucmd, moveHelper );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Precache()
{
	// Precache the player models and gibs.
	PrecachePlayerModels();

	// Precache the player sounds.
	PrecacheScriptSound( "Player.Spawn" );
	PrecacheScriptSound( "Player.CritHit" );
	PrecacheScriptSound( "Player.FreezeCam" );
	PrecacheScriptSound( "Player.Drown" );
	PrecacheScriptSound( "Player.Jump" );
	PrecacheScriptSound( "PowerupSpeedBoost.WearOff" );
	PrecacheScriptSound( "Player.Disconnect" );
	PrecacheScriptSound( "Player.HeadshotImpactAttacker" );
	PrecacheScriptSound( "Player.HeadshotImpactVictim" );
	PrecacheScriptSound( "Player.HeadshotImpactOther" );
	PrecacheScriptSound( "Player.Stomp" );
	PrecacheScriptSound( "BurnSpeedBoost.Start" );
	PrecacheScriptSound( "BurnSpeedBoost.WearOff" );
	PrecacheScriptSound( "BurnSpeedBoost.Extinguish" );
	PrecacheScriptSound( "Player.SlideStart" );

	// Precache particle systems
	PrecacheParticleSystem( "crit_text" );
	PrecacheParticleSystem( "headshot_text" );
	PrecacheParticleSystem( "speech_typing" );
	PrecacheParticleSystem( "speech_voice" );
	PrecacheTeamParticles( "player_recent_teleport_%s", true );
	PrecacheTeamParticles( "particle_nemesis_%s", true );
	PrecacheTeamParticles( "burningplayer_%s", true );
	PrecacheParticleSystem( "burningplayer_corpse" );
	PrecacheParticleSystem( "burninggibs" );
	PrecacheTeamParticles( "critgun_weaponmodel_%s", true, g_aTeamNamesShort );
	PrecacheTeamParticles( "healthlost_%s", true, g_aTeamNamesShort );
	PrecacheTeamParticles( "healthgained_%s", true, g_aTeamNamesShort );
	PrecacheTeamParticles( "healthgained_%s_large", true, g_aTeamNamesShort );
	PrecacheTeamParticles( "overhealedplayer_%s_pluses", true );
	PrecacheParticleSystem( "blood_spray_red_01" );
	PrecacheParticleSystem( "blood_spray_red_01_far" );
	PrecacheParticleSystem( "water_blood_impact_red_01" );
	PrecacheParticleSystem( "blood_impact_red_01" );
	PrecacheParticleSystem( "blood_headshot" );
	PrecacheParticleSystem( "water_playerdive" );
	PrecacheParticleSystem( "water_playeremerge" );
	PrecacheParticleSystem( "rocketjump_smoke" );
	PrecacheParticleSystem( "speed_boost_trail" );
	PrecacheParticleSystem( "player_poof" );
	PrecacheParticleSystem( "dm_respawn_13" );
					 
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Precache the player models and player model gibs.
//-----------------------------------------------------------------------------
void CTDCPlayer::PrecachePlayerModels( void )
{
	int i;
	for ( i = 0; i < TDC_CLASS_COUNT_ALL; i++ )
	{
		const char *pszModel = GetPlayerClassData( i )->m_szModelName;
		if ( pszModel[0] )
		{
			int iModel = PrecacheModel( pszModel );
			PrecacheGibsForModel( iModel );
		}

		if ( !IsX360() )
		{
			// Precache the hardware facial morphed models as well.
			const char *pszHWMModel = GetPlayerClassData( i )->m_szHWMModelName;
			if ( pszHWMModel[0] )
			{
				PrecacheModel( pszHWMModel );
			}
		}

		const char *pszHandModel = GetPlayerClassData( i )->m_szModelHandsName;
		if ( pszHandModel[0] )
		{
			PrecacheModel( pszHandModel );
		}
	}

	// Precache player class sounds
	for ( i = TDC_FIRST_NORMAL_CLASS; i < TDC_CLASS_COUNT_ALL; ++i )
	{
		TDCPlayerClassData_t *pData = GetPlayerClassData( i );

		PrecacheScriptSound( pData->m_szDeathSound );
		PrecacheScriptSound( pData->m_szCritDeathSound );
		PrecacheScriptSound( pData->m_szMeleeDeathSound );
		PrecacheScriptSound( pData->m_szExplosionDeathSound );
		PrecacheScriptSound( pData->m_szJumpSound );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsReadyToPlay( void )
{
	if ( m_iDesiredPlayerClass == TDC_CLASS_UNDEFINED )
		return false;

	if ( TDCGameRules()->IsInDuelMode() )
	{
		return ( !m_bArenaSpectator && GetTeamNumber() != TEAM_UNASSIGNED );
	}

	return ( GetTeamNumber() >= FIRST_GAME_TEAM );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsReadyToSpawn( void )
{
	//if ( RespawnRequiresAction() && tdc_respawn_requires_action.GetBool() )
	//{
		if (!(m_nButtons & IN_JUMP))
			return false;
		return true;
	//}
	//return ( StateGet() != TDC_STATE_DYING );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player should be allowed to instantly spawn
//			when they next finish picking a class.
//-----------------------------------------------------------------------------
bool CTDCPlayer::ShouldGainInstantSpawn( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores
//-----------------------------------------------------------------------------
void CTDCPlayer::ResetPerRoundStats( void )
{
	m_Shared.SetKillstreak( 0 );
	m_nMoneyPacks = 0;
	BaseClass::ResetPerRoundStats();
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores
//-----------------------------------------------------------------------------
void CTDCPlayer::ResetScores( void )
{
	CTDC_GameStats.ResetPlayerStats( this );
	RemoveNemesisRelationships();
	BaseClass::ResetScores();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	// Sets some default cvar values for bots.
	engine->SetFakeClientConVarValue( edict(), "cl_autorezoom", "1" );
	engine->SetFakeClientConVarValue( edict(), "cl_flipviewmodels", "0" );

	engine->SetFakeClientConVarValue( edict(), "fov_desired", "90" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_fov", "54" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_offset_x", "0" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_offset_y", "0" );
	engine->SetFakeClientConVarValue( edict(), "viewmodel_offset_z", "0" );

	engine->SetFakeClientConVarValue( edict(), "tdc_merc_color_r", "255" );
	engine->SetFakeClientConVarValue( edict(), "tdc_merc_color_g", "255" );
	engine->SetFakeClientConVarValue( edict(), "tdc_merc_color_b", "255" );
	engine->SetFakeClientConVarValue( edict(), "tdc_merc_winanim", "1" );

	engine->SetFakeClientConVarValue( edict(), "tdc_merc_skintone", "0" );

	engine->SetFakeClientConVarValue( edict(), "tdc_zoom_hold", "0" );
	engine->SetFakeClientConVarValue( edict(), "tdc_dev_mark", "1" );

	m_bIsPlayerADev = PlayerHasPowerplay();

	CTDC_GameStats.Event_MaxSentryKills( this, 0 );
	UpdatePlayerColor();
	UpdatePlayerSkinTone();

	StateEnter( TDC_STATE_WELCOME );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Spawn()
{
	MDLCACHE_CRITICAL_SECTION();

	m_bIsABot = IsBot();

	m_flSpawnTime = gpGlobals->curtime;
	m_bRespawnRequiresAction = true;
	UpdateModel();

	SetMoveType( MOVETYPE_WALK );
	BaseClass::Spawn();
	//This fixes platforms but breaks projectile collision.
	//We'll have to look into this another time.
	//SetCollisionGroup( COLLISION_GROUP_PLAYER_MOVEMENT );

	m_iSpawnCounter++;
	m_bAllowInstantSpawn = false;

	ClearDamagerHistory();

	m_flLastDamageTime = 0;

	m_flNextVoiceCommandTime = gpGlobals->curtime;

	ClearZoomOwner();
	SetFOV( this, 0 );

	SetViewOffset( GetClassEyeHeight() );

	ClearExpression();
	m_flNextSpeakWeaponFire = gpGlobals->curtime;

	m_bIsIdle = false;

	m_nBlastJumpFlags = 0;

	m_Shared.SetDesiredWeaponIndex( WEAPON_NONE );
	m_Shared.ResetDamageSourceType();
	m_Shared.SetWaitingToRespawn( TDCGameRules()->GetNextRespawnWave( GetTeamNumber(), this ) != 0.0f );

	m_nMoneyPacks = 0;

	m_flSprintPower = 100.0f;
	m_flSprintPowerLastCheckTime = 0.0f;
	m_flSprintRegenStartTime = 0.0f;

	// Since you must press Jump to respawn, set this flag so that the player doesn't jump immediately.
	m_bWasHoldingJump = true;

	// This makes the surrounding box always the same size as the standing collision box
	// helps with parts of the hitboxes that extend out of the crouching hitbox, eg with the
	// heavyweapons guy
	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	// Hack to hide the chat on the background map.
	if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		m_Local.m_iHideHUD |= HIDEHUD_CHAT;
	}

	// Kind of lame, but CBasePlayer::Spawn resets a lot of the state that we initially want on.
	// So if we're in the welcome state, call its enter function to reset 
	if ( m_Shared.InState( TDC_STATE_WELCOME ) )
	{
		StateEnterWELCOME();
	}

	// If they were dead, then they're respawning. Put them in the active state.
	if ( m_Shared.InState( TDC_STATE_DYING ) )
	{
		StateTransition( TDC_STATE_ACTIVE );
	}

	// If they're spawning into the world as fresh meat, give them items and stuff.
	if ( m_Shared.InState( TDC_STATE_ACTIVE ) )
	{
		if ( TDCGameRules()->State_Get() != GR_STATE_PREGAME )
		{
			EmitSound( "Player.Spawn" );

			CEffectData data;
			data.m_vOrigin = GetAbsOrigin();
			data.m_nEntIndex = entindex();
			data.m_bCustomColors = !TDCGameRules()->IsTeamplay();
			data.m_CustomColors.m_vecColor1 = m_vecPlayerColor;

			DispatchEffect( "SpawnEffect", data );
		}
		
		InitClass();
		m_Shared.RemoveAllCond(); // Remove conc'd, burning, rotting, hallucinating, etc.

		UpdateSkin( GetTeamNumber() );
		TeamFortress_SetSpeed();

		// Prevent firing for a second so players don't blow their faces off
		SetNextAttack( gpGlobals->curtime + 1.0 );

		DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
		m_Shared.AddCond( TDC_COND_INVULNERABLE_SPAWN_PROTECT, tdc_spawnprotecttime.GetFloat() );

		if ( IsInCommentaryMode() && !IsFakeClient() )
		{
			// Player is spawning in commentary mode. Tell the commentary system.
			CBaseEntity *pEnt = NULL;
			variant_t emptyVariant;
			while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "commentary_auto" )) != NULL )
			{
				pEnt->AcceptInput( "MultiplayerSpawned", this, this, emptyVariant, 0 );
			}
		}
	}

	CTDC_GameStats.Event_PlayerSpawned( this );

	IGameEvent *event = gameeventmanager->CreateEvent( "player_spawn" );

	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "class", GetPlayerClass()->GetClassIndex() );
		gameeventmanager->FireEvent( event );
	}
}

extern IVoiceGameMgrHelper *g_pVoiceGameMgrHelper;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCPlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( CBaseEntity::Instance( pInfo->m_pClientEnt ) );
	Assert( pPlayer );

	// Always transmit all players to spectators.
	if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return FL_EDICT_ALWAYS;

	bool bProximity;
	if ( g_pVoiceGameMgrHelper->CanPlayerHearPlayer( pPlayer, this, bProximity ) && bProximity )
		return FL_EDICT_ALWAYS;

	// Transmit last man standing.
	if ( m_Shared.InCond( TDC_COND_LASTSTANDING ) )
		return FL_EDICT_ALWAYS;

	if ( InSameTeam( pPlayer ) )
	{
		// Transmit teammates to dead players.
		if ( !pPlayer->IsAlive() )
			return FL_EDICT_ALWAYS;

		// Transmit flag carrier.
		if ( HasTheFlag() )
			return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all nemesis relationships between this player and others
//-----------------------------------------------------------------------------
void CTDCPlayer::RemoveNemesisRelationships( bool bTeammatesOnly /*= false*/ )
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTDCPlayer *pTemp = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != this && ( !bTeammatesOnly || !IsEnemy( pTemp ) ) )
		{
			// set this player to be not dominating anyone else
			m_Shared.SetPlayerDominated( pTemp, false );

			// set no one else to be dominating this player
			pTemp->m_Shared.SetPlayerDominated( this, false );
			pTemp->UpdateDominationsCount();
		}
	}

	UpdateDominationsCount();

	// reset the matrix of who has killed whom with respect to this player
	CTDC_GameStats.ResetKillHistory( this, bTeammatesOnly );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::Restock( bool bHealth, bool bAmmo )
{
	if ( bHealth )
	{
		SetHealth( Max( GetHealth(), GetMaxHealth() ) );
		m_Shared.HealNegativeConds();
	}

	if ( bAmmo )
	{
		// Refill clip in all weapons.
		for ( int i = 0; i < WeaponCount(); i++ )
		{
			CBaseCombatWeapon *pWeapon = GetWeapon( i );
			if ( !pWeapon )
				continue;

			pWeapon->GiveDefaultAmmo();
		}

		for ( int i = TDC_AMMO_PRIMARY; i < TDC_AMMO_COUNT; i++ )
		{
			SetAmmoCount( GetMaxAmmo( i ), i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::InitClass( void )
{
	// Set initial health and armor based on class.
	SetMaxHealth( GetPlayerClass()->GetMaxHealth() );
	SetHealth( GetMaxHealth() );

	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	// Give default items for class.
	GiveDefaultItems();

	// Update player's color.
	UpdatePlayerColor();

	// Update player's skin tone.
	UpdatePlayerSkinTone();

	// Update player's bodygroups.
	RecalculatePlayerBodygroups();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::CreateViewModel( int iViewModel )
{
	Assert( iViewModel >= 0 && iViewModel < MAX_VIEWMODELS );

	if ( GetViewModel( iViewModel ) )
		return;

	CTDCViewModel *pViewModel = ( CTDCViewModel * )CreateEntityByName( "tdc_viewmodel" );
	if ( pViewModel )
	{
		pViewModel->SetAbsOrigin( GetAbsOrigin() );
		pViewModel->SetOwner( this );
		pViewModel->SetIndex( iViewModel );
		DispatchSpawn( pViewModel );
		pViewModel->FollowEntity( this, false );
		pViewModel->SetOwnerEntity( this );
		m_hViewModel.Set( iViewModel, pViewModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the view model for the player's off hand
//-----------------------------------------------------------------------------
CBaseViewModel *CTDCPlayer::GetOffHandViewModel()
{
	// off hand model is slot 1
	return GetViewModel( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends the specified animation activity to the off hand view model
//-----------------------------------------------------------------------------
void CTDCPlayer::SendOffHandViewModelActivity( Activity activity )
{
	CBaseViewModel *pViewModel = GetOffHandViewModel();
	if ( pViewModel )
	{
		int sequence = pViewModel->SelectWeightedSequence( activity );
		pViewModel->SendViewModelMatchingSequence( sequence );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::SetEquipEntity( CTDCPlayerEquip *pEquipEnt )
{
	m_hEquipEnt = pEquipEnt;
}

//-----------------------------------------------------------------------------
// Purpose: Set the player up with the default weapons, ammo, etc.
//-----------------------------------------------------------------------------
void CTDCPlayer::GiveDefaultItems()
{
	// Get the player class data.
	TDCPlayerClassData_t *pData = m_PlayerClass.GetData();

	RemoveAllAmmo();

	// Give ammo. Must be done before weapons, so weapons know the player has ammo for them.
	for ( int iAmmo = 0; iAmmo < TDC_AMMO_COUNT; ++iAmmo )
	{
		GiveAmmo( GetMaxAmmo( iAmmo ), iAmmo, true, TDC_AMMO_SOURCE_RESUPPLY );
	}

	// Give weapons.
	if ( !m_hEquipEnt )
	{
		// Check if there's an active player equip entity.
		for ( CTDCPlayerEquip *pEquipEnt : CTDCPlayerEquip::AutoList() )
		{
			if ( pEquipEnt->CanEquip( this, true ) )
			{
				SetEquipEntity( pEquipEnt );
				break;
			}
		}
	}

	ManageRegularWeapons( pData );
	SetEquipEntity( NULL );

	// Now that we've got weapons update our ammo counts since weapons may override max ammo.
	for ( int iAmmo = 0; iAmmo < TDC_AMMO_COUNT; ++iAmmo )
	{
		SetAmmoCount( GetMaxAmmo( iAmmo ), iAmmo );
	}

	if ( m_bRegenerating == false )
	{
		SetActiveWeapon( NULL );
		Weapon_Switch( Weapon_GetSlot( 0 ) );
		Weapon_SetLast( Weapon_GetSlot( 1 ) );
	}

	// We may have swapped away our current weapon at resupply locker.
	if ( GetActiveWeapon() == NULL )
		SwitchToNextBestWeapon( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ManageRegularWeapons( TDCPlayerClassData_t *pData )
{
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTDCWeaponBase *pWeapon = GetTDCWeapon( i );
		if ( !pWeapon )
			continue;

		if ( GetLoadoutWeapon( pWeapon->GetSlot() ) != pWeapon->GetWeaponID() )
		{
			// If it's not the weapon we're supposed to carry in this slot nuke it.
			pWeapon->UnEquip();
		}
		else
		{
			pWeapon->ChangeTeam( GetTeamNumber() );
			pWeapon->GiveDefaultAmmo();
			pWeapon->WeaponRegenerate();

			if ( m_bRegenerating == false )
			{
				pWeapon->WeaponReset();
			}
		}
	}

	for ( int iWeapon = 0; iWeapon < TDC_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		ETDCWeaponID iWeaponID = GetLoadoutWeapon( iWeapon );
		if ( iWeaponID == WEAPON_NONE )
			continue;

		if ( Weapon_OwnsThisID( iWeaponID ) )
			continue;

		const char *pszWeaponName = WeaponIdToClassname( iWeaponID );
		GiveNamedItem( pszWeaponName );
	}

	// NOTE: This doesn't account for multiple viewmodels
	CTDCViewModel *vm = static_cast<CTDCViewModel *>( GetViewModel() );

	if ( CanWearCosmetics() )
	{
		int iClass = GetPlayerClass()->GetClassIndex();

		bool bBlockedSlots[TDC_WEARABLE_COUNT] = {};

		for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		{
			// Get item ID we should carry in this slot.
			int iItemID = m_ItemPreset[iClass][i];
			WearableDef_t *pItemDef = g_TDCPlayerItems.GetWearable( iItemID );
			if ( pItemDef )
			{
				bBlockedSlots[i] |= ( pItemDef->additional_slots.Find( (ETDCWearableSlot)i ) != pItemDef->additional_slots.InvalidIndex() );
			}

			CTDCWearable *pWearable = m_hWearables[i];
			if ( pWearable )
			{
				if ( pWearable->GetItemID() != iItemID || bBlockedSlots[i] || !pItemDef )
				{
					// Wrong item, nuke it.
					UTIL_Remove( pWearable );
				}
				else
				{
					// Nothing to do here.
					pWearable->ChangeTeam( GetTeamNumber() );
					pWearable->RemoveEffects( EF_NODRAW );
					continue;
				}
			}

			if ( bBlockedSlots[i] || !pItemDef )
			{
				if ( vm )
				{
					vm->RemoveViewmodelWearable( (ETDCWearableSlot)i );
				}
				continue;
			}

			CTDCWearable::Create( iItemID, this );

			// NOTE: This doesn't account for multiple viewmodels	
			if ( vm )
			{
				vm->UpdateViewmodelWearable( (ETDCWearableSlot)i, pItemDef->iViewmodelIndex );
			}
		}
	}
	else
	{
		RemoveAllWearables();

		if ( vm )
		{
			for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
			{
				vm->RemoveViewmodelWearable( (ETDCWearableSlot)i );
			}
		}
	}

	if ( vm )
	{
		vm->UpdateArmsModel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ETDCWeaponID CTDCPlayer::GetLoadoutWeapon( int iSlot )
{
	// Allow tdc_player_equip to override the loadout.
	if ( m_hEquipEnt )
	{
		return m_hEquipEnt->GetWeapon( iSlot );
	}

	// Melee is per-class.
	if ( iSlot == 2 )
	{
		return GetPlayerClass()->GetData()->m_iMeleeWeapon;
	}

	// TODO: Move these to script files.
	static ETDCWeaponID aDeathmatchLoadout[TDC_PLAYER_WEAPON_COUNT] =
	{
		WEAPON_NONE,
		WEAPON_PISTOL,
		WEAPON_CROWBAR,
	};

	static ETDCWeaponID aCTDCLoadout[TDC_PLAYER_WEAPON_COUNT] =
	{
		WEAPON_ASSAULTRIFLE,
		WEAPON_STENGUN,
		WEAPON_CROWBAR,
	};

	static ETDCWeaponID aInfectedHumanLoadout[TDC_PLAYER_WEAPON_COUNT] =
	{
		WEAPON_HUNTINGSHOTGUN,
		WEAPON_PISTOL,
		WEAPON_CROWBAR,
	};

	static ETDCWeaponID aInfectedZombieLoadout[TDC_PLAYER_WEAPON_COUNT] =
	{
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_CLAWS,
	};

	static ETDCWeaponID aVIPLoadout[TDC_PLAYER_WEAPON_COUNT] =
	{
		WEAPON_NONE,
		WEAPON_NONE,
		WEAPON_UMBRELLA,
	};

	static ETDCWeaponID aInstagibLoadout[TDC_PLAYER_WEAPON_COUNT] =
	{
		WEAPON_LEVERRIFLE,
		WEAPON_NONE,
		WEAPON_CROWBAR,
	};

	if ( TDCGameRules()->IsInstagib() && IsNormalClass() )
	{
		return aInstagibLoadout[iSlot];
	}

	switch ( TDCGameRules()->GetGameType() )
	{
	case TDC_GAMETYPE_CTF:
	case TDC_GAMETYPE_ATTACK_DEFEND:
	case TDC_GAMETYPE_INVADE:
		return aCTDCLoadout[iSlot];
	case TDC_GAMETYPE_INFECTION:
		return IsZombie() ? aInfectedZombieLoadout[iSlot] : aInfectedHumanLoadout[iSlot];
	case TDC_GAMETYPE_VIP:
		return IsVIP() ? aVIPLoadout[iSlot] : aDeathmatchLoadout[iSlot];
	default:
		return aDeathmatchLoadout[iSlot];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::RecalculatePlayerBodygroups( void )
{
	// Reset all bodygroups to default.
	m_nBody = 0;

	if ( CanWearCosmetics() )
	{
		for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		{
			if ( m_hWearables[i] )
			{
				m_hWearables[i]->UpdatePlayerBodygroups();
			}
		}
	}
	else if ( IsZombie() )
	{
		// Random bodygroups for Zombies.
		for ( const ZombieBodygroup_t &group : g_TDCPlayerItems.GetZombieBodygroups() )
		{
			int iGroup = FindBodygroupByName( group.name );
			if ( iGroup >= 0 )
			{
				SetBodygroup( iGroup, RandomInt( 0, group.states - 1 ) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::CanWearCosmetics( void )
{
	return IsNormalClass();
	//return IsPlayerClass( TDC_CLASS_GRUNT_NORMAL );
}

//-----------------------------------------------------------------------------
// Purpose: Create and give the named item to the player, setting the item ID. Then return it.
//-----------------------------------------------------------------------------
CBaseEntity	*CTDCPlayer::GiveNamedItem( const char *pszName, int iSubType, int iAmmo /*= TDC_GIVEAMMO_NONE*/ )
{
	// If I already own this type don't create one
	if ( Weapon_OwnsThisType( pszName ) )
		return NULL;

	CBaseEntity *pEntity = CBaseEntity::CreateNoSpawn( pszName, GetAbsOrigin(), vec3_angle );
	
	if ( pEntity == NULL )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}

	pEntity->AddSpawnFlags( SF_NORESPAWN );
	DispatchSpawn( pEntity );
	pEntity->Activate();

	if ( pEntity->IsMarkedForDeletion() )
		return NULL;

	CTDCWeaponBase *pWeapon = dynamic_cast<CTDCWeaponBase *>( pEntity );

	if ( pWeapon )
	{
		pWeapon->SetSubType( iSubType );

		// Give ammo for this weapon if asked.
		if ( pWeapon->UsesPrimaryAmmo() )
		{
			if ( iAmmo == TDC_GIVEAMMO_MAX )
			{
				SetAmmoCount( GetMaxAmmo( pWeapon->GetPrimaryAmmoType() ), pWeapon->GetPrimaryAmmoType() );
			}
			else if ( iAmmo == TDC_GIVEAMMO_INITIAL )
			{
				SetAmmoCount( pWeapon->GetInitialAmmo(), pWeapon->GetPrimaryAmmoType() );
			}
			else if ( iAmmo != TDC_GIVEAMMO_NONE )
			{
				SetAmmoCount( iAmmo, pWeapon->GetPrimaryAmmoType() );
			}
		}

		pWeapon->GiveTo( this );

		if ( iAmmo == TDC_GIVEAMMO_MAX && pWeapon->UsesPrimaryAmmo() )
		{
			// If we want max ammo then update the ammo count once more so we actually get the proper amount.
			SetAmmoCount( GetMaxAmmo( pWeapon->GetPrimaryAmmoType() ), pWeapon->GetPrimaryAmmoType() );
		}
	}
	else
	{
		pEntity->Touch( this );
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Find a spawn point for the player.
//-----------------------------------------------------------------------------
CBaseEntity* CTDCPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot = g_pLastSpawnPoints[ GetTeamNumber() ];
	const char *pSpawnPointName = "";

	if ( GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		bool bSuccess = false;
		switch ( TDCGameRules()->GetGameType() )
		{
		case TDC_GAMETYPE_CTF:
		case TDC_GAMETYPE_ATTACK_DEFEND:
		case TDC_GAMETYPE_INVADE:
			pSpawnPointName = "info_player_teamspawn";
			bSuccess = SelectSpawnSpot( pSpawnPointName, pSpot );
			break;
		default:
			// Don't telefrag at round start. Since players respawn instantly in pre-round state if the map doesn't
			// have enough spawn points it will cause constant tele-fragging and massive lag spikes.
			pSpawnPointName = "info_player_teamspawn";
			bSuccess = SelectFurthestSpawnSpot( pSpawnPointName, pSpot, TDCGameRules()->State_Get() != GR_STATE_PREROUND );
			break;
		}

		if ( bSuccess )
		{
			g_pLastSpawnPoints[GetTeamNumber()] = pSpot;
		}
	}
	else
	{
		pSpot = CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	if ( !pSpot )
	{
		Warning( "PutClientInServer: no %s on level\n", pSpawnPointName );
		return CBaseEntity::Instance( INDEXENT(0) );
	}

	return pSpot;
} 

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayer::SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot )
{
	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( !pSpot )
	{
		// Since we're not searching from the start the first result can be NULL.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}

	if ( !pSpot )
	{
		// Still NULL? That means there're no spawn points at all, bail.
		return false;
	}

	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawnpoint that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	bool bIgnorePlayers = false;

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if( TDCGameRules()->IsSpawnPointValid( pSpot, this, bIgnorePlayers ) )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}

				// Found a valid spawn point.
				return true;
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

		if ( pSpot == pFirstSpot && !bIgnorePlayers )
		{
			// Loop through again, ignoring players
			bIgnorePlayers = true;
			pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
		}
	} 
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot ); 

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Special spawn point search function for DM.
//-----------------------------------------------------------------------------
bool CTDCPlayer::SelectFurthestSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot, bool bTelefrag /*= true*/ )
{
	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( !pSpot )
	{
		// Since we're not searching from the start the first result can be NULL.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}

	if ( !pSpot )
	{
		// Still NULL? That means there're no spawn points at all, bail.
		return false;
	}

	// Randomize the start spot in DM.
	for ( int i = random->RandomInt( 0, 4 ); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

	// Find spawn point that is furthest from all other players.
	CBaseEntity *pFirstSpot = pSpot;
	float flFurthest = 0.0f;
	CBaseEntity *pFurthest = NULL;
	bool bIgnorePlayers = !TDCGameRules()->IsTeamplay();

	do
	{
		if ( pSpot )
		{
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if ( TDCGameRules()->IsSpawnPointValid( pSpot, this, bIgnorePlayers ) )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}

				// Check distance from other players.
				bool bOtherPlayersPresent = false;
				float flClosestPlayerDist = FLT_MAX;
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer || pPlayer == this || !pPlayer->IsAlive() || !IsEnemy( pPlayer ) )
						continue;

					bOtherPlayersPresent = true;

					float flDistSqr = pPlayer->GetAbsOrigin().DistToSqr( pSpot->GetAbsOrigin() );
					if ( flDistSqr < flClosestPlayerDist )
					{
						flClosestPlayerDist = flDistSqr;
					}
				}

				// If there are no other players just pick the first valid spawn point.
				if ( !bOtherPlayersPresent )
				{
					pFurthest = pSpot;
					break;
				}

				// Avoid Displacer teleport destinations.
				bool bSpotTargeted = false;

				for ( CWeaponDisplacer *pDisplacer : CWeaponDisplacer::AutoList() )
				{
					CBaseEntity *pDestination = pDisplacer->GetTeleportSpot();
					if ( !pDestination )
						continue;

					if ( pDestination == pSpot && !bIgnorePlayers )
					{
						bSpotTargeted = true;
						break;
					}

					if ( !IsEnemy( pDisplacer ) )
						continue;

					float flDistSqr = pDestination->GetAbsOrigin().DistToSqr( pSpot->GetAbsOrigin() );
					if ( flDistSqr < flClosestPlayerDist )
					{
						flClosestPlayerDist = flDistSqr;
					}
				}

				if ( !bSpotTargeted && flClosestPlayerDist > flFurthest )
				{
					flFurthest = flClosestPlayerDist;
					pFurthest = pSpot;
				}
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

		if ( pSpot == pFirstSpot && !pFurthest && !bIgnorePlayers )
		{
			// Loop through again, ignoring players
			bIgnorePlayers = true;
			pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
		}
	}
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot );

	if ( pFurthest )
	{
		pSpot = pFurthest;

		if ( bTelefrag )
		{
			// Kill off anyone occupying this spot if it's somehow busy.
			CBaseEntity *pList[MAX_PLAYERS];
			Vector vecMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
			Vector vecMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;
			int count = UTIL_EntitiesInBox( pList, MAX_PLAYERS, vecMins, vecMaxs, FL_CLIENT );

			for ( int i = 0; i < count; i++ )
			{
				CBaseEntity *pEntity = pList[i];
				if ( pEntity != this && IsEnemy( pEntity ) )
				{
					CTakeDamageInfo info( this, this, 1000, DMG_CRUSH | DMG_ALWAYSGIB, TDC_DMG_TELEFRAG );
					pEntity->TakeDamage( info );
				}
			}
		}

		// Found a valid spawn point.
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PhysObjectSleep()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Sleep();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PhysObjectWake()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::GetAutoTeam( void )
{
	// Don't bother with all these shenanigans in FFA as there's only one team.
	if ( !TDCGameRules()->IsTeamplay() )
		return FIRST_GAME_TEAM;

	int iTeam = TEAM_SPECTATOR;

	int iLightestTeam = TEAM_UNASSIGNED, iHeaviestTeam = TEAM_UNASSIGNED;

	int iMostPlayers = 0;
	int iLeastPlayers = MAX_PLAYERS + 1;

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );

		int iNumPlayers = pTeam->GetNumPlayers();

		if ( iNumPlayers < iLeastPlayers )
		{
			iLeastPlayers = iNumPlayers;
			iLightestTeam = i;
		}

		if ( iNumPlayers > iMostPlayers )
		{
			iMostPlayers = iNumPlayers;
			iHeaviestTeam = i;
		}
	}

	if ( iLightestTeam == iHeaviestTeam )
	{
		// Teams are equal, just a pick a random team.
		iTeam = RandomInt( FIRST_GAME_TEAM, GetNumberOfTeams() - 1 );
	}
	else
	{
		// Join the lightest team.
		iTeam = iLightestTeam;
	}

	return iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleCommand_JoinTeam( const char *pTeamName )
{
	// Losers can't change team during bonus time.
	if ( !IsBot() &&
		TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
		GetTeamNumber() != TDCGameRules()->GetWinningTeam() &&
		GetTeamNumber() >= FIRST_GAME_TEAM )
		return;

	if ( TDCGameRules()->IsInDuelMode() )
	{
		HandleCommand_JoinTeam_Duel( pTeamName );
		return;
	}

	if ( TDCGameRules()->IsInfectionMode() )
	{
		HandleCommand_JoinTeam_Infected( pTeamName );
		return;
	}

	int iTeam = TEAM_INVALID;
	bool bAuto = false;

	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = GetAutoTeam();
		bAuto = true;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
	{
		ClientPrint( this, HUD_PRINTCONSOLE, UTIL_VarArgs( "Invalid team \"%s\".", pTeamName ) );
		return;
	}

	if ( iTeam == GetTeamNumber() )
	{
		return;	// we wouldn't change the team
	}

	if ( iTeam == TEAM_SPECTATOR )
	{
		// Prevent this if the cvar is set
		if ( !mp_allowspectators.GetInt() && !IsHLTV() )
		{
			CSingleUserRecipientFilter filter( this );
			TDCGameRules()->SendHudNotification( filter, "#Cannot_Be_Spectator", "ico_notify_flag_moving", GetTeamNumber() );
			return;
		}

		ChangeTeam( TEAM_SPECTATOR );
	}
	else
	{
		// if this join would unbalance the teams, refuse
		// come up with a better way to tell the player they tried to join a full team!
		if ( !IsBot() && TDCGameRules()->WouldChangeUnbalanceTeams( iTeam, GetTeamNumber() ) )
		{
			ShowTeamMenu();
			return;
		}

		ChangeTeam( iTeam, bAuto, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleCommand_JoinTeam_Duel( const char *pTeamName )
{
	int iTeam = TEAM_INVALID;

	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = FIRST_GAME_TEAM;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( V_stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
		return;

	bool bWantSpectate = ( iTeam < FIRST_GAME_TEAM );

	// We're not actually changing teams, jointeam command puts us in or out of the queue instead.
	if ( bWantSpectate == m_bArenaSpectator && GetTeamNumber() != TEAM_UNASSIGNED )
		return;

	if ( bWantSpectate )
	{
		// Prevent this if the cvar is set
		if ( !mp_allowspectators.GetInt() && !IsHLTV() )
		{
			CSingleUserRecipientFilter filter( this );
			TDCGameRules()->SendHudNotification( filter, "#Cannot_Be_Spectator", "ico_notify_flag_moving", GetTeamNumber() );
			return;
		}

		m_bArenaSpectator = true;

		ChangeTeam( TEAM_SPECTATOR, false, false );
	}
	else
	{
		m_bArenaSpectator = false;

		// If we're in waiting for players we can just spawn normally.
		// Everyone will be moved back to spec once the game starts.
		if ( TDCGameRules()->State_Get() == GR_STATE_PREGAME )
		{
			ChangeTeam( FIRST_GAME_TEAM, false, true );
		}
		else
		{
			ChangeTeam( TEAM_SPECTATOR, false, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleCommand_JoinTeam_Infected( const char *pTeamName )
{
	int iTeam = TEAM_INVALID;

	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = FIRST_GAME_TEAM;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( V_stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
		return;

	// We cannot switch between Humans and Infected manually, we can only switch to spec and back.
	bool bWantSpectate = ( iTeam == TEAM_SPECTATOR );
	bool bSpectating = ( GetTeamNumber() == TEAM_SPECTATOR );

	if ( bWantSpectate == bSpectating && GetTeamNumber() != TEAM_UNASSIGNED )
		return;

	if ( bWantSpectate )
	{
		// Prevent this if the cvar is set
		if ( !mp_allowspectators.GetInt() && !IsHLTV() )
		{
			CSingleUserRecipientFilter filter( this );
			TDCGameRules()->SendHudNotification( filter, "#Cannot_Be_Spectator", "ico_notify_flag_moving", GetTeamNumber() );
			return;
		}

		ChangeTeam( TEAM_SPECTATOR, false, false );
	}
	else
	{
		if ( TDCGameRules()->State_Get() == GR_STATE_PREGAME )
		{
			// If we're in waiting for players just assign players randomly.
			ChangeTeam( GetAutoTeam(), true, false );
		}
		else
		{
			// Newly joined players always become infected.
			ChangeTeam( TDC_TEAM_ZOMBIES, true, false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without using the game menus
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleCommand_JoinTeam_NoMenus( const char *pTeamName )
{
	Assert( IsX360() );

	Msg( "Client command HandleCommand_JoinTeam_NoMenus: %s\n", pTeamName );

	// Only expected to be used on the 360 when players leave the lobby to start a new game
	if ( !IsInCommentaryMode() )
	{
		Assert( GetTeamNumber() == TEAM_UNASSIGNED );
		Assert( IsX360() );
	}

	int iTeam = TEAM_SPECTATOR;
	if ( Q_stricmp( pTeamName, "spectate" ) )
	{
		for ( int i = 0; i < GetNumberOfTeams(); ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	ForceChangeTeam( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without suiciding
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleCommand_JoinTeam_NoKill( const char *pTeamName )
{
	int iTeam = TEAM_INVALID;
	if ( stricmp( pTeamName, "auto" ) == 0 )
	{
		iTeam = GetAutoTeam();
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else
	{
		for ( int i = TEAM_SPECTATOR; i < GetNumberOfTeams(); ++i )
		{
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	if ( iTeam == TEAM_INVALID )
	{
		ClientPrint( this, HUD_PRINTCONSOLE, UTIL_VarArgs( "Invalid team \"%s\".", pTeamName ) );
		return;
	}

	if ( iTeam == GetTeamNumber() )
	{
		return;	// we wouldn't change the team
	}

	BaseClass::ChangeTeam( iTeam, false, true );
}

//-----------------------------------------------------------------------------
// Purpose: Player has been forcefully changed to another team
//-----------------------------------------------------------------------------
void CTDCPlayer::ForceChangeTeam( int iTeamNum, bool bForceRespawn )
{
	int iNewTeam = iTeamNum;
	bool bAuto = false;

	if ( iNewTeam == TDC_TEAM_AUTOASSIGN )
	{
		iNewTeam = GetAutoTeam();
		bAuto = true;
	}

	if ( !TFTeamMgr()->IsValidTeam( iNewTeam ) )
	{
		Warning( "CTDCPlayer::ForceChangeTeam( %d ) - invalid team index.\n", iNewTeam );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iNewTeam == iOldTeam )
		return;

	if ( iNewTeam == TEAM_UNASSIGNED )
	{
		StateTransition( TDC_STATE_OBSERVER );
	}
	else if ( iNewTeam == TEAM_SPECTATOR )
	{
		CTDC_GameStats.Event_PlayerMovedToSpectators( this );
		CleanupOnDeath( false );

		m_flSpectatorTime = gpGlobals->curtime;
		m_bIsIdle = false;
		StateTransition( TDC_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();

		// do we have fadetoblack on? (need to fade their screen back in)
		if ( mp_fadetoblack.GetBool() )
		{
			color32_s clr = { 0, 0, 0, 255 };
			UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
		}
	}

	// Don't modify living players in any way

	BaseClass::ChangeTeam( iNewTeam, bAuto, true );

	if ( bForceRespawn )
	{
		ForceRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleFadeToBlack( void )
{
	if ( mp_fadetoblack.GetBool() )
	{
		color32_s clr = { 0,0,0,255 };
		UTIL_ScreenFade( this, clr, 0.75, 0, FFADE_OUT | FFADE_STAYOUT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ChangeTeam( int iTeamNum, bool bAutoTeam /*= false*/, bool bSilent /*= false*/ )
{
	if ( !TFTeamMgr()->IsValidTeam( iTeamNum ) )
	{
		Warning( "CTDCPlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iTeamNum == iOldTeam )
		return;

	RemoveAllOwnedEntitiesFromWorld( false );
	DropFlag();

	if ( TDCGameRules()->IsVIPMode() && IsVIP() )
	{
		TDCGameRules()->ResetVIP();
	}

	if ( iTeamNum == TEAM_UNASSIGNED )
	{
		StateTransition( TDC_STATE_OBSERVER );
	}
	else // active player
	{
		if ( iTeamNum == TEAM_SPECTATOR )
		{
			CTDC_GameStats.Event_PlayerMovedToSpectators( this );
			CleanupOnDeath( true );

			m_flSpectatorTime = gpGlobals->curtime;
			m_bIsIdle = false;
			StateTransition( TDC_STATE_OBSERVER );

			RemoveAllWeapons();
			DestroyViewModels();

			// do we have fadetoblack on? (need to fade their screen back in)
			if ( mp_fadetoblack.GetBool() )
			{
				color32_s clr = { 0, 0, 0, 255 };
				UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
			}
		}
		else if ( IsAlive() )
		{
			// Kill player if switching teams while alive
			m_bRespawnRequiresAction = false;
			CommitSuicide( false, true );
		}
		else if ( iOldTeam < FIRST_GAME_TEAM && iTeamNum >= FIRST_GAME_TEAM )
		{
			SetObserverMode( OBS_MODE_CHASE );
			HandleFadeToBlack();
		}
	}

	BaseClass::ChangeTeam( iTeamNum, bAutoTeam, bSilent );

	RemoveNemesisRelationships( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ShowTeamMenu( bool bShow /*= true*/ )
{
	if ( !TDCGameRules()->IsTeamplay() || TDCGameRules()->IsInfectionMode() )
	{
		ShowViewPortPanel( PANEL_DEATHMATCHTEAMSELECT, bShow );
	}
	else
	{
		ShowViewPortPanel( PANEL_TEAM, bShow );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::GetSpawnClass( void )
{
	if ( TDCGameRules()->IsInfectionMode() )
	{
		if ( GetTeamNumber() == TDC_TEAM_ZOMBIES )
		{
			return TDC_CLASS_ZOMBIE;
		}
	}

	if ( TDCGameRules()->IsVIPMode() && this == TDCGameRules()->GetVIP() )
	{
		return TDC_CLASS_VIP;
	}

	int iClass = m_iDesiredPlayerClass;
	const char *pclassname = tdc_player_restrict_class.GetString();
	if ( !FStrEq( pclassname, "" ) )
	{
		int iRestrictedClass = UTIL_StringFieldToInt( pclassname, g_aPlayerClassNames_NonLocalized, TDC_CLASS_COUNT_ALL );
		if ( iRestrictedClass >= TDC_FIRST_NORMAL_CLASS && iRestrictedClass <= TDC_LAST_NORMAL_CLASS )
		{
			iClass = iRestrictedClass;
		}
	}
	return iClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	
	m_flLastAction = gpGlobals->curtime;

	if ( FStrEq( pcmd, "addcond" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				int iCond = clamp( atoi( args[1] ), 0, TDC_COND_LAST-1 );

				CTDCPlayer *pTargetPlayer = this;
				if ( args.ArgC() >= 4 )
				{
					// Find the matching netname
					for ( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex(i) );
						if ( pPlayer )
						{
							if ( V_strstr( pPlayer->GetPlayerName(), args[3] ) )
							{
								pTargetPlayer = pPlayer;
								break;
							}
						}
					}
				}

				if ( args.ArgC() >= 3 )
				{
					float flDuration = atof( args[2] );
					pTargetPlayer->m_Shared.AddCond( (ETDCCond)iCond, flDuration );
				}
				else
				{
					pTargetPlayer->m_Shared.AddCond( (ETDCCond)iCond );
				}
			}
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "removecond" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				int iCond = clamp( atoi( args[1] ), 0, TDC_COND_LAST-1 );
				m_Shared.RemoveCond( (ETDCCond)iCond );
			}
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "burn" ) ) 
	{
		if ( sv_cheats->GetBool() )
		{
			m_Shared.Burn( this );
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "jointeam" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam( args[1] );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			HandleCommand_JoinTeam( "spectate" );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "jointeam_nomenus" ) )
	{
		if ( IsX360() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam_NoMenus( args[1] );
			}
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "jointeam_nokill" ) )
	{
		if ( sv_cheats->GetBool() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam_NoKill( args[1] );
			}
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "joinclass" ) )
	{
		if ( args.ArgC() < 2 )
			return true;

		// TODO: Put it in a seperate function.
		if ( GetTeamNumber() < FIRST_GAME_TEAM )
			return true;

		if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN &&
			GetTeamNumber() != TDCGameRules()->GetWinningTeam() )
			return true;

		if ( TDCGameRules()->IsInfectionMode() && GetTeamNumber() == TDC_TEAM_ZOMBIES )
			return true;

		// did this for debugging, evaluates to the same thing during build - Graham
		const char* sClass = args[1];
		int iClass = UTIL_StringFieldToInt( sClass, g_aPlayerClassNames_NonLocalized, TDC_CLASS_COUNT_ALL );
		if ( iClass < TDC_FIRST_NORMAL_CLASS || iClass > TDC_LAST_NORMAL_CLASS )
			return true;

		m_iDesiredPlayerClass = iClass;

		if ( IsAlive() )
		{
			m_bRespawnRequiresAction = false;
			CommitSuicide();
		}

		return true;
	}
	else if ( FStrEq( pcmd, "closedwelcomemenu" ) )
	{
		if ( GetTeamNumber() == TEAM_UNASSIGNED )
		{
			ShowTeamMenu( true );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "setitempreset" ) )
	{
		if ( args.ArgC() >= 3 )
		{
			int iClass = atoi( args[1] );
			int iSlot = atoi( args[2] );
			int iItemID = atoi( args[3] );
			if ( ( iClass <= TDC_CLASS_UNDEFINED || iClass > TDC_CLASS_COUNT_ALL ) ||
				( iSlot < 0 || iSlot >= TDC_WEARABLE_COUNT ) )
				return true;

			WearableDef_t *pItemDef = g_TDCPlayerItems.GetWearable( iItemID );
			if ( !pItemDef || pItemDef->slot != iSlot || !pItemDef->used_by_classes[iClass] )
			{
				iItemID = g_TDCPlayerItems.GetFirstWearableIDForSlot( (ETDCWearableSlot)iSlot, iClass );
			}

			m_ItemPreset[iClass][iSlot] = iItemID;
		}

		return true;
	}
	else if ( FStrEq( pcmd, "mp_playgesture" ) )
	{
		if ( args.ArgC() == 1 )
		{
			Warning( "mp_playgesture: Gesture activity or sequence must be specified!\n" );
			return true;
		}

		if ( sv_cheats->GetBool() )
		{
			if ( !PlayGesture( args[1] ) )
			{
				Warning( "mp_playgesture: unknown sequence or activity name \"%s\"\n", args[1] );
				return true;
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "mp_playanimation" ) )
	{
		if ( args.ArgC() == 1 )
		{
			Warning( "mp_playanimation: Activity or sequence must be specified!\n" );
			return true;
		}

		if ( sv_cheats->GetBool() )
		{
			if ( !PlaySpecificSequence( args[1] ) )
			{
				Warning( "mp_playanimation: Unknown sequence or activity name \"%s\"\n", args[1] );
				return true;
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "taunt" ) )
	{
		if ( !IsAlive() )
			return true;

		Taunt();
		return true;
	}
	else if ( FStrEq( pcmd, "extendfreeze" ) )
	{
		m_flDeathTime += 2.0f;
		return true;
	}
#if 0
	else if ( FStrEq( pcmd, "condump_on" ) )
	{
		if ( !PlayerHasPowerplay() )
		{
			Msg("Console dumping on.\n");
			return true;
		}
		else 
		{
			if ( args.ArgC() == 2 && GetTeam() )
			{
				for ( int i = 0; i < GetTeam()->GetNumPlayers(); i++ )
				{
					CTDCPlayer *pTeamPlayer = ToTDCPlayer( GetTeam()->GetPlayer(i) );
					if ( pTeamPlayer )
					{
						pTeamPlayer->SetPowerplayEnabled( true );
					}
				}
				return true;
			}
			else
			{
				if ( SetPowerplayEnabled( true ) )
					return true;
			}
		}
	}
	else if ( FStrEq( pcmd, "condump_off" ) )
	{
		if ( !PlayerHasPowerplay() )
		{
			Msg("Console dumping off.\n");
			return true;
		}
		else
		{
			if ( args.ArgC() == 2 && GetTeam() )
			{
				for ( int i = 0; i < GetTeam()->GetNumPlayers(); i++ )
				{
					CTDCPlayer *pTeamPlayer = ToTDCPlayer( GetTeam()->GetPlayer(i) );
					if ( pTeamPlayer )
					{
						pTeamPlayer->SetPowerplayEnabled( false );
					}
				}
				return true;
			}
			else
			{
				if ( SetPowerplayEnabled( false ) )
					return true;
			}
		}
	}
#endif

	return BaseClass::ClientCommand( args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::PlayGesture( const char *pGestureName )
{
	Activity nActivity = (Activity)LookupActivity( pGestureName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pGestureName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::PlaySpecificSequence( const char *pAnimationName )
{
	Activity nActivity = (Activity)LookupActivity( pAnimationName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pAnimationName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}

ConVar tdc_damagescale_headshot( "tdc_damage_headshot", "1.35" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( m_takedamage != DAMAGE_YES )
		return;

	// Prevent team damage here so blood doesn't appear
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) )
		return;

	// Save this bone for the ragdoll.
	m_nForceBone = ptr->physicsbone;

	SetLastHitGroup( ptr->hitgroup );

	// Ignore hitboxes for all weapons except the sniper rifle
	CTakeDamageInfo info_modified = info;

	if ( info_modified.GetDamageType() & DMG_USE_HITLOCATIONS )
	{
		switch ( ptr->hitgroup )
		{
		case HITGROUP_HEAD:
		{
			// Sniper Rifle does crit damage on scoped headshot.
			CWeaponLeverRifle *pWeapon = dynamic_cast<CWeaponLeverRifle *>( info_modified.GetWeapon() );

			if ( pWeapon && pWeapon->CanCritHeadshot() )
			{
				info_modified.AddDamageType( DMG_CRITICAL );
			}
			else
			{
				info_modified.ScaleDamage( tdc_damagescale_headshot.GetFloat() );
			}

			info_modified.SetDamageCustom( TDC_DMG_CUSTOM_HEADSHOT );
			break;
		}
		default:
			break;
		}
	}

	if ( m_Shared.IsInvulnerable() )
	{ 
		// Make bullet impacts
		g_pEffects->Ricochet( ptr->endpos - (vecDir * 8), -vecDir );
	}
	else
	{	
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		CDisablePredictionFiltering disabler;

		// This does smaller splotches on the guy and splats blood on the world.
		TraceBleed( info_modified.GetDamage(), vecDir, ptr, info_modified.GetDamageType() );
	}

	AddMultiDamage( info_modified, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	int iResult = false;

	// If the bit's set, add over the max health
	if ( bitsDamageType & HEAL_IGNORE_MAXHEALTH )
	{
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= ~(bitsDamageType & ~iTimeBasedDamage);

		if ( bitsDamageType & HEAL_MAXBUFFCAP )
		{
			if ( flHealth > m_Shared.GetMaxBuffedHealth() - m_iHealth )
			{
				flHealth = m_Shared.GetMaxBuffedHealth() - m_iHealth;
			}
		}

		if ( flHealth <= 0 )
		{
			iResult = 0;
		}
		else
		{
			m_iHealth += flHealth;
			iResult = (int)flHealth;
		}
	}
	else
	{
		float flHealthToAdd = flHealth;
		float flMaxHealth = GetMaxHealth();
		
		// don't want to add more than we're allowed to have
		if ( flHealthToAdd > flMaxHealth - m_iHealth )
		{
			flHealthToAdd = flMaxHealth - m_iHealth;
		}

		if ( flHealthToAdd <= 0 )
		{
			iResult = 0;
		}
		else
		{
			iResult = BaseClass::TakeHealth( flHealthToAdd, bitsDamageType );
		}
	}

	if ( ( bitsDamageType & HEAL_NOTIFY ) && iResult > 0 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );

		if ( event )
		{
			event->SetInt( "amount", iResult );
			event->SetInt( "entindex", entindex() );

			gameeventmanager->FireEvent( event );
		}
	}

	if ( iResult > 0 )
		m_Shared.ResetDamageSourceType();

	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::DropFlag( void )
{
	CCaptureFlag *pFlag = GetTheFlag();
	if ( pFlag )
	{
		pFlag->Drop( this, true, true );
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TDC_FLAGEVENT_DROPPED );
			event->SetInt( "priority", 8 );

			gameeventmanager->FireEvent( event );
		}
	}
}

static float DamageForce( const Vector &size, float damage, float scale )
{
	float force = damage * ((48 * 48 * 82.0) / (size.x * size.y * size.z)) * scale;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}

ConVar tdc_debug_damage( "tdc_debug_damage", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	if ( GetFlags() & FL_GODMODE )
		return 0;

	if ( IsInCommentaryMode() )
		return 0;

	// Early out if there's no damage
	if ( !info.GetDamage() )
		return 0;

	if ( !IsAlive() )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	CTDCWeaponBase *pWeapon = dynamic_cast<CTDCWeaponBase *>( info.GetWeapon() );

	if ( m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
		info.ScaleDamage( RemapValClamped( tdc_player_berserk_dmgresist.GetFloat(), 0, 1, 1, 0 ) );

	int iHealthBefore = GetHealth();

	bool bDebug = tdc_debug_damage.GetBool();
	if ( bDebug )
	{
		Warning( "%s taking damage from %s, via %s. Damage: %.2f\n", GetDebugName(), pInflictor ? pInflictor->GetDebugName() : "Unknown Inflictor", pAttacker ? pAttacker->GetDebugName() : "Unknown Attacker", info.GetDamage() );
	}

	// Make sure the player can take damage from the attacking entity
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker, info ) )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player can't take damage from that attacker.\n" );
		}
		return 0;
	}

	AddDamagerToHistory( pAttacker, pWeapon );

	// keep track of amount of damage last sustained
	m_lastDamageAmount = info.GetDamage();
	m_LastDamageType = info.GetDamageType();

	if ( !( info.GetDamageType() & DMG_FALL ) )
	{
		m_Shared.NoteLastDamageTime( m_lastDamageAmount );
	}

	// Save damage force for ragdolls.
	m_vecTotalBulletForce = info.GetDamageForce();
	m_vecTotalBulletForce.x = clamp( m_vecTotalBulletForce.x, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.y = clamp( m_vecTotalBulletForce.y, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.z = clamp( m_vecTotalBulletForce.z, -15000.0f, 15000.0f );

	if ( TDCGameRules()->IsInstagib() && pAttacker->IsPlayer() && pAttacker != this )
	{
		// All players die in one hit in instagib.
		info.SetDamage( 1000 );
		info.AddDamageType( DMG_ALWAYSGIB );
	}
 
	int bitsDamage = info.GetDamageType();

	// If we're invulnerable, force ourselves to only take damage events only, so we still get pushed
	if ( m_Shared.IsInvulnerable() )
	{
		bool bAllowDamage = false;

		// check to see if our attacker is a trigger_hurt entity (and allow it to kill us even if we're invuln)
		if ( pAttacker && pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) )
		{
			CTriggerHurt *pTrigger = dynamic_cast<CTriggerHurt *>( pAttacker );
			if ( pTrigger )
			{
				bAllowDamage = true;
			}
		}

		// Ubercharge does not save from being crushed (this includes telefrags as they do crush damage).
		if ( info.GetDamageType() & DMG_CRUSH )
		{
			bAllowDamage = true;
		}

		if ( !bAllowDamage )
		{
			int iOldTakeDamage = m_takedamage;
			m_takedamage = DAMAGE_EVENTS_ONLY;
			// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
			CBaseCombatCharacter::OnTakeDamage( info );
			m_takedamage = iOldTakeDamage;

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) )
			{
				SpeakConceptIfAllowed( MP_CONCEPT_HURT );
			}
			return 0;
		}
	}

	CTDCPlayer *pTFAttacker = ToTDCPlayer( pAttacker );

	if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_HEADSHOT && pTFAttacker )
	{
		// Exclude attacker and receiver.
		CPASAttenuationFilter othersFilter( GetAbsOrigin(), "Player.HeadshotImpactOther" );
		othersFilter.RemoveRecipient( this );
		othersFilter.RemoveRecipient( pTFAttacker );
		EmitSound( othersFilter, entindex(), "Player.HeadshotImpactOther" );

		CSingleUserRecipientFilter victimFilter( this );
		EmitSound( victimFilter, entindex(), "Player.HeadshotImpactVictim" );

		CSingleUserRecipientFilter attackerFilter( pTFAttacker );
		EmitSound( attackerFilter, entindex(), "Player.HeadshotImpactAttacker" );
	}

	// If we're not damaging ourselves, apply randomness
	if ( pAttacker != this && !( bitsDamage & ( DMG_DROWN | DMG_FALL ) ) )
	{
		float flDamage = 0;
		if ( bitsDamage & DMG_CRITICAL )
		{
			if ( bDebug )
			{
				Warning( "    CRITICAL!\n");
			}

			flDamage = info.GetDamage() * TDC_DAMAGE_CRIT_MULTIPLIER;

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) )
			{
				SpeakConceptIfAllowed( MP_CONCEPT_HURT, "damagecritical:1" );
			}
		}
		else
		{
			float flRandomDamage = info.GetDamage() * 0.5f;
			float flMin = 0.4;
			float flMax = 0.6;
			float flCenter = 0.5;

			if ( bitsDamage & DMG_USEDISTANCEMOD )
			{
				float flDistance = Max( 1.0f, ( WorldSpaceCenter() - pAttacker->WorldSpaceCenter() ).Length() );
				float flOptimalDistance = 512.0;

				flCenter = RemapValClamped( flDistance / flOptimalDistance, 0.0, 2.0, 1.0, 0.0 );
				if ( bitsDamage & DMG_NOCLOSEDISTANCEMOD )
				{
					if ( flCenter > 0.5 )
					{
						// Reduce the damage bonus at close range
						flCenter = RemapVal( flCenter, 0.5, 1.0, 0.5, 0.65 );
					}
				}
				flMin = Max( 0.0, flCenter - 0.1 );
				flMax = Min( 1.0, flCenter + 0.1 );

				if ( bDebug )
				{
					Warning( "    RANDOM: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}

			//Msg("Range: %.2f - %.2f\n", flMin, flMax )

			if ( flCenter > 0.5 )
			{
				// Rocket launcher, Sticky launcher and Scattergun have different short range bonuses
				if ( pWeapon )
				{
					switch ( pWeapon->GetWeaponID() )
					{
					case WEAPON_ROCKETLAUNCHER:
						// Rocket launcher and sticky launcher only have half the bonus of the other weapons at short range
						flRandomDamage *= 0.5;
						break;
					}
				}
			}

			float flOut = SimpleSplineRemapValClamped( flCenter, 0, 1, -flRandomDamage, flRandomDamage );
			flDamage = info.GetDamage() + flOut;

			/*
			for ( float flVal = flMin; flVal <= flMax; flVal += 0.05 )
			{
			float flOut = SimpleSplineRemapValClamped( flVal, 0, 1, -flRandomDamage, flRandomDamage );
			Msg("Val: %.2f, Out: %.2f, Dmg: %.2f\n", flVal, flOut, info.GetDamage() + flOut );
			}
			*/

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) )
			{
				SpeakConceptIfAllowed( MP_CONCEPT_HURT );
			}
		}

		info.SetDamage( flDamage );
	}

	// Do this after our damage has been properly calculated
	if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_HEADSHOT )
	{
		float flHeadshotTime = RemapValClamped( info.GetDamage(),
			tdc_headshoteffect_mindmg.GetFloat(), tdc_headshoteffect_maxdmg.GetFloat(),
			tdc_headshoteffect_mintime.GetFloat(), tdc_headshoteffect_maxtime.GetFloat() );
		m_flHeadshotFadeTime = gpGlobals->curtime + flHeadshotTime;
	}

	if ( m_debugOverlays & OVERLAY_BUDDHA_MODE )
	{
		if ( ( m_iHealth - info.GetDamage() ) <= 0 )
		{
			m_iHealth = 1;
			return 0;
		}
	}

	// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
	int iResult = CBaseCombatCharacter::OnTakeDamage( info );

	// Early out if the base class took no damage
	if ( !iResult )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player failed to take the damage.\n" );
		}
		return 0;
	}

	if ( bDebug )
	{
		Warning( "    DEALT: Player took %.2f damage.\n", info.GetDamage() );
		Warning( "    HEALTH LEFT: %d\n", GetHealth() );
	}

	// Send the damage message to the client for the hud damage indicator
	// Don't do this for damage types that don't use the indicator
	if ( !(bitsDamage & (DMG_DROWN | DMG_FALL | DMG_BURN) ) )
	{
		// Try and figure out where the damage is coming from
		Vector vecDamageOrigin = info.GetReportedPosition();

		// If we didn't get an origin to use, try using the attacker's origin
		if ( vecDamageOrigin == vec3_origin && pInflictor )
		{
			vecDamageOrigin = pInflictor->GetAbsOrigin();
		}

		CSingleUserRecipientFilter user( this );
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( clamp( (int)info.GetDamage(), 0, 255 ) );
			WRITE_VEC3COORD( vecDamageOrigin );
		MessageEnd();
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( pInflictor && UTIL_IsValidEntity( pInflictor ) )
	{
		m_DmgOrigin = pInflictor->GetAbsOrigin();
	}

	m_DmgTake += (int)info.GetDamage();

	// Reset damage time countdown for each type of time based damage player just sustained
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		// Make sure the damage type is really time-based.
		// This is kind of hacky but necessary until we setup DamageType as an enum.
		int iDamage = ( DMG_PARALYZE << i );
		if ( ( info.GetDamageType() & iDamage ) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
	}

	// Display any effect associate with this damage type
	DamageEffect( info.GetDamage(),bitsDamage );

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	m_Local.m_vecPunchAngle.SetX( -2 );

	// Do special explosion damage effect
	if ( bitsDamage & DMG_BLAST )
	{
		OnDamagedByExplosion( info );
	}

	PainSound( info );

	PlayFlinch( info );

	// Detect drops below 25% health and restart expression, so that characters look worried.
	int iHealthBoundary = (GetMaxHealth() * 0.25);
	if ( GetHealth() <= iHealthBoundary && iHealthBefore > iHealthBoundary )
	{
		ClearExpression();
	}

	CTDC_GameStats.Event_PlayerDamage( this, info, iHealthBefore - GetHealth() );

	if ( iHealthBefore - GetHealth() < 0 )
		m_Shared.ResetDamageSourceType();

	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::DamageEffect(float flDamage, int fDamageType)
{
	if (fDamageType & DMG_CRUSH)
	{
		//Red damage indicator
		color32 red = {128,0,0,128};
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_DROWN)
	{
		//Red damage indicator
		color32 blue = {0,0,128,128};
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_SLASH)
	{
		// If slash damage shoot some blood
		SpawnBlood( EyePosition(), g_vecAttackDir, BloodColor(), flDamage );
	}
	else if ( fDamageType & DMG_BULLET )
	{
		EmitSound( "Flesh.BulletImpact" );
	}
}

//---------------------------------------
// Is the player the passed player class?
//---------------------------------------
bool CTDCPlayer::IsPlayerClass( int iClass ) const
{
	return m_PlayerClass.IsClass( iClass );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsNormalClass( void ) const
{
	return ( m_PlayerClass.GetClassIndex() <= TDC_LAST_NORMAL_CLASS );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the player's a zombie! (Used in infection)
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsZombie( void ) const
{
	return ( m_PlayerClass.GetClassIndex() == TDC_CLASS_ZOMBIE );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the player's a VIP! (Used in VIP)
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsVIP( void ) const
{
	return ( m_PlayerClass.GetClassIndex() == TDC_CLASS_VIP );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::CommitSuicide( bool bExplode /* = false */, bool bForce /*= false*/ )
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state
	if ( IsPlayerClass( TDC_CLASS_UNDEFINED ) || !m_Shared.InState( TDC_STATE_ACTIVE ) )
		return;

	// Don't suicide during the "bonus time" if we're not on the winning team
	if ( !bForce && TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		 GetTeamNumber() != TDCGameRules()->GetWinningTeam() )
	{
		return;
	}
	// Can't have the VIP kill themselves
	if ( TDCGameRules()->IsVIPMode() && IsVIP() && TDCGameRules()->State_Get() != GR_STATE_TEAM_WIN )
	{
		return;
	}

	m_iSuicideCustomKillFlags = TDC_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
int CTDCPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Grab the vector of the incoming attack. 
	// (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	CBaseEntity *pAttacker = info.GetAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	CTDCWeaponBase *pWeapon = dynamic_cast<CTDCWeaponBase *>( info.GetWeapon() );

	Vector vecDir = vec3_origin;
	if ( pInflictor )
	{
		vecDir = pInflictor->WorldSpaceCenter() - Vector( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}
	g_vecAttackDir = vecDir;

	// Do the damage.
	// NOTE: None of the damage modifiers used here affect the knockback force.
	m_bitsDamageType |= info.GetDamageType();
	float flDamage = info.GetDamage();

	if ( flDamage == 0.0f )
		return 0;

	// Self-damage modifiers.
	if ( pAttacker == this )
	{
		if ( pWeapon && pWeapon->IsWeapon( WEAPON_DISPLACER ) )
		{
			flDamage *= 0.5f;
		}

		if ( info.GetDamageType() & DMG_BLAST && GetGroundEntity() == NULL )
		{
			flDamage *= GetPlayerClass()->GetData()->m_flSelfDamageScale;
		}
	}

	int iOldHealth = m_iHealth;
	bool bIgniting = false;

	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		// Start burning if we took ignition damage
		bIgniting = ( ( info.GetDamageType() & DMG_IGNITE ) && ( GetWaterLevel() < WL_Waist ) );

		// Take damage - round to the nearest integer.
		m_iHealth -= ( flDamage + 0.5f );
	}

	m_flLastDamageTime = gpGlobals->curtime;

	if ( !pAttacker )
		return 0;

	// Apply a damage force. Use unmodified damage for calculating.
	ApplyPushFromDamage( info, vecDir );

	CTDCPlayer *pTFAttacker = ToTDCPlayer( pAttacker );

	if ( bIgniting )
	{
		m_Shared.Burn( pTFAttacker, pWeapon );
	}

	// Fire a global game event - "player_hurt"
	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "health", Max( 0, m_iHealth.Get() ) );
		event->SetInt( "damageamount", ( iOldHealth - m_iHealth ) );
		event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) != 0 );
		event->SetInt( "custom", info.GetDamageCustom() );

		// HLTV event priority, not transmitted
		event->SetInt( "priority", 5 );	

		// Hurt by another player.
		if ( pTFAttacker )
		{
			event->SetInt( "attacker", pTFAttacker->GetUserID() );
		}
		// Hurt by world.
		else
		{
			event->SetInt( "attacker", 0 );
		}

        gameeventmanager->FireEvent( event );
	}

	if ( !m_Shared.IsInvulnerable() && ( info.GetDamageType() & ( DMG_IGNITE | DMG_SLASH ) ) == 0 )
	{
		Vector vDamagePos = info.GetDamagePosition();

		if ( vDamagePos == vec3_origin )
		{
			vDamagePos = WorldSpaceCenter();
		}

		CPVSFilter filter( vDamagePos );
		TE_TFBlood( filter, 0.0, vDamagePos, -vecDir, entindex(), (ETDCDmgCustom)info.GetDamageCustom() );
	}

	// Done.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::ApplyPushFromDamage( const CTakeDamageInfo &info, const Vector &vecDir )
{
	// Don't do this if we just died. Prevents flying ragdolls.
	if ( m_iHealth <= 0 )
		return;

	CBaseEntity *pAttacker = info.GetAttacker();

	if ( info.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE )
		return;

	if( !info.GetInflictor() ||
		( GetMoveType() != MOVETYPE_WALK ) ||
		pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) )
		return;

	Vector vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), 6.0f );

	if ( pAttacker == this )
	{
		if ( info.GetDamageType() & DMG_BLAST && GetBlastJumpFlags() == 0 )
		{
			// Set the rocket jumping state.
			SetBlastJumpState( TDC_JUMP_ROCKET, false );
		}
	}
	else
	{
		vecForce *= GetPlayerClass()->GetData()->m_flDamageForceScale;

		if ( info.GetDamageType() & DMG_BLAST )
		{
			m_bBlastLaunched = true;
		}
	}

	ApplyAbsVelocityImpulse( vecForce );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::SetBlastJumpState( int iJumpType, bool bPlaySound )
{
	m_nBlastJumpFlags |= iJumpType;
	m_Shared.AddCond( TDC_COND_BLASTJUMPING );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::ClearBlastJumpState( void )
{
	m_nBlastJumpFlags = 0;
	m_bJumpEffect = false;
	m_Shared.RemoveCond( TDC_COND_BLASTJUMPING );
}

//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people who damaged player
//-----------------------------------------------------------------------------
void CTDCPlayer::AddDamagerToHistory( EHANDLE hDamager, CTDCWeaponBase *pWeapon /*= NULL*/ )
{
	// Ignore teammates and ourselves.
	CTDCPlayer *pDamager = ToTDCPlayer( hDamager.Get() );
	if ( !pDamager || pDamager == this || !IsEnemy( pDamager ) )
		return;

	// If this damager is different from the most recent damager, shift the
	// damagers down and drop the oldest damager.  (If this damager is already
	// the most recent, we will just update the damage time but not remove
	// other damagers from history.)
	if ( m_DamagerHistory[0].hDamager != hDamager )
	{
		for ( int i = 1; i < ARRAYSIZE( m_DamagerHistory ); i++ )
		{
			m_DamagerHistory[i] = m_DamagerHistory[i-1];
		}		
	}	
	// set this damager as most recent and note the time
	m_DamagerHistory[0].hDamager = hDamager;
	m_DamagerHistory[0].flTimeDamage = gpGlobals->curtime;

	// Remember the weapon that he used to damage us.
	m_DamagerHistory[0].hWeapon = pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Clears damager history
//-----------------------------------------------------------------------------
void CTDCPlayer::ClearDamagerHistory()
{
	for ( int i = 0; i < ARRAYSIZE( m_DamagerHistory ); i++ )
	{
		m_DamagerHistory[i].Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCPlayer::ShouldGib( const CTakeDamageInfo &info )
{
	// Zombies always gib.
	if ( IsZombie() )
		return true;

	// Check to see if we should allow players to gib.
	int nGibCvar = tdc_playergib.GetInt();
	if ( nGibCvar == 0 )
		return false;

	if ( nGibCvar == 2 )
		return true;

	if ( info.GetDamageType() & DMG_NEVERGIB )
		return false;

	if ( info.GetDamageType() & DMG_ALWAYSGIB )
		return true;

	if ( info.GetDamageType() & DMG_BLAST )
	{
		if ( ( info.GetDamageType() & DMG_CRITICAL ) || m_iHealth < -9 )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );

	if ( pVictim->IsPlayer() )
	{
		CTDCPlayer *pTDCVictim = ToTDCPlayer(pVictim);

		// Custom death handlers
		const char *pszCustomDeath = "customdeath:none";

		if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_HEADSHOT )
		{				
			pszCustomDeath = "customdeath:headshot";
		}
		else if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_BACKSTAB )
		{
			pszCustomDeath = "customdeath:backstab";
		}
		else if ( info.GetDamageCustom() == TDC_DMG_CUSTOM_BURNING )
		{
			pszCustomDeath = "customdeath:burning";
		}

		// Revenge handler
		const char *pszDomination = "domination:none";
		if ( pTDCVictim->GetDeathFlags() & TDC_DEATH_REVENGE )
		{
			pszDomination = "domination:revenge";
		}
		else if ( pTDCVictim->GetDeathFlags() & TDC_DEATH_DOMINATION )
		{
			pszDomination = "domination:dominated";
		}

		CFmtStrN<128> modifiers( "%s,%s,victimclass:%s", pszCustomDeath, pszDomination, g_aPlayerClassNames_NonLocalized[pTDCVictim->GetPlayerClass()->GetClassIndex()] );
		SpeakConceptIfAllowed( MP_CONCEPT_KILLED_PLAYER, modifiers );

		if ( IsAlive() )
		{
			m_Shared.IncKillstreak();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	SpeakConceptIfAllowed( MP_CONCEPT_DIED );

	StateTransition( TDC_STATE_DYING );	// Transition into the dying state.

	CBaseEntity *pAttacker = info.GetAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	CTDCPlayer *pTDCAttacker = ToTDCPlayer( pAttacker );

	// If the player has a capture flag and was killed by another player, award that player a defense
	if ( HasTheFlag() && pTDCAttacker && ( pTDCAttacker != this ) )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", pTDCAttacker->entindex() );
			event->SetInt( "eventtype", TDC_FLAGEVENT_DEFEND );
			event->SetInt( "priority", 8 );
			gameeventmanager->FireEvent( event );
		}

		CTDC_GameStats.Event_PlayerDefendedPoint( pTDCAttacker );
	}

	int nRagdollFlags = 0;

	// we want the rag doll to burn if the player was burning and was not a pyro (who only burns momentarily)
	if ( m_Shared.InCond( TDC_COND_BURNING ) )
		nRagdollFlags |= TDC_RAGDOLL_BURNING;

	if ( GetFlags() & FL_ONGROUND )
		nRagdollFlags |= TDC_RAGDOLL_ONGROUND;

	CTakeDamageInfo info_modified = info;

	// See if we should gib.
	if ( ShouldGib( info ) )
	{
		nRagdollFlags |= TDC_RAGDOLL_GIB;
	}

	CleanupOnDeath( true );

	// show killer in death cam mode
	// chopped down version of SetObserverTarget without the team check
	if ( pTDCAttacker && !tdc_disablefreezecam.GetBool() )
	{
		// Look at the player
		m_hObserverTarget.Set( pAttacker );

		// reset fov to default
		SetFOV( this, 0 );
	}
	else
	{
		m_hObserverTarget.Set( NULL );
	}

	bool bUsedSuicideButton = ( info_modified.GetDamageCustom() == TDC_DMG_CUSTOM_SUICIDE );

	if ( bUsedSuicideButton || pAttacker == this || pAttacker->IsBSPModel() )
	{
		// Recalculate attacker if player killed himself or this was environmental death.
		float flCreditTime = bUsedSuicideButton ? 10.0f : 5.0f;
		CBasePlayer *pDamager;

		if ( TDCGameRules()->IsInDuelMode() && TDCGameRules()->State_Get() != GR_STATE_PREGAME )
		{
			// In Duel we want to always credit suicides to the opponent.
			pDamager = TDCGameRules()->GetDuelOpponent( this );
		}
		else
		{
			pDamager = TDCGameRules()->GetRecentDamager( this, 0, flCreditTime );
		}

		if ( pDamager )
		{
			pTDCAttacker = (CTDCPlayer *)pDamager;
			info_modified.SetAttacker( pDamager );
			info_modified.SetInflictor( NULL );

			// If player pressed the suicide button set to the weapon used by attacker so they
			// still get on-kill effects.
			info_modified.SetWeapon( bUsedSuicideButton ? m_DamagerHistory[0].hWeapon : NULL );

			info_modified.SetDamageType( DMG_GENERIC );
			info_modified.SetDamageCustom( TDC_DMG_CUSTOM_SUICIDE );
		}
	}

	m_OnDeath.FireOutput( this, this );

	BaseClass::Event_Killed( info_modified );

	if ( pTDCAttacker )
	{
		if ( TDC_DMG_CUSTOM_HEADSHOT == info.GetDamageCustom() )
		{
			CTDC_GameStats.Event_Headshot( pTDCAttacker );
		}
		else if ( TDC_DMG_CUSTOM_BACKSTAB == info.GetDamageCustom() )
		{
			CTDC_GameStats.Event_Backstab( pTDCAttacker );
		}
	}

	// Create the ragdoll entity.
	CreateRagdollEntity( nRagdollFlags, m_Shared.m_flInvisibility, (ETDCDmgCustom)info.GetDamageCustom() );

	// Don't overflow the value for this.
	m_iHealth = 0;

	if ( pAttacker && pAttacker == pInflictor && pAttacker->IsBSPModel() )
	{
		CTDCPlayer *pDamager = TDCGameRules()->GetRecentDamager( this, 0, 5.0f );

		if ( pDamager )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "environmental_death" );

			if ( event )
			{
				event->SetInt( "killer", pDamager->GetUserID() );
				event->SetInt( "victim", GetUserID() );
				event->SetInt( "priority", 9 ); // HLTV event priority, not transmitted
				
				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Put any cleanups that should happen when player either dies or is force swithed to spec here.
//-----------------------------------------------------------------------------
void CTDCPlayer::CleanupOnDeath( bool bDropItems /*= false*/ )
{
	if ( !IsAlive() )
		return;

	if ( bDropItems )
	{
		// Drop my weapons.
		if ( !TDCGameRules()->IsInstagib() )
		{
			bool bDropped = false;

			for ( int i = 0; i < WeaponCount(); i++ )
			{
				CTDCWeaponBase *pWeapon = GetTDCWeapon( i );
				if ( !pWeapon )
					continue;

				if ( pWeapon->GetTDCWpnData().m_bAlwaysDrop )
				{
					DropWeapon( pWeapon, true );
					bDropped = true;
					break;
				}
			}

			if ( !bDropped )
			{
				DropWeapon( GetActiveTFWeapon(), true );
			}
		}

		RemoveAllAmmo();
		DropPowerups( TDC_POWERUP_DROP_EXPLODE );

		if ( TDCGameRules()->IsBloodMoney() )
		{
			// Drop all money packs I'm carrying plus the one I own.
			int iMaxDrop = tdc_bloodmoney_maxdrop.GetInt();
			int count = iMaxDrop > 0 ? Min( m_nMoneyPacks + 1, iMaxDrop ) : m_nMoneyPacks + 1;

			for ( int i = 0; i < count; i++ )
			{
				CMoneyPack::Create( WorldSpaceCenter(), GetAbsAngles(), this );
			}
		}
	}

	// Remove all conditions...
	m_Shared.RemoveAllCond();

	// Remove all items...
	RemoveAllItems( true );

	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->SendWeaponAnim( ACT_VM_IDLE );
		GetActiveWeapon()->Holster();
		SetActiveWeapon( NULL );
	}

	for ( int iWeapon = 0; iWeapon < WeaponCount(); ++iWeapon )
	{
		CTDCWeaponBase *pWeapon = GetTDCWeapon( iWeapon );

		if ( pWeapon )
		{
			pWeapon->WeaponReset();
		}
	}

	ClearZoomOwner();

	// Hide my cosmetics.
	for ( int i = 0; i < m_hWearables.Count(); i++ )
	{
		if ( m_hWearables[i] )
		{
			m_hWearables[i]->AddEffects( EF_NODRAW );
		}
	}

	m_Shared.SetKillstreak( 0 );
	m_nMoneyPacks = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::Event_Gibbed( const CTakeDamageInfo &info )
{
	// CTDCRagdoll takes care of gibbing.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	if ( CanBecomeRagdoll() )
	{
		VPhysicsDestroyObject();
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_nRenderFX = kRenderFxRagdoll;

		ClampRagdollForce( forceVector, &m_vecForce.GetForModify() );

		SetParent( NULL );

		AddFlag( FL_TRANSRAGDOLL );

		SetMoveType( MOVETYPE_NONE );

		return true;
	}

	return false;
}

ConVar tdc_debug_weapondrop( "tdc_debug_weapondrop", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//			&vecOrigin - 
//			&vecAngles - 
//-----------------------------------------------------------------------------
bool CTDCPlayer::CalculateAmmoPackPositionAndAngles( CTDCWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles )
{
	// Look up the hand and weapon bones.
	int iHandBone = LookupBone( "weapon_bone" );
	if ( iHandBone == -1 )
		return false;

	GetBonePosition( iHandBone, vecOrigin, vecAngles );

	// Draw the position and angles.
	Vector vecDebugForward2, vecDebugRight2, vecDebugUp2;
	AngleVectors( vecAngles, &vecDebugForward2, &vecDebugRight2, &vecDebugUp2 );

	if ( tdc_debug_weapondrop.GetBool() )
	{
		NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugForward2 * 25.0f ), 255, 0, 0, false, 10.0f );
		NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugRight2 * 25.0f ), 0, 255, 0, false, 10.0f );
		NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugUp2 * 25.0f ), 0, 0, 255, false, 10.0f );
	}

	VectorAngles( vecDebugUp2, vecAngles );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Clean up dropped weapons to avoid overpopulation.
//-----------------------------------------------------------------------------
void CTDCPlayer::DroppedWeaponCleanUp( void )
{
	// If we have more than 3 ammo packs out now, destroy the oldest one.
	int iNumPacks = 0;
	CTDCDroppedWeapon *pOldestWeapon = NULL;

	// Cycle through all ammobox in the world and remove them
	for ( CTDCDroppedWeapon *pWeapon : CTDCDroppedWeapon::AutoList() )
	{
		CBaseEntity *pOwner = pWeapon->GetOwnerEntity();

		if ( pOwner == this )
		{
			iNumPacks++;

			// Find the oldest one
			if ( pOldestWeapon == NULL || pOldestWeapon->GetCreationTime() > pWeapon->GetCreationTime() )
			{
				pOldestWeapon = pWeapon;
			}
		}
	}

	// If they have more than 3 packs active, remove the oldest one
	if ( iNumPacks > 3 && pOldestWeapon )
	{
		UTIL_Remove( pOldestWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::DropHealthPack( void )
{
	CHealthKit *pHealth = static_cast<CHealthKit *>( CBaseEntity::CreateNoSpawn( "item_healthkit_small", WorldSpaceCenter(), vec3_angle ) );

	if ( pHealth )
	{
		// Throw a health pack in a random direction.
		Vector vecVelocity;
		QAngle angDir;
		angDir[PITCH] = RandomFloat( -20.0f, -90.0f );
		angDir[YAW] = RandomFloat( -180.0f, 180.0f );
		AngleVectors( angDir, &vecVelocity );
		vecVelocity *= 250.0f;

		pHealth->DropSingleInstance( vecVelocity, NULL, 0.0f, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates tf_dropped_weapon based on selected weapon
//-----------------------------------------------------------------------------
bool CTDCPlayer::DropWeapon( CTDCWeaponBase *pWeapon, bool bKilled /*= false*/, bool bDissolve /*= false*/ )
{
	// Weapons have infinite ammo in instagib so no sense in dropping them.
	if ( TDCGameRules()->IsInstagib() )
		return false;

	if ( !pWeapon || pWeapon->GetTDCWpnData().m_bDontDrop )
	{
		// Can't drop this weapon
		return false;
	}

	int iClip = pWeapon->Clip1();
	int iAmmo = GetAmmoCount( pWeapon->GetPrimaryAmmoType() );

	// Don't drop empty weapons.
	if ( iAmmo == 0 && pWeapon->UsesPrimaryAmmo() && !bDissolve )
		return false;

	// Find the position and angle of the weapons so the dropped entity matches.
	Vector vecPackOrigin;
	QAngle vecPackAngles;
	if ( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
	{
		vecPackOrigin = EyePosition();
		vecPackAngles = EyeAngles();
	}

	// Create dropped weapon entity.
	CTDCDroppedWeapon *pDroppedWeapon = CTDCDroppedWeapon::Create( vecPackOrigin, vecPackAngles, this, pWeapon, bDissolve );
	Assert( pDroppedWeapon );

	if ( pDroppedWeapon )
	{
		// Give the dropped weapon entity our ammo.
		pDroppedWeapon->SetClip( iClip );
		pDroppedWeapon->SetAmmo( iAmmo );
		pDroppedWeapon->SetMaxAmmo( GetMaxAmmo( pWeapon->GetPrimaryAmmoType() ) );

		IPhysicsObject *pPhysObj = pDroppedWeapon->VPhysicsGetObject();

		if ( pPhysObj )
		{
			if ( bKilled )
			{
				// Randomize velocity if we dropped weapon upon being killed.
				Vector vecRight, vecUp;
				AngleVectors( EyeAngles(), NULL, &vecRight, &vecUp );

				// Calculate the initial impulse on the weapon.
				Vector vecImpulse( 0.0f, 0.0f, 0.0f );

				vecImpulse += vecUp * RandomFloat( -0.25, 0.25 );
				vecImpulse += vecRight * RandomFloat( -0.25, 0.25 );
				VectorNormalize( vecImpulse );
				vecImpulse *= RandomFloat( 100.0f, 150.0f );
				vecImpulse += GetAbsVelocity() + m_vecTotalBulletForce * pPhysObj->GetInvMass();

				// Cap the impulse.
				float flSpeed = vecImpulse.Length();
				if ( flSpeed > 300.0f )
				{
					VectorScale( vecImpulse, 300.0f / flSpeed, vecImpulse );
				}

				AngularImpulse angImpulse( 0, RandomFloat( 0, 100 ), 0 );
				pPhysObj->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
			}
			else
			{
				Vector vecForward, vecThrow;
				AngleVectors( EyeAngles(), &vecForward );

				vecThrow = vecForward * 400.0f;
				pPhysObj->SetVelocityInstantaneous( &vecThrow, NULL );
			}
		}

		pDroppedWeapon->m_nSkin = GetTeamSkin( pWeapon->GetTeamNumber() );

		// Clean up old dropped weapons if they exist in the world
		DroppedWeaponCleanUp();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::DropPowerups( ETDCPowerupDropStyle dropStyle )
{
	bool bThrow = (dropStyle == TDC_POWERUP_DROP_THROW);
	for ( int i = 0; g_aPowerups[i].cond != TDC_COND_LAST; i++ )
	{
		ETDCCond nCond = g_aPowerups[i].cond;
		if ( g_aPowerups[i].drop_on_death && m_Shared.InCond( nCond ) )
		{
			Vector vecOrigin = bThrow ? EyePosition() + Vector( 0, 0, -10 ) : WorldSpaceCenter();
			const char *pszClassname = g_aPowerups[i].name;
			float flDuration = m_Shared.GetConditionDuration( nCond );

			CTDCPowerupBase *pPowerup = CTDCPowerupBase::Create( vecOrigin, vec3_angle, this, pszClassname, flDuration );

			if ( pPowerup )
			{
				Vector vecThrow;
				switch (dropStyle)
				{
					case TDC_POWERUP_DROP_THROW:
						// This is a manual throw, launch it forward and remove cond.
						AngleVectors( EyeAngles(), &vecThrow );
						vecThrow *= tdc_player_powerup_throwforce.GetFloat();
						break;
					case TDC_POWERUP_DROP_EXPLODE:
						// This is an "Explosion", launch them everywhere!
						AngleVectors( RandomAngle(-360.0f, 360.0f), &vecThrow );
						vecThrow.z = 1;
						vecThrow *= tdc_player_powerup_explodeforce.GetFloat();
						break;
					default:
						vecThrow = Vector(0, 0, 0);
						break;
				}
				pPowerup->SetAbsVelocity( vecThrow );
				m_Shared.RemoveCond( g_aPowerups[i].cond );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PlayerDeathThink( void )
{
	//overridden, do nothing
}

//-----------------------------------------------------------------------------
// Purpose: Remove the tf items from the player then call into the base class
//          removal of items.
//-----------------------------------------------------------------------------
void CTDCPlayer::RemoveAllItems( bool removeSuit )
{
	// If the player has a capture flag, drop it.
	if ( HasItem() )
	{
		GetItem()->Drop( this, true );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TDC_FLAGEVENT_DROPPED );
			event->SetInt( "priority", 8 );
			gameeventmanager->FireEvent( event );
		}
	}

	Weapon_SetLast( NULL );
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::RemoveAllWeapons( void )
{
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTDCWeaponBase *pWeapon = GetTDCWeapon( i );
		if ( !pWeapon )
			continue;

		pWeapon->UnEquip();
	}

	RemoveAllWearables();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::RemoveAllWearables( void )
{
	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		UTIL_Remove( m_hWearables[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ignite a player
//-----------------------------------------------------------------------------
void CTDCPlayer::InputIgnitePlayer( inputdata_t &inputdata )
{
	m_Shared.Burn( ToTDCPlayer( inputdata.pActivator ), NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Extinguish a player
//-----------------------------------------------------------------------------
void CTDCPlayer::InputExtinguishPlayer( inputdata_t &inputdata )
{
	if ( m_Shared.InCond( TDC_COND_BURNING ) )
	{
		EmitSound( "Player.FlameOut" );
		m_Shared.RemoveCond( TDC_COND_BURNING );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdateModel( void )
{
	SetModel( GetPlayerClass()->GetModelName() );
	m_PlayerAnimState->OnNewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSkin - 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdateSkin( int iTeam )
{
	if ( TDCGameRules()->IsTeamplay() )
	{
		switch ( iTeam )
		{
		case TDC_TEAM_RED:
			m_nSkin = 0;
			break;
		case TDC_TEAM_BLUE:
			m_nSkin = 1;
			break;
		default:
			m_nSkin = 0;
			break;
		}
	}
	else
	{
		m_nSkin = 8;
	}
}

//=========================================================================
// Called when the player disconnects from the server.
void CTDCPlayer::TeamFortress_ClientDisconnected( void )
{
	if ( IsAlive() )
	{
		DispatchParticleEffect( "player_poof", GetAbsOrigin(), vec3_angle );

		// Playing the sound from world since the player is about to be removed.
		CPASAttenuationFilter filter( WorldSpaceCenter(), "Player.Disconnect" );
		EmitSound( filter, 0, "Player.Disconnect", &WorldSpaceCenter() );
	}

	CleanupOnDeath( true );
	RemoveAllOwnedEntitiesFromWorld( false );
	RemoveNemesisRelationships();
	RemoveAllWeapons();
	StopTaunt();
}

//=========================================================================
// Removes everything this player has (buildings, grenades, etc.) from the world
void CTDCPlayer::RemoveAllOwnedEntitiesFromWorld( bool bSilent /* = true */ )
{
	RemoveOwnedProjectiles();
}

//=========================================================================
// Removes all projectiles player has fired into the world.
void CTDCPlayer::RemoveOwnedProjectiles( void )
{
	// TF version
	FOR_EACH_VEC( IBaseProjectileAutoList::AutoList(), i )
	{
		CBaseProjectile *pProjectile = static_cast< CBaseProjectile* >( IBaseProjectileAutoList::AutoList()[i] );
		// If the player owns this entity, remove it.
		if ( pProjectile->GetOwnerEntity() == this )
		{
			pProjectile->SetThink( &CBaseEntity::SUB_Remove );
			pProjectile->SetNextThink( gpGlobals->curtime );
			pProjectile->SetTouch( NULL );
			pProjectile->AddEffects( EF_NODRAW );
		}
	};

#if 0
	// TDC version
	for ( CBaseProjectile *pProjectile : CBaseProjectile::AutoList() )
	{
		// If the player owns this entity, remove it.
		if ( pProjectile->GetOwnerEntity() == this )
		{
			pProjectile->SetThink( &CBaseEntity::SUB_Remove );
			pProjectile->SetNextThink( gpGlobals->curtime );
			pProjectile->SetTouch( NULL );
			pProjectile->AddEffects( EF_NODRAW );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::NoteWeaponFired( CTDCWeaponBase *pWeapon )
{
	if ( m_Shared.InCond( TDC_COND_POWERUP_CLOAK ) )
	{
		// Blink cloak.
		m_Shared.OnSpyTouchedByEnemy();
	}

	// Remove spawn protection in DM.
	if ( m_Shared.InCond( TDC_COND_INVULNERABLE_SPAWN_PROTECT ) )
	{
		m_Shared.RemoveCond( TDC_COND_INVULNERABLE_SPAWN_PROTECT );
	}

	SpeakWeaponFire();

	CTDC_GameStats.Event_PlayerFiredWeapon( this, pWeapon->IsCurrentAttackACrit() );
}

//=============================================================================
//
// Player state functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPlayerStateInfo *CTDCPlayer::StateLookupInfo( int nState )
{
	// This table MUST match the 
	static CPlayerStateInfo playerStateInfos[] =
	{
		{ TDC_STATE_ACTIVE,				"TDC_STATE_ACTIVE",				&CTDCPlayer::StateEnterACTIVE,				NULL,	NULL },
		{ TDC_STATE_WELCOME,				"TDC_STATE_WELCOME",				&CTDCPlayer::StateEnterWELCOME,				NULL,	&CTDCPlayer::StateThinkWELCOME },
		{ TDC_STATE_OBSERVER,			"TDC_STATE_OBSERVER",			&CTDCPlayer::StateEnterOBSERVER,				NULL,	&CTDCPlayer::StateThinkOBSERVER },
		{ TDC_STATE_DYING,				"TDC_STATE_DYING",				&CTDCPlayer::StateEnterDYING,				NULL,	&CTDCPlayer::StateThinkDYING },
	};

	for ( int iState = 0; iState < ARRAYSIZE( playerStateInfos ); ++iState )
	{
		if ( playerStateInfos[iState].m_nPlayerState == nState )
			return &playerStateInfos[iState];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateEnter( int nState )
{
	m_Shared.m_nPlayerState = nState;
	m_pStateInfo = StateLookupInfo( nState );

	// Initialize the new state.
	if ( m_pStateInfo && m_pStateInfo->pfnEnterState )
	{
		(this->*m_pStateInfo->pfnEnterState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateLeave( void )
{
	if ( m_pStateInfo && m_pStateInfo->pfnLeaveState )
	{
		(this->*m_pStateInfo->pfnLeaveState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateTransition( int nState )
{
	StateLeave();
	StateEnter( nState );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateEnterWELCOME( void )
{
	PickWelcomeObserverPoint();  
	
	StartObserverMode( OBS_MODE_FIXED );

	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );		

	PhysObjectSleep();

	if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		ChangeTeam( TEAM_SPECTATOR );
	}
	else if ( (TDCGameRules() && TDCGameRules()->IsLoadingBugBaitReport()) )
	{
		ChangeTeam( TDC_TEAM_BLUE );
		ForceRespawn();
	}
	else
	{
		ShowViewPortPanel( PANEL_MAPINFO, true );
	}

	m_bIsIdle = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::StateThinkWELCOME( void )
{
	if ( IsInCommentaryMode() && !IsFakeClient() )
	{
		ChangeTeam( TDC_TEAM_BLUE );
		ForceRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateEnterACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveEffects( EF_NODRAW | EF_NOSHADOW );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	m_flLastAction = gpGlobals->curtime;
	m_bIsIdle = false;

	// Start thinking to regen myself
	SetContextThink( &CTDCPlayer::RegenThink, gpGlobals->curtime + 1.0f, "RegenThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::SetObserverMode(int mode)
{
	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;

	// Skip OBS_MODE_POI as we're not using that.
	if ( mode == OBS_MODE_POI )
	{
		mode++;
	}

	// Skip over OBS_MODE_ROAMING for dead players
	if( GetTeamNumber() > TEAM_SPECTATOR )
	{
		if ( IsDead() && ( mode > OBS_MODE_FIXED ) && mp_fadetoblack.GetBool() )
		{
			mode = OBS_MODE_CHASE;
		}
		else if ( mode == OBS_MODE_ROAMING )
		{
			mode = OBS_MODE_IN_EYE;
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;
	m_flLastAction = gpGlobals->curtime;

	switch ( mode )
	{
	case OBS_MODE_NONE:
	case OBS_MODE_FIXED :
	case OBS_MODE_DEATHCAM :
		SetFOV( this, 0 );	// Reset FOV
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_NONE );
		break;

	case OBS_MODE_CHASE :
	case OBS_MODE_IN_EYE :	
		// udpate FOV and viewmodels
		SetObserverTarget( m_hObserverTarget );	
		SetMoveType( MOVETYPE_OBSERVER );
		break;

	case OBS_MODE_ROAMING :
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
		
	case OBS_MODE_FREEZECAM:
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
	}

	CheckObserverSettings();

	return true;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateEnterOBSERVER( void )
{
	// Always start a spectator session in chase mode
	m_iObserverLastMode = OBS_MODE_CHASE;

	if( m_hObserverTarget == NULL )
	{
		// find a new observer target
		CheckObserverSettings();
	}

	if ( !m_bAbortFreezeCam )
	{
		FindInitialObserverTarget();
	}

	StartObserverMode( m_iObserverLastMode );

	PhysObjectSleep();

	m_bIsIdle = false;

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		HandleFadeToBlack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::StateThinkOBSERVER()
{
	// Make sure nobody has changed any of our state.
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StateEnterDYING( void )
{
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bPlayedFreezeCamSound = false;
	m_bAbortFreezeCam = false;

	float flRespawnTime = TDCGameRules()->GetNextRespawnWave( GetTeamNumber(), this );
	m_Shared.SetWaitingToRespawn( flRespawnTime != 0.0f );
	if ( flRespawnTime )
		m_Shared.SetRespawnTime( flRespawnTime );
}

//-----------------------------------------------------------------------------
// Purpose: Move the player to observer mode once the dying process is over
//-----------------------------------------------------------------------------
void CTDCPlayer::StateThinkDYING( void )
{
	// If we have a ragdoll, it's time to go to deathcam
	if ( !m_bAbortFreezeCam && m_hRagdoll && 
		(m_lifeState == LIFE_DYING || m_lifeState == LIFE_DEAD) && 
		GetObserverMode() != OBS_MODE_FREEZECAM )
	{
		if ( GetObserverMode() != OBS_MODE_DEATHCAM )
		{
			StartObserverMode( OBS_MODE_DEATHCAM );	// go to observer mode
		}
		RemoveEffects( EF_NODRAW | EF_NOSHADOW );	// still draw player body
	}

	float flTimeInFreeze = spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();
	float flFreezeEnd = (m_flDeathTime + TDC_DEATH_ANIMATION_TIME + flTimeInFreeze );
	if ( !m_bPlayedFreezeCamSound  && GetObserverTarget() && GetObserverTarget() != this )
	{
		// Start the sound so that it ends at the freezecam lock on time
		float flFreezeSoundLength = 0.3;
		float flFreezeSoundTime = (m_flDeathTime + TDC_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() - flFreezeSoundLength;
		if ( gpGlobals->curtime >= flFreezeSoundTime )
		{
			CSingleUserRecipientFilter filter( this );
			EmitSound( filter, entindex(), "Player.FreezeCam" );

			m_bPlayedFreezeCamSound = true;
		}
	}

	if ( gpGlobals->curtime >= (m_flDeathTime + TDC_DEATH_ANIMATION_TIME ) )	// allow x seconds death animation / death cam
	{
		if ( GetObserverTarget() && GetObserverTarget() != this )
		{
			if ( !m_bAbortFreezeCam && gpGlobals->curtime < flFreezeEnd )
			{
				if ( GetObserverMode() != OBS_MODE_FREEZECAM )
				{
					StartObserverMode( OBS_MODE_FREEZECAM );
					PhysObjectSleep();
				}
				return;
			}
		}

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			// If we're in freezecam, and we want out, abort.  (only if server is not using mp_fadetoblack)
			if ( m_bAbortFreezeCam && !mp_fadetoblack.GetBool() )
			{
				if ( m_hObserverTarget == NULL )
				{
					// find a new observer target
					CheckObserverSettings();
				}

				FindInitialObserverTarget();
				SetObserverMode( OBS_MODE_CHASE );
				ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(OBS_MODE_CHASE) );
			}
		}

		// Don't allow anyone to respawn until freeze time is over, even if they're not
		// in freezecam. This prevents players skipping freezecam to spawn faster.
		if ( gpGlobals->curtime < flFreezeEnd )
			return;

		m_lifeState = LIFE_RESPAWNABLE;

		StopAnimation();

		AddEffects( EF_NOINTERP );

		if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
			SetMoveType( MOVETYPE_NONE );

		StateTransition( TDC_STATE_OBSERVER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::AttemptToExitFreezeCam( void )
{
	float flFreezeTravelTime = (m_flDeathTime + TDC_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() + 0.5;
	if ( gpGlobals->curtime < flFreezeTravelTime )
		return;

	m_bAbortFreezeCam = true;
}

int CTDCPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource ammosource )
{
	if ( iCount <= 0 )
		return 0;

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
		return 0;

	int iMaxAmmo = GetMaxAmmo( iAmmoIndex, true );
	int iAmmoCount = GetAmmoCount( iAmmoIndex );
	int iAdd = Min( iCount, iMaxAmmo - iAmmoCount );

	if ( iAdd < 1 )
	{
		return 0;
	}

	float flAlive = GetAliveDuration();
	if ( flAlive > 0 ) //We need to be alive for more than one tick.
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_ammo_gained" );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "amount", iAdd );
			event->SetInt( "ammotype", iAmmoIndex );
			gameeventmanager->FireEvent( event );
		}
	}

	// Ammo pickup sound
	if ( !bSuppressSound )
	{
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	m_iAmmo.Set( iAmmoIndex, m_iAmmo[iAmmoIndex] + iAdd );

	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTDCPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound )
{
	return GiveAmmo( iCount, iAmmoIndex, bSuppressSound, TDC_AMMO_SOURCE_AMMOPACK );
}

//-----------------------------------------------------------------------------
// Purpose: Reset player's information and force him to spawn
//-----------------------------------------------------------------------------
void CTDCPlayer::ForceRespawn( void )
{
	if ( GetTeamNumber() < FIRST_GAME_TEAM )
		return;

	int iDesiredClass = GetSpawnClass();
	if ( iDesiredClass == TDC_CLASS_UNDEFINED )
		return;

	CTDC_GameStats.Event_PlayerForceRespawn( this );

	m_flSpawnTime = gpGlobals->curtime;
	m_bRespawnRequiresAction = true;

	DropFlag();

	if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
	{
		Assert( iDesiredClass > TDC_CLASS_UNDEFINED && iDesiredClass < TDC_CLASS_COUNT_ALL );

		// clean up any pipebombs/buildings in the world (no explosions)
		RemoveAllOwnedEntitiesFromWorld();

		GetPlayerClass()->Init( iDesiredClass );

		CTDC_GameStats.Event_PlayerChangedClass( this );
	}

	m_Shared.RemoveAllCond();

	RemoveAllItems( true );

	// Reset ground state for airwalk animations
	SetGroundEntity( NULL );

	// remove invisibility very quickly	
	m_Shared.FadeInvis( 0.1 );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	StateTransition( TDC_STATE_ACTIVE );
	Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CTDCPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Handle cheat commands
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CTDCPlayer::ImpulseCommands( void )
{
	int iImpulse = GetImpulse();
	
	switch ( iImpulse )
	{
	case 200:
	case 201:
	case 202:
		// Block these, we don't want them.
		break;
	default:
		BaseClass::ImpulseCommands();
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle cheat commands
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CTDCPlayer::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 81:
		GiveNamedItem( "tf_weapon_cubemap" );
		break;
	case 101:
		if ( sv_cheats->GetBool() )
		{
			Restock( true, true );
		}
		break;
	default:
		BaseClass::CheatImpulseCommands( iImpulse );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PlayFlinch( const CTakeDamageInfo &info )
{
	// Don't play flinches if we just died. 
	if ( !IsAlive() )
		return;

	PlayerAnimEvent_t flinchEvent;

	switch ( LastHitGroup() )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchEvent = PLAYERANIMEVENT_FLINCH_HEAD;
		break;
	case HITGROUP_LEFTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_STOMACH:
	case HITGROUP_CHEST:
	case HITGROUP_GEAR:
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchEvent = PLAYERANIMEVENT_FLINCH_CHEST;
		break;
	}

	DoAnimationEvent( flinchEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the crit sound that players that get crit hear
//-----------------------------------------------------------------------------
float CTDCPlayer::PlayCritReceivedSound( void )
{
	float flCritPainLength = 0;
	// Play a custom pain sound to the guy taking the damage
	CSingleUserRecipientFilter receiverfilter( this );
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "Player.CritPain";
	params.m_pflSoundDuration = &flCritPainLength;
	EmitSound( receiverfilter, entindex(), params );

	return flCritPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PainSound( const CTakeDamageInfo &info )
{
	// Don't make sounds if we just died. DeathSound will handle that.
	if ( !IsAlive() )
		return;

	if ( m_flNextPainSoundTime > gpGlobals->curtime )
		return;

	// Don't play falling pain sounds, they have their own system
	if ( info.GetDamageType() & DMG_FALL )
		return;

	if ( info.GetDamageType() & DMG_DROWN )
	{
		EmitSound( "Player.Drown" );
		return;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		// Looping fire pain sound is done in CTDCPlayerShared::ConditionThink
		return;
	}

	float flPainLength = 0;

	// speak a pain concept here, send to everyone but the attacker
	CPASFilter filter( GetSoundEmissionOrigin() );

	// play a crit sound to the victim ( us )
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		flPainLength = PlayCritReceivedSound();

		// remove us from hearing our own pain sound if we hear the crit sound
		filter.RemoveRecipient( this );
	}

	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	// Speak a louder pain concept when low on health.
	int iConcept = GetHealth() < (int)( GetMaxHealth() * 0.25f ) ? MP_CONCEPT_PLAYER_ATTACKER_PAIN : MP_CONCEPT_PLAYER_PAIN;

	if ( SpeakConceptIfAllowed( iConcept, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &filter ) )
	{
		flPainLength = Max( GetSceneDuration( szResponse ), flPainLength );
	}

	m_flNextPainSoundTime = gpGlobals->curtime + flPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::DeathSound( const CTakeDamageInfo &info )
{
	// Don't make death sounds when choosing a class
	if ( IsPlayerClass( TDC_CLASS_UNDEFINED ) )
		return;

	TDCPlayerClassData_t *pData = GetPlayerClass()->GetData();

	if ( m_LastDamageType & DMG_FALL ) // Did we die from falling?
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else if ( m_LastDamageType & DMG_BLAST )
	{
		EmitSound( pData->m_szExplosionDeathSound );
	}
	else if ( m_LastDamageType & DMG_CRITICAL )
	{
		EmitSound( pData->m_szCritDeathSound );

		PlayCritReceivedSound();
	}
	else if ( m_LastDamageType & ( DMG_SLASH | DMG_CLUB ) )
	{
		EmitSound( pData->m_szMeleeDeathSound );
	}
	else
	{
		EmitSound( pData->m_szDeathSound );
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when this player burns another player
//-----------------------------------------------------------------------------
void CTDCPlayer::OnBurnOther( CTDCPlayer *pTFPlayerVictim )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCTeam *CTDCPlayer::GetTDCTeam( void ) const
{
	return assert_cast<CTDCTeam *>( GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Give this player the "i just teleported" effect for 12 seconds
//-----------------------------------------------------------------------------
void CTDCPlayer::TeleportEffect( int iTeam )
{
	m_Shared.AddCond( TDC_COND_TELEPORTED, 12.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PlayerUse( void )
{
	if ( tdc_allow_player_use.GetBool() || IsInCommentaryMode() )
		BaseClass::PlayerUse();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::CreateRagdollEntity( void )
{
	CreateRagdollEntity( 0, 0.0f, TDC_DMG_CUSTOM_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll entity to pass to the client.
//-----------------------------------------------------------------------------
void CTDCPlayer::CreateRagdollEntity( int nFlags, float flInvisLevel, ETDCDmgCustom iDamageCustom )
{
	// If we already have a ragdoll destroy it.
	CTDCRagdoll *pRagdoll = dynamic_cast<CTDCRagdoll*>( m_hRagdoll.Get() );
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	// Create a ragdoll.
	pRagdoll = dynamic_cast<CTDCRagdoll*>( CreateEntityByName( "tdc_ragdoll" ) );
	if ( pRagdoll )
	{
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->m_nForceBone = m_nForceBone;
		Assert( entindex() >= 1 && entindex() <= MAX_PLAYERS );
		pRagdoll->SetOwnerEntity( this );
		pRagdoll->m_nRagdollFlags = nFlags;
		pRagdoll->m_iDamageCustom = iDamageCustom;
		pRagdoll->m_iTeam = GetTeamNumber();
		pRagdoll->m_iClass = GetPlayerClass()->GetClassIndex();
	}

	// Turn off the player.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );
	SetMoveType( MOVETYPE_NONE );

	// Add additional gib setup.
	if ( nFlags & TDC_RAGDOLL_GIB )
	{
		EmitSound( "BaseCombatCharacter.CorpseGib" ); // Squish!
		m_nRenderFX = kRenderFxRagdoll;
	}

	// Save ragdoll handle.
	m_hRagdoll = pRagdoll;
}

// Purpose: Destroy's a ragdoll, called with a player is disconnecting.
//-----------------------------------------------------------------------------
void CTDCPlayer::DestroyRagdoll( void )
{
	CTDCRagdoll *pRagdoll = dynamic_cast<CTDCRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::StopRagdollDeathAnim( void )
{
	CTDCRagdoll *pRagdoll = dynamic_cast<CTDCRagdoll*>( m_hRagdoll.Get() );
	if ( pRagdoll )
	{
		pRagdoll->m_iDamageCustom = TDC_DMG_CUSTOM_NONE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case AE_WPN_HIDE:
	case AE_WPN_UNHIDE:
	case AE_WPN_PLAYWPNSOUND:
		// These are supposed to be processed on client-side only, swallow...
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Weapon_FrameUpdate( void )
{
	BaseClass::Weapon_FrameUpdate();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTDCPlayer::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::Weapon_HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTDCPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ) 
{
	
}

//-----------------------------------------------------------------------------
// Purpose: drops the flag
//-----------------------------------------------------------------------------
CON_COMMAND( dropitem, "Drop the flag." )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer )
	{
		pPlayer->DropFlag();
	}
}

CON_COMMAND( dropweapon, "Drop the current weapon." )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	// Only in teamplay.
	if ( !TDCGameRules()->IsTeamplay() )
		return;

	CTDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
	if ( !pWeapon )
		return;

	// If we fail to create dropped weapon entity don't unequip the weapon.
	if ( pPlayer->DropWeapon( pWeapon ) )
	{
		pPlayer->SwitchToNextBestWeapon( pWeapon );
		pWeapon->UnEquip();
	}
}

CON_COMMAND( droppowerup, "Drop the active powerup." )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	// Only in teamplay.
	if ( !( tdc_player_powerup_allowdrop.GetBool() || TDCGameRules()->IsTeamplay() ) )
		return;

	pPlayer->DropPowerups( TDC_POWERUP_DROP_THROW );
}

class CObserverPoint : public CPointEntity
{
	DECLARE_CLASS( CObserverPoint, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual void Activate( void )
	{
		BaseClass::Activate();

		if ( m_iszAssociateTeamEntityName != NULL_STRING )
		{
			m_hAssociatedTeamEntity = gEntList.FindEntityByName( NULL, m_iszAssociateTeamEntityName );
			if ( !m_hAssociatedTeamEntity )
			{
				Warning("info_observer_point (%s) couldn't find associated team entity named '%s'\n", GetDebugName(), STRING(m_iszAssociateTeamEntityName) );
			}
		}
	}

	bool CanUseObserverPoint( CTDCPlayer *pPlayer )
	{
		if ( m_bDisabled )
			return false;

		if ( m_hAssociatedTeamEntity && ( mp_forcecamera.GetInt() == OBS_ALLOW_TEAM ) )
		{
			// If we don't own the associated team entity, we can't use this point
			if ( m_hAssociatedTeamEntity->GetTeamNumber() != pPlayer->GetTeamNumber() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
				return false;
		}

		return true;
	}

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void InputEnable( inputdata_t &inputdata )
	{
		m_bDisabled = false;
	}
	void InputDisable( inputdata_t &inputdata )
	{
		m_bDisabled = true;
	}
	bool IsDefaultWelcome( void ) { return m_bDefaultWelcome; }

public:
	bool		m_bDisabled;
	bool		m_bDefaultWelcome;
	EHANDLE		m_hAssociatedTeamEntity;
	string_t	m_iszAssociateTeamEntityName;
	float		m_flFOV;
};

BEGIN_DATADESC( CObserverPoint )
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_bDefaultWelcome, FIELD_BOOLEAN, "defaultwelcome" ),
	DEFINE_KEYFIELD( m_iszAssociateTeamEntityName,	FIELD_STRING,	"associated_team_entity" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT,	"fov" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_observer_point, CObserverPoint );
LINK_ENTITY_TO_CLASS( game_intro_viewpoint, CPointEntity ); // For compatibility.

//-----------------------------------------------------------------------------
// Purpose: Builds a list of entities that this player can observe.
//			Returns the index into the list of the player's current observer target.
//-----------------------------------------------------------------------------
int CTDCPlayer::BuildObservableEntityList( void )
{
	m_hObservableEntities.Purge();
	int iCurrentIndex = -1;

	// Check if an override is set.
	if ( TDCGameRules()->GetRequiredObserverTarget() )
	{
		return m_hObservableEntities.AddToTail( TDCGameRules()->GetRequiredObserverTarget() );
		return 0;
	}

	// Add all the map-placed observer points
	CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
	while ( pObserverPoint )
	{
		m_hObservableEntities.AddToTail( pObserverPoint );

		if ( m_hObserverTarget.Get() == pObserverPoint )
		{
			iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
		}

		pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}

	// Add all the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			m_hObservableEntities.AddToTail( pPlayer );

			if ( m_hObserverTarget.Get() == pPlayer )
			{
				iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
			}
		}
	}

	return iCurrentIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1;
	int startIndex = BuildObservableEntityList();
	int iMax = m_hObservableEntities.Count() - 1;

	startIndex += iDir;
	if ( startIndex > iMax )
		startIndex = 0;
	else if ( startIndex < 0 )
		startIndex = iMax;

	return startIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTDCPlayer::FindNextObserverTarget( bool bReverse )
{
	int startIndex = GetNextObserverSearchStartPoint( bReverse );

	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1;

	int iMax = m_hObservableEntities.Count() - 1;

	// Make sure the current index is within the max. Can happen if we were previously
	// spectating an object which has been destroyed.
	if ( startIndex > iMax )
	{
		currentIndex = startIndex = 1;
	}

	do
	{
		CBaseEntity *nextTarget = m_hObservableEntities[currentIndex];

		if ( IsValidObserverTarget( nextTarget ) )
			return nextTarget;

		currentIndex += iDir;

		// Loop through the entities
		if ( currentIndex > iMax )
		{
			currentIndex = 0;
		}
		else if ( currentIndex < 0 )
		{
			currentIndex = iMax;
		}
	} while ( currentIndex != startIndex );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsValidObserverTarget( CBaseEntity * target )
{
	if ( !target )
		return false;

	if ( TDCGameRules()->GetRequiredObserverTarget() && target != TDCGameRules()->GetRequiredObserverTarget() )
		return false;

	if ( target->IsPlayer() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( target );

		if ( pPlayer == this )
			return false; // We can't observe ourselves.

		if ( pPlayer->IsEffectActive( EF_NODRAW ) ) // don't watch invisible players
			return false;

		// Don't spectate dead players.
		if ( !pPlayer->IsAlive() )
		{
			if ( ( pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
			{
				return false;	// allow watching until 3 seconds after death to see death animation
			}
		}
	}
	else
	{
		CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
		if ( pObsPoint && !pObsPoint->CanUseObserverPoint( this ) )
			return false;
	}

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		switch ( mp_forcecamera.GetInt() )
		{
		case OBS_ALLOW_ALL:
			// No team restrictions.
			break;
		case OBS_ALLOW_TEAM:
			// Can only spectate teammates and unassigned cameras.
			if ( target->IsPlayer() )
			{
				if ( GetTeamNumber() == TEAM_UNASSIGNED )
					return false;

				if ( TDCGameRules()->IsTeamplay() )
				{
					if ( IsEnemy( target ) )
						return false;
				}
				else
				{
					if ( !tdc_spec_ffa_allowplayers.GetBool() )
						return false;
				}
			}
			else
			{
				if ( target->GetTeamNumber() != TEAM_UNASSIGNED && GetTeamNumber() != target->GetTeamNumber() )
					return false;
			}
			break;
		case OBS_ALLOW_NONE:
			// Can't spectate anyone.
			return false;
		}
	}

	return true;
}


void CTDCPlayer::PickWelcomeObserverPoint( void )
{
	//Don't just spawn at the world origin, find a nice spot to look from while we choose our team and class.
	CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );

	while ( pObserverPoint )
	{
		if ( IsValidObserverTarget( pObserverPoint ) )
		{
			SetObserverTarget( pObserverPoint );
		}

		if ( pObserverPoint->IsDefaultWelcome() )
			break;

		pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::SetObserverTarget(CBaseEntity *target)
{
	ClearZoomOwner();
	SetFOV( this, 0 );
		
	if ( !BaseClass::SetObserverTarget(target) )
		return false;

	CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
	if ( pObsPoint )
	{
		SetViewOffset( vec3_origin );
		JumptoPosition( pObsPoint->GetAbsOrigin(), pObsPoint->EyeAngles() );

		if ( m_iObserverMode != OBS_MODE_ROAMING )
		{
			SetFOV( pObsPoint, pObsPoint->m_flFOV );
		}
	}

	m_flLastAction = gpGlobals->curtime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest team member within the distance of the origin.
//			Favor players who are the same class.
//-----------------------------------------------------------------------------
CBaseEntity *CTDCPlayer::FindNearestObservableTarget( Vector vecOrigin, float flMaxDist )
{
	CBaseEntity *pReturnTarget = NULL;
	bool bFoundClass = false;
	float flCurDistSqr = (flMaxDist * flMaxDist);

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget( pPlayer ) )
			continue;

		float flDistSqr = ( pPlayer->GetAbsOrigin() - vecOrigin ).LengthSqr();

		if ( flDistSqr < flCurDistSqr )
		{
			// If we've found a player matching our class already, this guy needs
			// to be a matching class and closer to boot.
			if ( !bFoundClass || pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;

				if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
				{
					bFoundClass = true;
				}
			}
		}
		else if ( !bFoundClass )
		{
			if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;
				bFoundClass = true;
			}
		}
	}	

	return pReturnTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::FindInitialObserverTarget( void )
{
	// Find the nearest guy near myself
	CBaseEntity *pTarget = FindNearestObservableTarget( GetAbsOrigin(), FLT_MAX );
	if ( pTarget )
	{
		m_hObserverTarget.Set( pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ValidateCurrentObserverTarget( void )
{
	// If our current target is a dead player who's gibbed / died, refind as if 
	// we were finding our initial target, so we end up somewhere useful.
	if ( m_hObserverTarget && m_hObserverTarget->IsPlayer() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( m_hObserverTarget.Get() );

		if ( !pPlayer->IsAlive() )
		{
			// Once we're past the pause after death, find a new target
			if ( ( pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
			{
				FindInitialObserverTarget();
			}

			return;
		}

		if ( pPlayer->m_Shared.InCond( TDC_COND_TAUNTING ) )
		{
			if ( m_iObserverMode == OBS_MODE_IN_EYE )
			{
				ForceObserverMode( OBS_MODE_CHASE );
			}
		}
	}

	BaseClass::ValidateCurrentObserverTarget();
}

/*void CTDCPlayer::GetPlayerDirection()
{
	N: x, 0, z
	NE: x -y, z
	E: 0, -y, z
	SE: -x, -y, z
	S: -x, 0, z
	SW: -x, y, z
	W: 0, y , z
	NW: x, y , z
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::StartTouch( CBaseEntity* pOther )
{
	BaseClass::StartTouch( pOther );
	CTDCPlayer *pPlayer = ToTDCPlayer( pOther );

	if ( pPlayer )
	{
		if ( m_Shared.InCond( TDC_COND_SPRINT ) )
		{
			Vector vecDir;
			QAngle angDir = GetAbsAngles();
			AngleVectors(angDir, &vecDir);
			
			//float flPitch = AngleNormalize( angDir[PITCH] );

			// If they're on the ground, always push them at least 30 degrees up.

			Vector vecPushDir;
			angDir[PITCH] = -30.0f;
			AngleVectors( angDir, &vecPushDir );

			// Add main angle direction with the pitch
			vecDir += vecPushDir;

			pPlayer->m_Shared.ShovePlayer(this, vecDir, 500);
			pPlayer->EmitSound("Player.AirBlastImpact");
		}

	}


}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Touch( CBaseEntity *pOther )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( pOther );

	if ( pPlayer )
	{
		CheckUncoveringSpies( pPlayer );
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this player has seen through an enemy spy's disguise
//-----------------------------------------------------------------------------
void CTDCPlayer::CheckUncoveringSpies( CTDCPlayer *pTouchedPlayer )
{
	// Only uncover enemies
	if ( !IsEnemy( pTouchedPlayer ) )
	{
		return;
	}

	// Only uncover if they're stealthed
	if ( !pTouchedPlayer->m_Shared.IsStealthed() )
	{
		return;
	}

	// pulse their invisibility
	pTouchedPlayer->m_Shared.OnSpyTouchedByEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::IsAllowedToTaunt( void )
{
	// Already taunting?
	if ( m_Shared.InCond( TDC_COND_TAUNTING ) )
		return false;

	// Check to see if we are in water (above our waist).
	if ( GetWaterLevel() > WL_Waist )
		return false;

	// Check to see if we are on the ground.
	if ( GetGroundEntity() == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::Taunt( void )
{
	if ( !IsAllowedToTaunt() )
		return;

	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();

	m_bInitTaunt = true;
	char szResponse[AI_Response::MAX_RESPONSE_NAME];
	if ( SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_TAUNT, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
	{
		OnTauntSucceeded( szResponse );
	}

	m_bInitTaunt = false;
	pExpresser->DisallowMultipleScenes();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayer::StopTaunt( void )
{
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_Shared.m_flTauntRemoveTime = 0.0f;
		m_hTauntScene = NULL;
	}

	// Restart idle expression.
	ClearExpression();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::OnTauntSucceeded( const char *pszScene )
{
	// Set player state as taunting.
	m_Shared.AddCond( TDC_COND_TAUNTING );
	m_Shared.m_flTauntRemoveTime = gpGlobals->curtime + GetSceneDuration( pszScene ) + 0.2f;

	// Stop idle expressions.
	m_flNextRandomExpressionTime = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::PlayTauntScene( const char *pszScene )
{
	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();

	m_bInitTaunt = true;
	PlayScene( pszScene );

	pExpresser->DisallowMultipleScenes();
}

//-----------------------------------------------------------------------------
// Purpose: Play a one-shot scene
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CTDCPlayer::PlayScene( const char *pszScene, float flDelay, AI_Response *response, IRecipientFilter *filter )
{
	// This is a lame way to detect a taunt!
	if ( m_bInitTaunt )
	{
		m_bInitTaunt = false;
		return InstancedScriptedScene( this, pszScene, &m_hTauntScene, flDelay, false, response, true, filter );
	}
	else
	{
		return InstancedScriptedScene( this, pszScene, NULL, flDelay, false, response, true, filter );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
	{
		info->m_nSequence = LookupSequence( event->GetParameters() );

		// make sure sequence exists
		if ( info->m_nSequence < 0 )
		{
			Warning( "CSceneEntity %s :\"%s\" unable to find sequence \"%s\"\n", STRING( GetEntityName() ), actor->GetName(), event->GetParameters() );
			return false;
		}

		// VCD animations are all assigned to specific gesture slot.
		info->m_iLayer = GESTURE_SLOT_VCD;
		info->m_pActor = actor;
		return true;
	}
	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
	{
		float flCycle = 0.0f;

		bool looping = ( ( GetSequenceFlags( GetModelPtr(), info->m_nSequence ) & STUDIO_LOOPING ) != 0 );
		if ( !looping )
		{
			float dt = scene->GetTime() - event->GetStartTime();
			float seq_duration = SequenceDuration( info->m_nSequence );
			flCycle = clamp( dt / seq_duration, 0.0f, 1.0f );
		}

		if ( !info->m_bStarted )
		{
			m_PlayerAnimState->ResetGestureSlot( info->m_iLayer );
			m_PlayerAnimState->AddVCDSequenceToGestureSlot( info->m_iLayer, info->m_nSequence, flCycle );
			SetLayerCycle( info->m_iLayer, flCycle, flCycle, 0.0f );
		}
		else
		{
			SetLayerCycle( info->m_iLayer, flCycle );
		}

		// Keep layer weight at 1, we don't want taunt animations to be interrupted by anything.
		SetLayerWeight( info->m_iLayer, 1.0f );

		return true;
	}
	default:
		return BaseClass::ProcessSceneEvent( info, scene, event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	// If we have 'disguiseclass' criteria, pretend that we are actually our
	// disguise class. That way we just look up the scene we would play as if 
	// we were that class.
	int disguiseIndex = criteriaSet.FindCriterionIndex( "disguiseclass" );

	if ( disguiseIndex != -1 )
	{
		criteriaSet.AppendCriteria( "playerclass", criteriaSet.GetValue(disguiseIndex) );
	}
	else
	{
		if ( GetPlayerClass() )
		{
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[ GetPlayerClass()->GetClassIndex() ] );
		}
	}

	//criteriaSet.AppendCriteria( "recentkills", UTIL_VarArgs("%d", m_Shared.GetNumKillsInTime(30.0)) );

	int iTotalKills = 0;
	PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( this );
	if ( pStats )
	{
		iTotalKills = pStats->statsCurrentLife.m_iStat[TFSTAT_KILLS] + pStats->statsCurrentLife.m_iStat[TFSTAT_KILLASSISTS]+ 
			pStats->statsCurrentLife.m_iStat[TFSTAT_BUILDINGSDESTROYED];
	}
	criteriaSet.AppendCriteria( "killsthislife", UTIL_VarArgs( "%d", iTotalKills ) );
	criteriaSet.AppendCriteria( "invulnerable", m_Shared.IsInvulnerable() ? "1" : "0" );
	criteriaSet.AppendCriteria( "waitingforplayers", (TDCGameRules()->IsInWaitingForPlayers() || TDCGameRules()->IsInPreMatch()) ? "1" : "0" );
	criteriaSet.AppendCriteria( "teamrole", GetTeamNumber() == TDC_TEAM_DEFENDERS ? "defense" : "offense" );

	// Current weapon role
	CTDCWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		if ( pActiveWeapon->IsWeapon( WEAPON_LEVERRIFLE ) )
		{
			if ( m_Shared.InCond( TDC_COND_ZOOMED ) )
			{
				criteriaSet.AppendCriteria( "sniperzoomed", "1" );
			}
		}
	}

	// Player under crosshair
	trace_t tr;
	Vector forward;
	EyeVectors( &forward );
	UTIL_TraceLine( EyePosition(), EyePosition() + forward * MAX_TRACE_LENGTH, MASK_VISIBLE_AND_NPCS, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && pEntity->IsPlayer() )
		{
			CTDCPlayer *pTFPlayer = ToTDCPlayer(pEntity);
			if ( pTFPlayer )
			{
				int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
				criteriaSet.AppendCriteria( "crosshair_enemy", IsEnemy( pTFPlayer ) ? "Yes" : "No" );

				if ( iClass > TDC_CLASS_UNDEFINED && iClass < TDC_CLASS_COUNT_ALL )
				{
					criteriaSet.AppendCriteria( "crosshair_on", g_aPlayerClassNames_NonLocalized[iClass] );
				}
			}
		}
	}

	// Previous round win
	bool bLoser = ( TDCGameRules()->GetPreviousRoundWinners() != TEAM_UNASSIGNED && TDCGameRules()->GetPreviousRoundWinners() != GetTeamNumber() );
	criteriaSet.AppendCriteria( "LostRound", UTIL_VarArgs( "%d", bLoser ) );

	if ( GetTeamNumber() == TDCGameRules()->GetWinningTeam() )
	{
		criteriaSet.AppendCriteria( "OnWinningTeam", "1" );
	}
	else
	{
		criteriaSet.AppendCriteria( "OnWinningTeam", "0" );
	}

	int iGameRoundState = TDCGameRules()->State_Get();
	criteriaSet.AppendCriteria( "GameRound", UTIL_VarArgs( "%d", iGameRoundState ) );

	bool bIsRedTeam = GetTeamNumber() == TDC_TEAM_RED;
	criteriaSet.AppendCriteria( "OnRedTeam", UTIL_VarArgs( "%d", bIsRedTeam ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship( this, pPlayer ) != GR_TEAMMATE )
		return false;

	// Separate rule for spectators.
	if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM && GetTeamNumber() >= FIRST_GAME_TEAM )
		return tdc_spectalk.GetBool();

	if ( !pPlayer->IsAlive() && IsAlive() )
	{
		// Everyone can chat like normal when the round/game ends
		if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN || TDCGameRules()->State_Get() == GR_STATE_GAME_OVER )
			return true;

		// Living players can't hear dead ones unless gravetalk is enabled.
		return tdc_gravetalk.GetBool();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::CanBeAutobalanced( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CTDCPlayer::GetResponseSystem()
{
	int iClass = GetPlayerClass()->GetClassIndex();
	bool bValidClass = ( iClass >= TDC_FIRST_NORMAL_CLASS && iClass < TDC_CLASS_COUNT_ALL );
	bool bValidConcept = ( m_iCurrentConcept >= 0 && m_iCurrentConcept < MP_TF_CONCEPT_COUNT );
	Assert( bValidClass );
	Assert( bValidConcept );

	if ( !bValidClass || !bValidConcept )
	{
		return BaseClass::GetResponseSystem();
	}
	else
	{
		return TDCGameRules()->m_ResponseRules[iClass].m_ResponseSystems[m_iCurrentConcept];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::SpeakConceptIfAllowed( int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter )
{
	if ( !IsAlive() )
		return false;

	if ( IsSpeaking() )
	{
		if ( iConcept != MP_CONCEPT_DIED )
			return false;
	}

	// Save the current concept.
	m_iCurrentConcept = iConcept;

	return SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdateExpression( void )
{
	AI_Response response;
	m_iCurrentConcept = MP_CONCEPT_PLAYER_EXPRESSION;

	if ( !SpeakFindResponse( response, g_pszMPConcepts[MP_CONCEPT_PLAYER_EXPRESSION] ) )
	{
		ClearExpression();
		m_flNextRandomExpressionTime = gpGlobals->curtime + RandomFloat(30,40);
		return;
	}
	
	// Ignore updates that choose the same scene
	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING(m_iszExpressionScene), response.GetResponsePtr() ) == 0 )
		return;

	if ( m_hExpressionSceneEnt )
	{
		ClearExpression();
	}

	m_iszExpressionScene = AllocPooledString( response.GetResponsePtr() );
	float flDuration = InstancedScriptedScene( this, response.GetResponsePtr(), &m_hExpressionSceneEnt, 0.0, true, NULL, true );
	m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ClearExpression( void )
{
	if ( m_hExpressionSceneEnt != NULL )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}
	m_flNextRandomExpressionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Only show subtitle to enemy if we're disguised as the enemy
//-----------------------------------------------------------------------------
bool CTDCPlayer::ShouldShowVoiceSubtitleToEnemy( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Don't allow rapid-fire voice commands
//-----------------------------------------------------------------------------
bool CTDCPlayer::CanSpeakVoiceCommand( void )
{
	return ( gpGlobals->curtime > m_flNextVoiceCommandTime );
}

//-----------------------------------------------------------------------------
// Purpose: Note the time we're allowed to next speak a voice command
//-----------------------------------------------------------------------------
void CTDCPlayer::NoteSpokeVoiceCommand( const char *pszScenePlayed )
{
	Assert( pszScenePlayed );

	float flSpeakDelay = tdc_max_voice_speak_delay.GetFloat();
	float flSceneDuration = GetSceneDuration( pszScenePlayed );
	if ( flSceneDuration )
		flSpeakDelay = Min( flSceneDuration, flSpeakDelay );
	m_flNextVoiceCommandTime = gpGlobals->curtime + flSpeakDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	// Don't lag compensate dead players.
	if ( pPlayer->m_lifeState != LIFE_ALIVE )
		return false;

	// Don't lag compensate allies.
	if ( !IsEnemy( pPlayer ) )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pPlayer->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// Lag compensate everyone nearby when using flamethrower.
	// FIXME: Magic number! Possibly calculate max range based on cvar values?
	CWeaponFlamethrower *pFlamethrower = static_cast<CWeaponFlamethrower *>( Weapon_OwnsThisID( WEAPON_FLAMETHROWER ) );
	if ( pFlamethrower && pFlamethrower->IsSimulatingFlames() )
		return ( vHisOrigin.DistToSqr( vMyOrigin ) < Square( 1024.0f ) );

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );

	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	float flCosAngle = 0.707107f;	// 45 degree angle
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Mapmaker input to force this player to speak a response rules concept
//-----------------------------------------------------------------------------
void CTDCPlayer::InputSpeakResponseConcept( inputdata_t &inputdata )
{
	int iConcept = GetMPConceptIndexFromString( inputdata.value.String() );
	if ( iConcept != MP_CONCEPT_NONE )
	{
		SpeakConceptIfAllowed( iConcept );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::SpeakWeaponFire( int iCustomConcept )
{
	if ( iCustomConcept == MP_CONCEPT_NONE )
	{
		if ( m_flNextSpeakWeaponFire > gpGlobals->curtime )
			return;

		iCustomConcept = MP_CONCEPT_FIREWEAPON;
	}

	m_flNextSpeakWeaponFire = gpGlobals->curtime + 5;

	// Don't play a weapon fire scene if we already have one
	if ( m_hWeaponFireSceneEnt )
		return;

	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( iCustomConcept, szScene, sizeof( szScene ) ) )
		return;

	float flDuration = InstancedScriptedScene(this, szScene, &m_hExpressionSceneEnt, 0.0, true, NULL, true );
	m_flNextSpeakWeaponFire = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::ClearWeaponFireScene( void )
{
	if ( m_hWeaponFireSceneEnt )
	{
		StopScriptedScene( this, m_hWeaponFireSceneEnt );
		m_hWeaponFireSceneEnt = NULL;
	}
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
}

int CTDCPlayer::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		V_sprintf_safe( tempstr, "Health: %d / %d ( %.1f )", GetHealth(), GetMaxHealth(), (float)GetHealth() / (float)GetMaxHealth() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Get response scene corresponding to concept
//-----------------------------------------------------------------------------
bool CTDCPlayer::GetResponseSceneFromConcept( int iConcept, char *pszSceneBuffer, int numSceneBufferBytes )
{
	AI_Response response;
	bool bResult = SpeakConcept( response, iConcept );

	if ( bResult )
	{
		if ( response.IsApplyContextToWorld() )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( response.GetContext() );
			}
		}
		else
		{
			AddContext( response.GetContext() );
		}

		V_strncpy( pszSceneBuffer, response.GetResponsePtr(), numSceneBufferBytes );
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose:calculate a score for this player. higher is more likely to be switched
//-----------------------------------------------------------------------------
int	CTDCPlayer::CalculateTeamBalanceScore( void )
{
	return BaseClass::CalculateTeamBalanceScore();
}

//-----------------------------------------------------------------------------
// Purpose: Update TF Nav Mesh visibility as the player moves from area to area
//-----------------------------------------------------------------------------
void CTDCPlayer::OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea )
{
#if 0
	VPROF_BUDGET( "CTDCPlayer::OnNavAreaChanged", "NextBot" );

	if ( !IsAlive() || GetTeamNumber() == TEAM_SPECTATOR ) return;

	if ( leftArea )
	{
		leftArea->ForAllPotentiallyVisibleAreas( [=]( CNavArea *pv_area ) {
			static_cast<CTDCNavArea *>( pv_area )->RemovePotentiallyVisibleActor( this );
			return true;
		} );
	}

	if ( enteredArea )
	{
		enteredArea->ForAllPotentiallyVisibleAreas( [=]( CNavArea *pv_area ) {
			static_cast<CTDCNavArea *>( pv_area )->AddPotentiallyVisibleActor( this );
			return true;
		} );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::CalculateTeamScrambleScore( void )
{
	CTDCPlayerResource *pResource = GetTDCPlayerResource();
	if ( !pResource )
	{
		m_flTeamScrambleScore = 0.0f;
		return;
	}

	int iMode = mp_scrambleteams_mode.GetInt();
	float flScore = 0.0f;

	switch ( iMode )
	{
	case TDC_SCRAMBLEMODE_SCORETIME_RATIO:
	default:
	{
		// Points per minute ratio.
		float flTime = GetConnectionTime() / 60.0f;
		float flScore = (float)pResource->GetTotalScore( entindex() );

		flScore = ( flScore / flTime );
		break;
	}
	case TDC_SCRAMBLEMODE_KILLDEATH_RATIO:
	{
		// Don't divide by zero.
		PlayerStats_t *pStats = CTDC_GameStats.FindPlayerStats( this );
		int iKills = pStats->statsAccumulated.m_iStat[TFSTAT_KILLS];
		int iDeaths = Max( 1, pStats->statsAccumulated.m_iStat[TFSTAT_DEATHS] );

		flScore = ( (float)iKills / (float)iDeaths );
		break;
	}
	case TDC_SCRAMBLEMODE_SCORE:
		flScore = (float)pResource->GetTotalScore( entindex() );
		break;
	case TDC_SCRAMBLEMODE_CLASS:
		flScore = (float)m_PlayerClass.GetClassIndex();
		break;
	}

	m_flTeamScrambleScore = flScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// Debugging Stuff
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
void DebugParticles( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );

		// print out their conditions
		pPlayer->m_Shared.DebugPrintConditions();	
	}
}

static ConCommand sv_debug_stuck_particles( "sv_debug_stuck_particles", DebugParticles, "Debugs particles attached to the player under your crosshair.", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestVCD( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity && pEntity->IsPlayer() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( pPlayer )
		{
			if ( args.ArgC() >= 2 )
			{
				InstancedScriptedScene( pPlayer, args[1], NULL, 0.0f, false, NULL, true );
			}
			else
			{
				InstancedScriptedScene( pPlayer, "scenes/heavy_test.vcd", NULL, 0.0f, false, NULL, true );
			}
		}
	}
}
static ConCommand tdc_testvcd( "tdc_testvcd", TestVCD, "Run a vcd on the player currently under your crosshair. Optional parameter is the .vcd name (default is 'scenes/heavy_test.vcd')", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestRR( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("No concept specified. Format is tdc_testrr <concept>\n");
		return;
	}

	CBaseEntity *pEntity = NULL;
	const char *pszConcept = args[1];

	if ( args.ArgC() == 3 )
	{
		pszConcept = args[2];
		pEntity = UTIL_PlayerByName( args[1] );
	}

	if ( !pEntity || !pEntity->IsPlayer() )
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		if ( !pEntity || !pEntity->IsPlayer() )
		{
			pEntity = ToTDCPlayer( UTIL_GetCommandClient() ); 
		}
	}

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( pPlayer )
		{
			int iConcept = GetMPConceptIndexFromString( pszConcept );
			if ( iConcept != MP_CONCEPT_NONE )
			{
				pPlayer->SpeakConceptIfAllowed( iConcept );
			}
			else
			{
				Msg( "Attempted to speak unknown multiplayer concept: %s\n", pszConcept );
			}
		}
	}
}
static ConCommand tdc_testrr( "tdc_testrr", TestRR, "Force the player under your crosshair to speak a response rule concept. Format is tdc_testrr <concept>, or tdc_testrr <player name> <concept>", FCVAR_CHEAT );

CON_COMMAND_F( give_weapon, "Give specified weapon.", FCVAR_CHEAT )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_GetCommandClient() );
	if ( args.ArgC() < 2 )
		return;

	const char *pszWeaponName = args[1];
	ETDCWeaponID iWeaponID = GetWeaponId( pszWeaponName );

	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( iWeaponID );
	if ( !pWeaponInfo )
		return;

	CTDCWeaponBase *pWeapon = (CTDCWeaponBase *)pPlayer->Weapon_GetSlot( pWeaponInfo->iSlot );
	//If we already have a weapon in this slot but is not the same type then nuke it
	if ( pWeapon && pWeapon->GetWeaponID() != iWeaponID )
	{
		pWeapon->UnEquip();
		pWeapon = NULL;
	}

	if ( !pWeapon )
	{
		pPlayer->GiveNamedItem( pWeaponInfo->szClassName, 0, TDC_GIVEAMMO_MAX );
	}
}

uint64 powerplaymask = 0xFAB2423BFFA352AF;
CREATE_DEV_LIST(powerplay_ids, powerplaymask)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::PlayerHasPowerplay( void )
{
	const CSteamID *pSteamID = engine->GetClientSteamID( edict() );
	if ( !pSteamID )
		return false;

	for ( int i = 0; i < ARRAYSIZE( powerplay_ids ); i++ )
	{
		if ( pSteamID->ConvertToUint64() == ( powerplay_ids[i] ^ powerplaymask ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::ShouldAnnouceAchievement( void )
{ 
	return !m_Shared.IsStealthed(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdateDominationsCount( void )
{
	m_iDominations = 0;

	for ( int i = 0; i <= MAX_PLAYERS; i++ )
	{
		if ( m_Shared.m_bPlayerDominated[i] )
			m_iDominations++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CTDCPlayer::PlayerSolidMask( bool brushOnly ) const
{
	unsigned int uMask = 0;

	if ( TDCGameRules()->IsTeamplay() )
	{
		switch ( GetTeamNumber() )
		{
		case TDC_TEAM_RED:
			uMask = CONTENTS_BLUETEAM;
			break;

		case TDC_TEAM_BLUE:
			uMask = CONTENTS_REDTEAM;
			break;
		}
	}
	else
	{
		uMask = CONTENTS_REDTEAM;
	}

	return ( uMask | BaseClass::PlayerSolidMask( brushOnly ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayer::GetClientConVarBoolValue( const char *pszValue )
{
	return !!atoi( engine->GetClientConVarValue( entindex(), pszValue ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayer::GetClientConVarIntValue( const char *pszValue )
{
	return atoi( engine->GetClientConVarValue( entindex(), pszValue ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCPlayer::GetClientConVarFloatValue( const char *pszValue )
{
	return atof( engine->GetClientConVarValue( entindex(), pszValue ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::UpdatePlayerColor( void )
{
	if ( TDCGameRules()->IsTeamplay() )
	{
		// Use team colors.
		m_vecPlayerColor = g_aTeamParticleColors[GetTeamNumber()];
	}
	else
	{
		// Update color from their convars
		Vector vecNewColor;
		vecNewColor.x = GetClientConVarIntValue( "tdc_merc_color_r" ) / 255.0f;
		vecNewColor.y = GetClientConVarIntValue( "tdc_merc_color_g" ) / 255.0f;
		vecNewColor.z = GetClientConVarIntValue( "tdc_merc_color_b" ) / 255.0f;

		m_vecPlayerColor = vecNewColor;
	}
}

void CTDCPlayer::UpdatePlayerSkinTone( void )
{
	const CUtlVector<SkinTone_t> &skinTones = g_TDCPlayerItems.GetSkinTones();
	if ( skinTones.IsEmpty() )
		return;

	int iToneIdx = GetClientConVarIntValue( "tdc_merc_skintone" );

	iToneIdx %= skinTones.Count();

	Vector vecNewColor = skinTones[iToneIdx].tone;
	vecNewColor /= 255.0f;

	m_vecPlayerSkinTone = vecNewColor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::OnPickedUpWeapon( CBaseEntity *pItem )
{
	m_bJustPickedWeapon = true;
	m_Shared.SetDesiredWeaponIndex( WEAPON_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::AddWearable( CTDCWearable *pWearable )
{
	ETDCWearableSlot iSlot = pWearable->GetStaticData()->slot;
	Assert( m_hWearables[iSlot].Get() == NULL );
	if ( m_hWearables[iSlot] )
	{
		UTIL_Remove( m_hWearables[iSlot] );
	}

	m_hWearables.Set( iSlot, pWearable );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayer::RemoveWearable( CTDCWearable *pWearable )
{
	ETDCWearableSlot iSlot = pWearable->GetStaticData()->slot;
	Assert( m_hWearables[iSlot].Get() == pWearable );
	if ( m_hWearables[iSlot].Get() == pWearable )
	{
		m_hWearables.Set( iSlot, NULL );
	}
}

extern IResponseSystem *g_pResponseSystem;
CChoreoScene *BlockingLoadScene( const char *filename );

bool WriteSceneFile( const char *filename )
{
	CChoreoScene *pScene = BlockingLoadScene( filename );

	if ( pScene )
	{
		if ( !filesystem->FileExists( filename ) )
		{
			char szDir[MAX_PATH];
			V_strcpy_safe( szDir, filename );
			V_StripFilename( szDir );

			filesystem->CreateDirHierarchy( szDir );
			if ( pScene->SaveToFile( filename ) )
			{
				Msg( "Wrote %s\n", filename );
			}
		}

		delete pScene;
		return true;
	}

	return false;
}

CON_COMMAND_F( tdc_writevcds_rr, "Writes all VCD files referenced by response rules.", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CUtlVector<AI_Response *> responses;
	g_pResponseSystem->GetAllResponses( &responses );

	FOR_EACH_VEC( responses, i )
	{
		if ( responses[i]->GetType() != RESPONSE_SCENE )
			continue;

		// fixup $gender references
		char file[_MAX_PATH];
		V_strcpy_safe( file, responses[i]->GetNamePtr() );
		char *gender = strstr( file, "$gender" );
		if ( gender )
		{
			// replace with male & female
			const char *postGender = gender + strlen( "$gender" );
			*gender = 0;
			char genderFile[_MAX_PATH];
			// male
			V_sprintf_safe( genderFile, "%smale%s", file, postGender );

			WriteSceneFile( genderFile );

			V_sprintf_safe( genderFile, "%sfemale%s", file, postGender );

			WriteSceneFile( genderFile );
		}
		else
		{
			WriteSceneFile( file );
		}
	}

	responses.PurgeAndDeleteElements();
}

CON_COMMAND_F( tdc_writevcd_name, "Writes VCD with the specified name.", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
		return;

	WriteSceneFile( args[1] );
}

extern ISoundEmitterSystemBase *soundemitterbase;

CON_COMMAND_F( tdc_generatevcds, "Generates VCDs for all sounds of the player's class.", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY )
{
	// Dedicated server has no sounds.
	if ( engine->IsDedicatedServer() )
		return;

	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	const char *pszClassname = g_aPlayerClassNames_NonLocalized[pPlayer->GetPlayerClass()->GetClassIndex()];
	int len = V_strlen( pszClassname );

	// Generate VCDs for all soundscripts starting with my class name.
	for ( int i = soundemitterbase->First(); i != soundemitterbase->InvalidIndex(); i = soundemitterbase->Next( i ) )
	{
		const char *pszSoundScriptName = soundemitterbase->GetSoundName( i );
		if ( !pszSoundScriptName || V_strnicmp( pszSoundScriptName, pszClassname, len ) != 0 )
			continue;

		const char *pszWavName = soundemitterbase->GetWavFileForSound( pszSoundScriptName, GENDER_MALE );

		float duration = enginesound->GetSoundDuration( pszWavName );
		if ( duration <= 0.0f )
		{
			Warning( "Couldn't determine duration of %s\n", pszWavName );
			continue;
		}

		CChoreoScene *scene = new CChoreoScene( NULL );
		if ( !scene )
		{
			Warning( "Failed to allocated new scene!!!\n" );
		}
		else
		{
			CChoreoActor *actor = scene->AllocActor();
			CChoreoChannel *channel = scene->AllocChannel();
			CChoreoEvent *event = scene->AllocEvent();

			Assert( actor );
			Assert( channel );
			Assert( event );

			if ( !actor || !channel || !event )
			{
				Warning( "CSceneEntity::GenerateSceneForSound:  Alloc of actor, channel, or event failed!!!\n" );
				delete scene;
				continue;
			}

			// Set us up the actorz
			actor->SetName( pszClassname );  // Could be pFlexActor->GetName()?
			actor->SetActive( true );

			// Set us up the channelz
			channel->SetName( "audio" );
			channel->SetActor( actor );

			// Add to actor
			actor->AddChannel( channel );

			// Set us up the eventz
			event->SetType( CChoreoEvent::SPEAK );
			event->SetName( pszSoundScriptName );
			event->SetParameters( pszSoundScriptName );
			event->SetStartTime( 0.0f );
			event->SetUsingRelativeTag( false );
			event->SetEndTime( duration );
			event->SnapTimes();

			// Add to channel
			channel->AddEvent( event );

			// Point back to our owners
			event->SetChannel( channel );
			event->SetActor( actor );

			// Now write it to VCD file.
			char szFile[MAX_PATH];
			char szWavNameNoExt[MAX_PATH];
			V_StripExtension( V_GetFileName( pszWavName ), szWavNameNoExt, MAX_PATH );
			V_sprintf_safe( szFile, "scenes/player/%s/low/%s.vcd", pszClassname, szWavNameNoExt );

			if ( !filesystem->FileExists( szFile ) )
			{
				char szDir[MAX_PATH];
				V_strcpy_safe( szDir, szFile );
				V_StripFilename( szDir );

				filesystem->CreateDirHierarchy( szDir );
				if ( scene->SaveToFile( szFile ) )
				{
					Msg( "Wrote %s\n", szFile );
				}
			}

			delete scene;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCPlayer::GetTimeSinceLastInjuryByAnyEnemyTeam() const
{
	float flTimeSinceInjured = FLT_MAX;

	ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeamNum ) {
		flTimeSinceInjured = Min( flTimeSinceInjured, GetTimeSinceLastInjury( iTeamNum ) );
		return true;
	} );

	return flTimeSinceInjured;
}

//-----------------------------------------------------------------------------
// Purpose: Set nearby nav areas "in-combat" so bots are aware of the danger
//-----------------------------------------------------------------------------
void CTDCPlayer::OnMyWeaponFired( CBaseCombatWeapon *weapon )
{
	BaseClass::OnMyWeaponFired( weapon );

#if 0
	if ( !m_ctNavCombatUpdate.IsElapsed() )
		return;

	auto pTFWeapon = static_cast<CTDCWeaponBase *>( weapon );
	if ( pTFWeapon->TFNavMesh_ShouldRaiseCombatLevelWhenFired() )
	{
		m_ctNavCombatUpdate.Start( 1.0f );

		GetLastKnownTFArea()->AddCombatToSurroundingAreas();
	}
#endif
}
