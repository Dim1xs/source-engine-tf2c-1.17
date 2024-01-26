//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_VIEWMODEL_H
#define TDC_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"
#include "tdc_weaponbase.h"

#ifdef CLIENT_DLL
#include "c_tdc_viewmodeladdon.h"
#endif

#if defined( CLIENT_DLL )
#define CTDCViewModel C_TDCViewModel
#endif

#define TDC_VM_MIN_OFFSET -5
#define TDC_VM_MAX_OFFSET 5

class CTDCViewModel : public CBaseViewModel
{
public:
	DECLARE_CLASS( CTDCViewModel, CBaseViewModel );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTDCViewModel( void );

	CTDCWeaponBase *GetTDCWeapon( void );

	virtual void CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles );
	virtual void AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles );

	virtual void SetWeaponModel( const char *pszModelname, CBaseCombatWeapon *weapon );

	void UpdateViewmodelWearable( ETDCWearableSlot iSlot, int iModelIndex );
	void RemoveViewmodelWearable( ETDCWearableSlot iSlot );

	void UpdateViewmodelAddon( int iModelIndex );
	void RemoveViewmodelAddon( void );

#ifdef GAME_DLL
	void UpdateArmsModel( void ) { m_iUpdateArmsParity++; }
#else
	virtual void	Simulate();

	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void SetDormant( bool bDormant );
	virtual void UpdateOnRemove( void );
	void UpdateAddonBodygroups( void );

	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

	virtual int GetSkin( void );
	virtual int	GetArmsSkin( void );
	virtual int GetBody( void );
	virtual void GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );
	BobState_t	&GetBobState() { return m_BobState; }

	virtual int DrawModel( int flags );
	virtual int	InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	CHandle<C_ViewmodelAttachmentModel> m_hViewmodelAddon;

	// We only really care about hand wearables, but for the sake of time and simplicity...
	CHandle< C_ViewmodelAttachmentModel > m_hWearables[ TDC_WEARABLE_COUNT ];

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

#endif

	// Gestures.
	void	ResetGestureSlots( void );
	void	ResetGestureSlot( int iGestureSlot );
	CAnimationLayer* GetGestureSlotLayer( int iGestureSlot );
	bool	IsGestureSlotActive( int iGestureSlot );
	bool	VerifyAnimLayerInSlot( int iGestureSlot );

	// Gesture Slots
	CUtlVector<GestureSlot_t>		m_aGestureSlots;
	bool	InitGestureSlots( void );
	void	ShutdownGestureSlots( void );
	void	StopGestureSlot( int iGestureSlot );
	bool	IsGestureSlotPlaying( int iGestureSlot, Activity iGestureActivity );
	void	AddToGestureSlot( int iGestureSlot, Activity iGestureActivity, bool bAutoKill );
	virtual void RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill = true );
	void	ComputeGestureSequence( CStudioHdr *pStudioHdr );
	void	UpdateGestureLayer( CStudioHdr *pStudioHdr, GestureSlot_t *pGesture );
	void	DebugGestureInfo( void );
	virtual float	GetGesturePlaybackRate( void ) { return 1.0f; }

private:

#ifdef CLIENT_DLL
	void	RunGestureSlotAnimEventsToCompletion( GestureSlot_t *pGesture );
#endif

#if defined( CLIENT_DLL )

	// This is used to lag the angles.
	BobState_t		m_BobState;		// view model head bob state

	CTDCViewModel( const CTDCViewModel & ); // not defined, not accessible

	QAngle m_vLoweredWeaponOffset;
	int m_iOldArmsParity;
	int m_iOldAddonIndex;
	bool m_bForceUpdateArms;

#endif

	CNetworkVar( int, m_iViewModelAddonModelIndex );
	CNetworkArray( int, m_iViewModelWearableIndices, TDC_WEARABLE_COUNT );
	CNetworkVar( int, m_iUpdateArmsParity );
};

#endif // TDC_VIEWMODEL_H
