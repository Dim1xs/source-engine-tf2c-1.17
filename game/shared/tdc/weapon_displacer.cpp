//=============================================================================
//
// Purpose: TELEMAX DISPLACER CANNON
// 
// Compucolor Pictures
//
//=============================================================================
#include "cbase.h"
#include "weapon_displacer.h"
#include "tdc_gamerules.h"
#include "particle_parse.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#include "tdc_projectile_plasma.h"
#else
#include "c_tdc_player.h"
#include "soundenvelope.h"
#endif

#define WEAPON_DISPLACER_TELEPORT_RECHARGE_TIME 30.0f

//=============================================================================
//
// Weapon Displacer tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDisplacer, DT_WeaponDisplacer );
BEGIN_NETWORK_TABLE( CWeaponDisplacer, DT_WeaponDisplacer )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flBlastTime ) ),
	RecvPropTime( RECVINFO( m_flTeleportTime ) ),
	RecvPropTime( RECVINFO( m_flTeleportRechargeTime ) ),
	RecvPropBool( RECVINFO( m_bLockedOn ) ),
	RecvPropBool( RECVINFO( m_bIdle ) ),
	RecvPropEHandle( RECVINFO( m_hHolsterModel ) ),
#else
	SendPropTime( SENDINFO( m_flBlastTime ) ),
	SendPropTime( SENDINFO( m_flTeleportTime ) ),
	SendPropTime( SENDINFO( m_flTeleportRechargeTime ) ),
	SendPropBool( SENDINFO( m_bLockedOn ) ),
	SendPropBool( SENDINFO( m_bIdle ) ),
	SendPropEHandle( SENDINFO( m_hHolsterModel ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDisplacer )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flBlastTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flTeleportTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flTeleportRechargeTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bLockedOn, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIdle, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_displacer, CWeaponDisplacer );
PRECACHE_WEAPON_REGISTER( weapon_displacer );

acttable_t CWeaponDisplacer::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_DISPLACER,			false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_DISPLACER,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_DISPLACER,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_DISPLACER,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_DISPLACER,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_DISPLACER,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_DISPLACER,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_DISPLACER,		false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_DISPLACER,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_DISPLACER,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_DISPLACER,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_DISPLACER,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_DISPLACER,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponDisplacer );

#ifdef GAME_DLL
extern EHANDLE g_pLastSpawnPoints[TDC_TEAM_COUNT];
#endif

//=============================================================================
//
// Weapon Displacer functions.
//

CWeaponDisplacer::CWeaponDisplacer()
{
#ifdef CLIENT_DLL
	m_pIdleEffect = NULL;
	m_pIdleSound = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::Precache( void )
{
	BaseClass::Precache();

#ifdef GAME_DLL
	PrecacheScriptSound( "Weapon_Displacer.Idle" );

	PrecacheTeamParticles( "displacer_chargeup_%s", true );
	PrecacheTeamParticles( "displacer_idle_%s", true );
	PrecacheTeamParticles( "displacer_teleport_warning_%s", true );
	PrecacheTeamParticles( "displacer_teleported_away_%s", true );
	PrecacheTeamParticles( "displacer_teleportedin_%s", true );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_flBlastTime = 0.0f;
	m_flTeleportTime = 0.0f;
	m_bLockedOn = false;

#ifdef GAME_DLL
	m_hTeleportSpot = NULL;

	// Normally when we call WeaponReset() player has no active weapon.
	// So holster model will be hidden once player switches to Displacer.
	ShowHolsterModel( true );
#else
	ManageIdleEffect();
	UpdateIdleSound();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponDisplacer::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
#ifdef CLIENT_DLL
		ManageIdleEffect();
		UpdateIdleSound();
#endif

		ShowHolsterModel( false );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponDisplacer::CanHolster( void ) const
{
	// Can't holster while charging up a shot.
	if ( IsChargingAnyShot() )
	{
		return false;
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponDisplacer::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flBlastTime = 0.0f;
	m_flTeleportTime = 0.0f;
	m_bLockedOn = false;

#ifdef GAME_DLL
	m_hTeleportSpot = NULL;
#else
	m_iState = WEAPON_IS_CARRIED_BY_PLAYER;
	ManageIdleEffect();
	UpdateIdleSound();
#endif

	ShowHolsterModel( true );

	bool bRet = BaseClass::Holster( pSwitchingTo );

#ifdef GAME_DLL
	if ( !HasAnyAmmo() && pSwitchingTo )
	{
		// Drop and dissolve when out of ammo.
		CTDCPlayer *pOwner = GetTDCPlayerOwner();

		if ( pOwner )
		{
			pOwner->DropWeapon( this, false, true );
			pOwner->Weapon_Detach( this );
			UTIL_Remove( this );
		}
	}
#endif

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDisplacer::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

#ifdef GAME_DLL
	// Create a holstered model and attach it to player's back.
	CBaseEntity *pEnt = CBaseEntity::Create( "weapon_holstermodel", GetAbsOrigin(), GetAbsAngles(), pOwner );

	if ( pEnt )
	{
		pEnt->SetModel( GetWorldModel() );
		pEnt->SetParent( pOwner, pOwner->LookupAttachment( "flag" ) );
		pEnt->SetLocalOrigin( vec3_origin );
		pEnt->SetLocalAngles( QAngle( 0, 90, 0 ) );

		m_hHolsterModel = pEnt;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::ItemPostFrame( void )
{
	if ( m_flBlastTime != 0.0f && gpGlobals->curtime >= m_flBlastTime )
	{
		// Deliberately skipping to base class since our function starts charging.
		m_flBlastTime = 0.0f;
		BaseClass::PrimaryAttack();
	}
	else if ( m_flTeleportTime != 0.0f && gpGlobals->curtime >= m_flTeleportTime )
	{
		FinishTeleport();
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::PrimaryAttack( void )
{
	if ( !CanAttack() )
		return;

	// Already charging up a shot?
	if ( IsChargingAnyShot() )
		return;

	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	SendWeaponAnim( ACT_VM_PULLBACK );

	// Start charging.
	WeaponSound( SPECIAL2 );
	m_flBlastTime = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flSmackDelay;

#ifdef CLIENT_DLL
	CreateWarmupEffect( false );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::SecondaryAttack( void )
{
	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Already charging up a shot?
	if ( IsChargingAnyShot() )
		return;

	if ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) < GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot )
	{
		if ( gpGlobals->curtime >= m_flNextEmptySoundTime )
		{
			WeaponSound( EMPTY );
			m_flNextEmptySoundTime = gpGlobals->curtime + 0.5f;
		}

		return;
	}

	m_iWeaponMode = TDC_WEAPON_SECONDARY_MODE;

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

#ifdef GAME_DLL
	// Find a furthest possible respawn point.
	CBaseEntity *pSpot = g_pLastSpawnPoints[pPlayer->GetTeamNumber()];

	if ( pPlayer->SelectFurthestSpawnSpot( "info_player_teamspawn", pSpot, false ) )
	{
		// Gotta remove prediction filtering since this code runs on server side only.
		CDisablePredictionFiltering disabler;

		// Create a warning effect for other players at the chosen destination.
		const char *pszTeleportedEffect = ConstructTeamParticle( "displacer_teleport_warning_%s", pPlayer->GetTeamNumber(), true );
		DispatchParticleEffect( pszTeleportedEffect, pSpot->GetAbsOrigin(), vec3_angle, pPlayer->m_vecPlayerColor, vec3_origin, true );

		g_pLastSpawnPoints[pPlayer->GetTeamNumber()] = pSpot;
		m_hTeleportSpot = pSpot;
		m_bLockedOn = true;
	}
	else
	{
		m_bLockedOn = false;
	}
#else
	m_bLockedOn = true;
#endif

	WeaponSound( SPECIAL3 );

	m_flTeleportTime = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponDisplacer::CanPerformSecondaryAttack( void ) const
{
	if ( gpGlobals->curtime < m_flTeleportRechargeTime )
		return false;

	return BaseClass::CanPerformSecondaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::FireProjectile( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Play shoot sound.
	PlayWeaponShootSound();

	// Send animations.
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	// Server only - create the rocket.
#ifdef GAME_DLL

	Vector vecSrc;
	Vector vecDir;
	GetProjectileFireSetup( pPlayer, Vector( 23.5f, 0.0f, -3.0f ), vecSrc, vecDir, false );

	Vector vecVelocity = vecDir * GetProjectileSpeed();
	float flDamage = GetProjectileDamage();
	float flRadius = GetTDCWpnData().m_flDamageRadius;
	bool bCritical = IsCurrentAttackACrit();

	CProjectile_Plasma::Create( vecSrc, vecVelocity, flDamage, flRadius, bCritical, pPlayer, this );

#endif

	// Subtract ammo
	int iAmmoCost = GetAmmoPerShot();
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 -= iAmmoCost;
	}
	else
	{
		pPlayer->RemoveAmmo( iAmmoCost, m_iPrimaryAmmoType );
	}

	// Do visual effects.
	DoMuzzleFlash();
	AddViewKick();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponDisplacer::SendWeaponAnim( int iActivity )
{
	m_bIdle = ( iActivity == ACT_VM_IDLE ||
		iActivity == ACT_VM_FIDGET ||
		iActivity == ACT_VM_SECONDARYATTACK );

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDisplacer::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	m_iState = WEAPON_NOT_CARRIED;
	ManageIdleEffect();
	UpdateIdleSound();
#else
	UTIL_Remove( m_hHolsterModel.Get() );
#endif

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponDisplacer::GetEffectBarProgress( void )
{
	float flTimeLeft = m_flTeleportRechargeTime - gpGlobals->curtime;

	if ( flTimeLeft > 0.0f )
	{
		return RemapValClamped( flTimeLeft,
			WEAPON_DISPLACER_TELEPORT_RECHARGE_TIME, 0.0f,
			0.0f, 1.0f );
	}

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponDisplacer::IsChargingAnyShot( void ) const
{
	return ( m_flBlastTime || m_flTeleportTime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::FinishTeleport( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer )
	{
#ifdef GAME_DLL
		AssertMsg( m_hTeleportSpot.Get() != NULL, "Displacer teleport was triggered with no destination." );
		bool bReady = ( m_hTeleportSpot.Get() != NULL );
#else
		bool bReady = m_bLockedOn;
#endif
		if ( bReady )
		{
#ifdef GAME_DLL
			// Gotta remove prediction filtering since this code runs on server side only.
			CDisablePredictionFiltering *pDisabler = new CDisablePredictionFiltering();

			// Telelefrag anyone in the way.
			CBaseEntity *pList[MAX_PLAYERS];
			Vector vecMins = m_hTeleportSpot->GetAbsOrigin() + VEC_HULL_MIN_SCALED( pPlayer );
			Vector vecMaxs = m_hTeleportSpot->GetAbsOrigin() + VEC_HULL_MAX_SCALED( pPlayer );
			int count = UTIL_EntitiesInBox( pList, MAX_PLAYERS, vecMins, vecMaxs, FL_CLIENT );

			for ( int i = 0; i < count; i++ )
			{
				CBaseEntity *pEntity = pList[i];
				if ( pEntity != pPlayer && pPlayer->IsEnemy( pEntity ) )
				{
					CTakeDamageInfo info( pPlayer, pPlayer, 1000, DMG_CRUSH | DMG_ALWAYSGIB, TDC_DMG_DISPLACER_TELEFRAG );
					pEntity->TakeDamage( info );
				}
			}

			// Play departure effect.
			const char *pszTeleportedEffect = ConstructTeamParticle( "displacer_teleported_away_%s", pPlayer->GetTeamNumber(), true );
			DispatchParticleEffect( pszTeleportedEffect, pPlayer->GetAbsOrigin(), vec3_angle, pPlayer->m_vecPlayerColor, vec3_origin, !TDCGameRules()->IsTeamplay() );

			pPlayer->Teleport( &m_hTeleportSpot->GetAbsOrigin(), &m_hTeleportSpot->GetAbsAngles(), &vec3_origin );

			// Play arrival effect.
			pszTeleportedEffect = ConstructTeamParticle( "displacer_teleportedin_%s", pPlayer->GetTeamNumber(), true );
			DispatchParticleEffect( pszTeleportedEffect, m_hTeleportSpot->GetAbsOrigin(), vec3_angle, pPlayer->m_vecPlayerColor, vec3_origin, !TDCGameRules()->IsTeamplay() );

			pPlayer->TeleportEffect( GetTeamNumber() );

			pPlayer->SetFOV( pPlayer, 0, 0.5f, pPlayer->GetDefaultFOV() + 30 );

			color32 fadeColor = { 255, 255, 255, 100 };
			UTIL_ScreenFade( pPlayer, fadeColor, 0.25, 0.4, FFADE_IN );

			delete pDisabler;
#endif

			WeaponSound( WPN_DOUBLE );
			pPlayer->RemoveAmmo( GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, GetSecondaryAmmoType() );
			m_flTeleportRechargeTime = gpGlobals->curtime + WEAPON_DISPLACER_TELEPORT_RECHARGE_TIME;
		}
	}

#ifdef GAME_DLL
	m_hTeleportSpot = NULL;
#endif

	m_flTeleportTime = 0.0f;
	m_bLockedOn = false;
	m_flNextSecondaryAttack = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::ShowHolsterModel( bool bShow )
{
	if ( m_hHolsterModel.Get() == NULL )
		return;

	// Don't show the model when player is dead.
	CTDCPlayer *pOwner = GetTDCPlayerOwner();

	if ( bShow && pOwner && pOwner->m_Shared.GetState() == TDC_STATE_ACTIVE )
	{
		m_hHolsterModel->RemoveEffects( EF_NODRAW );
	}
	else
	{
		m_hHolsterModel->AddEffects( EF_NODRAW );
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CWeaponDisplacer::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#else
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bWasChargingBlast = ( m_flBlastTime != 0.0f );
	m_bWasChargingTeleport = ( m_flTeleportTime != 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( !m_bWasChargingBlast && m_flBlastTime != 0.0f && !GetPredictable() )
	{
		CreateWarmupEffect( false );
	}

	ManageIdleEffect();
	UpdateIdleSound();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	if ( m_pIdleEffect )
	{
		if ( m_hEffectHost.Get() )
		{
			m_hEffectHost->ParticleProp()->StopEmissionAndDestroyImmediately( m_pIdleEffect );
		}

		m_pIdleEffect = NULL;

		ManageIdleEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::CreateWarmupEffect( bool bSecondary )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	C_BaseEntity *pEntity = GetWeaponForEffect();

	const char *pszEffect = ConstructTeamParticle( "displacer_chargeup_%s", pPlayer->GetTeamNumber(), true );
	DispatchParticleEffect( pszEffect, PATTACH_POINT_FOLLOW, pEntity, "muzzle", pPlayer->m_vecPlayerColor, vec3_origin, !TDCGameRules()->IsTeamplay() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::ManageIdleEffect( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !IsHolstered() && m_bIdle )
	{
		if ( !m_pIdleEffect )
		{
			C_BaseEntity *pModel = GetWeaponForEffect();
			const char *pszEffect = ConstructTeamParticle( "displacer_idle_%s", pPlayer->GetTeamNumber(), true );

			m_pIdleEffect = pModel->ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "muzzle" );
			m_hEffectHost = pModel;

			pPlayer->m_Shared.SetParticleToMercColor( m_pIdleEffect );
		}
	}
	else if ( m_pIdleEffect )
	{
		if ( m_hEffectHost.Get() )
		{
			m_hEffectHost->ParticleProp()->StopEmissionAndDestroyImmediately( m_pIdleEffect );
		}

		m_pIdleEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDisplacer::UpdateIdleSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_iState == WEAPON_IS_ACTIVE )
	{
		if ( !m_pIdleSound )
		{
			CLocalPlayerFilter filter;
			m_pIdleSound = controller.SoundCreate( filter, entindex(), "Weapon_Displacer.Idle" );
			controller.Play( m_pIdleSound, 1.0f, 100 );
		}
	}
	else
	{
		if ( m_pIdleSound )
		{
			controller.SoundDestroy( m_pIdleSound );
			m_pIdleSound = NULL;
		}
	}
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponHolster, DT_WeaponHoster );
BEGIN_NETWORK_TABLE( CWeaponHolster, DT_WeaponHoster )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( weapon_holstermodel, CWeaponHolster );

CWeaponHolster::CWeaponHolster()
{
	SetPredictionEligible( true );
}

#ifdef GAME_DLL
int CWeaponHolster::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

int CWeaponHolster::ShouldTransmit( CCheckTransmitInfo *pInfo )
{
	// Always transmit to the owner.
	if ( GetOwnerEntity() && pInfo->m_pClientEnt == GetOwnerEntity()->edict() )
	{
		return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}
#else
bool CWeaponHolster::ShouldDraw( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwnerEntity() );

	if ( !pPlayer || !pPlayer->ShouldDrawThisPlayer() )
		return false;

	return BaseClass::ShouldDraw();
}

ShadowType_t CWeaponHolster::ShadowCastType( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwnerEntity() );

	if ( !pPlayer || !pPlayer->ShouldDrawThisPlayer() )
		return SHADOWS_NONE;

	return BaseClass::ShadowCastType();
}
#endif
