//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tdc_fx_shared.h"
#include "weapon_leverrifle.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"
#include "c_tdc_player.h"

// For TDCGameRules() and Player resources
#include "tdc_gamerules.h"
#include "c_tdc_playerresource.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

#else
#include "tdc_player.h"
#endif

#define TDC_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponLeverRifle, DT_LeverRifle )

BEGIN_NETWORK_TABLE( CWeaponLeverRifle, DT_LeverRifle )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flZoomTime ) ),
#else
	SendPropTime( SENDINFO( m_flZoomTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponLeverRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flZoomTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_leverrifle, CWeaponLeverRifle );
PRECACHE_WEAPON_REGISTER( weapon_leverrifle );

acttable_t CWeaponLeverRifle::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE,	ACT_MP_STAND_LEVERRIFLE,		false },
	{ ACT_MP_CROUCH_IDLE,	ACT_MP_CROUCH_LEVERRIFLE,		false },
	{ ACT_MP_RUN,			ACT_MP_RUN_LEVERRIFLE,			false },
	{ ACT_MP_AIRWALK,		ACT_MP_AIRWALK_LEVERRIFLE,		false },
	{ ACT_MP_CROUCHWALK,	ACT_MP_CROUCHWALK_LEVERRIFLE,	false },
	{ ACT_MP_JUMP_START,	ACT_MP_JUMP_START_LEVERRIFLE,	false },
	{ ACT_MP_JUMP_FLOAT,	ACT_MP_JUMP_FLOAT_LEVERRIFLE,	false },
	{ ACT_MP_JUMP_LAND,		ACT_MP_JUMP_LAND_LEVERRIFLE,	false },
	{ ACT_MP_SWIM,			ACT_MP_SWIM_LEVERRIFLE,			false },

	{ ACT_MP_CROUCH_DEPLOYED_IDLE,	ACT_MP_CROUCH_DEPLOYED_IDLE_LEVERRIFLE,	false },
	{ ACT_MP_CROUCHWALK_DEPLOYED,	ACT_MP_CROUCHWALK_DEPLOYED_LEVERRIFLE,	false },
	{ ACT_MP_DEPLOYED_IDLE,			ACT_MP_DEPLOYED_IDLE_LEVERRIFLE,		false },
	{ ACT_MP_SWIM_DEPLOYED,			ACT_MP_SWIM_DEPLOYED_LEVERRIFLE,		false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_LEVERRIFLE,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_LEVERRIFLE,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_LEVERRIFLE,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_LEVERRIFLE,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_SECONDARYATTACK_STAND_LEVERRIFLE,	false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_SECONDARYATTACK_CROUCH_LEVERRIFLE,	false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_SECONDARYATTACK_SWIM_LEVERRIFLE,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_SECONDARYATTACK_AIRWALK_LEVERRIFLE,	false },
};

IMPLEMENT_TEMP_ACTTABLE( CWeaponLeverRifle );

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CWeaponLeverRifle::CWeaponLeverRifle()
{
#ifdef CLIENT_DLL
	m_flZoomTransitionTime = 0.0f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CWeaponLeverRifle::~CWeaponLeverRifle()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::Precache()
{
	BaseClass::Precache();

#ifdef GAME_DLL
	PrecacheParticleSystem( "laser_sight_beam" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::Deploy( void )
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::CanHolster( void ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer )
	{
		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( pPlayer->GetFOV() < pPlayer->GetDefaultFOV() && m_flNextPrimaryAttack > gpGlobals->curtime )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( IsZoomed() )
	{
		ZoomOut();
	}
	else if ( m_flZoomTime )
	{
		m_flZoomTime = 0.0f;
	}

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponLeverRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifdef GAME_DLL
	ZoomOut();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponLeverRifle::GetWeaponSpread( void )
{
	// Perfectly accurate when zoomed in.
	if ( IsZoomed() )
	{
		return 0.0f;
	}

	return GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flSpread;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CWeaponLeverRifle::GetProjectileDamage( void )
{
	if ( IsZoomed() )
		return (float)GetTDCWpnData().GetWeaponData( TDC_WEAPON_SECONDARY_MODE ).m_nDamage;

	return (float)GetTDCWpnData().GetWeaponData( TDC_WEAPON_PRIMARY_MODE ).m_nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::CanFireAccurateShot( int nBulletsPerShot )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::ItemPostFrame( void )
{
	// Get the owning player.
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		return;
	}

	// debounce InAttack flags
	if ( m_bInAttack && !( pPlayer->m_nButtons & IN_ATTACK ) )
	{
		m_bInAttack = false;
	}

	if ( m_bInAttack2 && !( pPlayer->m_nButtons & IN_ATTACK2 ) )
	{
		m_bInAttack2 = false;
	}

	HandleZooms();

	if ( ( pPlayer->m_nButtons & IN_ATTACK ) && gpGlobals->curtime >= m_flNextPrimaryAttack )
	{
		if ( HasPrimaryAmmoToFire() )
		{
			PrimaryAttack();
		}
		else
		{
			HandleFireOnEmpty();
		}
	}

	// Idle.
	if ( !( ( pPlayer->m_nButtons & IN_ATTACK ) || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && !IsReloading() )
		{
			WeaponIdle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::FireProjectile( void )
{
	if ( IsZoomed() )
	{
		// Unzoom after a brief delay.
		m_flZoomTime = gpGlobals->curtime + 0.5f;
	}

	BaseClass::FireProjectile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::WeaponIdle( void )
{
	if ( m_flZoomTime || IsZoomed() )
		return;

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CWeaponLeverRifle::GetPrimaryAttackActivity( void )
{
	if ( m_flZoomTime )
		return ACT_VM_PRIMARYATTACK_DEPLOYED;

	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::HandleZooms( void )
{
	// Get the owning player.
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Zoom in/out once transition is finished.
	if ( m_flZoomTime && gpGlobals->curtime >= m_flZoomTime )
	{
		if ( !IsZoomed() )
		{
			ZoomIn();
		}
		else
		{
			ZoomOut();
		}
	}

#ifdef CLIENT_DLL
	if ( m_flZoomTransitionTime < 0.0f && gpGlobals->curtime >= -m_flZoomTransitionTime )
	{
		m_flZoomTransitionTime = 0.0f;
	}
#endif

	// Unzoom if we're jumping or taunting.
	if ( !HasPrimaryAmmoToFire() ||
		pPlayer->m_Shared.IsJumping() ||
		pPlayer->m_Shared.InCond( TDC_COND_TAUNTING ) )
	{
		if ( IsZoomed() && !m_flZoomTime )
		{
			ToggleZoom();
		}

		return;
	}

	// Toggle zoom if can fire and not in the process of switching.
	if ( gpGlobals->curtime >= m_flNextPrimaryAttack && !m_flZoomTime )
	{
		if ( pPlayer->ShouldHoldToZoom() )
		{
			if ( pPlayer->m_nButtons & IN_ATTACK2 )
			{
				if ( !IsZoomed() )
				{
					ToggleZoom();
				}
			}
			else
			{
				if ( IsZoomed() )
				{
					ToggleZoom();
				}
			}
		}
		else
		{
			if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && !m_bInAttack2 )
			{
				ToggleZoom();
				m_bInAttack2 = true;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::ToggleZoom( void )
{
	float flDelay;

	// Toggle the zoom.
	if ( !IsZoomed() )
	{
		// Play transition in and zoom when it's finished.
		SendWeaponAnim( ACT_VM_PULLBACK );
		flDelay = GetTDCWpnData().m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flSmackDelay;
		m_flZoomTime = gpGlobals->curtime + flDelay;

#ifdef CLIENT_DLL
		m_flZoomTransitionTime = m_flZoomTime;
#endif
	}
	else
	{
		// Unzoom and play transition out.
		ZoomOut();
		SendWeaponAnim( ACT_VM_RELEASE );
		flDelay = GetTDCWpnData().m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flSmackDelay;

#ifdef CLIENT_DLL
		m_flZoomTransitionTime = -(gpGlobals->curtime + flDelay);
#endif
	}

	m_flNextPrimaryAttack = Max( m_flNextPrimaryAttack.Get(), gpGlobals->curtime + flDelay );

	CBaseViewModel *vm = GetPlayerViewModel();
	if ( vm )
	{
		vm->SetPlaybackRate( vm->SequenceDuration() / flDelay );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::ZoomIn( void )
{
	// Start aiming.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	if ( !pPlayer )
		return;

	pPlayer->SetFOV( pPlayer, 20 );
	pPlayer->m_Shared.AddCond( TDC_COND_ZOOMED );
	pPlayer->m_Shared.AddCond( TDC_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();
	m_flZoomTime = 0.0f;

#ifdef CLIENT_DLL
	m_flZoomTransitionTime = 0.0f;
#endif

#ifdef GAME_DLL
	pPlayer->ClearExpression();
#endif
}

bool CWeaponLeverRifle::IsZoomed( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TDC_COND_ZOOMED );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::ZoomOut( void )
{
	// Stop aiming
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TDC_COND_ZOOMED ) )
	{
		// Set the FOV to 0 set the default FOV.
		pPlayer->SetFOV( pPlayer, 0, 0.1f );
		pPlayer->m_Shared.RemoveCond( TDC_COND_ZOOMED );
	}

	pPlayer->m_Shared.RemoveCond( TDC_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();
	m_flZoomTime = 0.0f;

#ifdef GAME_DLL
	pPlayer->ClearExpression();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::CanHeadshot( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::CanCritHeadshot( void )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer )
	{
		// no crits if they're not zoomed
		if ( !pPlayer->m_Shared.InCond( TDC_COND_ZOOMED ) )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::GetTracerOrigin( Vector &vecPos )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

#ifdef CLIENT_DLL
	if ( IsZoomed() && UsingViewModel() )
	{
		// Fire tracer from the center of the screen when zoomed in first person to match the laser.
		vecPos = pPlayer->EyePosition() - Vector( 0, 0, 2 );
		return;
	}
#endif

	BaseClass::GetTracerOrigin( vecPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

#ifdef CLIENT_DLL
	UpdateLaserBeam( false );
#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CWeaponLeverRifle::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#else
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CStudioHdr *CWeaponLeverRifle::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	m_iBeamAttachment = LookupAttachment( "laser_emitter" );
	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::UpdateLaserBeam( bool bEnable )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	if ( bEnable )
	{
		if ( !m_pLaserBeamEffect )
		{
			m_pLaserBeamEffect = ParticleProp()->Create( "laser_sight_beam", PATTACH_CUSTOMORIGIN );

			// FIXME: Replace this with proper paintable versions later.
			switch ( GetTeamNumber() )
			{
			case TDC_TEAM_RED:
				m_pLaserBeamEffect->SetControlPoint( 2, Vector( 255, 0, 0 ) );
				break;
			case TDC_TEAM_BLUE:
				m_pLaserBeamEffect->SetControlPoint( 2, Vector( 0, 0, 255 ) );
				break;
			}
		}

		if ( m_pLaserBeamEffect )
		{
			Vector vecEmitPos;
			if ( IsZoomed() && UsingViewModel() )
			{
				vecEmitPos = pPlayer->EyePosition() - Vector( 0, 0, 4 );
			}
			else
			{
				GetWeaponForEffect()->GetAttachment( m_iBeamAttachment, vecEmitPos );
			}

			m_pLaserBeamEffect->SetControlPoint( 0, vecEmitPos );

			trace_t tr;
			Vector vecStart, vecDir, vecEnd;
			vecStart = pPlayer->Weapon_ShootPosition();
			AngleVectors( pPlayer->EyeAngles(), &vecDir );
			vecEnd = vecStart + vecDir * 8192;
			UTIL_TraceLine( vecStart, vecEnd, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

			m_pLaserBeamEffect->SetControlPoint( 1, tr.endpos );
		}
	}
	else
	{
		if ( m_pLaserBeamEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pLaserBeamEffect );
			m_pLaserBeamEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponLeverRifle::Simulate( void )
{
	BaseClass::Simulate();
	UpdateLaserBeam( m_flZoomTime || IsZoomed() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponLeverRifle::ShouldDrawCrosshair( void )
{
	return !IsZoomed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponLeverRifle::ViewModelOffsetScale( void )
{
	if ( IsZoomed() )
	{
		return 0.0f;
	}
	else if ( m_flZoomTransitionTime )
	{
		bool bIsZoomIn = m_flZoomTransitionTime > 0.0f;
		float flDelay = GetTDCWpnData().m_WeaponData[bIsZoomIn ? TDC_WEAPON_PRIMARY_MODE : TDC_WEAPON_SECONDARY_MODE].m_flSmackDelay * 1.5f;
		float flScale = 1.0f;

		if ( bIsZoomIn )
		{
			flScale = ( m_flZoomTransitionTime - gpGlobals->curtime ) / flDelay;
		}
		else
		{
			flScale = 1.0f - ( ( -m_flZoomTransitionTime ) - gpGlobals->curtime ) / flDelay;
		}
		
		flScale = Clamp( flScale, 0.0f, 1.0f );
		flScale = SmoothCurve_Tweak( flScale, 1.0f );
		if ( isnan( flScale ) )
		{
			flScale = 1.0f;
		}

		return flScale;
	}
	else
	{
		return 1.0f;
	}
}
#endif
