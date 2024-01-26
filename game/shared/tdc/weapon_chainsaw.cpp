//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "weapon_chainsaw.h"
#include "in_buttons.h"
#include "gamemovement.h"
#include "eventlist.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tdc_player.h"
#include "soundenvelope.h"
#include "prediction.h"
#include "decals.h"
#include "c_impact_effects.h"
#include "fx_impact.h"
// Server specific.
#else
#include "tdc_player.h"
#endif

ConVar tdc_chainsaw_pounce_recharge_time( "tdc_chainsaw_pounce_recharge_time", "8", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

extern CMoveData *g_pMoveData;

//=============================================================================
//
// Weapon Chainsaw tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponChainsaw, DT_Chainsaw );
BEGIN_NETWORK_TABLE( CWeaponChainsaw, DT_Chainsaw )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponState ) ),
	RecvPropTime( RECVINFO( m_flStateUpdateTime ) ),
	RecvPropBool( RECVINFO( m_bCritShot ) ),
	RecvPropTime( RECVINFO( m_flPounceTime ) ),
	RecvPropInt( RECVINFO ( m_iNumHits ) ),
#else
	SendPropInt( SENDINFO( m_iWeaponState ), 2, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropTime( SENDINFO( m_flStateUpdateTime ) ),
	SendPropBool( SENDINFO( m_bCritShot ) ),
	SendPropTime( SENDINFO( m_flPounceTime ) ),
	SendPropInt( SENDINFO ( m_iNumHits ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( weapon_chainsaw, CWeaponChainsaw );
PRECACHE_WEAPON_REGISTER( weapon_chainsaw );

BEGIN_PREDICTION_DATA( CWeaponChainsaw )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flStateUpdateTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD( m_bCritShot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flPounceTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iNumHits, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

acttable_t CWeaponChainsaw::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_CHAINSAW,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_CHAINSAW,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_CHAINSAW,		false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_CHAINSAW,	false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_CHAINSAW, false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_CHAINSAW,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_CHAINSAW,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_CHAINSAW,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_CHAINSAW,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_MP_ATTACK_STAND_CHAINSAW,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_CHAINSAW,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,	ACT_MP_ATTACK_SWIM_CHAINSAW,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_CHAINSAW,	false },

	{ ACT_MP_ATTACK_STAND_PREFIRE,		ACT_MP_ATTACK_STAND_PREFIRE_CHAINSAW,	false },
	{ ACT_MP_ATTACK_STAND_POSTFIRE,		ACT_MP_ATTACK_STAND_POSTFIRE_CHAINSAW,	false },
	{ ACT_MP_ATTACK_CROUCH_PREFIRE,		ACT_MP_ATTACK_CROUCH_PREFIRE_CHAINSAW,	false },
	{ ACT_MP_ATTACK_CROUCH_POSTFIRE,	ACT_MP_ATTACK_CROUCH_POSTFIRE_CHAINSAW,	false },
	{ ACT_MP_ATTACK_SWIM_PREFIRE,		ACT_MP_ATTACK_SWIM_PREFIRE_CHAINSAW,	false },
	{ ACT_MP_ATTACK_SWIM_POSTFIRE,		ACT_MP_ATTACK_SWIM_POSTFIRE_CHAINSAW,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponChainsaw );

//=============================================================================
//
// Weapon Chainsaw functions.
//

CWeaponChainsaw::CWeaponChainsaw()
{
	m_bBackstab = false;

#ifdef CLIENT_DLL
	m_iChainPoseParameter = -1;
	m_pSoundCur = NULL;
	m_iSoundCur = -1;
	m_pImpactSound = NULL;
	m_iImpactSound = SAW_IMPACT_NONE;
	m_iImpactType = SAW_IMPACT_NONE;
	m_iOldWeaponState = SAW_STATE_OFF;
	m_iImpactMaterial = 0;
	m_pImpactEffect = NULL;
#endif

	WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef GAME_DLL
	m_iWeaponState = SAW_STATE_OFF;
	m_bCritShot = false;
	m_flStateUpdateTime = 0.0f;
	m_flPounceTime = 0.0f;
	//m_iNumHits = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::ItemPostFrame( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		Smack();
		m_flSmackTime = -1.0f;
	}

	if ( ( pPlayer->m_afButtonPressed & IN_ATTACK2 ) && gpGlobals->curtime >= m_flPounceTime && CanAttack() )
	{
		Pounce();
	}

	if ( ( pPlayer->m_nButtons & IN_ATTACK ) && CanAttack() && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		ChainsawCut();
	}
	else
	{
		// Wind down if player releases fire buttons.
		if ( m_iWeaponState != SAW_STATE_IDLE )
		{
			if ( gpGlobals->curtime >= m_flStateUpdateTime )
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
				WindDown();
			}
		}
		else if ( !ReloadOrSwitchWeapons() )
		{
			WeaponIdle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponChainsaw::HasPrimaryAmmo( void )
{
	// Skipping base melee since chainsaw uses ammo for attacking.
	return CTDCWeaponBase::HasPrimaryAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponChainsaw::CanBeSelected( void )
{
	return CTDCWeaponBase::CanBeSelected();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponChainsaw::ChainsawCut( void )
{
	if ( gpGlobals->curtime < m_flNextPrimaryAttack )
		return;

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	switch ( m_iWeaponState )
	{
	default:
	case SAW_STATE_IDLE:
	{
		WindUp();

		const float flFireDelay = 0.2f;
		m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
		m_flStateUpdateTime = gpGlobals->curtime + flFireDelay;
		//pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
		break;
	}
	case SAW_STATE_STARTFIRING:
	{
		// Start playing the looping fire sound
		if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
		{
			m_iWeaponState = SAW_STATE_FIRING;

#ifdef CLIENT_DLL
			WeaponSoundUpdate();
#endif
		}
		break;
	}
	case SAW_STATE_FIRING:
	{
		m_bBackstab = false;
		CalcIsAttackCritical();

#if !defined( CLIENT_DLL ) 
		pPlayer->NoteWeaponFired( this );
#endif

		Smack();
		m_bCritShot = IsCurrentAttackACrit();

#ifdef CLIENT_DLL
		WeaponSoundUpdate();
#endif

		pPlayer->RemoveAmmo( GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iPrimaryAmmoType );

		// Set next attack times.
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponChainsaw::Pounce( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	// Not in the water.
	if ( pPlayer->GetWaterLevel() >= WL_Waist )
		return;

	// Not in mid-air.
	if ( pPlayer->GetGroundEntity() == NULL )
		return;

	// Copy movement amounts
	float flForwardMove = g_pMoveData->m_flForwardMove;
	float flSideMove = g_pMoveData->m_flSideMove;

	// No hopping in place.
	if ( flForwardMove == 0.0f && flSideMove == 0.0f )
		return;

	// Play the pouncing sound.
	WeaponSound( SPECIAL3 );

	// Start jump animation and player sound.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_JUMP );
	pPlayer->PlayStepSound( (Vector &)pPlayer->GetAbsOrigin(), pPlayer->GetSurfaceData(), 1.0, true );
	pPlayer->m_Shared.SetJumping( true );

	// In the air now.
	pPlayer->SetGroundEntity( NULL );

	// Get the wish direction.
	// FIXME: Possibly use something else rather than g_pMoveData?
	Vector vecForward, vecRight;
	AngleVectors( g_pMoveData->m_vecViewAngles, &vecForward, &vecRight, NULL );
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;
	VectorNormalize( vecForward );
	VectorNormalize( vecRight );

	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection( ( ( vecForward.x * flForwardMove ) + ( vecRight.x * flSideMove ) ),
		( ( vecForward.y * flForwardMove ) + ( vecRight.y * flSideMove ) ),
		0.0f );

	VectorNormalize( vecWishDirection );

	// Update the velocity.
	float flSpeedSqr = Max( Square( 800.0f ), pPlayer->GetAbsVelocity().Length2DSqr() );
	Vector vecPounceVel = vecWishDirection * sqrt( flSpeedSqr );
	vecPounceVel.z += 160.0f;
	pPlayer->SetAbsVelocity( vecPounceVel );

	m_flPounceTime = gpGlobals->curtime + tdc_chainsaw_pounce_recharge_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponChainsaw::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_iWeaponState > SAW_STATE_IDLE )
	{
		WindDown();
	}

	m_iWeaponState = SAW_STATE_OFF;

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::DoImpactEffect( trace_t &tr, int nDamageType )
{
	// Overriding since we're using custom impact effects.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponChainsaw::CalcIsAttackCriticalHelper( void )
{
	// Crit from behind.
	trace_t tr;
	if ( DoSwingTrace( tr ) )
	{
		// Get the forward view vector of the target, ignore Z
		Vector vecVictimForward;
		AngleVectors( tr.m_pEnt->EyeAngles(), &vecVictimForward );
		vecVictimForward.z = 0.0f;
		vecVictimForward.NormalizeInPlace();

		// Get a vector from my origin to my targets origin
		Vector vecToTarget;
		vecToTarget = tr.m_pEnt->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
		vecToTarget.z = 0.0f;
		vecToTarget.NormalizeInPlace();

		// Get a forward vector of the attacker.
		Vector vecOwnerForward;
		AngleVectors( GetOwner()->EyeAngles(), &vecOwnerForward );
		vecOwnerForward.z = 0.0f;
		vecOwnerForward.NormalizeInPlace();

		float flDotOwner = DotProduct( vecOwnerForward, vecToTarget );
		float flDotVictim = DotProduct( vecVictimForward, vecToTarget );

		// Make sure they're actually facing the target.
		// This needs to be done because lag compensation can place target slightly behind the attacker.
		if ( flDotOwner > 0.5f && flDotVictim > 0.7f )
		{
			m_bBackstab = true;
			return true;
		}
	}

	m_bBackstab = false;
	return BaseClass::CalcIsAttackCriticalHelper();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponChainsaw::GetMeleeDamage( CBaseEntity *pTarget, ETDCDmgCustom &iCustomDamage )
{
	if ( m_bBackstab )
	{
		iCustomDamage = TDC_DMG_CHAINSAW_BACKSTAB;
	}

	return BaseClass::GetMeleeDamage( pTarget, iCustomDamage );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponChainsaw::GetSwingRange( void )
{
	return 72;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponChainsaw::SendWeaponAnim( int iActivity )
{
	// When we start firing, play the startup firing anim first
	if ( iActivity == ACT_VM_PRIMARYATTACK )
	{
		// If we're already playing the fire anim, let it continue. It loops.
		if ( GetActivity() == ACT_VM_PRIMARYATTACK )
			return true;
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponChainsaw::GetEffectBarProgress( void )
{
	float flTimeLeft = m_flPounceTime - gpGlobals->curtime;

	if ( flTimeLeft > 0.0f )
	{
		return RemapValClamped( flTimeLeft, tdc_chainsaw_pounce_recharge_time.GetFloat(), 0.0f, 0.0f, 1.0f );
	}

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponChainsaw::WindUp( void )
{
	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Play wind-up animation and sound (SPECIAL1).
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Set the appropriate firing state.
	m_iWeaponState = SAW_STATE_STARTFIRING;
	//pPlayer->m_Shared.AddCond( TDC_COND_AIMING );

#ifdef CLIENT_DLL 
	WeaponSoundUpdate();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponChainsaw::WindDown( void )
{
	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_VM_IDLE );

	// Set the appropriate firing state.
	m_iWeaponState = SAW_STATE_IDLE;
	//pPlayer->m_Shared.RemoveCond( TDC_COND_AIMING );

#ifdef CLIENT_DLL
	WeaponSoundUpdate();
#endif

	m_flStateUpdateTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::OnEntityHit( CBaseEntity *pEntity )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );
	if ( pPlayer )
	{
		if ( m_iNumHits < 1 )
		{
			++m_iNumHits;
		}
	}
	BaseClass::OnEntityHit( pEntity );
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CWeaponChainsaw::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponChainsaw::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	if ( ( pEvent->type & AE_TYPE_NEWEVENTSYSTEM ) /*&& (pEvent->type & AE_TYPE_SERVER)*/ )
	{
		if ( pEvent->event == AE_WPN_DRAW )
		{
			if ( m_iWeaponState == SAW_STATE_OFF )
			{
				m_iWeaponState = SAW_STATE_IDLE;
			}
			return;
		}
	}

	BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::UpdateOnRemove( void )
{
	m_iState = WEAPON_NOT_CARRIED;
	m_iWeaponState = SAW_STATE_OFF;
	WeaponSoundUpdate();

	if ( m_pImpactEffect )
	{
		ParticleProp()->StopEmission( m_pImpactEffect );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldWeaponState = m_iWeaponState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iWeaponState != m_iOldWeaponState )
	{
		WeaponSoundUpdate( m_iOldWeaponState == SAW_STATE_OFF );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CWeaponChainsaw::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	m_iChainPoseParameter = LookupPoseParameter( "chain" );
	return hdr;
}

static void SetImpactControlPoint( CNewParticleEffect *pEffect, int nPoint, const Vector &vecImpactPoint, const Vector &vecForward, C_BaseEntity *pEntity )
{
	Vector vecImpactY, vecImpactZ;
	VectorVectors( vecForward, vecImpactY, vecImpactZ );
	vecImpactY *= -1.0f;

	pEffect->SetControlPoint( nPoint, vecImpactPoint );
	pEffect->SetControlPointOrientation( nPoint, vecForward, vecImpactY, vecImpactZ );
	pEffect->SetControlPointEntity( nPoint, pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponChainsaw::Simulate( void )
{
	BaseClass::Simulate();

	// Move the chain.
	if ( m_iChainPoseParameter != -1 && m_iWeaponState != SAW_STATE_OFF )
	{
		float flChainVal = GetPoseParameter( m_iChainPoseParameter );

		switch ( m_iWeaponState )
		{
		case SAW_STATE_FIRING:
		case SAW_STATE_STARTFIRING:
			flChainVal += 2.0f * gpGlobals->frametime;
			break;
		case SAW_STATE_IDLE:
		default:
			flChainVal += 0.5f * gpGlobals->frametime;
			break;
		}

		flChainVal = fmod( flChainVal, 1.0f );
		SetPoseParameter( m_iChainPoseParameter, flChainVal );
	}

	// Update impact effects.
	trace_t tr;
	int iMaterial = 0;

	// Figure the type of the surface we're hitting.
	if ( m_iWeaponState == SAW_STATE_FIRING )
	{
		if ( DoSwingTrace( tr ) )
		{
			surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
			iMaterial = pdata->game.material;
		}
	}

	if ( iMaterial )
	{
		m_iImpactType = ( iMaterial == CHAR_TEX_FLESH ) ? SAW_IMPACT_FLESH : SAW_IMPACT_WORLD;

		if ( iMaterial != m_iImpactMaterial )
		{
			// Kill the effect and re-create it if material changes.
			if ( m_pImpactEffect )
			{
				ParticleProp()->StopEmission( m_pImpactEffect );
				m_pImpactEffect = NULL;
			}

			const char *pImpactName;

			switch ( iMaterial )
			{
			case CHAR_TEX_CONCRETE:
			case CHAR_TEX_TILE:
				pImpactName = "chainsaw_impact_concrete";
				break;
			case CHAR_TEX_DIRT:
			case CHAR_TEX_SAND:
				pImpactName = "chainsaw_impact_dirt";
				break;
			case CHAR_TEX_METAL:
			case CHAR_TEX_VENT:
				pImpactName = "chainsaw_impact_metal";
				break;
			case CHAR_TEX_WOOD:
				pImpactName = "chainsaw_impact_wood";
				break;
			case CHAR_TEX_GLASS:
				pImpactName = "chainsaw_impact_glass";
				break;
			case CHAR_TEX_FLESH:
				pImpactName = "chainsaw_impact_flesh";
				break;
			default:
				// No impact effect for this material.
				pImpactName = NULL;
				break;
			}

			if ( pImpactName )
			{
				m_pImpactEffect = ParticleProp()->Create( pImpactName, PATTACH_CUSTOMORIGIN );
			}
		}

		// Update the effect position and color.
		if ( m_pImpactEffect && m_pImpactEffect->IsValid() )
		{
			Vector vecImpactPoint = tr.endpos;
			Vector vecShotDir = vecImpactPoint - tr.startpos;

			Vector	vecReflect;
			float	flDot = DotProduct( vecShotDir, tr.plane.normal );
			VectorMA( vecShotDir, -2.0f * flDot, tr.plane.normal, vecReflect );

			Vector vecShotBackward;
			VectorMultiply( vecShotDir, -1.0f, vecShotBackward );

			SetImpactControlPoint( m_pImpactEffect, 0, vecImpactPoint, tr.plane.normal, tr.m_pEnt );
			SetImpactControlPoint( m_pImpactEffect, 1, vecImpactPoint, vecReflect, tr.m_pEnt );
			SetImpactControlPoint( m_pImpactEffect, 2, vecImpactPoint, vecShotBackward, tr.m_pEnt );
			m_pImpactEffect->SetControlPoint( 3, Vector( 1.0f ) );
			if ( m_pImpactEffect->m_pDef->ReadsControlPoint( 4 ) )
			{
				Vector vecColor;
				GetColorForSurface( &tr, &vecColor );
				m_pImpactEffect->SetControlPoint( 4, vecColor );
			}
		}

#if 0
		// Add decal
		const char *pchDecalName = GetImpactDecal( tr.m_pEnt, iMaterial, DMG_CLUB );
		int decalNumber = decalsystem->GetDecalIndexForName( pchDecalName );
		if ( decalNumber != -1 )
		{
			// Setup our shot information
			Vector shotDir = tr.endpos - tr.startpos;
			float flLength = VectorNormalize( shotDir );
			Vector traceExt;
			VectorMA( tr.startpos, flLength + 8.0f, shotDir, traceExt );

			if ( ( tr.m_pEnt->entindex() == 0 ) && ( tr.hitbox != 0 ) )
			{
				staticpropmgr->AddDecalToStaticProp( tr.startpos, traceExt, tr.hitbox - 1, decalNumber, true, tr );
			}
			else if ( tr.m_pEnt )
			{
				// Here we deal with decals on entities.
				tr.m_pEnt->AddDecal( tr.startpos, traceExt, tr.endpos, tr.hitbox, decalNumber, true, tr );
			}
		}
#endif
	}
	else
	{
		m_iImpactType = SAW_IMPACT_NONE;

		if ( m_pImpactEffect )
		{
			ParticleProp()->StopEmission( m_pImpactEffect );
			m_pImpactEffect = NULL;
		}
	}

	m_iImpactMaterial = iMaterial;
	CutSoundUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponChainsaw::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_WPN_DRAW )
	{
		if ( GetPredictable() && m_iWeaponState == SAW_STATE_OFF )
		{
			m_iWeaponState = SAW_STATE_IDLE;
			WeaponSoundUpdate( true );
		}
		return true;
	}

	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWeaponChainsaw::GetSkin( void )
{
	return m_iNumHits < 1 ? 0 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the correct sound (including silence) is playing for 
//			current weapon state.
//-----------------------------------------------------------------------------
void CWeaponChainsaw::WeaponSoundUpdate( bool bEngineStarted )
{
	// determine the desired sound for our current state
	int iSound = -1;

	switch ( m_iWeaponState )
	{
	case SAW_STATE_IDLE:
	{
		if ( m_iState == WEAPON_IS_ACTIVE )
		{
			// Use alternate loop if we just started the engine
			if ( bEngineStarted || m_iSoundCur == WPN_DOUBLE || m_iSoundCur == DOUBLE_NPC )
			{
				iSound = IsCarriedByLocalPlayer() ? WPN_DOUBLE : DOUBLE_NPC;
			}
			else
			{
				iSound = IsCarriedByLocalPlayer() ? SPECIAL1 : SPECIAL2;
			}
		}

		break;
	}
	case SAW_STATE_STARTFIRING:
	case SAW_STATE_FIRING:
	{
		if ( m_bCritShot == true )
		{
			iSound = BURST;	// Crit sound
		}
		else
		{
			iSound = SINGLE; // firing sound
		}

		break;
	}
	case SAW_STATE_OFF:
		break;
	default:
		Assert( false );
		break;
	}

	// if we're already playing the desired sound, nothing to do
	if ( m_iSoundCur == iSound )
		return;

	// if we're playing some other sound, stop it
	if ( m_pSoundCur )
	{
		// Stop the previous sound immediately
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
		m_pSoundCur = NULL;
	}
	m_iSoundCur = iSound;

	// if there's no sound to play for current state, we're done
	if ( -1 == iSound )
		return;

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// play the appropriate sound
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	const char *shootsound = GetShootSound( iSound );
	CLocalPlayerFilter filter;
	m_pSoundCur = controller.SoundCreate( filter, pPlayer->entindex(), shootsound );
	controller.Play( m_pSoundCur, 1.0, 100 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponChainsaw::CutSoundUpdate( void )
{
	if ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() )
		return;

	// Are we already playing the correct sound?
	if ( m_iImpactSound == m_iImpactType )
		return;

	m_iImpactSound = m_iImpactType;
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// Stop the current sound.
	if ( m_pImpactSound )
	{
		controller.SoundDestroy( m_pImpactSound );
		m_pImpactSound = NULL;
	}

	// Determine which sound to play.
	int iShootSound = -1;

	switch ( m_iImpactSound )
	{
	case SAW_IMPACT_WORLD:
		iShootSound = MELEE_HIT_WORLD;
		break;
	case SAW_IMPACT_FLESH:
		iShootSound = MELEE_HIT;
		break;
	case SAW_IMPACT_NONE:
	default:
		return;
	}

	const char *pszSound = GetShootSound( iShootSound );
	if ( !pszSound[0] )
		return;

	// Play it.
	CLocalPlayerFilter filter;
	m_pImpactSound = controller.SoundCreate( filter, entindex(), pszSound );
	controller.Play( m_pImpactSound, 1.0, 100 );
}

#endif
