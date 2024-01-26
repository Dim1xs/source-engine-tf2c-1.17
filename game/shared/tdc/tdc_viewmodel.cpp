//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//
#include "cbase.h"
#include "tdc_wearable.h"
#include "tdc_viewmodel.h"
#include "tdc_shareddefs.h"

#ifdef CLIENT_DLL
#include "c_tdc_player.h"

// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "prediction.h"
#include "view.h"

#endif

#include "bone_setup.h"	//temp
#include "eventlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tdc_viewmodel, CTDCViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( TDCViewModel, DT_TDCViewModel )

BEGIN_NETWORK_TABLE( CTDCViewModel, DT_TDCViewModel )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hOwnerEntity ) ),
	RecvPropInt( RECVINFO( m_iViewModelAddonModelIndex ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_iViewModelWearableIndices ), RecvPropInt( RECVINFO( m_iViewModelWearableIndices[0] ) ) ),
	RecvPropInt( RECVINFO( m_iUpdateArmsParity ) ),
#else
	SendPropEHandle( SENDINFO( m_hOwnerEntity ) ),
	SendPropModelIndex( SENDINFO( m_iViewModelAddonModelIndex ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iViewModelWearableIndices ), SendPropInt( SENDINFO_ARRAY( m_iViewModelWearableIndices ) ) ),
	SendPropInt( SENDINFO( m_iUpdateArmsParity ), 4 ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTDCViewModel )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iViewModelAddonModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
#endif
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTDCViewModel::CTDCViewModel()
{
	m_iViewModelAddonModelIndex = -1;
	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		m_iViewModelWearableIndices.Set( i, -1 );

#ifdef CLIENT_DLL
	m_vLoweredWeaponOffset.Init();

	m_iOldArmsParity = 0;
	m_iOldAddonIndex = -1;
	m_bForceUpdateArms = false;
#endif

	InitGestureSlots();
}

#ifdef CLIENT_DLL
ConVar viewmodel_offset_x( "viewmodel_offset_x", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "", true, TDC_VM_MIN_OFFSET, true, TDC_VM_MAX_OFFSET );
ConVar viewmodel_offset_y( "viewmodel_offset_y", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "", true, TDC_VM_MIN_OFFSET, true, TDC_VM_MAX_OFFSET );
ConVar viewmodel_offset_z( "viewmodel_offset_z", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "", true, TDC_VM_MIN_OFFSET, true, TDC_VM_MAX_OFFSET );

void viewmodel_preset_callback( IConVar *var, const char *pOldValue, float flOldValue );
ConVar viewmodel_preset("viewmodel_preset", "0", 0, "", viewmodel_preset_callback);
#endif

//-----------------------------------------------------------------------------
// Purpose:  Adds head bob for off hand models
//-----------------------------------------------------------------------------
void CTDCViewModel::AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles )
{
#ifdef CLIENT_DLL
	// if we are an off hand view model (index 1) and we have a model, add head bob.
	// (Head bob for main hand model added by the weapon itself.)
	if ( ViewModelIndex() == 1 && GetModel() != null )
	{
		CalcViewModelBobHelper( owner, &m_BobState );
		AddViewModelBobHelper( eyePosition, eyeAngles, &m_BobState );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCWeaponBase *CTDCViewModel::GetTDCWeapon( void )
{
	return static_cast<CTDCWeaponBase *>( GetOwningWeapon() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::SetWeaponModel( const char *modelname, CBaseCombatWeapon *weapon )
{
	if ( !modelname || !modelname[0] )
		RemoveViewmodelAddon();

	BaseClass::SetWeaponModel( modelname, weapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::UpdateViewmodelWearable( ETDCWearableSlot iSlot, int iModelIndex )
{
	m_iViewModelWearableIndices.Set( iSlot, iModelIndex );

#ifdef CLIENT_DLL
	C_ViewmodelAttachmentModel *pWearable = m_hWearables[ iSlot ].Get();
	if ( pWearable )
	{
		pWearable->SetModelIndex( iModelIndex );
		return;
	}

	pWearable = new C_ViewmodelAttachmentModel();
	if ( pWearable->InitializeAsClientEntity( NULL, RENDER_GROUP_VIEW_MODEL_OPAQUE ) == false )
	{
		pWearable->Release();
		return;
	}

	m_hWearables[ iSlot ] = pWearable;

	pWearable->SetModelIndex( iModelIndex );
	pWearable->FollowEntity( this );
	pWearable->SetViewmodel( this );
	pWearable->SetOwnerEntity( GetOwnerEntity() );
	pWearable->UpdateVisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::RemoveViewmodelWearable( ETDCWearableSlot iSlot )
{
	m_iViewModelWearableIndices.Set( iSlot, -1 );
#ifdef CLIENT_DLL
	if ( m_hWearables[ iSlot ].Get() )
	{
		m_hWearables[ iSlot ]->Release();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::UpdateViewmodelAddon( int iModelIndex )
{
	m_iViewModelAddonModelIndex = iModelIndex;

#ifdef CLIENT_DLL
	C_ViewmodelAttachmentModel *pAddon = m_hViewmodelAddon.Get();
	if ( pAddon )
	{
		pAddon->SetModelIndex( iModelIndex );
		return;
	}

	pAddon = new C_ViewmodelAttachmentModel();

	if ( pAddon->InitializeAsClientEntity( NULL, RENDER_GROUP_VIEW_MODEL_OPAQUE ) == false )
	{
		pAddon->Release();
		return;
	}

	m_hViewmodelAddon = pAddon;

	pAddon->SetModelIndex( iModelIndex );
	pAddon->FollowEntity( this );
	pAddon->SetViewmodel( this );
	pAddon->SetOwnerEntity( GetOwnerEntity() );
	pAddon->UpdateVisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::RemoveViewmodelAddon( void )
{
	m_iViewModelAddonModelIndex = -1;

#ifdef CLIENT_DLL
	if ( m_hViewmodelAddon.Get() )
	{
		m_hViewmodelAddon->Release();
	}
#endif
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::Simulate()
{
	BaseClass::Simulate();

	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	ComputeGestureSequence( pStudioHdr );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldArmsParity = m_iUpdateArmsParity;
	m_iOldAddonIndex = m_iViewModelAddonModelIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iOldArmsParity != m_iUpdateArmsParity ||
		m_iOldAddonIndex != m_iViewModelAddonModelIndex ||
		m_bForceUpdateArms ||
		updateType == DATA_UPDATE_CREATED )
	{
		if ( m_iViewModelAddonModelIndex != -1 )
		{
			UpdateViewmodelAddon( m_iViewModelAddonModelIndex );
		}
		else
		{
			RemoveViewmodelAddon();
		}

		for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		{
			if ( m_iViewModelWearableIndices[i] != -1 )
			{
				UpdateViewmodelWearable( (ETDCWearableSlot)i, m_iViewModelWearableIndices[i] );
			}
			else
			{
				RemoveViewmodelWearable( (ETDCWearableSlot)i );
			}
		}

		UpdateAddonBodygroups();
		m_bForceUpdateArms = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::SetDormant( bool bDormant )
{
	if ( bDormant )
	{
		RemoveViewmodelAddon();

		for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
			RemoveViewmodelWearable( ( ETDCWearableSlot )i );
	}
	else
	{
		m_bForceUpdateArms = true;
	}

	BaseClass::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::UpdateOnRemove( void )
{
	RemoveViewmodelAddon();

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		RemoveViewmodelWearable( ( ETDCWearableSlot )i );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::UpdateAddonBodygroups( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( !m_hViewmodelAddon )
		return;

	// Reset bodygroups.
	m_hViewmodelAddon->m_nBody = 0;

	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		CTDCWearable *pWearable = pPlayer->m_hWearables[i];
		if ( !pWearable )
			continue;

		WearableDef_t *pItemDef = pWearable->GetStaticData();
		for ( auto i = pItemDef->visuals[iClass].viewmodelbodygroups.First(); i != pItemDef->visuals[iClass].viewmodelbodygroups.InvalidIndex(); i = pItemDef->visuals[iClass].viewmodelbodygroups.Next( i ) )
		{
			const char *pszName = pItemDef->visuals[iClass].viewmodelbodygroups.GetElementName( i );
			int iState = pItemDef->visuals[iClass].viewmodelbodygroups.Element( i );

			int iGroup = m_hViewmodelAddon->FindBodygroupByName( pszName );
			if ( iGroup != -1 )
			{
				m_hViewmodelAddon->SetBodygroup( iGroup, iState );
			}
		}
	}
}

#endif

void CTDCViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
#if 0
	if ( prediction->InPrediction() )
	{
		return;
	}

	if ( cl_wpn_sway_interp.GetFloat() <= 0.0f )
	{
		return;
	}

	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
	Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= cl_wpn_sway_scale.GetFloat();
	origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;

#endif
}

#ifdef CLIENT_DLL
ConVar cl_gunlowerangle( "cl_gunlowerangle", "90" );
ConVar cl_gunlowerspeed( "cl_gunlowerspeed", "120" );
#endif

void CTDCViewModel::CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
#if defined( CLIENT_DLL )

	Vector vecNewOrigin = eyePosition;
	QAngle vecNewAngles = eyeAngles;

	if ( GetOwner() )
	{
		owner = ToBasePlayer( GetOwner() );
	}

	// Check for lowering the weapon
	C_TDCPlayer *pPlayer = ToTDCPlayer( owner );

	Assert( pPlayer );

	bool bLowered = pPlayer->IsWeaponLowered();

	QAngle vecLoweredAngles( 0, 0, 0 );

	m_vLoweredWeaponOffset.x = Approach( bLowered ? cl_gunlowerangle.GetFloat() : 0, m_vLoweredWeaponOffset.x, cl_gunlowerspeed.GetFloat() * gpGlobals->frametime );
	vecLoweredAngles.x += m_vLoweredWeaponOffset.x;

	vecNewAngles += vecLoweredAngles;

	if ( GetTDCWeapon() && GetTDCWeapon()->AllowViewModelOffset() )
	{
		// Viewmodel offset
		Vector vecForward, vecRight, vecUp;
		AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );

		Vector vecOffset = vecForward * pPlayer->GetViewModelOffset().x +
			vecRight * pPlayer->GetViewModelOffset().y +
			vecUp * pPlayer->GetViewModelOffset().z;

		vecNewOrigin += vecOffset * GetTDCWeapon()->ViewModelOffsetScale();
	}

	BaseClass::CalcViewModelView( owner, vecNewOrigin, vecNewAngles );

#endif
}

//=============================================================================
//
// Multiplayer gesture code.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCViewModel::InitGestureSlots( void )
{
	// Setup the number of gesture slots.
	m_aGestureSlots.AddMultipleToTail( GESTURE_SLOT_COUNT );

	// Assign all of the the CAnimationLayer pointers to null early in case we bail.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		m_aGestureSlots[iGesture].m_pAnimLayer = NULL;
	}

	// Set the number of animation overlays we will use.
	SetNumAnimOverlays( GESTURE_SLOT_COUNT );

	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		m_aGestureSlots[iGesture].m_pAnimLayer = GetAnimOverlay( iGesture );
		if ( !m_aGestureSlots[iGesture].m_pAnimLayer )
			return false;

		ResetGestureSlot( iGesture );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewModel::ShutdownGestureSlots( void )
{
	// Clean up the gesture slots.
	m_aGestureSlots.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewModel::StopGestureSlot( int iGestureSlot )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	GestureSlot_t *pGestureSlot = &m_aGestureSlots[iGestureSlot];

	pGestureSlot->m_iActivity = ACT_INVALID;
	pGestureSlot->m_bAutoKill = false;
	pGestureSlot->m_bActive = false;
	if ( pGestureSlot->m_pAnimLayer )
	{
		pGestureSlot->m_pAnimLayer->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );

#ifdef CLIENT_DLL
		pGestureSlot->m_pAnimLayer->Reset();
#endif 
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewModel::ResetGestureSlots( void )
{
	// Clear out all the gesture slots.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		ResetGestureSlot( iGesture );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewModel::ResetGestureSlot( int iGestureSlot )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	if ( !VerifyAnimLayerInSlot( iGestureSlot ) )
		return;

	GestureSlot_t *pGestureSlot = &m_aGestureSlots[iGestureSlot];
	if ( pGestureSlot )
	{
#ifdef CLIENT_DLL
		// briefly set to 1.0 so we catch the events, before we reset the slot
		pGestureSlot->m_pAnimLayer->m_flCycle = 1.0;

		RunGestureSlotAnimEventsToCompletion( pGestureSlot );
#endif

		pGestureSlot->m_iGestureSlot = GESTURE_SLOT_INVALID;
		pGestureSlot->m_iActivity = ACT_INVALID;
		pGestureSlot->m_bAutoKill = false;
		pGestureSlot->m_bActive = false;
		if ( pGestureSlot->m_pAnimLayer )
		{
			pGestureSlot->m_pAnimLayer->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifdef CLIENT_DLL
			pGestureSlot->m_pAnimLayer->Reset();
#endif
		}
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewModel::RunGestureSlotAnimEventsToCompletion( GestureSlot_t *pGesture )
{
	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Do all the anim events between previous cycle and 1.0, inclusive
	mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( pGesture->m_pAnimLayer->m_nSequence );
	if ( seqdesc.numevents > 0 )
	{
		mstudioevent_t *pevent = seqdesc.pEvent( 0 );

		for ( int i = 0; i < (int)seqdesc.numevents; i++ )
		{
			if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
			{
				if ( !( pevent[i].type & AE_TYPE_CLIENT ) )
					continue;
			}
			else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
				continue;

			if ( pevent[i].cycle > pGesture->m_pAnimLayer->m_flPrevCycle &&
				pevent[i].cycle <= pGesture->m_pAnimLayer->m_flCycle )
			{
				FireEvent( GetAbsOrigin(), GetAbsAngles(), pevent[i].event, pevent[i].pszOptions() );
			}
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCViewModel::IsGestureSlotActive(int iGestureSlot)
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );
	return m_aGestureSlots[iGestureSlot].m_bActive;
}


//-----------------------------------------------------------------------------
// Purpose: Track down a crash
//-----------------------------------------------------------------------------
bool CTDCViewModel::VerifyAnimLayerInSlot(int iGestureSlot)
{
	if ( iGestureSlot < 0 || iGestureSlot >= GESTURE_SLOT_COUNT )
	{
		return false;
	}

	if ( GetNumAnimOverlays() < iGestureSlot + 1 )
	{
//		AssertMsg2( false, "Viewmodel %d doesn't have gesture slot %d any more.", GetBasePlayer()->entindex(), iGestureSlot );
		Msg( "Viewmodel %d doesn't have gesture slot %d any more.\n", entindex(), iGestureSlot );
		m_aGestureSlots[iGestureSlot].m_pAnimLayer = NULL;
		return false;
	}

	CAnimationLayer *pExpected = GetAnimOverlay( iGestureSlot );
	if ( m_aGestureSlots[iGestureSlot].m_pAnimLayer != pExpected )
	{
//		AssertMsg3( false, "Gesture slot %d pointing to wrong address %p. Updating to new address %p.", iGestureSlot, m_aGestureSlots[iGestureSlot].m_pAnimLayer, pExpected );
		Msg( "Gesture slot %d pointing to wrong address %p. Updating to new address %p.\n", iGestureSlot, m_aGestureSlots[iGestureSlot].m_pAnimLayer, pExpected );
		m_aGestureSlots[iGestureSlot].m_pAnimLayer = pExpected;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCViewModel::IsGestureSlotPlaying( int iGestureSlot, Activity iGestureActivity )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	// Check to see if the slot is active.
	if ( !IsGestureSlotActive(iGestureSlot) )
		return false;

	return ( m_aGestureSlots[iGestureSlot].m_iActivity == iGestureActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	if ( !VerifyAnimLayerInSlot(iGestureSlot) )
		return;

	if ( !IsGestureSlotPlaying( iGestureSlot, iGestureActivity ) )
	{
#ifdef CLIENT_DLL
		if ( IsGestureSlotActive( iGestureSlot ) )
		{
			GestureSlot_t *pGesture = &m_aGestureSlots[iGestureSlot];
			if ( pGesture && pGesture->m_pAnimLayer && pGesture->m_pAnimLayer->m_nSequence > 0 )
			{
				pGesture->m_pAnimLayer->m_flCycle = 1.0; // run until the end
				RunGestureSlotAnimEventsToCompletion( &m_aGestureSlots[iGestureSlot] );
			}
		}
#endif

		AddToGestureSlot( iGestureSlot, iGestureActivity, bAutoKill );
		return;
	}

	if ( m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence > 0 )
	{
		// Reset the cycle = restart the gesture.
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::AddToGestureSlot( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	// Make sure we have a valid animation layer to fill out.
	if ( !m_aGestureSlots[iGestureSlot].m_pAnimLayer )
		return;

	if ( !VerifyAnimLayerInSlot(iGestureSlot) )
		return;

	// Get the sequence.
	int iGestureSequence = SelectWeightedSequence( iGestureActivity );
	if ( iGestureSequence <= 0 )
		return;

#ifdef CLIENT_DLL 

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerAnimtime = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerFadeOuttime = 0.0f;

	m_flOverlayPrevEventCycle[iGestureSlot] = -1.0;

#else

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nPriority = 0;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendIn = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendOut = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bSequenceFinished = false;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = gpGlobals->curtime;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bLooping = IsSequenceLooping( iGestureSequence );
	if ( bAutoKill )
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_ACTIVE;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAnimationLayer *CTDCViewModel::GetGestureSlotLayer( int iGestureSlot )
{
	return m_aGestureSlots[iGestureSlot].m_pAnimLayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CTDCViewModel::ComputeGestureSequence( CStudioHdr *pStudioHdr )
{
	// Update all active gesture layers.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		if ( !m_aGestureSlots[iGesture].m_bActive )
			continue;

		if ( !VerifyAnimLayerInSlot( iGesture ) )
			continue;

		UpdateGestureLayer( pStudioHdr, &m_aGestureSlots[iGesture] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCViewModel::UpdateGestureLayer( CStudioHdr *pStudioHdr, GestureSlot_t *pGesture )
{
	// Sanity check.
	if ( !pStudioHdr || !pGesture )
		return;

#ifdef CLIENT_DLL 

	// Get the current cycle.
	float flCycle = pGesture->m_pAnimLayer->m_flCycle;
	flCycle += GetSequenceCycleRate( pStudioHdr, pGesture->m_pAnimLayer->m_nSequence ) * gpGlobals->frametime * GetGesturePlaybackRate() * pGesture->m_pAnimLayer->m_flPlaybackRate;

	pGesture->m_pAnimLayer->m_flPrevCycle = pGesture->m_pAnimLayer->m_flCycle;
	pGesture->m_pAnimLayer->m_flCycle = flCycle;

	if ( flCycle > 1.0f )
	{
		RunGestureSlotAnimEventsToCompletion( pGesture );

		if ( pGesture->m_bAutoKill )
		{
			ResetGestureSlot( pGesture->m_iGestureSlot );
			return;
		}
		else
		{
			pGesture->m_pAnimLayer->m_flCycle = 1.0f;
		}
	}

#else

	if ( pGesture->m_iActivity != ACT_INVALID && pGesture->m_pAnimLayer->m_nActivity == ACT_INVALID )
	{
		ResetGestureSlot( pGesture->m_iGestureSlot );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewModel::DebugGestureInfo( void )
{
	/*CBasePlayer *pPlayer = GetBasePlayer();
	if ( !pPlayer )
		return;

	int iLine = ( pPlayer->IsServer() ? 12 : ( 14 + GESTURE_SLOT_COUNT ) );

	Anim_StatePrintf( iLine++, "%s\n", ( pPlayer->IsServer() ? "Server" : "Client" ) );

	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		GestureSlot_t *pGesture = &m_aGestureSlots[iGesture];
		if ( pGesture )
		{
			if( pGesture->m_bActive )
			{
				Anim_StatePrintf( iLine++, "Gesture Slot %d(%s): %s %s(A:%s, C:%f P:%f)\n",
					iGesture,
					s_aGestureSlotNames[iGesture],
					ActivityList_NameForIndex( pGesture->m_iActivity ),
					GetSequenceName( pPlayer->GetModelPtr(), pGesture->m_pAnimLayer->m_nSequence ),
					( pGesture->m_bAutoKill ? "true" : "false" ),
					(float)pGesture->m_pAnimLayer->m_flCycle, (float)pGesture->m_pAnimLayer->m_flPlaybackRate );
			}
			else
			{
				Anim_StatePrintf( iLine++, "Gesture Slot %d(%s): NOT ACTIVE!\n", iGesture, s_aGestureSlotNames[iGesture] );
			}
		}
	}*/
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Don't render the weapon if its supposed to be lowered and we have 
// finished the lowering animation
//-----------------------------------------------------------------------------
int CTDCViewModel::DrawModel( int flags )
{
	// Check for lowering the weapon
	C_TDCPlayer *pPlayer = ToTDCPlayer( GetOwner() );;

	// Don't draw viewmodels of dead players.
	if ( !pPlayer || !pPlayer->IsAlive() )
		return 0;

	// Only draw viewmodel of the spectated player.
	if ( GetLocalObservedPlayer( true ) != pPlayer )
		return 0;

	bool bLowered = pPlayer->IsWeaponLowered();

	if ( bLowered && fabs( m_vLoweredWeaponOffset.x - cl_gunlowerangle.GetFloat() ) < 0.1 )
	{
		// fully lowered, stop drawing
		return 1;
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCViewModel::InternalDrawModel( int flags )
{
	CMatRenderContextPtr pRenderContext( materials );

	if ( ShouldFlipViewModel() )
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	// Draw the attachments together with the viewmodel so any effects applied to VM are applied to attachments as well.
	if ( m_hViewmodelAddon.Get() )
	{
		// Necessary for lighting blending
		m_hViewmodelAddon->CreateModelInstance();
		m_hViewmodelAddon->InternalDrawModel( flags );
	}

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		if ( m_hWearables[i].Get() )
		{
			m_hWearables[i]->CreateModelInstance();
			m_hWearables[i]->InternalDrawModel( flags );
		}
	}

	int ret = CBaseAnimating::InternalDrawModel( flags );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	return ret;
}\

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCViewModel::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Use camera position for lighting origin.
	pInfo->pLightingOrigin = &MainViewOrigin();
	return BaseClass::OnInternalDrawModel( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCViewModel::GetSkin()
{
	C_TDCWeaponBase *pWeapon = static_cast<C_TDCWeaponBase *>( GetOwningWeapon() );
	if ( !pWeapon ) 
		return 0;

	return pWeapon->GetSkin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCViewModel::GetArmsSkin( void )
{
	C_BasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	return GetTeamSkin( pOwner->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: Mimic the weapon's bodygroups.
//-----------------------------------------------------------------------------
int CTDCViewModel::GetBody( void )
{
	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->GetModelIndex() == GetModelIndex() )
	{
		return pWeapon->GetBody();
	}

	return BaseClass::GetBody();
}

//-----------------------------------------------------------------------------
// Purpose: Mimic the weapon's pose parameters.
//-----------------------------------------------------------------------------
void CTDCViewModel::GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] )
{
	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->GetModelIndex() == GetModelIndex() )
	{
		return pWeapon->GetPoseParameters( pWeapon->GetModelPtr(), poseParameter );
	}

	return BaseClass::GetPoseParameters( pStudioHdr, poseParameter );
}

//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void CTDCViewModel::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	// Don't process animevents if it's not drawn.
	C_BasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( !pOwner || pOwner->ShouldDrawThisPlayer() )
		return;

	BaseClass::FireEvent( origin, angles, event, options );
}

struct ViewModelPreset_t
{
	float fFov;
	Vector vOffset;
};

static const ViewModelPreset_t s_viewModelPresets[] =
{
	{ 65, Vector( 2.0f, 1.5f, -2.0f ) },	//Defualt
	{ 60, Vector( 5.0f, 1.0f, -5.0f) },		//Minimized
	{ 54, Vector( 0.0f, 0.0f, 0.0f ) },		//Couch
	{ 54, Vector( 5.0f, -5.0f, -2.0f) },	//Front and Center
	{ 70, Vector( 3.0f, 2.0f, -3.0f) },		//Theoretical Physicist
	{ 60, Vector( 5.0f, -2.0f, -5.0f) },	//Earthquake
	{ 70, Vector( 3.0f, 3.5f, -5.0f) },		//Secret Agent
	{ 68, Vector( 2.5f, 1.0f, -1.5f) },		//Gold Source
	{ 60, Vector( 1.0f, 1.0f, -1.5f) },		//Counter Terrorist
	{ 54, Vector( 5.0f, 0.0f, -0.8f) },		//Super Soldier
};

//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void viewmodel_preset_callback( IConVar* /*var*/, const char* /*pOldValue*/, float /*flOldValue*/ )
{
	int newPreset = viewmodel_preset.GetInt();
	if ( newPreset < 0 || newPreset >= ARRAYSIZE( s_viewModelPresets ) )
		return;

	const ViewModelPreset_t &preset = s_viewModelPresets[newPreset];
	engine->ClientCmd( VarArgs( "viewmodel_fov %f", preset.fFov ) );
	viewmodel_offset_x.SetValue( preset.vOffset.x );
	viewmodel_offset_y.SetValue( preset.vOffset.y );
	viewmodel_offset_z.SetValue( preset.vOffset.z );
}

#endif // CLIENT_DLL
