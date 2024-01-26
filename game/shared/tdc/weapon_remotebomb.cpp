//=============================================================================
//
// Purpose: RC Bomb
//
//=============================================================================
#include "cbase.h"
#include "weapon_remotebomb.h"
#include "in_buttons.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#else
#include "c_tdc_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME: Hardcoded bomb size, need to update if the model is changed.
static const Vector g_vecBombSize( 8, 8, 8 );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponRemoteBomb, DT_WeaponRemoteBomb )
BEGIN_NETWORK_TABLE( CWeaponRemoteBomb, DT_WeaponRemoteBomb )
#ifdef GAME_DLL
	SendPropTime( SENDINFO( m_flTimeToThrow ) ),
	SendPropTime( SENDINFO( m_flLiveTime ) ),
	SendPropBool( SENDINFO( m_bAlternateThrow ) ),
	SendPropInt( SENDINFO( m_iScreenState ), 2, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flScreenOffTime ) ),
#else
	RecvPropTime( RECVINFO( m_flTimeToThrow ) ),
	RecvPropTime( RECVINFO( m_flLiveTime ) ),
	RecvPropBool( RECVINFO( m_bAlternateThrow ) ),
	RecvPropInt( RECVINFO( m_iScreenState ) ),
	RecvPropTime( RECVINFO( m_flScreenOffTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponRemoteBomb )
	DEFINE_PRED_FIELD( m_flTimeToThrow, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLiveTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAlternateThrow, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iScreenState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flScreenOffTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_remotebomb, CWeaponRemoteBomb );
PRECACHE_WEAPON_REGISTER( weapon_remotebomb );

acttable_t CWeaponRemoteBomb::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_REMOTEBOMB,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_REMOTEBOMB,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_REMOTEBOMB,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_REMOTEBOMB,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_REMOTEBOMB,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_REMOTEBOMB,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_REMOTEBOMB,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_REMOTEBOMB,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_REMOTEBOMB,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_REMOTEBOMB,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_REMOTEBOMB,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_REMOTEBOMB,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_REMOTEBOMB,	false },
};

acttable_t CWeaponRemoteBomb::m_acttable_alt[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_REMOTEBOMB_ALT,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_REMOTEBOMB_ALT,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_REMOTEBOMB_ALT,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_REMOTEBOMB_ALT,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_REMOTEBOMB_ALT,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_REMOTEBOMB_ALT,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_REMOTEBOMB_ALT,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_REMOTEBOMB_ALT,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_REMOTEBOMB_ALT,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_REMOTEBOMB_ALT,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_REMOTEBOMB_ALT,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_REMOTEBOMB_ALT,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_REMOTEBOMB_ALT,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponRemoteBomb );

CWeaponRemoteBomb::CWeaponRemoteBomb()
{
	m_bInDetonation = false;

#ifdef CLIENT_DLL
	m_iBombBodygroup = -1;
	m_iRemoteBodygroup = -1;
	m_iScreenBodygroup = -1;
	m_OldWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	m_iOldScreenState = BOMBSCREEN_OFF;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponRemoteBomb::HasPrimaryAmmo( void )
{
	// Allow using it with no bombs 
	if ( m_iWeaponMode == TDC_WEAPON_SECONDARY_MODE )
		return true;

	return BaseClass::HasPrimaryAmmo();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponRemoteBomb::CanBeSelected( void )
{
	if ( m_iWeaponMode == TDC_WEAPON_SECONDARY_MODE )
		return true;

	return BaseClass::CanBeSelected();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponRemoteBomb::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bInAttack = false;
	m_bInAttack2 = false;
	m_flTimeToThrow = 0.0f;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef GAME_DLL
	// Destroy the bomb.
	if ( m_hBomb )
	{
		m_hBomb->Fizzle();
	}
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	m_flTimeToThrow = 0.0f;
	m_iScreenState = BOMBSCREEN_OFF;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::ItemPostFrame( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( m_iScreenState > BOMBSCREEN_ACTIVE && gpGlobals->curtime >= m_flScreenOffTime )
	{
		ChangeScreenState( BOMBSCREEN_OFF );
	}

	// Lauch the bomb once the delay expires and the button is released.
	if ( m_flTimeToThrow != 0.0f && gpGlobals->curtime >= m_flTimeToThrow )
	{
		if ( !m_bAlternateThrow )
		{
			if ( !( pPlayer->m_nButtons & IN_ATTACK ) )
			{
				LaunchBomb( BOMB_THROW );
			}
		}
		else
		{
			if ( !( pPlayer->m_nButtons & IN_ATTACK2 ) )
			{
				// Roll the bomb if ducking and on the ground.
				if ( pPlayer->m_Local.m_bDucked && pPlayer->GetGroundEntity() != NULL )
				{
					LaunchBomb( BOMB_ROLL );
				}
				else
				{
					LaunchBomb( BOMB_LOB );
				}
			}
		}
	}

	BaseClass::ItemPostFrame();

#ifdef CLIENT_DLL
	UpdateBombBodygroups();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::PrimaryAttack( void )
{
	if ( m_flTimeToThrow != 0.0f )
		return;

	if ( m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE )
	{
		PrepareThrow( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::SecondaryAttack( void )
{
	if ( m_bInAttack2 )
		return;

	m_bInAttack2 = true;

	if ( m_flTimeToThrow != 0.0f )
		return;

	if ( m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE )
	{
		PrepareThrow( true );
	}
	else
	{
		DetonateBomb();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponRemoteBomb::SendWeaponAnim( int iActivity )
{
	if ( m_iWeaponMode == TDC_WEAPON_SECONDARY_MODE )
	{
		switch ( iActivity )
		{
		case ACT_VM_IDLE:
			iActivity = ACT_VM_IDLE_SPECIAL;
			break;
		case ACT_VM_DRAW:
			iActivity = ACT_VM_DRAW_SPECIAL;
			break;
		}
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::WeaponIdle( void )
{
	if ( m_flTimeToThrow != 0.0f )
		return;
	
	return BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::UpdateOnRemove( void )
{
#ifdef GAME_DLL
	if ( m_hBomb )
	{
		m_hBomb->Fizzle();
	}
#endif

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::PrepareThrow( bool bAlternateThrow )
{
	if ( gpGlobals->curtime < m_flNextPrimaryAttack )
		return;

	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !HasPrimaryAmmoToFire() )
		return;

	if ( !CanAttack() )
		return;

	SendWeaponAnim( bAlternateThrow ? ACT_VM_PULLBACK_LOW : ACT_VM_PULLBACK_HIGH );
	m_flTimeToThrow = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( TDC_WEAPON_PRIMARY_MODE ).m_flSmackDelay;
	m_bAlternateThrow = bAlternateThrow;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::LaunchBomb( EBombLaunch iMode )
{
	m_flTimeToThrow = 0.0f;

	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !HasPrimaryAmmoToFire() )
		return;

	if ( !CanAttack() )
		return;

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	pPlayer->NoteWeaponFired( this );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	// Play shoot sound.
	PlayWeaponShootSound();

	// Send animations.
	SendWeaponAnim( iMode == BOMB_THROW ? ACT_VM_PRIMARYATTACK : ACT_VM_SECONDARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

#ifdef GAME_DLL

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );
	float flDamage = GetProjectileDamage();
	float flRadius = GetTDCWpnData().m_flDamageRadius;
	bool bCritical = IsCurrentAttackACrit();

	CProjectile_RemoteBomb *pProjectile = NULL;

	if ( iMode == BOMB_THROW )
	{
		// Throw
		Vector vecSrc = pPlayer->EyePosition() + vecForward * 16.0f + vecUp * -6.0f;
		Vector vecVelocity = vecForward * GetProjectileSpeed() + vecUp * 200.0f + pPlayer->GetAbsVelocity();
		AngularImpulse angVelocity( 600, RandomInt( -1200, 1200 ), 0 );
		VerifyBombPosition( pPlayer, vecSrc );

		pProjectile = CProjectile_RemoteBomb::Create( vecSrc, pPlayer->EyeAngles(),
			vecVelocity, angVelocity,
			flDamage, flRadius, bCritical, GetTDCWpnData().m_flPrimerTime,
			pPlayer, this );
	}
	else if ( iMode == BOMB_LOB )
	{
		// Lob
		Vector vecSrc = pPlayer->EyePosition() + vecForward * 16.0f + vecUp * -14.0f;
		Vector vecVelocity = vecForward * 350.0f + vecUp * 50.0f + pPlayer->GetAbsVelocity();
		AngularImpulse angVelocity( 200, RandomInt( -600, 600 ), 0 );
		VerifyBombPosition( pPlayer, vecSrc );

		pProjectile = CProjectile_RemoteBomb::Create( vecSrc, pPlayer->EyeAngles(),
			vecVelocity, angVelocity,
			flDamage, flRadius, bCritical, GetTDCWpnData().m_flPrimerTime,
			pPlayer, this );
	}
	else if ( iMode == BOMB_ROLL )
	{
		// Roll
		Vector vecSrc = pPlayer->GetAbsOrigin();
		vecSrc.z += g_vecBombSize.z;

		// no up/down direction
		Vector vecFacing; AngleVectors( pPlayer->EyeAngles(), &vecFacing );
		vecFacing.z = 0.0f;
		VectorNormalize( vecFacing );

		trace_t tr;
		UTIL_TraceLine( vecSrc, vecSrc - Vector( 0, 0, 16 ), MASK_SOLID, pPlayer, TDC_COLLISIONGROUP_GRENADES, &tr );
		if ( tr.fraction != 1.0 )
		{
			// compute forward vec parallel to floor plane and roll grenade along that
			Vector tangent;
			CrossProduct( vecFacing, tr.plane.normal, tangent );
			CrossProduct( tr.plane.normal, tangent, vecFacing );
		}
		vecSrc += vecFacing * 18.0;
		VerifyBombPosition( pPlayer, vecSrc );

		Vector vecVelocity = vecFacing * 700.0f + pPlayer->GetAbsVelocity();
		// put it on its side
		QAngle orientation( 0, pPlayer->EyeAngles()[YAW] + 90.0f, 0 );
		// roll it
		AngularImpulse rotSpeed( 720.0f, 0, 0 );

		pProjectile = CProjectile_RemoteBomb::Create( vecSrc, orientation,
			vecVelocity, rotSpeed,
			flDamage, flRadius, bCritical, GetTDCWpnData().m_flPrimerTime,
			pPlayer, this );
	}

	m_hBomb = pProjectile;

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

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flLiveTime = gpGlobals->curtime + GetTDCWpnData().m_flPrimerTime;

	// Using remote now.
	m_iWeaponMode = TDC_WEAPON_SECONDARY_MODE;
	ChangeScreenState( BOMBSCREEN_ACTIVE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::DetonateBomb( void )
{
	if ( gpGlobals->curtime < m_flLiveTime )
		return;

	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	m_bInDetonation = true;

#ifndef CLIENT_DLL
	pPlayer->NoteWeaponFired( this );
#endif

	SendWeaponAnim( ACT_VM_PRIMARYATTACK_SPECIAL );
	WeaponSound( WPN_DOUBLE );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

#ifdef GAME_DLL
	if ( m_hBomb )
	{
		m_hBomb->Detonate();
	}
#endif

	m_bInDetonation = false;
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	ChangeScreenState( BOMBSCREEN_DETONATED, 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Check to ensure the bomb is not stuck in a wall.
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::VerifyBombPosition( CTDCPlayer *pPlayer, Vector &vecPos )
{
	trace_t tr;
	UTIL_TraceHull( pPlayer->EyePosition(), vecPos, -g_vecBombSize, g_vecBombSize, MASK_SOLID, pPlayer, TDC_COLLISIONGROUP_GRENADES, &tr );

	if ( tr.DidHit() )
	{
		vecPos = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::ChangeScreenState( EBombScreen iState, float flOffTime /*= 0.0f*/ )
{
	m_iScreenState = iState;

	if ( flOffTime )
	{
		m_flScreenOffTime = gpGlobals->curtime + flOffTime;
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::DeathNotice( CBaseEntity *pVictim )
{
	CProjectile_RemoteBomb *pBomb = dynamic_cast<CProjectile_RemoteBomb *>( pVictim );
	if ( pBomb && pBomb == m_hBomb )
	{
		m_hBomb = NULL;

		if ( !m_bInDetonation )
		{
			CTDCPlayer *pPlayer = GetTDCPlayerOwner();
			if ( pPlayer && pPlayer->GetActiveTFWeapon() == this )
			{
				SendWeaponAnim( ACT_VM_RELEASE );
				ChangeScreenState( BOMBSCREEN_DETONATED_ENEMY, 1.0f );
			}
		}

		m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	}
}
#else
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_OldWeaponMode = m_iWeaponMode;
	m_iOldScreenState = m_iScreenState;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( !GetPredictable() )
	{
		if ( m_OldWeaponMode != m_iWeaponMode || m_iOldScreenState != m_iScreenState )
		{
			UpdateBombBodygroups();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CStudioHdr *CWeaponRemoteBomb::OnNewModel( void )
{
	CStudioHdr *pHdr = BaseClass::OnNewModel();

	m_iBombBodygroup = FindBodygroupByName( "bomb" );
	m_iRemoteBodygroup = FindBodygroupByName( "remote" );
	m_iScreenBodygroup = FindBodygroupByName( "screen" );

	return pHdr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRemoteBomb::UpdateBombBodygroups( void )
{
	if ( m_iBombBodygroup != -1 )
	{
		SetBodygroup( m_iBombBodygroup, m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE ? 0 : 1 );
	}

	if ( m_iRemoteBodygroup != -1 )
	{
		if ( UsingViewModel() )
		{
			// Remote is always visible in first person.
			SetBodygroup( m_iRemoteBodygroup, 1 );
		}
		else
		{
			SetBodygroup( m_iRemoteBodygroup, m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE ? 0 : 1 );
		}
	}

	if ( m_iScreenBodygroup != -1 )
	{
		SetBodygroup( m_iScreenBodygroup, m_iScreenState );
	}
}
#endif
