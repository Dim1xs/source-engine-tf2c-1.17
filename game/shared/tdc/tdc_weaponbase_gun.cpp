//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#include "cbase.h"
#include "tdc_weaponbase_gun.h"
#include "tdc_fx_shared.h"
#include "tdc_projectile_nail.h"
#include "in_buttons.h"
#include "tdc_lagcompensation.h"
#include "takedamageinfo.h"

#if !defined( CLIENT_DLL )	// Server specific.

	#include "tdc_player.h"
	#include "te_effect_dispatch.h"

#else	// Client specific.

	#include "c_tdc_player.h"
	#include "c_te_effect_dispatch.h"
	#include "engine/ivdebugoverlay.h"

#endif

#ifdef GAME_DLL
ConVar tdc_debug_bullets( "tdc_debug_bullets", "0", FCVAR_DEVELOPMENTONLY, "Visualize bullet traces." );
#endif

ConVar sv_showimpacts( "sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)" );
ConVar sv_showplayerhitboxes( "sv_showplayerhitboxes", "0", FCVAR_REPLICATED, "Show lag compensated hitboxes for the specified player index whenever a player fires." );

//=============================================================================
//
// TFWeaponBase Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TDCWeaponBaseGun, DT_TDCWeaponBaseGun )

BEGIN_NETWORK_TABLE( CTDCWeaponBaseGun, DT_TDCWeaponBaseGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTDCWeaponBaseGun )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, 0 ),
#endif
END_PREDICTION_DATA()

//=============================================================================
//
// TFWeaponBase Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTDCWeaponBaseGun::CTDCWeaponBaseGun()
{
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	m_bInZoom = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::ItemPostFrame( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Unzoom.
	if ( !( pPlayer->m_nButtons & IN_ATTACK2 ) )
	{
		HandleSoftZoom( false );
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::ItemBusyFrame( void )
{
	BaseClass::ItemBusyFrame();

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Unzoom.
	if ( !( pPlayer->m_nButtons & IN_ATTACK2 ) )
	{
		HandleSoftZoom( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	pPlayer->NoteWeaponFired( this );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	FireProjectile();

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	m_iReloadMode = TDC_RELOAD_START;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::SecondaryAttack( void )
{
	// Zoom in. Putting this here because we want any weapons with an actual secondary attack
	// to override zoom.
	HandleSoftZoom( true );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Hits triggers with raycasts
//-----------------------------------------------------------------------------
class CTDCTriggerTraceEnum : public IEntityEnumerator
{
public:
	CTDCTriggerTraceEnum( Ray_t *pRay, const CTakeDamageInfo &info, const Vector& dir, int contentsMask ) :
		m_info( info ), m_VecDir( dir ), m_ContentsMask( contentsMask ), m_pRay( pRay )
	{
	}

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		trace_t tr;

		CBaseEntity *pEnt = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );

		// Done to avoid hitting an entity that's both solid & a trigger.
		if ( pEnt->IsSolid() )
			return true;

		enginetrace->ClipRayToEntity( *m_pRay, m_ContentsMask, pHandleEntity, &tr );
		if ( tr.fraction < 1.0f )
		{
			pEnt->DispatchTraceAttack( m_info, m_VecDir, &tr );
		}

		return true;
	}

private:
	Vector m_VecDir;
	int m_ContentsMask;
	Ray_t *m_pRay;
	CTakeDamageInfo m_info;
};
#endif

//-----------------------------------------------------------------------------
// Purpose: Fire bullets by default, override in derived classes.
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::FireProjectile( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Play shoot sound.
	PlayWeaponShootSound();

	// Send animations.
	SendWeaponAnim( GetPrimaryAttackActivity() );

	if ( m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	}

	Vector vecShootOrigin = pPlayer->Weapon_ShootPosition();
	QAngle vecShootAngle = pPlayer->EyeAngles() + pPlayer->GetPunchAngle();
	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	float flSpread = GetWeaponSpread();
	int nBulletsPerShot = GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_nBulletsPerShot;

#ifdef GAME_DLL
	float flDamage = GetProjectileDamage();
	int iDamageType = GetDamageType();
	ETDCDmgCustom iCustomDamage = GetCustomDamageType();

	if ( IsCurrentAttackACrit() )
	{
		iDamageType |= DMG_CRITICAL;
	}
	if ( CanHeadshot() )
	{
		iDamageType |= DMG_USE_HITLOCATIONS;
	}
#endif

	// Fire bullets, calculate impacts & effects.

	// Get the shooting angles.
	Vector vecShootForward, vecShootRight, vecShootUp;
	AngleVectors( vecShootAngle, &vecShootForward, &vecShootRight, &vecShootUp );

	// Reset multi-damage structures.
	ClearMultiDamage();

	// Only shotguns should get fixed spread pattern.
	bool bFixedSpread = ( GetDamageType() & DMG_BUCKSHOT ) && nBulletsPerShot > 1;
	int nSpreadPellets;
	const Vector2D *pSpreadPattern = GetSpreadPattern( nSpreadPellets );

	// Move other players back to history positions based on local player's lag
	START_LAG_COMPENSATION( pPlayer, pPlayer->GetCurrentCommand() );

	if ( sv_showplayerhitboxes.GetInt() > 0 )
	{
		CBasePlayer *lagPlayer = UTIL_PlayerByIndex( sv_showplayerhitboxes.GetInt() );
		if ( lagPlayer )
		{
#ifdef CLIENT_DLL
			lagPlayer->DrawClientHitboxes( 4, true );
#else
			lagPlayer->DrawServerHitboxes( 4, true );
#endif
		}
	}

	for ( int iBullet = 0; iBullet < nBulletsPerShot; ++iBullet )
	{
		// Initialize random system with this seed.
		RandomSeed( iSeed );

		float x = 0.0f;
		float y = 0.0f;

		bool bPerfectAccuracy = false;

		// See if we're using pre-determined spread pattern.
		if ( bFixedSpread )
		{
			int idx = iBullet % nSpreadPellets;
			x = pSpreadPattern[idx].x;
			y = pSpreadPattern[idx].y;
		}
		else
		{
			// Determine if the first bullet should be perfectly accurate.
			bPerfectAccuracy = ( iBullet == 0 && CanFireAccurateShot( nBulletsPerShot ) );
		}

		// Apply random spread if none of the above conditions are true.
		if ( !bPerfectAccuracy && !bFixedSpread )
		{
			// Get circular gaussian spread.
			x = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
			y = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
		}

		// Initialize the varialbe firing information.
		Vector vecBulletDir = vecShootForward + ( x *  flSpread * vecShootRight ) + ( y * flSpread * vecShootUp );
		VectorNormalize( vecBulletDir );

		// Fire a bullet (ignoring the shooter).
		Vector vecEnd = vecShootOrigin + vecBulletDir * GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flRange;
		trace_t trace;
		UTIL_TraceLine( vecShootOrigin, vecEnd, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &trace );

#ifdef GAME_DLL
		if ( tdc_debug_bullets.GetBool() )
		{
			NDebugOverlay::Line( vecShootOrigin, trace.endpos, 0, 255, 0, true, 30 );
		}
#endif

#ifdef CLIENT_DLL
		if ( sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2 )
		{
			// draw red client impact markers
			debugoverlay->AddBoxOverlay( trace.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 0, 0, 127, 4 );

			if ( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
			{
				C_BasePlayer *player = ToBasePlayer( trace.m_pEnt );
				player->DrawClientHitboxes( 4, true );
			}
		}
#else
		if ( sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3 )
		{
			// draw blue server impact markers
			NDebugOverlay::Box( trace.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0, 0, 255, 127, 4 );

			if ( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
			{
				CBasePlayer *player = ToBasePlayer( trace.m_pEnt );
				player->DrawServerHitboxes( 4, true );
			}
		}
#endif

		if ( trace.fraction < 1.0 )
		{
			// Verify we have an entity at the point of impact.
			Assert( trace.m_pEnt );

			// If shot starts out of water and ends in water
			if ( !( enginetrace->GetPointContents( trace.startpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) &&
				( enginetrace->GetPointContents( trace.endpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) )
			{
				// Water impact effects.
				ImpactWaterTrace( pPlayer, trace, vecShootOrigin );
			}
			else
			{
				// Regular impact effects.

				// don't decal your teammates or objects on your team
				if ( pPlayer->IsEnemy( trace.m_pEnt ) )
				{
					UTIL_ImpactTrace( &trace, DMG_BULLET );
				}
			}

			static int tracerCount;
			if ( GetTDCWpnData().m_iTracerFreq != 0 && ( tracerCount++ % GetTDCWpnData().m_iTracerFreq ) == 0 )
			{
				MakeTracer( vecShootOrigin, trace );
			}

			if ( IsWeapon( WEAPON_LEVERRIFLE ) )
			{
				if ( trace.m_pEnt->IsPlayer() &&
					pPlayer->IsEnemy( trace.m_pEnt ) &&
					trace.hitgroup == HITGROUP_HEAD &&
					CanCritHeadshot() )
				{
					// play the critical shot sound to the shooter	
					WeaponSound( BURST );
				}
			}

			// Server specific.
#ifndef CLIENT_DLL
			// See what material we hit.
			CTakeDamageInfo dmgInfo( pPlayer, pPlayer, this, flDamage, iDamageType, iCustomDamage );
			CalculateBulletDamageForce( &dmgInfo, GetPrimaryAmmoType(), vecBulletDir, trace.endpos, 1.0 );
			trace.m_pEnt->DispatchTraceAttack( dmgInfo, vecBulletDir, &trace );

			// Trace the bullet against triggers.
			Ray_t ray;
			ray.Init( vecShootOrigin, trace.endpos );

			CTDCTriggerTraceEnum triggerTraceEnum( &ray, dmgInfo, vecBulletDir, MASK_SHOT );
			enginetrace->EnumerateEntities( ray, true, &triggerTraceEnum );
#endif
		}

		// Use new seed for next bullet.
		++iSeed;
	}

	FINISH_LAG_COMPENSATION();

	// Apply damage if any.
	{
		CDisablePredictionFiltering disabler;
		ApplyMultiDamage();
	}

	m_flLastFireTime = gpGlobals->curtime;

	int iAmmoCost = GetAmmoPerShot();

	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 -= iAmmoCost;
	}
	else
	{
		if ( m_iWeaponMode == TDC_WEAPON_PRIMARY_MODE )
		{
			pPlayer->RemoveAmmo( iAmmoCost, m_iPrimaryAmmoType );
		}
		else
		{
			pPlayer->RemoveAmmo( iAmmoCost, m_iSecondaryAmmoType );
		}
	}

	DoMuzzleFlash();
	AddViewKick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::AddViewKick( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flPunchAngle;

	if ( flPunchAngle > 0 )
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
		pPlayer->SetPunchAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::ImpactWaterTrace( CTDCPlayer *pPlayer, trace_t &trace, const Vector &vecStart )
{
	trace_t traceWater;
	UTIL_TraceLine( vecStart, trace.endpos, ( MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME ),
		pPlayer, COLLISION_GROUP_NONE, &traceWater );
	if ( traceWater.fraction < 1.0f )
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat( 8, 12 );
		if ( traceWater.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect( "tf_gunshotsplash", data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return angles for a projectile reflected by airblast
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::GetProjectileReflectSetup( CTDCPlayer *pPlayer, const Vector &vecPos, Vector &vecDeflect, bool bHitTeammates /* = true */, bool bUseHitboxes /* = false */ )
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;
	int nMask = MASK_SOLID;

	if ( bUseHitboxes )
	{
		nMask |= CONTENTS_HITBOX;
	}

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, nMask, &filter, &tr );
	}

	// vecPos is projectile's current position. Use that to find angles.

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	if ( tr.fraction > 0.1 || bUseHitboxes )
	{
		vecDeflect = tr.endpos - vecPos;
	}
	else
	{
		vecDeflect = endPos - vecPos;
	}

	VectorNormalize( vecDeflect );
}

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::GetProjectileFireSetup( CTDCPlayer *pPlayer, const Vector &vecOffset, Vector &vecSrc, Vector &vecOutDir, bool bHitTeammates /* = true */ )
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}

#ifdef GAME_DLL
	Vector vecFinalOffset = vecOffset;

	if ( IsViewModelFlipped() )
	{
		// If viewmodel is flipped fire from the other side.
		vecFinalOffset.y *= -1.0f;
	}

	// Offset actual start point
	vecSrc = pPlayer->EyePosition() + vecForward * vecFinalOffset.x + vecRight * vecFinalOffset.y + vecUp * vecFinalOffset.z;
#else
	// Fire nails from the weapon's muzzle.
	if ( pPlayer )
	{
		if ( !UsingViewModel() )
		{
			GetAttachment( m_iMuzzleAttachment, vecSrc );
		}
		else
		{
			C_BaseEntity *pViewModel = GetPlayerViewModel();

			if ( pViewModel )
			{
				QAngle vecAngles;
				pViewModel->GetAttachment( m_iMuzzleAttachment, vecSrc, vecAngles );

				Vector vForward;
				AngleVectors( vecAngles, &vForward );

				trace_t trace;
				UTIL_TraceLine( vecSrc + vForward * -50, vecSrc, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );

				vecSrc = trace.endpos;
			}
		}
	}
#endif

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	vecOutDir = ( tr.fraction > 0.1f ) ? tr.endpos - vecSrc : endPos - vecSrc;
	VectorNormalize( vecOutDir );
	
	float flSpread = GetWeaponSpread();
	if ( flSpread != 0.0f )
	{
		VectorVectors( vecForward, vecRight, vecUp );

		// Get circular gaussian spread.
		RandomSeed( GetPredictionRandomSeed() & 255 );
		float x = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
		float y = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );

		vecOutDir += vecRight * x * flSpread + vecUp * y * flSpread;
		VectorNormalize( vecOutDir );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::PlayWeaponShootSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCWeaponBaseGun::GetAmmoPerShot( void )
{
	return GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCWeaponBaseGun::GetWeaponSpread( void )
{
	return GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flSpread;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTDCWeaponBaseGun::GetProjectileDamage( void )
{
	return (float)GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for speed
//-----------------------------------------------------------------------------
float CTDCWeaponBaseGun::GetProjectileSpeed( void )
{
	return GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flProjectileSpeed;
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: Should this be put into fire gun
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::DoMuzzleFlash()
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return;

	CEffectData data;
	data.m_vOrigin = pOwner->GetAbsOrigin();
#ifdef GAME_DLL
	data.m_nEntIndex = entindex();
#else
	data.m_hEntity = this;
#endif

	DispatchEffect( "TDC_MuzzleFlash", data );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBaseGun::HandleSoftZoom( bool bZoomIn )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer || !pPlayer->IsNormalClass() )
		return;

	if ( pPlayer->ShouldHoldToZoom() )
	{
		if ( bZoomIn )
		{
			if ( !pPlayer->m_Shared.InCond( TDC_COND_SOFTZOOM ) )
			{
				pPlayer->m_Shared.AddCond( TDC_COND_SOFTZOOM );
			}

			m_bInZoom = true;
		}
		else
		{
			if ( pPlayer->m_Shared.InCond( TDC_COND_SOFTZOOM ) )
			{
				pPlayer->m_Shared.RemoveCond( TDC_COND_SOFTZOOM );
			}

			m_bInZoom = false;
		}
	}
	else
	{
		if ( bZoomIn )
		{
			if ( !m_bInZoom )
			{
				if ( !pPlayer->m_Shared.InCond( TDC_COND_SOFTZOOM ) )
				{
					pPlayer->m_Shared.AddCond( TDC_COND_SOFTZOOM );
				}
				else
				{
					pPlayer->m_Shared.RemoveCond( TDC_COND_SOFTZOOM );
				}

				m_bInZoom = true;
			}
		}
		else
		{
			m_bInZoom = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CTDCWeaponBaseGun::GetPrimaryAttackActivity( void )
{
	if ( AtLastShot( m_iPrimaryAmmoType ) && HasActivity( ACT_VM_PRIMARYATTACK_LAST ) )
	{
		return ACT_VM_PRIMARYATTACK_LAST;
	}
		
	return BaseClass::GetPrimaryAttackActivity();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWeaponBaseGun::AtLastShot( int iAmmoType )
{
	if ( iAmmoType == m_iPrimaryAmmoType && UsesClipsForAmmo1() )
	{
		return Clip1() == 1;
	}
	else if ( iAmmoType == m_iSecondaryAmmoType && UsesClipsForAmmo2() )
	{
		return Clip2() == 1;
	}

	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( pOwner )
	{
		return pOwner->GetAmmoCount( iAmmoType ) == 1;
	}

	return false;
}
