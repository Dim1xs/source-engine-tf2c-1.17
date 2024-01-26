//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//=============================================================================
#include "cbase.h"
#include "tdc_weaponbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "tdc_gamerules.h"
#include "eventlist.h"
#include "tdc_viewmodel.h"

// Server specific.
#if !defined( CLIENT_DLL )
#include "tdc_player.h"
#include "te_effect_dispatch.h"
// Client specific.
#else
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "c_tdc_player.h"
#include "tdc_viewmodel.h"
#include "hud_crosshair.h"
#include "c_tdc_playerresource.h"
#include "clientmode_tdc.h"
#include "r_efx.h"
#include "dlight.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "toolframework_client.h"
#include "c_droppedmagazine.h"

// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#endif

extern ConVar r_drawviewmodel;

#ifdef CLIENT_DLL
extern ConVar tdc_model_muzzleflash;
extern ConVar tdc_muzzlelight;
#endif

ConVar tdc_weapon_noreload( "tdc_weapon_noreload", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disables reloading for all weapons." );
#define FIDGET_DELAY 0.6f

//=============================================================================
//
// Global functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, ITraceFilter *pFilter )
{
	int	i, j, k;
	trace_t tmpTrace;
	Vector vecEnd;
	float distance = 1e6f;
	Vector minmaxs[2] = { mins, maxs };
	Vector vecHullEnd = tr.endpos;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pFilter, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pFilter, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.endpos - vecSrc ).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

#ifdef CLIENT_DLL
void RecvProxy_Sequence( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_WeaponSequence( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TDCWeaponBase *pWeapon = (C_TDCWeaponBase *)pStruct;

	// Weapons carried by other players have different models on server and client
	// so we should ignore sequence changes in such case.
	if ( !pWeapon->GetOwner() || pWeapon->UsingViewModel() )
	{
		RecvProxy_Sequence( pData, pStruct, pOut );
	}
}
#endif

//=============================================================================
//
// TFWeaponBase tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TDCWeaponBase, DT_TDCWeaponBase )

BEGIN_NETWORK_TABLE_NOBASE( CTDCWeaponBase, DT_LocalTDCWeaponData )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iReloadMode ) ),
	RecvPropTime( RECVINFO( m_flClipRefillTime ) ),
	RecvPropBool( RECVINFO( m_bReloadedThroughAnimEvent ) ),
	RecvPropTime( RECVINFO( m_flLastCritCheckTime ) ),
	RecvPropTime( RECVINFO( m_flCritTime ) ),
	RecvPropTime( RECVINFO( m_flLastFireTime ) ),
	RecvPropTime( RECVINFO( m_flEffectBarRegenTime ) ),
	RecvPropTime( RECVINFO( m_flNextFidgetTime ) ),
#else
	SendPropInt( SENDINFO( m_iReloadMode ), 2, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flClipRefillTime ) ),
	SendPropBool( SENDINFO( m_bReloadedThroughAnimEvent ) ),
	SendPropTime( SENDINFO( m_flLastCritCheckTime ) ),
	SendPropTime( SENDINFO( m_flCritTime ) ),
	SendPropTime( SENDINFO( m_flLastFireTime ) ),
	SendPropTime( SENDINFO( m_flEffectBarRegenTime ) ),
	SendPropTime( SENDINFO( m_flNextFidgetTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTDCWeaponBase, DT_TDCWeaponBase )
	// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iWeaponMode ) ),
	RecvPropBool( RECVINFO( m_bResetParity ) ),

	RecvPropDataTable( "LocalActiveTDCWeaponData", 0, 0, &REFERENCE_RECV_TABLE( DT_LocalTDCWeaponData ) ),

	RecvPropInt( RECVINFO( m_nSequence ), 0, RecvProxy_WeaponSequence ),
	// Server specific.
#else
	SendPropInt( SENDINFO( m_iWeaponMode ), 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bResetParity ) ),

	SendPropDataTable( "LocalActiveTDCWeaponData", 0, &REFERENCE_SEND_TABLE( DT_LocalTDCWeaponData ), SendProxy_SendLocalWeaponDataTable ),

	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropInt( SENDINFO( m_nSequence ), ANIMATION_SEQUENCE_BITS, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTDCWeaponBase )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iWeaponMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCurrentAttackIsCrit, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bInAttack, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bInAttack2, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_iReloadMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flClipRefillTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bReloadedThroughAnimEvent, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLastCritCheckTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCritTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iReloadStartClipAmount, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD( m_flLastFireTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flEffectBarRegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_flNextFidgetTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInFidget, FIELD_BOOLEAN, 0 ),
#endif
END_PREDICTION_DATA()

//=============================================================================
//
// TFWeaponBase shared functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTDCWeaponBase::CTDCWeaponBase()
{
	SetPredictionEligible( true );

	// Nothing collides with these, but they get touch calls.
	AddSolidFlags( FSOLID_TRIGGER );

	// Weapons can fire underwater.
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;

	// Initialize the weapon modes.
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;
	m_iReloadMode.Set( TDC_RELOAD_START );

	m_bInAttack = false;
	m_bInAttack2 = false;
	m_flCritTime = 0;
	m_flLastCritCheckTime = 0;
	m_iLastCritCheckFrame = 0;
	m_bCurrentAttackIsCrit = false;
	m_flLastFireTime = 0.0f;
	m_flNextFidgetTime = 0;
	m_bInFidget = false;

#ifdef CLIENT_DLL
	m_pModelKeyValues = NULL;
	m_pModelWeaponData = NULL;

	m_iMuzzleAttachment = -1;
	m_iBrassAttachment = -1;
	m_iMagBodygroup = -1;
	m_iMagAttachment = -1;
#endif

	m_szTracerName[0] = '\0';

	m_flGivenTime = 0.0f;
}

CTDCWeaponBase::~CTDCWeaponBase()
{
#ifdef CLIENT_DLL
	if ( m_pModelKeyValues )
	{
		m_pModelKeyValues->deleteThis();
	}
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBase::Spawn()
{
	// Base class spawn.
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::Precache()
{
	BaseClass::Precache();

	CBaseEntity::PrecacheModel( GetViewModelPerClass( 0, TDC_CLASS_GRUNT_NORMAL ) );
	CBaseEntity::PrecacheModel( GetViewModelPerClass( 0, TDC_CLASS_GRUNT_HEAVY ) );
	CBaseEntity::PrecacheModel( GetViewModelPerClass( 0, TDC_CLASS_GRUNT_LIGHT ) );

#ifdef GAME_DLL
	const CTDCWeaponInfo *pTFInfo = &GetTDCWpnData();

	// Explosion sound.
	if ( pTFInfo->m_szExplosionSound[0] )
	{
		PrecacheScriptSound( pTFInfo->m_szExplosionSound );
	}

	// Eject brass shells model.
	if ( pTFInfo->m_szBrassModel[0] )
	{
		PrecacheModel( pTFInfo->m_szBrassModel );
	}

	// Ejected magazine model.
	if ( pTFInfo->m_szMagazineModel[0] )
	{
		PrecacheModel( pTFInfo->m_szMagazineModel );
	}

	// Muzzle particle.
	if ( pTFInfo->m_szMuzzleFlashParticleEffect[0] )
	{
		PrecacheParticleSystem( pTFInfo->m_szMuzzleFlashParticleEffect );
	}

	if ( !pTFInfo->m_bHasTeamColoredExplosions )
	{
		// Not using per-team explosion effects.
		for ( int i = 0; i < TDC_EXPLOSION_COUNT; i++ )
		{
			if ( pTFInfo->m_szExplosionEffects[i][0] )
			{
				PrecacheParticleSystem( pTFInfo->m_szExplosionEffects[i] );
			}
		}
	}

	ForEachTeamName( [=]( const char *pszTeamName )
	{
		// Tracers.
		if ( pTFInfo->m_szTracerEffect[0] )
		{
			PrecacheParticleSystem( UTIL_VarArgs( "%s_%s", pTFInfo->m_szTracerEffect, pszTeamName ) );
			PrecacheParticleSystem( UTIL_VarArgs( "%s_%s_crit", pTFInfo->m_szTracerEffect, pszTeamName ) );
		}

		for ( int i = 0; i < TDC_EXPLOSION_COUNT; i++ )
		{
			if ( pTFInfo->m_szExplosionEffects[i][0] )
			{
				if ( pTFInfo->m_bHasTeamColoredExplosions )
				{
					PrecacheParticleSystem( UTIL_VarArgs( "%s_%s", pTFInfo->m_szExplosionEffects[i], pszTeamName ) );
				}

				if ( pTFInfo->m_bHasCritExplosions )
				{
					PrecacheParticleSystem( UTIL_VarArgs( "%s_%s_crit", pTFInfo->m_szExplosionEffects[i], pszTeamName ) );
				}
			}
		}
	}, true );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const CTDCWeaponInfo &CTDCWeaponBase::GetTDCWpnData() const
{
	return static_cast<const CTDCWeaponInfo &>( GetWpnData() );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
ETDCWeaponID CTDCWeaponBase::GetWeaponID( void ) const
{
	Assert( false );
	return WEAPON_NONE;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTDCWeaponBase::IsWeapon( ETDCWeaponID iWeapon ) const
{
	return GetWeaponID() == iWeapon;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTDCWeaponBase::GetFireRate( void )
{
	float flFireDelay = GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	
	if ( pOwner && pOwner->m_Shared.IsHasting() )
	{
		flFireDelay *= TDC_POWERUP_SPEEDBOOST_INV_FACTOR;
	}

	return flFireDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::SetViewModel()
{
	CBaseViewModel *vm = GetPlayerViewModel();
	if ( !vm )
		return;

	Assert( vm->ViewModelIndex() == m_nViewModelIndex );

	const char *pszModelName = GetViewModel( m_nViewModelIndex );
	m_iViewModelIndex = modelinfo->GetModelIndex( pszModelName );
	vm->SetWeaponModel( pszModelName, this );

	UpdateViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: Get the proper attachment model for VM.
//-----------------------------------------------------------------------------
void CTDCWeaponBase::UpdateViewModel( void )
{
	CTDCPlayer *pTFPlayer = GetTDCPlayerOwner();
	if ( !pTFPlayer )
		return;

	CTDCViewModel *vm = static_cast<CTDCViewModel *>( GetPlayerViewModel() );
	if ( !vm )
		return;

	const char *pszAddonModel = NULL;

	if ( GetTDCWpnData().m_bUseHands )
	{
		pszAddonModel = pTFPlayer->GetPlayerClass()->GetHandModelName();
	}

	if ( pszAddonModel && pszAddonModel[0] != '\0' )
	{
		vm->UpdateViewmodelAddon( modelinfo->GetModelIndex( pszAddonModel ) );
	}
	else
	{
		vm->RemoveViewmodelAddon();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseViewModel *CTDCWeaponBase::GetPlayerViewModel( bool bObserverOK /*= false*/ ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return NULL;

	return pPlayer->GetViewModel( m_nViewModelIndex, bObserverOK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCWeaponBase::GetViewModel( int iViewModel ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( pPlayer )
	{
		return GetViewModelPerClass( iViewModel, pPlayer->GetPlayerClass()->GetClassIndex() );
	}

	return BaseClass::GetViewModel( iViewModel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCWeaponBase::GetViewModelPerClass( int iViewModel, int iClassIdx ) const
{
	const CTDCWeaponInfo *pWpnInfo = &GetTDCWpnData();

	switch ( iClassIdx )
	{
	case TDC_CLASS_GRUNT_HEAVY:
		if ( pWpnInfo->m_szViewModelHeavy[0] )
		{
			return pWpnInfo->m_szViewModelHeavy;
		}
		break;

	case TDC_CLASS_GRUNT_LIGHT:
		if ( pWpnInfo->m_szViewModelLight[0] )
		{
			return pWpnInfo->m_szViewModelLight;
		}
		break;
	}
	return BaseClass::GetViewModel( iViewModel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCWeaponBase::GetWorldModel( void ) const
{
	return BaseClass::GetWorldModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::CanHolster( void ) const
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( pOwner->m_Shared.IsMovementLocked() )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	AbortReload();

	// Zoom out.
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( pOwner )
	{
		pOwner->m_Shared.RemoveCondIfPresent( TDC_COND_SOFTZOOM );
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::Deploy( void )
{
	float flOriginalPrimaryAttack = m_flNextPrimaryAttack;
	float flOriginalSecondaryAttack = m_flNextSecondaryAttack;
	float flOriginalFidget = m_flNextFidgetTime;

	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( GetOwner() );
		if ( !pPlayer )
			return false;

		// Overrides the anim length for calculating ready time.
		// Don't override primary attacks that are already further out than this. This prevents
		// people exploiting weapon switches to allow weapons to fire faster.
		float flDeployTime = GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flDeployTime;

		m_flNextPrimaryAttack = Max( flOriginalPrimaryAttack, gpGlobals->curtime + flDeployTime );
		m_flNextSecondaryAttack = Max( flOriginalSecondaryAttack, gpGlobals->curtime + flDeployTime );
		pPlayer->SetNextAttack( m_flNextPrimaryAttack );
		m_flNextFidgetTime = Max( flOriginalFidget, gpGlobals->curtime + FIDGET_DELAY );

		SetModel( GetViewModelPerClass(0, pPlayer->GetPlayerClass()->GetClassIndex() ) );
	}

	return bDeploy;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::GiveTo( CBaseEntity *pEntity )
{
	m_flGivenTime = gpGlobals->curtime;

#ifdef GAME_DLL
	// Base code kind of assumes that we're getting weapons by picking them up like in HL2
	// but in TF2 we only get weapons at spawn and through special ents and all those extra checks
	// can mess with this.
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );
	if ( pPlayer )
	{
		AddEffects( EF_NODRAW );
		pPlayer->Weapon_Equip( this );
		OnPickedUp( pPlayer );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::UnEquip( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();

	if ( pOwner )
	{
		if ( pOwner->GetActiveWeapon() == this )
		{
			pOwner->ResetAutoaim();
			Holster();
			SetThink( NULL );
			pOwner->ClearActiveWeapon();
		}

		if ( pOwner->GetLastWeapon() == this )
		{
			pOwner->Weapon_SetLast( NULL );
		}

		pOwner->Weapon_Detach( this );
	}

	UTIL_Remove( this );
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::IsViewModelFlipped( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();

	if ( pOwner )
	{
		return ( m_bFlipViewModel != pOwner->ShouldFlipViewModel() );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::PrimaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TDC_WEAPON_PRIMARY_MODE;

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTDCWeaponBase::SecondaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TDC_WEAPON_SECONDARY_MODE;

	// Don't hook secondary for now.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Most calls use the prediction seed
//-----------------------------------------------------------------------------
void CTDCWeaponBase::CalcIsAttackCritical( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( gpGlobals->framecount == m_iLastCritCheckFrame )
		return;

	m_iLastCritCheckFrame = gpGlobals->framecount;

	if ( pPlayer->m_Shared.IsCritBoosted() )
	{
		m_bCurrentAttackIsCrit = true;
	}
	else
	{
		// call the weapon-specific helper method
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelper();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Weapon-specific helper method to calculate if attack is crit
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::CalcIsAttackCriticalHelper()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::CanHeadshot( void )
{
	return GetTDCWpnData().m_bCanHeadshot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCWeaponBase::GetMaxClip1( void ) const
{
	int iClip = BaseClass::GetMaxClip1();

	if ( tdc_weapon_noreload.GetBool() && iClip != 1 )
	{
		return WEAPON_NOCLIP;
	}

	// Round to the nearest integer.
	return iClip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCWeaponBase::GetDefaultClip1( void ) const
{
	int iClip = BaseClass::GetDefaultClip1();

	if ( tdc_weapon_noreload.GetBool() && iClip != 1 )
	{
		return WEAPON_NOCLIP;
	}

	// Round to the nearest integer.
	return iClip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCWeaponBase::GetMaxAmmo( void )
{
	return GetTDCWpnData().m_iMaxAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCWeaponBase::GetInitialAmmo( void )
{
	return GetTDCWpnData().m_iSpawnAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: Whether the gun is loaded or not.
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::HasPrimaryAmmoToFire( void )
{
	if ( !UsesPrimaryAmmo() )
		return true;

	if ( UsesClipsForAmmo1() )
		return ( Clip1() > 0 );

	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return false;

	return ( pOwner->GetAmmoCount( GetPrimaryAmmoType() ) > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::Reload( void )
{
	// Sorry, people, no speeding it up.
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	CTDCPlayer *pOwner = GetTDCPlayerOwner();

	if ( IsReloading() )
		return false;

	// If I don't have any spare ammo, I can't reload
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return false;

	if ( Clip1() >= GetMaxClip1() )
		return false;

	// Reload one object at a time.
	if ( ReloadsSingly() )
		return ReloadSingly();

	// Normal reload.
	return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::CheckReload( void )
{
	if ( !IsReloading() )
		return;

	if ( ReloadsSingly() )
	{
		ReloadSingly();
	}
	else
	{
		if ( gpGlobals->curtime >= m_flNextPrimaryAttack )
		{
			FinishReload();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::PerformReloadRefill(bool bDropRemainingMag, bool bSingle, bool bReloadPrimary, bool bReloadSecondary)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if ( pOwner )
	{
		int iLost = 0;

		// If I use primary clips, reload primary
		if ( UsesClipsForAmmo1() && bReloadPrimary )
		{
			int primary = bSingle ? 1 : Min( bDropRemainingMag ? GetMaxClip1() : GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount( m_iPrimaryAmmoType ) );
			if ( bDropRemainingMag )
				iLost += m_iClip1;
			m_iClip1 = (bDropRemainingMag ? primary : m_iClip1 + primary);
			pOwner->RemoveAmmo( primary, m_iPrimaryAmmoType );
		}

		// If I use secondary clips, reload secondary
		if ( UsesClipsForAmmo2() && bReloadSecondary )
		{
			int secondary = bSingle ? 1 : Min( bDropRemainingMag ? GetMaxClip2() : GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount( m_iSecondaryAmmoType ) );
			if ( bDropRemainingMag )
				iLost += m_iClip2;
			m_iClip2 = (bDropRemainingMag ? secondary : m_iClip2 + secondary);
			pOwner->RemoveAmmo( secondary, m_iSecondaryAmmoType );
		}

		if ( bDropRemainingMag )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "player_ammo_lost" );
			if ( event )
			{
				event->SetInt( "userid", ToTDCPlayer(pOwner)->GetUserID() );
				event->SetInt( "amount", -iLost );
				event->SetInt( "ammotype", m_iPrimaryAmmoType );
				event->SetInt( "secondary_ammotype", m_iSecondaryAmmoType );
				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::FinishReload( void )
{
	bool bDropMagOnReload = GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_bDropMagOnReload;
	PerformReloadRefill( bDropMagOnReload );
	m_iReloadMode.Set( TDC_RELOAD_START );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::AbortReload( void )
{
	if ( IsReloading() )
	{
		StopWeaponSound( RELOAD );
		m_iReloadMode.Set( TDC_RELOAD_START );
		m_flNextPrimaryAttack = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::ReloadSingly( void )
{
	// Get the current player.
	CTDCPlayer *pPlayer = ToTDCPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	// check to see if we're ready to reload
	switch ( m_iReloadMode )
	{
	case TDC_RELOAD_START:
	{
		// Play weapon and player animations.
		 SendWeaponAnim( ACT_RELOAD_START );
		 UpdateReloadTimers( true );

		// Next reload the shells.
		m_iReloadMode.Set( TDC_RELOADING );

		m_iReloadStartClipAmount = Clip1();

		return true;
	}
	case TDC_RELOADING:
	{
		// Waiting for the reload start animation to complete.
		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return false;

		if ( Clip1() == GetMaxClip1() )
		{
			// Clip was refilled during reload start animation, abort.
			m_iReloadMode.Set( TDC_RELOAD_FINISH );
			return true;
		}

		// Play weapon reload animations and sound.
		if ( Clip1() == m_iReloadStartClipAmount )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
		}
		else
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
		}

		m_bReloadedThroughAnimEvent = false;

		SendWeaponAnim( ACT_VM_RELOAD );
		UpdateReloadTimers( false );
		OnReloadSinglyUpdate();

		PlayReloadSound();

		// Next continue to reload shells?
		m_iReloadMode.Set( TDC_RELOADING_CONTINUE );

		return true;
	}
	case TDC_RELOADING_CONTINUE:
	{
		// Check if this weapon refills clip midway through reload.
		if ( !m_bReloadedThroughAnimEvent && m_flClipRefillTime != 0.0f && gpGlobals->curtime >= m_flClipRefillTime )
		{
			// If we have ammo, remove ammo and add it to clip
			if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 && m_iClip1 < GetMaxClip1() )
			{
				PerformReloadRefill(false, true, true, false);
			}

			m_bReloadedThroughAnimEvent = true;
			return true;
		}

		// Still waiting for the reload to complete.
		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return false;

		if ( !m_bReloadedThroughAnimEvent )
		{
			// If we have ammo, remove ammo and add it to clip
			if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 && m_iClip1 < GetMaxClip1() )
			{
				PerformReloadRefill(false, true, true, false);
			}
		}

		if ( Clip1() == GetMaxClip1() || pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		{
			m_iReloadMode.Set( TDC_RELOAD_FINISH );
		}
		else
		{
			m_iReloadMode.Set( TDC_RELOADING );
		}

		return true;
	}

	case TDC_RELOAD_FINISH:
	default:
	{
		SendWeaponAnim( ACT_RELOAD_FINISH );

		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );
		OnReloadSinglyUpdate();

		m_iReloadMode.Set( TDC_RELOAD_START );
		return true;
	}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTDCWeaponBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup and check for reload.
	bool bReloadPrimary = false;
	bool bReloadSecondary = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// need to reload primary clip?
		int primary = Min( iClipSize1 - m_iClip1, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		if ( primary != 0 )
		{
			bReloadPrimary = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// need to reload secondary clip?
		int secondary = Min( iClipSize2 - m_iClip2, pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) );
		if ( secondary != 0 )
		{
			bReloadSecondary = true;
		}
	}

	// We didn't reload.
	if ( !( bReloadPrimary || bReloadSecondary ) )
		return false;

	// Play reload
	PlayReloadSound();

	// Play the player's reload animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	SendWeaponAnim( iActivity );

	// Don't rely on animations, always use script time.
	UpdateReloadTimers( false );
	m_iReloadMode.Set( TDC_RELOADING );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::UpdateReloadTimers( bool bStart )
{
	// Starting a reload?
	if ( bStart )
	{
		// Get the reload start time.
		SetReloadTimer( GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeReloadStart );
	}
	// In reload.
	else
	{
		SetReloadTimer( GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeReload, GetTDCWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeReloadRefill );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::SetReloadTimer( float flReloadTime, float flRefillTime /*= 0.0f*/ )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

	float flModifiedTime = flReloadTime;

	if ( pPlayer->m_Shared.IsHasting() )
	{
		flModifiedTime *= TDC_POWERUP_SPEEDBOOST_INV_FACTOR;
	}

	CBaseViewModel *vm = GetPlayerViewModel();
	if ( vm )
	{
		vm->SetPlaybackRate( flReloadTime / flModifiedTime );
	}

	float flTime = gpGlobals->curtime + flModifiedTime;
	float flTimeRatio = flModifiedTime / flReloadTime;

	// Set next weapon attack times (based on reloading).
	m_flNextPrimaryAttack = flTime;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = flTime;

	if ( flRefillTime != 0.0f )
	{
		m_flClipRefillTime = gpGlobals->curtime + flRefillTime * flTimeRatio;
	}
	else
	{
		m_flClipRefillTime = 0.0f;
	}

	// Set next idle time (based on reloading).
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() * flTimeRatio );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBase::PlayReloadSound( void )
{
#ifdef CLIENT_DLL
	// Don't play world reload sound in first person, viewmodel will take care of this.
	if ( UsingViewModel() )
		return;
#endif

	WeaponSound( RELOAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::ItemBusyFrame( void )
{
	// Call into the base ItemBusyFrame.
	BaseClass::ItemBusyFrame();

	CheckEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::ItemPostFrame( void )
{
	CTDCPlayer *pOwner = ToTDCPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// Interrupt a reload.
	if ( IsReloading() && ReloadsSingly() && Clip1() > 0 && ( pOwner->m_nButtons & IN_ATTACK ) )
	{
		AbortReload();
	}

	// debounce InAttack flags
	if ( m_bInAttack && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		m_bInAttack = false;
	}

	if ( m_bInAttack2 && !( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		m_bInAttack2 = false;
	}

	CheckEffectBarRegen();

	if ( m_bInFidget && !( pOwner->m_nButtons & IN_RELOAD ) )
	{
		m_bInFidget = false;
	}
	else if ( !m_bInFidget && pOwner->m_nButtons & IN_RELOAD )
	{
		Fidget();
	}

	// Call the base item post frame.
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::ItemHolsterFrame( void )
{
	CheckEffectBarRegen();
	BaseClass::ItemHolsterFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::SetWeaponVisible( bool visible )
{
	if ( visible && WeaponShouldBeVisible() )
	{
		RemoveEffects( EF_NODRAW );
	}
	else
	{
		AddEffects( EF_NODRAW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::WeaponShouldBeVisible( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return true;

	if ( pOwner->m_Shared.IsLoser() ||
		pOwner->m_Shared.InCond( TDC_COND_STUNNED ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: If the current weapon has more ammo, reload it. Otherwise, switch 
//			to the next best weapon we've got. Returns true if it took any action.
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::ReloadOrSwitchWeapons( void )
{
	CTDCPlayer *pOwner = ToTDCPlayer( GetOwner() );
	Assert( pOwner );

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// weapon isn't useable, switch.
		if ( ( ( GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY ) == false ) && ( g_pGameRules->SwitchToNextBestWeapon( pOwner, this ) ) )
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
			return true;
		}
	}
	else
	{
		// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		// Also auto-reload if owner has auto-reload enabled.
		if ( UsesClipsForAmmo1() && !AutoFiresFullClip() &&
			m_iClip1 == 0 &&
			( GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD ) == false &&
			m_flNextPrimaryAttack < gpGlobals->curtime &&
			m_flNextSecondaryAttack < gpGlobals->curtime )
		{
			// if we're successfully reloading, we're done
			if ( Reload() )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::CanFidget( void )
{
	if ( m_flNextFidgetTime > gpGlobals->curtime )
		return false;

	if ( UsesClipsForAmmo1() )
	{
		if ( Clip1() != GetMaxClip1() )
			return false;
	}
	else
	{
		CTDCPlayer *pOwner = GetTDCPlayerOwner();
		if ( !pOwner )
			return false;

		if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) == 0 )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Player is bored and mashing keys
//-----------------------------------------------------------------------------
void CTDCWeaponBase::Fidget( void )
{
	if ( CanFidget() )
	{
		m_bInFidget = true;
		m_flNextFidgetTime = gpGlobals->curtime + FIDGET_DELAY;
		SendWeaponAnim( GetFidgetActivity() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::SendWeaponAnim( int iActivity )
{
	// Speed up the firing animation when under Haste powerup effects.
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	CBaseViewModel *vm = GetPlayerViewModel();

	if ( BaseClass::SendWeaponAnim( iActivity ) )
	{
		if ( vm && iActivity == ACT_VM_PRIMARYATTACK && pOwner && pOwner->m_Shared.IsHasting() )
		{
			vm->SetPlaybackRate( 1.0f / TDC_POWERUP_SPEEDBOOST_INV_FACTOR );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CTDCWeaponBase::WeaponIdle( void )
{
	if ( IsReloading() )
		return;

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::HasActivity(Activity activity)
{
	return SelectWeightedSequence(activity) != -1;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon draw animation
//-----------------------------------------------------------------------------
Activity CTDCWeaponBase::GetDrawActivity( void )
{
	if ( (gpGlobals->curtime - m_flGivenTime) < 0.1f )
	{
		m_flGivenTime = 0.0f;
		return ACT_VM_PICKUP;
	}
	else if ( Clip1() == 0 )
	{
		return ACT_VM_DRAW_EMPTY;
	}
	else if ( Clip1() < GetMaxClip1() )
	{
		return ACT_VM_DRAW_DEPLOYED;
	}
	else
	{
		return BaseClass::GetDrawActivity();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon fidget animation
//-----------------------------------------------------------------------------
Activity CTDCWeaponBase::GetFidgetActivity( void )
{
	return ACT_VM_FIDGET;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTDCWeaponBase::GetMuzzleFlashParticleEffect( void )
{
	const char *pszPEffect = GetTDCWpnData().m_szMuzzleFlashParticleEffect;

	if ( pszPEffect[0] != '\0' )
	{
		return pszPEffect;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCWeaponBase::GetTracerType( void )
{
	if ( !m_szTracerName[0] )
	{
		const char *pszTracerName = GetTDCWpnData().m_szTracerEffect;

		if ( pszTracerName[0] )
		{
			const char *pszTeamName = GetTeamSuffix( GetTeamNumber(), true );
			V_sprintf_safe( m_szTracerName, "%s_%s", pszTracerName, pszTeamName );
		}
	}

	if ( IsCurrentAttackACrit() && m_szTracerName[0] )
	{
		static char szCritEffect[MAX_TRACER_NAME];
		V_sprintf_safe( szCritEffect, "%s_crit", m_szTracerName );
		return szCritEffect;
	}

	return m_szTracerName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return;

	const char *pszTracerEffect = GetTracerType();
	if ( pszTracerEffect && pszTracerEffect[0] )
	{
		int iAttachment = GetTracerAttachment();

		CEffectData data;
		data.m_vStart = vecTracerSrc;
		data.m_vOrigin = tr.endpos;
#ifdef CLIENT_DLL
		data.m_hEntity = this;
#else
		data.m_nEntIndex = entindex();
#endif
		data.m_nHitBox = GetParticleSystemIndex( pszTracerEffect );

		// Flags
		data.m_fFlags |= TRACER_FLAG_WHIZ;

		if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
		{
			data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
			data.m_nAttachmentIndex = iAttachment;
		}

		data.m_bCustomColors = !TDCGameRules()->IsTeamplay();
		data.m_CustomColors.m_vecColor1 = pOwner->m_vecPlayerColor;

		DispatchEffect( "TFParticleTracer", data );
	}
}

//-----------------------------------------------------------------------------
// Default tracer attachment
//-----------------------------------------------------------------------------
int CTDCWeaponBase::GetTracerAttachment( void )
{
#ifdef GAME_DLL
	// NOTE: The server has no way of knowing the correct index due to differing models.
	// So we're only returning 1 to indicate that this weapon does use an attachment for tracers.
	return 1;
#else
	return m_iMuzzleAttachment;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::GetTracerOrigin( Vector &vecPos )
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return;

#ifdef CLIENT_DLL
	int iAttachment = GetTracerAttachment();
	if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
	{
		GetWeaponForEffect()->GetAttachment( iAttachment, vecPos );

		if ( UsingViewModel() )
		{
			// Adjust view model tracers
			QAngle	vangles;
			Vector	vforward, vright, vup;

			engine->GetViewAngles( vangles );
			AngleVectors( vangles, &vforward, &vright, &vup );

			VectorMA( vecPos, 4, vright, vecPos );
			vecPos.z -= 0.5f;
		}
		return;
	}
#endif

	vecPos = pPlayer->Weapon_ShootPosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::StartEffectBarRegen( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return;

	// Don't recharge unless we actually need recharging.
	if ( gpGlobals->curtime > m_flEffectBarRegenTime ||
		pOwner->GetAmmoCount( m_iPrimaryAmmoType ) + 1 <= pOwner->GetMaxAmmo( m_iPrimaryAmmoType ) )
	{
		m_flEffectBarRegenTime = gpGlobals->curtime + InternalGetEffectBarRechargeTime();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::CanFireAccurateShot( int nBulletsPerShot )
{
	float flFireInterval = gpGlobals->curtime - m_flLastFireTime;
	return ( ( nBulletsPerShot == 1 ) ? flFireInterval > 1.25f : flFireInterval > 0.25f );
}

//-----------------------------------------------------------------------------
// Purpose: Spread pattern for tf_use_fixed_weaponspreads.
//-----------------------------------------------------------------------------
const Vector2D *CTDCWeaponBase::GetSpreadPattern( int &iNumBullets )
{
	static Vector2D vecFixedWpnSpreadPellets[] =
	{
		Vector2D( 0, 0 ),
		Vector2D( 0.5f, 0 ),
		Vector2D( -0.5f, 0 ),
		Vector2D( 0, -0.5f ),
		Vector2D( 0, 0.5f ),
		Vector2D( 0.425f, -0.425f ),
		Vector2D( 0.425f, 0.425f ),
		Vector2D( -0.425f, -0.425f ),
		Vector2D( -0.425f, 0.425f ),
		Vector2D( 0, 0 ),
	};

	iNumBullets = ARRAYSIZE( vecFixedWpnSpreadPellets );
	return vecFixedWpnSpreadPellets;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::EffectBarRegenFinished( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return;

#ifdef GAME_DLL
	pOwner->GiveAmmo( 1, m_iPrimaryAmmoType, true, TDC_AMMO_SOURCE_RESUPPLY );
#endif

	// Keep recharging until we're full on ammo.
#ifdef GAME_DLL
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) < pOwner->GetMaxAmmo( m_iPrimaryAmmoType ) )
#else
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) + 1 < pOwner->GetMaxAmmo( m_iPrimaryAmmoType ) )
#endif
	{
		StartEffectBarRegen();
	}
	else
	{
		m_flEffectBarRegenTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::CheckEffectBarRegen( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_flEffectBarRegenTime != 0.0f )
	{
		// Stop recharging if we're restocked on ammo.
		if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) == pOwner->GetMaxAmmo( m_iPrimaryAmmoType ) )
		{
			m_flEffectBarRegenTime = 0.0f;
		}
		else if ( gpGlobals->curtime >= m_flEffectBarRegenTime )
		{
			m_flEffectBarRegenTime = 0.0f;
			EffectBarRegenFinished();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCWeaponBase::GetEffectBarProgress( void )
{
	CTDCPlayer *pOwner = GetTDCPlayerOwner();
	if ( pOwner && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) < pOwner->GetMaxAmmo( m_iPrimaryAmmoType ) )
	{
		float flTimeLeft = m_flEffectBarRegenTime - gpGlobals->curtime;
		float flRechargeTime = InternalGetEffectBarRechargeTime();
		return RemapValClamped( flTimeLeft, flRechargeTime, 0.0f, 0.0f, 1.0f );
	}

	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::OnControlStunned( void )
{
	AbortReload();
	SetWeaponVisible( false );
}

//=============================================================================
//
// TFWeaponBase functions (Server specific).
//
#if !defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	if ( ( pEvent->type & AE_TYPE_NEWEVENTSYSTEM ) /*&& (pEvent->type & AE_TYPE_SERVER)*/ )
	{
		if ( pEvent->event == AE_WPN_INCREMENTAMMO )
		{
			// Do nothing.
			return;
		}
	}

	BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBase::FallInit( void )
{
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTDCWeaponBase::CheckRespawn()
{
	// Do not respawn.
}

void CTDCWeaponBase::WeaponReset( void )
{
	m_iReloadMode.Set( TDC_RELOAD_START );

	m_bResetParity = !m_bResetParity;

	m_szTracerName[0] = '\0';
	m_flCritTime = 0.0f;
	m_flLastFireTime = 0.0f;
	m_flEffectBarRegenTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
const Vector &CTDCWeaponBase::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

#else

void TE_DynamicLight( IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC );

//=============================================================================
//
// TFWeaponBase functions (Client specific).
//
void CTDCWeaponBase::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt )
{
	Vector vecOrigin;
	QAngle angAngles;

	const char *pszMuzzleFlashParticleEffect = GetMuzzleFlashParticleEffect();

	// If we have an attachment, then stick a light on it.
	if ( m_iMuzzleAttachment > 0 && pszMuzzleFlashParticleEffect )
	{
		pAttachEnt->GetAttachment( m_iMuzzleAttachment, vecOrigin, angAngles );

		// Muzzleflash light
		if ( tdc_muzzlelight.GetBool() )
		{
			CLocalPlayerFilter filter;
			TE_DynamicLight( filter, 0.0f, &vecOrigin, 255, 192, 64, 5, 70.0f, 0.05f, 70.0f / 0.05f, LIGHT_INDEX_MUZZLEFLASH );
		}

		if ( pszMuzzleFlashParticleEffect )
		{
			// Don't do the particle effect for minigun since it already has a looping effect.
			DispatchParticleEffect( pszMuzzleFlashParticleEffect, PATTACH_POINT_FOLLOW, pAttachEnt, m_iMuzzleAttachment );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTDCWeaponBase::InternalDrawModel( int flags )
{
	C_TDCPlayer *pOwner = GetTDCPlayerOwner();
	bool bUseInvulnMaterial = ( pOwner && pOwner->m_Shared.IsInvulnerable() );
	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Use same lighting origin as player when carried.
	if ( GetMoveParent() )
	{
		pInfo->pLightingOrigin = &GetMoveParent()->WorldSpaceCenter();
	}

	return BaseClass::OnInternalDrawModel( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTDCWeaponBase::ShouldDraw( void )
{
	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t CTDCWeaponBase::ShadowCastType( void )
{
	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) )
		return SHADOWS_NONE;

	CBasePlayer *pOwner = GetPlayerOwner();

	if ( pOwner )
	{
		if ( m_iState != WEAPON_IS_ACTIVE )
			return SHADOWS_NONE;

		if ( !pOwner->ShouldDrawThisPlayer() )
			return SHADOWS_NONE;
	}

	return SHADOWS_RENDER_TO_TEXTURE;
}

//-----------------------------------------------------------------------------
// Purpose: Use the correct model based on this player's camera.
// ----------------------------------------------------------------------------
int CTDCWeaponBase::CalcOverrideModelIndex()
{
	if ( UsingViewModel() )
	{
		return m_iViewModelIndex;
	}

	return GetWorldModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTDCWeaponBase::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
	{
		return true;
	}

	return BaseClass::ShouldPredict();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTDCWeaponBase::WeaponReset( void )
{
	m_szTracerName[0] = '\0';

	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTDCWeaponBase::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_bOldResetParity = m_bResetParity;

}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTDCWeaponBase::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( GetPredictable() && !ShouldPredict() )
	{
		ShutdownPredictable();
	}

	if ( m_bResetParity != m_bOldResetParity )
	{
		WeaponReset();
	}

	//Here we go...
	//Since we can't get a repro for the invisible weapon thing, I'll fix it right up here:
	C_TDCPlayer *pOwner = GetTDCPlayerOwner();

	//Our owner is alive
	if ( pOwner && pOwner->IsAlive() == true )
	{
		//And he is NOT taunting
		if ( pOwner->m_Shared.InCond( TDC_COND_TAUNTING ) == false && WeaponShouldBeVisible() )
		{
			//Then why the hell am I NODRAW?
			if ( pOwner->GetActiveWeapon() == this && IsEffectActive( EF_NODRAW ) )
			{
				RemoveEffects( EF_NODRAW );
				UpdateVisibility();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTDCWeaponBase::GetWorldModelIndex( void )
{
	return BaseClass::GetWorldModelIndex();
}

bool CTDCWeaponBase::ShouldDrawCrosshair( void )
{
	return GetTDCWpnData().m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_bDrawCrosshair;
}

void CTDCWeaponBase::Redraw()
{
	if ( ShouldDrawCrosshair() && g_pClientMode->ShouldDrawCrosshair() )
	{
		DrawCrosshair();
	}
}

#endif

acttable_t CTDCWeaponBase::m_acttablePrimary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_PRIMARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_PRIMARY, false },
	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_PRIMARY, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED, false },
	{ ACT_MP_RUN, ACT_MP_RUN_PRIMARY, false },
	{ ACT_MP_WALK, ACT_MP_WALK_PRIMARY, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_PRIMARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_PRIMARY, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_PRIMARY, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_PRIMARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_PRIMARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_PRIMARY, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_PRIMARY, false },
	{ ACT_MP_SWIM_DEPLOYED, ACT_MP_SWIM_DEPLOYED_PRIMARY, false },
	//{ ACT_MP_DEPLOYED,		ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PRIMARY, false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_PRIMARY, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PRIMARY, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_PRIMARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_PRIMARY, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_PRIMARY_END, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_PRIMARY, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_PRIMARY_END, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_PRIMARY, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_PRIMARY_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_PRIMARY, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_PRIMARY_END, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_PRIMARY_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_PRIMARY_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_PRIMARY_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_PRIMARY_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_PRIMARY_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_PRIMARY_GRENADE2_ATTACK, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_PRIMARY, false },
};

acttable_t CTDCWeaponBase::m_acttableSecondary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_SECONDARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_SECONDARY, false },
	{ ACT_MP_RUN, ACT_MP_RUN_SECONDARY, false },
	{ ACT_MP_WALK, ACT_MP_WALK_SECONDARY, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_SECONDARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_SECONDARY, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_SECONDARY, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_SECONDARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_SECONDARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_SECONDARY, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_SECONDARY, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_SECONDARY, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_SECONDARY, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_SECONDARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_SECONDARY, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_SECONDARY_END, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_SECONDARY, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_SECONDARY_END, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_SECONDARY, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_SECONDARY_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_SECONDARY, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_SECONDARY_END, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_SECONDARY, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_SECONDARY_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_SECONDARY_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_SECONDARY_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_SECONDARY_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_SECONDARY_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_SECONDARY_GRENADE2_ATTACK, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_SECONDARY, false },
};

acttable_t CTDCWeaponBase::m_acttableMelee[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_MELEE, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_MELEE, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE, false },
	{ ACT_MP_WALK, ACT_MP_WALK_MELEE, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_MELEE, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_MELEE, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_MELEE, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_MELEE, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_MELEE, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_MELEE, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_MELEE, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_MELEE, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_MELEE, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_MELEE, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_MELEE_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_MELEE_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_MELEE_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_MELEE_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_MELEE_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_MELEE_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_MELEE, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_MELEE, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_MELEE, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_MELEE, false },
};

acttable_t CTDCWeaponBase::m_acttableBuilding[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_BUILDING, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_BUILDING, false },
	{ ACT_MP_RUN, ACT_MP_RUN_BUILDING, false },
	{ ACT_MP_WALK, ACT_MP_WALK_BUILDING, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_BUILDING, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_BUILDING, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_BUILDING, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_BUILDING, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_BUILDING, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_BUILDING, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_BUILDING, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_BUILDING, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_BUILDING, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_BUILDING, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_BUILDING, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_BUILDING, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_BUILDING, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_BUILDING, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_BUILDING, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_BUILDING, false },
};


acttable_t CTDCWeaponBase::m_acttablePDA[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_PDA, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_PDA, false },
	{ ACT_MP_RUN, ACT_MP_RUN_PDA, false },
	{ ACT_MP_WALK, ACT_MP_WALK_PDA, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_PDA, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_PDA, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_PDA, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_PDA, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_PDA, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_PDA, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_PDA, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PDA, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PDA, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_PDA, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_PDA, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_PDA, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_PDA, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_PDA, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_PDA, false },
};

acttable_t CTDCWeaponBase::m_acttableItem1[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_ITEM1, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_ITEM1, false },
	{ ACT_MP_RUN, ACT_MP_RUN_ITEM1, false },
	{ ACT_MP_WALK, ACT_MP_WALK_ITEM1, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_ITEM1, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_ITEM1, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_ITEM1, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_ITEM1, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_ITEM1, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_ITEM1, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_ITEM1, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_ITEM1, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_ITEM1, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM1, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_ITEM1, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM1, false },
	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_ITEM1_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_ITEM1, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM1, false },

	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_ITEM1, false },
	{ ACT_MP_DEPLOYED_IDLE, ACT_MP_DEPLOYED_IDLE_ITEM1, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED_ITEM1, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM1, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_ITEM1, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_ITEM1_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_ITEM1_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_ITEM1_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_ITEM1_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_ITEM1_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_ITEM1_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_ITEM1, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_ITEM1, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_ITEM1, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_ITEM1, false },
};

acttable_t CTDCWeaponBase::m_acttableItem2[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_ITEM2, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_ITEM2, false },
	{ ACT_MP_RUN, ACT_MP_RUN_ITEM2, false },
	{ ACT_MP_WALK, ACT_MP_WALK_ITEM2, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_ITEM2, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_ITEM2, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_ITEM2, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_ITEM2, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_ITEM2, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_ITEM2, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_ITEM2, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_ITEM2, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_ITEM2, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_ITEM2, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_ITEM2, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_ITEM2, false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_ITEM2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM2, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_ITEM2, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM2, false },

	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_ITEM2, false },
	{ ACT_MP_DEPLOYED_IDLE, ACT_MP_DEPLOYED_IDLE_ITEM2, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED_ITEM2, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM2, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_ITEM2_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_ITEM2, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_ITEM2, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_ITEM2, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_ITEM2_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_ITEM2_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_ITEM2_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_ITEM2_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_ITEM2_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_ITEM2_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_ITEM2, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_ITEM2, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_ITEM2, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_ITEM2, false },


	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_ITEM2_END, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_ITEM2_END, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_ITEM2_END, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_ITEM2_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_ITEM2_END, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED_ITEM2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED, ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED_ITEM2, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE_ITEM2, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_CROUCH_GRENADE_ITEM2, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_SWIM_GRENADE_ITEM2, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_AIRWALK_GRENADE_ITEM2, false },

};

acttable_t CTDCWeaponBase::m_acttableMeleeAllClass[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_MELEE_ALLCLASS, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_MELEE_ALLCLASS, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE_ALLCLASS, false },
	{ ACT_MP_WALK, ACT_MP_WALK_MELEE_ALLCLASS, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_MELEE_ALLCLASS, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_MELEE_ALLCLASS, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_MP_ATTACK_STAND_MELEE_SECONDARY_ALLCLASS, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY_ALLCLASS, false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE, ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS, false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE, ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS, false },

	{ ACT_MP_GESTURE_FLINCH, ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW, ACT_MP_MELEE_GRENADE1_DRAW, false },
	{ ACT_MP_GRENADE1_IDLE, ACT_MP_MELEE_GRENADE1_IDLE, false },
	{ ACT_MP_GRENADE1_ATTACK, ACT_MP_MELEE_GRENADE1_ATTACK, false },
	{ ACT_MP_GRENADE2_DRAW, ACT_MP_MELEE_GRENADE2_DRAW, false },
	{ ACT_MP_GRENADE2_IDLE, ACT_MP_MELEE_GRENADE2_IDLE, false },
	{ ACT_MP_GRENADE2_ATTACK, ACT_MP_MELEE_GRENADE2_ATTACK, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_MELEE, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_MELEE, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_MELEE, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_MELEE, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_MELEE, false },
};

acttable_t CTDCWeaponBase::m_acttableSecondary2[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_SECONDARY2, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_SECONDARY2, false },
	{ ACT_MP_RUN, ACT_MP_RUN_SECONDARY2, false },
	{ ACT_MP_WALK, ACT_MP_WALK_SECONDARY2, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_SECONDARY2, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_SECONDARY2, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_SECONDARY2, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_SECONDARY2, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_SECONDARY2, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_SECONDARY2, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_SECONDARY2, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_SECONDARY2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_SECONDARY2, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_SECONDARY2, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_SECONDARY2, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_SECONDARY2, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_SECONDARY2_END, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_SECONDARY2, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_SECONDARY2_END, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_SECONDARY2, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_SECONDARY2_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_SECONDARY2, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_SECONDARY2_LOOP, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_SECONDARY2_END, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_SECONDARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_SECONDARY, false },

};

acttable_t CTDCWeaponBase::m_acttablePrimary2[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MP_STAND_PRIMARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MP_CROUCH_PRIMARY, false },
	{ ACT_MP_DEPLOYED, ACT_MP_DEPLOYED_PRIMARY, false },
	{ ACT_MP_CROUCH_DEPLOYED, ACT_MP_CROUCHWALK_DEPLOYED, false },
	{ ACT_MP_CROUCH_DEPLOYED_IDLE, ACT_MP_CROUCH_DEPLOYED_IDLE, false },
	{ ACT_MP_RUN, ACT_MP_RUN_PRIMARY, false },
	{ ACT_MP_WALK, ACT_MP_WALK_PRIMARY, false },
	{ ACT_MP_AIRWALK, ACT_MP_AIRWALK_PRIMARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MP_CROUCHWALK_PRIMARY, false },
	{ ACT_MP_JUMP, ACT_MP_JUMP_PRIMARY, false },
	{ ACT_MP_JUMP_START, ACT_MP_JUMP_START_PRIMARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MP_JUMP_FLOAT_PRIMARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MP_JUMP_LAND_PRIMARY, false },
	{ ACT_MP_SWIM, ACT_MP_SWIM_PRIMARY, false },
	{ ACT_MP_SWIM_DEPLOYED, ACT_MP_SWIM_DEPLOYED_PRIMARY, false },
	{ ACT_MP_DOUBLEJUMP_CROUCH, ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY, false },
	{ ACT_MP_ATTACK_STAND_PRIMARY_SUPER, ACT_MP_ATTACK_STAND_PRIMARY_SUPER, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER, ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARY_SUPER, ACT_MP_ATTACK_SWIM_PRIMARY_SUPER, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PRIMARY_ALT, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MP_ATTACK_CROUCH_PRIMARY_ALT, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PRIMARY_ALT, false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_MP_ATTACK_AIRWALK_PRIMARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MP_RELOAD_STAND_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_STAND_LOOP, ACT_MP_RELOAD_STAND_PRIMARY_LOOP_ALT, false },
	{ ACT_MP_RELOAD_STAND_END, ACT_MP_RELOAD_STAND_PRIMARY_END_ALT, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MP_RELOAD_CROUCH_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_CROUCH_LOOP, ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP_ALT, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_MP_RELOAD_CROUCH_PRIMARY_END_ALT, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MP_RELOAD_SWIM_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_SWIM_LOOP, ACT_MP_RELOAD_SWIM_PRIMARY_LOOP, false },
	{ ACT_MP_RELOAD_SWIM_END, ACT_MP_RELOAD_SWIM_PRIMARY_END, false },
	{ ACT_MP_RELOAD_AIRWALK, ACT_MP_RELOAD_AIRWALK_PRIMARY_ALT, false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP, ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP_ALT, false },
	{ ACT_MP_RELOAD_AIRWALK_END, ACT_MP_RELOAD_AIRWALK_PRIMARY_END_ALT, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_SWIM_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_MP_ATTACK_STAND_GRENADE, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH, ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT, ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_FISTPUMP, ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_THUMBSUP, ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODYES, ACT_MP_GESTURE_VC_NODYES_PRIMARY, false },
	{ ACT_MP_GESTURE_VC_NODNO, ACT_MP_GESTURE_VC_NODNO_PRIMARY, false },
};

acttable_t *CTDCWeaponBase::ActivityList( int &iActivityCount )
{
	// THIS WHOLE CODE IS TEMPORARY!
	// Remove this once all the weapons have their own act tables.
	enum ETDCWeaponType
	{
		TDC_WPN_TYPE_PRIMARY = 0,
		TDC_WPN_TYPE_SECONDARY,
		TDC_WPN_TYPE_MELEE,
		TDC_WPN_TYPE_GRENADE,
		TDC_WPN_TYPE_BUILDING,
		TDC_WPN_TYPE_PDA,
		TDC_WPN_TYPE_ITEM1,
		TDC_WPN_TYPE_ITEM2,
		TDC_WPN_TYPE_HEAD, // Not sure why these two are here...
		TDC_WPN_TYPE_MISC,
		TDC_WPN_TYPE_MELEE_ALLCLASS,
		TDC_WPN_TYPE_SECONDARY2,
		TDC_WPN_TYPE_PRIMARY2,
		TDC_WPN_TYPE_COUNT,

		TDC_WPN_TYPE_INVALID = -1,
		TDC_WPN_TYPE_NOT_USED = -2,
	};

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();
	if ( !pPlayer )
		return NULL;

	ETDCWeaponType aWeaponTables[][TDC_CLASS_COUNT_ALL] =
	{
		// WEAPON_NONE,
		{},
		// WEAPON_CUBEMAP,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_CROWBAR,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_TIREIRON,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_LEADPIPE,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_UMBRELLA,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_HAMMERFISTS,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_SHOTGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_PISTOL,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_ROCKETLAUNCHER,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_ITEM2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_GRENADELAUNCHER,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_ITEM2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_SUPERSHOTGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_STENGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_REVOLVER,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_CHAINSAW,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_ITEM2,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_ASSAULTRIFLE,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_ITEM2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_DISPLACER,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_LEVERRIFLE,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_NAILGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_ITEM2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_SUPERNAILGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_ITEM2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_FLAMETHROWER,
		{
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_FLAREGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY2,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_REMOTEBOMB,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_FLAG,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_HUNTINGSHOTGUN,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_PRIMARY,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
		// WEAPON_CLAWS,
		{
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_MELEE,
			TDC_WPN_TYPE_SECONDARY,
			TDC_WPN_TYPE_MELEE,
		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE( aWeaponTables ) == WEAPON_COUNT );

	acttable_t *pTable;
	ETDCWeaponType iWeaponRole = aWeaponTables[GetWeaponID()][pPlayer->GetPlayerClass()->GetClassIndex()];

	switch ( iWeaponRole )
	{
	case TDC_WPN_TYPE_PRIMARY:
	default:
		pTable = m_acttablePrimary;
		iActivityCount = ARRAYSIZE( m_acttablePrimary );
		break;
	case TDC_WPN_TYPE_SECONDARY:
		pTable = m_acttableSecondary;
		iActivityCount = ARRAYSIZE( m_acttableSecondary );
		break;
	case TDC_WPN_TYPE_MELEE:
		pTable = m_acttableMelee;
		iActivityCount = ARRAYSIZE( m_acttableMelee );
		break;
	case TDC_WPN_TYPE_BUILDING:
		pTable = m_acttableBuilding;
		iActivityCount = ARRAYSIZE( m_acttableBuilding );
		break;
	case TDC_WPN_TYPE_PDA:
		pTable = m_acttablePDA;
		iActivityCount = ARRAYSIZE( m_acttablePDA );
		break;
	case TDC_WPN_TYPE_ITEM1:
		pTable = m_acttableItem1;
		iActivityCount = ARRAYSIZE( m_acttableItem1 );
		break;
	case TDC_WPN_TYPE_ITEM2:
		pTable = m_acttableItem2;
		iActivityCount = ARRAYSIZE( m_acttableItem2 );
		break;
	case TDC_WPN_TYPE_MELEE_ALLCLASS:
		pTable = m_acttableMeleeAllClass;
		iActivityCount = ARRAYSIZE( m_acttableMeleeAllClass );
		break;
	case TDC_WPN_TYPE_SECONDARY2:
		pTable = m_acttableSecondary2;
		iActivityCount = ARRAYSIZE( m_acttableSecondary2 );
		break;
	case TDC_WPN_TYPE_PRIMARY2:
		pTable = m_acttablePrimary2;
		iActivityCount = ARRAYSIZE( m_acttablePrimary2 );
		break;
	}

	return pTable;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBasePlayer *CTDCWeaponBase::GetPlayerOwner() const
{
	return ToBasePlayer( GetOwner() );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CTDCPlayer *CTDCWeaponBase::GetTDCPlayerOwner() const
{
	return ToTDCPlayer( GetOwner() );
}

#ifdef CLIENT_DLL
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTDCWeaponBase::UsingViewModel( void )
{
	C_TDCPlayer *pOwner = GetTDCPlayerOwner();

	if ( pOwner && !pOwner->ShouldDrawThisPlayer() )
		return true;

	return false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
C_BaseAnimating *CTDCWeaponBase::GetWeaponForEffect()
{
	C_TDCPlayer *pOwner = GetTDCPlayerOwner();

	if ( pOwner && !pOwner->ShouldDrawThisPlayer() )
	{
		C_BaseViewModel *pViewModel = pOwner->GetViewModel( m_nViewModelIndex, false );

		if ( pViewModel )
			return pViewModel;
	}

	return this;
}
#endif

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTDCWeaponBase::CanAttack( void ) const
{
	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	if ( pPlayer )
		return pPlayer->CanAttack();

	return false;
}


#if defined( CLIENT_DLL )

static ConVar	cl_bobcycle( "cl_bobcycle", "0.8" );
static ConVar	cl_bobup( "cl_bobup", "0.5" );

//-----------------------------------------------------------------------------
// Purpose: Helper function to calculate head bob
//-----------------------------------------------------------------------------
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return 0;

	float	cycle;

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	Vector vecVelocity;
	player->EstimateAbsVelocity( vecVelocity );

	float speed = vecVelocity.Length2D();
	float flmaxSpeedDelta = Max( 0.0f, ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * 320.0f );

	// don't allow too big speed changes
	speed = clamp( speed, pBobState->m_flLastSpeed - flmaxSpeedDelta, pBobState->m_flLastSpeed + flmaxSpeedDelta );
	speed = clamp( speed, -320, 320 );

	pBobState->m_flLastSpeed = speed;

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	pBobState->m_flBobTime += ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * bob_offset;
	pBobState->m_flLastBobTime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = pBobState->m_flBobTime - (int)( pBobState->m_flBobTime / cl_bobcycle.GetFloat() )*cl_bobcycle.GetFloat();
	cycle /= cl_bobcycle.GetFloat();

	if ( cycle < cl_bobup.GetFloat() )
	{
		cycle = M_PI_F * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle - cl_bobup.GetFloat() ) / ( 1.0 - cl_bobup.GetFloat() );
	}

	pBobState->m_flVerticalBob = speed*0.005f;
	pBobState->m_flVerticalBob = pBobState->m_flVerticalBob*0.3 + pBobState->m_flVerticalBob*0.7*sin( cycle );

	pBobState->m_flVerticalBob = clamp( pBobState->m_flVerticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = pBobState->m_flBobTime - (int)( pBobState->m_flBobTime / cl_bobcycle.GetFloat() * 2 )*cl_bobcycle.GetFloat() * 2;
	cycle /= cl_bobcycle.GetFloat() * 2;

	if ( cycle < cl_bobup.GetFloat() )
	{
		cycle = M_PI_F * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI_F + M_PI_F*( cycle - cl_bobup.GetFloat() ) / ( 1.0 - cl_bobup.GetFloat() );
	}

	pBobState->m_flLateralBob = speed*0.005f;
	pBobState->m_flLateralBob = pBobState->m_flLateralBob*0.3 + pBobState->m_flLateralBob*0.7*sin( cycle );
	pBobState->m_flLateralBob = clamp( pBobState->m_flLateralBob, -7.0f, 4.0f );

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add head bob
//-----------------------------------------------------------------------------
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return;

	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	// Apply bob, but scaled down to 40%
	VectorMA( origin, pBobState->m_flVerticalBob * 0.4f, forward, origin );

	// Z bob a bit more
	origin[2] += pBobState->m_flVerticalBob * 0.1f;

	// bob the angles
	angles[ROLL] += pBobState->m_flVerticalBob * 0.5f;
	angles[PITCH] -= pBobState->m_flVerticalBob * 0.4f;
	angles[YAW] -= pBobState->m_flLateralBob  * 0.3f;

	VectorMA( origin, pBobState->m_flLateralBob * 0.2f, right, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTDCWeaponBase::CalcViewmodelBob( void )
{
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
		return ::CalcViewModelBobHelper( player, pBobState );
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CTDCWeaponBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	// call helper functions to do the calculation
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
	{
		CalcViewmodelBob();
		::AddViewModelBobHelper( origin, angles, pBobState );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the head bob state for this weapon, which is stored
//			in the view model.  Note that this this function can return
//			NULL if the player is dead or the view model is otherwise not present.
//-----------------------------------------------------------------------------
BobState_t *CTDCWeaponBase::GetBobState()
{
	// get the view model for this weapon
	CTDCViewModel *viewModel = static_cast<CTDCViewModel *>( GetPlayerViewModel() );
	Assert( viewModel );

	// get the bob state out of the view model
	return &( viewModel->GetBobState() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::AllowViewModelOffset( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTDCWeaponBase::ViewModelOffsetScale( void )
{
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCWeaponBase::GetSkin( void )
{
	return GetTeamSkin( GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWeaponBase::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	switch (event)
	{
		
	case TDC_AE_WPN_EJECTBRASS:
	{
		EjectBrass( pViewModel, atoi( options ) );
		return true;
	}
	case AE_CL_DUMP_BRASS:
	{
		DumpBrass( pViewModel, atoi( options ) );
		return true;
	}
	case AE_WPN_INCREMENTAMMO:
	{
		// Do nothing.
		return true;
	}
	case AE_CL_MAG_EJECT:
	{
		C_TDCPlayer *pPlayer = GetTDCPlayerOwner();
		if ( !pPlayer )
			return true;

		// Check if we're actually in the middle of reload anim and it wasn't just interrupted by firing.
		C_AnimationLayer *pAnimLayer = pPlayer-> m_PlayerAnimState->GetGestureSlotLayer( GESTURE_SLOT_ATTACK_AND_RELOAD );
		if ( pAnimLayer->m_flCycle == 1.0f )
			return true;

		Vector vecForce;
		UTIL_StringToVector( vecForce.Base(), options );

		EjectMagazine( pViewModel, vecForce );
		return true;
	}
	case AE_CL_MAG_EJECT_UNHIDE:
	{
		UnhideMagazine( pViewModel );
		return true;
	}
	}

	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::EjectBrass( C_BaseAnimating *pEntity, int iAttachment /*= -1*/ )
{
	if ( iAttachment <= 0 )
	{
		iAttachment = m_iBrassAttachment;
	}

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	CEffectData data;
	pEntity->GetAttachment( iAttachment, data.m_vOrigin, data.m_vAngles );
	data.m_nHitBox = (int)GetWeaponID();
	data.m_nDamageType = GetDamageType();
	if ( pPlayer )
	{
		pPlayer->EstimateAbsVelocity( data.m_vStart );
	}

	DispatchEffect( "TDC_EjectBrass", data );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::DumpBrass( C_BaseAnimating *pEntity, int iAttachment /*= -1*/ )
{
	if ( iAttachment <= 0 )
	{
		iAttachment = m_iBrassAttachment;
	}

	CTDCPlayer *pPlayer = GetTDCPlayerOwner();

	CEffectData data;
	pEntity->GetAttachment( iAttachment, data.m_vOrigin, data.m_vAngles );
	data.m_nHitBox = (int)GetWeaponID();
	data.m_nDamageType = GetDamageType();
	if ( pPlayer )
	{
		pPlayer->EstimateAbsVelocity( data.m_vStart );
	}
	data.m_nAttachmentIndex = Clip1();

	DispatchEffect( "TDC_DumpBrass", data );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::EjectMagazine( C_BaseAnimating *pEntity, const Vector &vecForce )
{
	if ( tdc_ejectmag_max_count.GetInt() <= 0 )
		return;

	const char *pszMagModel = GetTDCWpnData().m_szMagazineModel;
	if ( !pszMagModel[0] )
		return;

	if ( m_iMagAttachment <= 0 )
		return;

	// Spawn the dropped mag at the attachment.
	Vector vecOrigin, vecVelocity;
	QAngle vecAngles;
	Quaternion angleVel;
	pEntity->GetAttachment( m_iMagAttachment, vecOrigin, vecAngles );
	pEntity->GetAttachmentVelocity( m_iMagAttachment, vecVelocity, angleVel );

	if ( vecForce != vec3_origin )
	{
		matrix3x4_t attachmentToWorld;
		pEntity->GetAttachment( m_iMagAttachment, attachmentToWorld );
		
		Vector vecAbsForce;
		VectorRotate( vecForce, attachmentToWorld, vecAbsForce );
		vecVelocity += vecAbsForce;
	}

	C_DroppedMagazine::Create( pszMagModel, vecOrigin, vecAngles, vecVelocity, this );

	// Hide the mag on weapon model.
	if ( m_iMagBodygroup != -1 )
	{
		pEntity->SetBodygroup( m_iMagBodygroup, 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::UnhideMagazine( C_BaseAnimating *pEntity )
{
	if ( m_iMagBodygroup != -1 )
	{
		pEntity->SetBodygroup( m_iMagBodygroup, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CStudioHdr *CTDCWeaponBase::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if ( m_pModelKeyValues )
	{
		m_pModelKeyValues->deleteThis();
	}

	m_pModelKeyValues = new KeyValues( "ModelKeys" );
	m_pModelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) );
	m_pModelWeaponData = m_pModelKeyValues->FindKey( "tf_weapon_data" );

	if ( hdr )
	{
		m_iMuzzleAttachment = LookupAttachment( "muzzle" );
		m_iBrassAttachment = LookupAttachment( "eject_brass" );
		m_iMagBodygroup = FindBodygroupByName( "magazine" );
		m_iMagAttachment = LookupAttachment( "mag_eject" );
	}
	else
	{
		m_iMuzzleAttachment = -1;
		m_iBrassAttachment = -1;
		m_iMagBodygroup = -1;
		m_iMagAttachment = -1;
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponBase::ThirdPersonSwitch( bool bThirdperson )
{
	// Set the model to the correct one.
	int overrideModelIndex = CalcOverrideModelIndex();
	if ( overrideModelIndex != -1 && overrideModelIndex != GetModelIndex() )
	{
		SetModelIndex( overrideModelIndex );
	}
}

#endif

CTDCWeaponInfo *GetTDCWeaponInfo( ETDCWeaponID iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
	{
		return NULL;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	CTDCWeaponInfo *pWeaponInfo = static_cast<CTDCWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

bool CTraceFilterIgnorePlayers::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
	return pEntity && !pEntity->IsPlayer();
}

bool CTraceFilterIgnoreTeammates::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	if ( TDCGameRules()->IsTeamplay() )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

#ifdef CLIENT_DLL
		if ( pEntity->IsBaseCombatCharacter() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
#else
		if ( pEntity->IsCombatCharacter() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
#endif
		{
			return false;
		}
	}

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}
