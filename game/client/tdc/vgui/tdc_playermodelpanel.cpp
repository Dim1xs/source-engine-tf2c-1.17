//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tdc_playermodelpanel.h"
#include "tdc_playerclass_shared.h"
#include "c_tdc_player.h"
#include "c_te_effect_dispatch.h"
#include <engine/IEngineSound.h>
#include "scenefilecache/ISceneFileCache.h"
#include "c_sceneentity.h"
#include "c_baseflex.h"
#include <sentence.h>
#include "animation.h"
#include <bone_setup.h>
#include "cl_animevent.h"
#include "tdc_gamerules.h"
#include "tdc_merc_customizations.h"
#include "tdc_wearable.h"
#include "c_tdc_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTDCPlayerModelPanel );

CTDCPlayerModelPanel::CTDCPlayerModelPanel( Panel *pParent, const char *pName ) : BaseClass( pParent, pName ),
m_LocalToGlobal( 0, 0, FlexSettingLessFunc )
{
	SetKeyBoardInputEnabled( false );

	m_iTeamNum = TEAM_UNASSIGNED;
	m_iClass = TDC_CLASS_UNDEFINED;
	m_iActiveWpnMDLIndex = -1;
	m_iTauntMDLIndex = -1;
	m_pSpawnEffectData = NULL;
	m_pStudioHdr = NULL;
	memset( m_bCustomClassData, 0, sizeof( m_bCustomClassData ) );
	memset( m_Wearables, 0, sizeof( m_Wearables ) );

	m_pScene = NULL;
	m_flCurrentTime = 0.0f;
	m_bFlexEvents = false;

	memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );
	memset( m_flexWeight, 0, sizeof( m_flexWeight ) );

	for ( int i = 0; i < TDC_PLAYER_WEAPON_COUNT; i++ )
	{
		m_aMergeMDLMap[i] = -1;
	}

	m_bUseMercCvars = false;
	m_vecModelTintColor.Init();
	m_vecModelSkinToneColor.Init( 255.0f, 255.0f, 255.0f );

	for ( int iClass = TDC_FIRST_NORMAL_CLASS; iClass < TDC_CLASS_COUNT_ALL; iClass++ )
	{
		// Load the models for each class.
		const char *pszClassModel = GetPlayerClassData( iClass )->m_szModelName;
		if ( pszClassModel[0] != '\0' )
			engine->LoadModel( pszClassModel );

		const char *pszClassHWMModel = GetPlayerClassData( iClass )->m_szHWMModelName;
		if ( pszClassHWMModel[0] != '\0' )
			engine->LoadModel( pszClassHWMModel );
	}

	// Load phonemes for MP3s.
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes.txt" );
	engine->AddPhonemeFile( NULL );
}

CTDCPlayerModelPanel::~CTDCPlayerModelPanel()
{
	StopVCD();
	UnlockStudioHdr();
	m_LocalToGlobal.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Load in the model portion of the panel's resource file.
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// Parse per class model settings.
	KeyValues *pClassKeys = inResourceData->FindKey( "customclassdata" );

	if ( pClassKeys )
	{
		for ( KeyValues *pSubData = pClassKeys->GetFirstSubKey(); pSubData; pSubData = pSubData->GetNextKey() )
		{
			int iClass = UTIL_StringFieldToInt( pSubData->GetName(), g_aPlayerClassNames_NonLocalized, TDC_CLASS_COUNT_ALL );
			if ( iClass == -1 )
				continue;

			m_ClassResData[iClass].m_flFOV = pSubData->GetFloat( "fov" );
			m_ClassResData[iClass].m_angModelPoseRot.Init( pSubData->GetFloat( "angles_x", 0.0f ), pSubData->GetFloat( "angles_y", 0.0f ), pSubData->GetFloat( "angles_z", 0.0f ) );
			m_ClassResData[iClass].m_vecOriginOffset.Init( pSubData->GetFloat( "origin_x", 110.0 ), pSubData->GetFloat( "origin_y", 5.0 ), pSubData->GetFloat( "origin_z", 5.0 ) );
			m_bCustomClassData[iClass] = true;
		}
	}

	if ( m_bCustomClassData[m_iClass] )
	{
		SetCameraFOV( m_ClassResData[m_iClass].m_flFOV );
		m_vecPlayerPos = m_ClassResData[m_iClass].m_vecOriginOffset;
		m_angPlayer = m_ClassResData[m_iClass].m_angModelPoseRot;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::PrePaint3D( IMatRenderContext *pRenderContext )
{
	BaseClass::PrePaint3D( pRenderContext );

	if ( m_pSpawnEffectData )
	{
		m_pSpawnEffectData->m_bIsUpdateToDate = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::PostPaint3D( IMatRenderContext *pRenderContext )
{
	BaseClass::PostPaint3D( pRenderContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::SetupFlexWeights( void )
{
	if ( m_RootMDL.m_MDL.m_MDLHandle != MDLHANDLE_INVALID && m_pStudioHdr )
	{
		if ( m_pStudioHdr && m_pStudioHdr->pFlexcontroller( LocalFlexController_t( 0 ) )->localToGlobal == -1 )
		{
			// Base class should have initialized flex controllers.
			Assert( false );
			for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
			{
				int j = C_BaseFlex::AddGlobalFlexController( m_pStudioHdr->pFlexcontroller( i )->pszName() );
				m_pStudioHdr->pFlexcontroller( i )->localToGlobal = j;
			}
		}

		memset( m_RootMDL.m_MDL.m_pFlexControls, 0, sizeof( m_RootMDL.m_MDL.m_pFlexControls ) );
		float *pFlexWeights = m_RootMDL.m_MDL.m_pFlexControls;

		if ( m_pScene )
		{
			// slowly decay to neutral expression
			for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
			{
				SetFlexWeight( i, GetFlexWeight( i ) * 0.95 );
			}

			// Run choreo scene.
			m_bFlexEvents = true;
			m_pScene->Think( m_flCurrentTime );
		}

		// Convert local weights from 0..1 to real dynamic range.
		// FIXME: Possibly get rid of this?
		for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
		{
			mstudioflexcontroller_t *pflex = m_pStudioHdr->pFlexcontroller( i );
			pFlexWeights[pflex->localToGlobal] = RemapValClamped( m_flexWeight[i], 0.0f, 1.0f, pflex->min, pflex->max );
		}

		if ( m_pScene )
		{
			// Run choreo scene.
			m_bFlexEvents = false;
			m_pScene->Think( m_flCurrentTime );
			m_flCurrentTime += gpGlobals->frametime;

			if ( m_pScene->SimulationFinished() )
			{
				StopVCD();
			}
		}

		// Drive the mouth from .wav file playback...
		ProcessVisemes( m_PhonemeClasses );

#if 0
		// Convert back to normalized weights
		for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
		{
			mstudioflexcontroller_t *pflex = m_pStudioHdr->pFlexcontroller( i );

			// rescale
			if ( pflex->max != pflex->min )
			{
				pFlexWeights[pflex->localToGlobal] = ( pFlexWeights[pflex->localToGlobal] - pflex->min ) / ( pflex->max - pflex->min );
			}
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::FireEvent( const char *pszEventName, const char *pszEventOptions )
{
	if ( V_stricmp( pszEventName, "AE_WPN_HIDE" ) == 0 )
	{
		if ( m_iTauntMDLIndex != -1 )
		{
			m_aMergeMDLs[m_iTauntMDLIndex].m_bDisabled = true;
		}
		else if ( m_iActiveWpnMDLIndex != -1 )
		{
			m_aMergeMDLs[m_iActiveWpnMDLIndex].m_bDisabled = true;
		}
	}
	else if ( V_stricmp( pszEventName, "AE_WPN_UNHIDE" ) == 0 )
	{
		if ( m_iTauntMDLIndex != -1 )
		{
			m_aMergeMDLs[m_iTauntMDLIndex].m_bDisabled = false;
		}
		else if ( m_iActiveWpnMDLIndex != -1 )
		{
			m_aMergeMDLs[m_iActiveWpnMDLIndex].m_bDisabled = false;
		}
	}
	else if ( V_stricmp( pszEventName, "AE_CL_PLAYSOUND" ) == 0 || atoi( pszEventName ) == CL_EVENT_SOUND )
	{
		C_RecipientFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, pszEventOptions );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::LockStudioHdr()
{
	MDLHandle_t hModel = m_RootMDL.m_MDL.m_MDLHandle;

	Assert( m_pStudioHdr == NULL );
	AUTO_LOCK( m_StudioHdrInitLock );

	if ( m_pStudioHdr != NULL )
	{
		Assert( m_pStudioHdr->GetRenderHdr() == mdlcache->GetStudioHdr( hModel ) );
		return;
	}

	if ( hModel == MDLHANDLE_INVALID )
		return;

	const studiohdr_t *pStudioHdr = mdlcache->LockStudioHdr( hModel );
	if ( !pStudioHdr )
		return;

	CStudioHdr *pNewWrapper = new CStudioHdr( pStudioHdr, mdlcache );
	Assert( pNewWrapper->IsValid() );

	if ( pNewWrapper->GetVirtualModel() )
	{
		MDLHandle_t hVirtualModel = (MDLHandle_t)(intp)( pStudioHdr->VirtualModel() ) & 0xffff;
		mdlcache->LockStudioHdr( hVirtualModel );
	}

	m_pStudioHdr = pNewWrapper; // must be last to ensure virtual model correctly set up
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::UnlockStudioHdr()
{
	MDLHandle_t hModel = m_RootMDL.m_MDL.m_MDLHandle;
	if ( hModel == MDLHANDLE_INVALID )
		return;

	studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( hModel );
	Assert( m_pStudioHdr && m_pStudioHdr->GetRenderHdr() == pStudioHdr );

	if ( pStudioHdr->GetVirtualModel() )
	{
		MDLHandle_t hVirtualModel = (MDLHandle_t)(intp)( pStudioHdr->VirtualModel() ) & 0xffff;
		mdlcache->UnlockStudioHdr( hVirtualModel );
	}

	mdlcache->UnlockStudioHdr( hModel );

	delete m_pStudioHdr;
	m_pStudioHdr = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCPlayerModelPanel::LookupSequence( const char *pszName )
{
	return ::LookupSequence( GetModelPtr(), pszName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCPlayerModelPanel::GetSequenceFrameRate( int nSequence )
{
	static float flPoseParameters[MAXSTUDIOPOSEPARAM] = {};
	return Studio_FPS( GetModelPtr(), nSequence, flPoseParameters );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::SetToPlayerClass( int iClass )
{
	if ( m_iClass == iClass )
		return;

	m_iClass = iClass;

	ClearCarriedItems();
	UnlockStudioHdr();
#if 1
	SetMDL( GetPlayerClassData( m_iClass )->m_szModelName, GetClientRenderable() );
#else
	SetMDL( GetPlayerClassData( m_iClass )->m_szHWMModelName, GetClientRenderable() );
#endif
	LockStudioHdr();
	
	ResetFlexWeights();
	InitPhonemeMappings();

	// Use custom class settings if we have them.
	if ( m_bCustomClassData[m_iClass] )
	{
		SetCameraFOV( m_ClassResData[m_iClass].m_flFOV );
		SetModelAnglesAndPosition( m_ClassResData[m_iClass].m_angModelPoseRot, m_ClassResData[m_iClass].m_vecOriginOffset );
	}
	else
	{
		SetCameraFOV( m_BMPResData.m_flFOV );
		SetModelAnglesAndPosition( m_BMPResData.m_angModelPoseRot, m_BMPResData.m_vecOriginOffset );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::SetTeam( int iTeam )
{
	m_iTeamNum = iTeam;

	// Update skins.
	SetSkin( GetTeamSkin( m_iTeamNum, false ) );

	for ( int i = 0; i < m_aMergeMDLs.Count(); i++ )
	{
		m_aMergeMDLs[i].m_MDL.m_nSkin = GetTeamSkin( m_iTeamNum, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::LoadItems( int iHoldSlot /*= -1*/ )
{
	ClearCarriedItems();

	ETDCWeaponID iWeapon = GetPlayerClassData( m_iClass )->m_iMeleeWeapon;
	if ( iWeapon )
	{
		AddCarriedItem( iWeapon );
	}

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		int id = g_TDCPlayerItems.GetWearableInSlot( m_iClass, (ETDCWearableSlot)i );
		AddWearable( id );
	}

	UpdateBodygroups();

	if ( iHoldSlot != -1 )
	{
		if ( !HoldItemInSlot( iHoldSlot ) )
		{
			// HoldFirstValidItem();
		}
	}
	else
	{
		HoldFirstValidItem();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Merges wearable items from the specified player index
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::LoadWearablesFromPlayer( int iPlayerIndex )
{
	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		int iItemID = g_TDC_PR->GetPlayerItemPreset( iPlayerIndex, ( ETDCWearableSlot )i );

		if ( iItemID != TDC_WEARABLE_INVALID )
			AddWearable( iItemID );
	}

	UpdateBodygroups();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ETDCWeaponID CTDCPlayerModelPanel::GetItemInSlot( int iSlot )
{
	for ( int i = 0; i < m_Weapons.Count(); i++ )
	{
		if ( GetTDCWeaponInfo( m_Weapons[i] )->iSlot == iSlot )
		{
			return m_Weapons[i];
		}
	}

	return WEAPON_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::HoldFirstValidItem( void )
{
	for ( int i = 0; i < TDC_PLAYER_WEAPON_COUNT; i++ )
	{
		if ( HoldItemInSlot( i ) )
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerModelPanel::HoldItemInSlot( int iSlot )
{
	ETDCWeaponID iWeapon = GetItemInSlot( iSlot );
	if ( !iWeapon )
		return false;

	// Hide the previous item and show the selected one.
	if ( m_iActiveWpnMDLIndex != -1 )
	{
		m_aMergeMDLs[m_iActiveWpnMDLIndex].m_bDisabled = true;
	}

	m_iActiveWpnMDLIndex = m_aMergeMDLMap[iSlot];
	if ( m_iActiveWpnMDLIndex != -1 )
	{
		m_aMergeMDLs[m_iActiveWpnMDLIndex].m_bDisabled = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::AddCarriedItem( ETDCWeaponID iWeaponID )
{
	if ( iWeaponID == WEAPON_NONE )
		return;

	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( iWeaponID );
	if ( !pWeaponInfo )
		return;

	int iSlot = pWeaponInfo->iSlot;
	m_Weapons.AddToTail( iWeaponID );

	const char *pszModel = pWeaponInfo->szWorldModel;
	if ( pszModel && pszModel[0] )
	{
		// Add the model and remember its index in merge MDLs list.
		MDLHandle_t hModel = SetMergeMDL( pszModel, NULL, GetTeamSkin( m_iTeamNum, false ) );
		m_aMergeMDLMap[iSlot] = GetMergeMDLIndex( hModel );

		// Hide the model.
		if ( m_aMergeMDLMap[iSlot] != -1 )
		{
			m_aMergeMDLs[m_aMergeMDLMap[iSlot]].m_bDisabled = true;
		}
	}
	else
	{
		m_aMergeMDLMap[iSlot] = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::AddWearable( int iItemID )
{
	WearableDef_t *pItemDef = g_TDCPlayerItems.GetWearable( iItemID );
	if ( !pItemDef || m_Wearables[pItemDef->slot] != NULL )
		return;

	m_Wearables[pItemDef->slot] = pItemDef;
	if ( pItemDef->visuals[m_iClass].model[0] )
	{
		const char *pszModel = pItemDef->visuals[m_iClass].model;
		SetMergeMDL( pszModel, GetClientRenderable(), GetTeamSkin( m_iTeamNum, false ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ClearCarriedItems( void )
{
	StopVCD();
	ClearMergeMDLs();
	m_Weapons.RemoveAll();
	memset( m_Wearables, 0, sizeof( m_Wearables ) );
	m_iActiveWpnMDLIndex = -1;

	for ( int i = 0; i < TDC_PLAYER_WEAPON_COUNT; i++ )
	{
		m_aMergeMDLMap[i] = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::UpdateBodygroups( void )
{
	m_RootMDL.m_MDL.m_nBody = 0;

	for ( int iSlot = 0; iSlot < TDC_WEARABLE_COUNT; iSlot++ )
	{
		WearableDef_t *pItemDef = m_Wearables[iSlot];
		if ( pItemDef != NULL )
		{
			for ( auto i = pItemDef->visuals[m_iClass].bodygroups.First(); i != pItemDef->visuals[m_iClass].bodygroups.InvalidIndex(); i = pItemDef->visuals[m_iClass].bodygroups.Next( i ) )
			{
				const char *pszName = pItemDef->visuals[m_iClass].bodygroups.GetElementName( i );
				int iState = pItemDef->visuals[m_iClass].bodygroups.Element( i );

				int iGroup = FindBodygroupByName( GetModelPtr(), pszName );
				if ( iGroup != -1 )
				{
					SetBodygroup( GetModelPtr(), m_RootMDL.m_MDL.m_nBody, iGroup, iState );
				}
			}
		}
	}
}

extern CFlexSceneFileManager g_FlexSceneFileManager;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::InitPhonemeMappings( void )
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		V_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		V_sprintf_safe( szExpressionName, "%s/phonemes/phonemes", szBasename );
		if ( g_FlexSceneFileManager.FindSceneFile( this, szExpressionName, true ) )
		{
			// Fill in phoneme class lookup
			memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

			Emphasized_Phoneme *normal = &m_PhonemeClasses[PHONEME_CLASS_NORMAL];
			V_strcpy_safe( normal->classname, szExpressionName );
			normal->required = true;

			Emphasized_Phoneme *weak = &m_PhonemeClasses[PHONEME_CLASS_WEAK];
			V_sprintf_safe( weak->classname, "%s_weak", szExpressionName );
			Emphasized_Phoneme *strong = &m_PhonemeClasses[PHONEME_CLASS_STRONG];
			V_sprintf_safe( strong->classname, "%s_strong", szExpressionName );
		}
		else
		{
			// Fill in phoneme class lookup
			memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

			Emphasized_Phoneme *normal = &m_PhonemeClasses[PHONEME_CLASS_NORMAL];
			V_strcpy_safe( normal->classname, "phonemes" );
			normal->required = true;

			Emphasized_Phoneme *weak = &m_PhonemeClasses[PHONEME_CLASS_WEAK];
			V_strcpy_safe( weak->classname, "phonemes_weak" );
			Emphasized_Phoneme *strong = &m_PhonemeClasses[PHONEME_CLASS_STRONG];
			V_strcpy_safe( strong->classname, "phonemes_strong" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::SetFlexWeight( LocalFlexController_t index, float value )
{
	if ( !m_pStudioHdr )
		return;

	if ( index >= 0 && index < m_pStudioHdr->numflexcontrollers() )
	{
		mstudioflexcontroller_t *pflexcontroller = m_pStudioHdr->pFlexcontroller( index );
		m_flexWeight[index] = RemapValClamped( value, pflexcontroller->min, pflexcontroller->max, 0.0f, 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTDCPlayerModelPanel::GetFlexWeight( LocalFlexController_t index )
{
	if ( !m_pStudioHdr )
		return 0.0f;

	if ( index >= 0 && index <  m_pStudioHdr->numflexcontrollers() )
	{
		mstudioflexcontroller_t *pflexcontroller = m_pStudioHdr->pFlexcontroller( index );
		return RemapValClamped( m_flexWeight[index], 0.0f, 1.0f, pflexcontroller->min, pflexcontroller->max );
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ResetFlexWeights( void )
{
	if ( !m_pStudioHdr || m_pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); ++i )
	{
		SetFlexWeight( i, 0.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look up instance specific mapping
//-----------------------------------------------------------------------------
int CTDCPlayerModelPanel::FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key )
{
	FS_LocalToGlobal_t entry( pSettinghdr );

	int idx = m_LocalToGlobal.Find( entry );
	if ( idx == m_LocalToGlobal.InvalidIndex() )
	{
		// This should never happen!!!
		Assert( 0 );
		Warning( "Unable to find mapping for flexcontroller %i, settings %p on CTDCPlayerModelPanel\n", key, pSettinghdr );
		EnsureTranslations( pSettinghdr );
		idx = m_LocalToGlobal.Find( entry );
		if ( idx == m_LocalToGlobal.InvalidIndex() )
		{
			Error( "CTDCPlayerModelPanel::FlexControllerLocalToGlobal failed!\n" );
		}
	}

	FS_LocalToGlobal_t& result = m_LocalToGlobal[idx];
	// Validate lookup
	Assert( result.m_nCount != 0 && key < result.m_nCount );
	int index = result.m_Mapping[key];
	return index;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
LocalFlexController_t CTDCPlayerModelPanel::FindFlexController( const char *szName )
{
	for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
	{
		if ( V_stricmp( m_pStudioHdr->pFlexcontroller( i )->pszName(), szName ) == 0 )
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return LocalFlexController_t( -1 );
}

#define STRONG_CROSSFADE_START		0.60f
#define WEAK_CROSSFADE_START		0.40f

//-----------------------------------------------------------------------------
// Purpose: 
// Here's the formula
// 0.5 is neutral 100 % of the default setting
// Crossfade starts at STRONG_CROSSFADE_START and is full at STRONG_CROSSFADE_END
// If there isn't a strong then the intensity of the underlying phoneme is fixed at 2 x STRONG_CROSSFADE_START
//  so we don't get huge numbers
// Input  : *classes - 
//			emphasis_intensity - 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity )
{
	// See which blends are available for the current phoneme
	bool has_weak = classes[PHONEME_CLASS_WEAK].valid;
	bool has_strong = classes[PHONEME_CLASS_STRONG].valid;

	// Better have phonemes in general
	Assert( classes[PHONEME_CLASS_NORMAL].valid );

	if ( emphasis_intensity > STRONG_CROSSFADE_START )
	{
		if ( has_strong )
		{
			// Blend in some of strong
			float dist_remaining = 1.0f - emphasis_intensity;
			float frac = dist_remaining / ( 1.0f - STRONG_CROSSFADE_START );

			classes[PHONEME_CLASS_NORMAL].amount = (frac)* 2.0f * STRONG_CROSSFADE_START;
			classes[PHONEME_CLASS_STRONG].amount = 1.0f - frac;
		}
		else
		{
			emphasis_intensity = Min( emphasis_intensity, STRONG_CROSSFADE_START );
			classes[PHONEME_CLASS_NORMAL].amount = 2.0f * emphasis_intensity;
		}
	}
	else if ( emphasis_intensity < WEAK_CROSSFADE_START )
	{
		if ( has_weak )
		{
			// Blend in some weak
			float dist_remaining = WEAK_CROSSFADE_START - emphasis_intensity;
			float frac = dist_remaining / ( WEAK_CROSSFADE_START );

			classes[PHONEME_CLASS_NORMAL].amount = ( 1.0f - frac ) * 2.0f * WEAK_CROSSFADE_START;
			classes[PHONEME_CLASS_WEAK].amount = frac;
		}
		else
		{
			emphasis_intensity = Max( emphasis_intensity, WEAK_CROSSFADE_START );
			classes[PHONEME_CLASS_NORMAL].amount = 2.0f * emphasis_intensity;
		}
	}
	else
	{
		// Assume 0.5 (neutral) becomes a scaling of 1.0f
		classes[PHONEME_CLASS_NORMAL].amount = 2.0f * emphasis_intensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			phoneme - 
//			scale - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression )
{
	int type;

	// Setup weights for any emphasis blends
	bool skip = SetupEmphasisBlend( classes, phoneme );
	// Uh-oh, missing or unknown phoneme???
	if ( skip )
	{
		return;
	}

	// Compute blend weights
	ComputeBlendedSetting( classes, emphasis_intensity );

	for ( type = 0; type < NUM_PHONEME_CLASSES; type++ )
	{
		Emphasized_Phoneme *info = &classes[type];
		if ( !info->valid || info->amount == 0.0f )
			continue;

		const flexsettinghdr_t *actual_flexsetting_header = info->base;
		const flexsetting_t *pSetting = actual_flexsetting_header->pIndexedSetting( phoneme );
		if ( !pSetting )
		{
			continue;
		}

		flexweight_t *pWeights = NULL;

		int truecount = pSetting->psetting( (byte *)actual_flexsetting_header, 0, &pWeights );
		if ( pWeights )
		{
			for ( int i = 0; i < truecount; i++ )
			{
				// Translate to global controller number
				int j = FlexControllerLocalToGlobal( actual_flexsetting_header, pWeights->key );
				// Add scaled weighting in
				m_RootMDL.m_MDL.m_pFlexControls[j] += info->amount * scale * pWeights->weight;
				// Go to next setting
				pWeights++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A lot of the one time setup and also resets amount to 0.0f default
//  for strong/weak/normal tracks
// Returning true == skip this phoneme
// Input  : *classes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTDCPlayerModelPanel::SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme )
{
	int i;

	bool skip = false;

	for ( i = 0; i < NUM_PHONEME_CLASSES; i++ )
	{
		Emphasized_Phoneme *info = &classes[i];

		// Assume it's bogus
		info->valid = false;
		info->amount = 0.0f;

		// One time setup
		if ( !info->basechecked )
		{
			info->basechecked = true;
			info->base = (flexsettinghdr_t *)g_FlexSceneFileManager.FindSceneFile( this, info->classname, true );
		}
		info->exp = NULL;
		if ( info->base )
		{
			Assert( info->base->id == ( 'V' << 16 ) + ( 'F' << 8 ) + ( 'E' ) );
			info->exp = info->base->pIndexedSetting( phoneme );
		}

		if ( info->required && ( !info->base || !info->exp ) )
		{
			skip = true;
			break;
		}

		if ( info->exp )
		{
			info->valid = true;
		}
	}

	return skip;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			*sentence - 
//			t - 
//			dt - 
//			juststarted - 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	int pcount = sentence->GetRuntimePhonemeCount();
	for ( int k = 0; k < pcount; k++ )
	{
		const CBasePhonemeTag *phoneme = sentence->GetRuntimePhoneme( k );

		if ( t > phoneme->GetStartTime() && t < phoneme->GetEndTime() )
		{
			// Let's just always do crossfade.
			if ( k < pcount - 1 )
			{
				const CBasePhonemeTag *next = sentence->GetRuntimePhoneme( k + 1 );
				// if I have a neighbor
				if ( next )
				{
					//  and they're touching
					if ( next->GetStartTime() == phoneme->GetEndTime() )
					{
						// no gap, so increase the blend length to the end of the next phoneme, as long as it's not longer than the current phoneme
						dt = Max( dt, Min( next->GetEndTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
					}
					else
					{
						// dead space, so increase the blend length to the start of the next phoneme, as long as it's not longer than the current phoneme
						dt = Max( dt, Min( next->GetStartTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
					}
				}
				else
				{
					// last phoneme in list, increase the blend length to the length of the current phoneme
					dt = Max( dt, phoneme->GetEndTime() - phoneme->GetStartTime() );
				}
			}
		}

		float t1 = ( phoneme->GetStartTime() - t ) / dt;
		float t2 = ( phoneme->GetEndTime() - t ) / dt;

		if ( t1 < 1.0 && t2 > 0 )
		{
			float scale;

			// clamp
			if ( t2 > 1 )
				t2 = 1;
			if ( t1 < 0 )
				t1 = 0;

			// FIXME: simple box filter.  Should use something fancier
			scale = ( t2 - t1 );

			AddViseme( classes, emphasis_intensity, phoneme->GetPhonemeCode(), scale, juststarted );
		}
	}
}

extern ConVar g_CV_PhonemeDelay;
extern ConVar g_CV_PhonemeFilter;
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ProcessVisemes( Emphasized_Phoneme *classes )
{
	// Any sounds being played?
	CMouthInfo *pMouth = clientdll->GetClientUIMouthInfo();
	if ( !pMouth->IsActive() )
		return;

	// Multiple phoneme tracks can overlap, look across all such tracks.
	for ( int source = 0; source < pMouth->GetNumVoiceSources(); source++ )
	{
		CVoiceData *vd = pMouth->GetVoiceSource( source );
		if ( !vd || vd->ShouldIgnorePhonemes() )
			continue;

		CSentence *sentence = engine->GetSentence( vd->GetSource() );
		if ( !sentence )
			continue;

		float	sentence_length = engine->GetSentenceLength( vd->GetSource() );
		float	timesincestart = vd->GetElapsedTime();

		// This sound should be done...why hasn't it been removed yet???
		if ( timesincestart >= ( sentence_length + 2.0f ) )
			continue;

		// Adjust actual time
		float t = timesincestart - g_CV_PhonemeDelay.GetFloat();

		// Get box filter duration
		float dt = g_CV_PhonemeFilter.GetFloat();

		// Streaming sounds get an additional delay...
		/*
		// Tracker 20534:  Probably not needed any more with the async sound stuff that
		//  we now have (we don't have a disk i/o hitch on startup which might have been
		//  messing up the startup timing a bit )
		bool streaming = engine->IsStreaming( vd->m_pAudioSource );
		if ( streaming )
		{
		t -= g_CV_PhonemeDelayStreaming.GetFloat();
		}
		*/

		// Assume sound has been playing for a while...
		bool juststarted = false;

		// Get intensity setting for this time (from spline)
		float emphasis_intensity = sentence->GetIntensity( t, sentence_length );

		// Blend and add visemes together
		AddVisemesForSentence( classes, emphasis_intensity, sentence, t, dt, juststarted );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Since everyone shared a pSettinghdr now, we need to set up the localtoglobal mapping per entity, but 
//  we just do this in memory with an array of integers (could be shorts, I suppose)
// Input  : *pSettinghdr - 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::EnsureTranslations( const flexsettinghdr_t *pSettinghdr )
{
	Assert( pSettinghdr );

	FS_LocalToGlobal_t entry( pSettinghdr );

	unsigned short idx = m_LocalToGlobal.Find( entry );
	if ( idx != m_LocalToGlobal.InvalidIndex() )
		return;

	entry.SetCount( pSettinghdr->numkeys );

	for ( int i = 0; i < pSettinghdr->numkeys; ++i )
	{
		entry.m_Mapping[i] = C_BaseFlex::AddGlobalFlexController( pSettinghdr->pLocalName( i ) );
	}

	m_LocalToGlobal.Insert( entry );
}

extern CChoreoStringPool g_ChoreoStringPool;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CChoreoScene *CTDCPlayerModelPanel::LoadScene( const char *filename )
{
	char loadfile[512];
	V_strcpy_safe( loadfile, filename );
	V_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	V_FixSlashes( loadfile );

	char *pBuffer = NULL;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize <= 0 )
		return NULL;

	pBuffer = new char[bufsize];
	if ( !scenefilecache->GetSceneData( filename, (byte *)pBuffer, bufsize ) )
	{
		delete[] pBuffer;
		return NULL;
	}

	CChoreoScene *pScene;
	if ( IsBufferBinaryVCD( pBuffer, bufsize ) )
	{
		pScene = new CChoreoScene( this );
		CUtlBuffer buf( pBuffer, bufsize, CUtlBuffer::READ_ONLY );
		if ( !pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
		{
			Warning( "Unable to restore binary scene '%s'\n", loadfile );
			delete pScene;
			pScene = NULL;
		}
		else
		{
			pScene->SetPrintFunc( Scene_Printf );
			pScene->SetEventCallbackInterface( this );
		}
	}
	else
	{
		g_TokenProcessor.SetBuffer( pBuffer );
		pScene = ChoreoLoadScene( loadfile, this, &g_TokenProcessor, Scene_Printf );
	}

	delete[] pBuffer;
	return pScene;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::PlayVCD( const char *pszFile )
{
	// Stop the previous scene.
	StopVCD();

	m_flCurrentTime = 0.0f;
	m_pScene = LoadScene( pszFile );

	if ( m_pScene )
	{
		int types[6];
		types[0] = CChoreoEvent::FLEXANIMATION;
		types[1] = CChoreoEvent::EXPRESSION;
		types[2] = CChoreoEvent::GESTURE;
		types[3] = CChoreoEvent::SEQUENCE;
		types[4] = CChoreoEvent::SPEAK;
		types[5] = CChoreoEvent::LOOP;
		m_pScene->RemoveEventsExceptTypes( types, 6 );

		m_pScene->ResetSimulation();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::StopVCD( void )
{
	if ( !m_pScene )
		return;

	delete m_pScene;
	m_pScene = NULL;
	SetSequenceLayers( NULL, 0 );

	ResetFlexWeights();

	// Remove taunt prop.
	if ( m_iTauntMDLIndex != -1 )
	{
		m_aMergeMDLs.Remove( m_iTauntMDLIndex );
		m_iTauntMDLIndex = -1;
	}

	// Always unhide the weapon because some stupid taunts don't fire AE_WPN_UNHIDE.
	if ( m_iActiveWpnMDLIndex != -1 )
	{
		m_aMergeMDLs[m_iActiveWpnMDLIndex].m_bDisabled = false;
	}

	if ( m_pStudioHdr )
	{
		m_RootMDL.m_MDL.m_flPlaybackRate = GetSequenceFrameRate( m_RootMDL.m_MDL.m_nSequence );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::LOOP );

	float backtime = (float)atof( event->GetParameters() );

	bool process = true;
	int counter = event->GetLoopCount();
	if ( counter != -1 )
	{
		int remaining = event->GetNumLoopsRemaining();
		if ( remaining <= 0 )
		{
			process = false;
		}
		else
		{
			event->SetNumLoopsRemaining( --remaining );
		}
	}

	if ( !process )
		return;

	scene->LoopToTime( backtime );

	// Reset the sequences.
	for ( int i = 0; i < m_nNumSequenceLayers; i++ )
	{
		m_SequenceLayers[i].m_flCycleBeganAt += m_flCurrentTime - backtime;
	}

	m_flCurrentTime = backtime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ProcessSequence( CChoreoScene *scene, CChoreoEvent *event )
{
	int nSequence = LookupSequence( event->GetParameters() );

	// make sure sequence exists
	if ( nSequence < 0 )
		return;

	MDLSquenceLayer_t seqLayer;
	seqLayer.m_nSequenceIndex = nSequence;
	seqLayer.m_flWeight = 1.0f;
	seqLayer.m_bNoLoop = true;
	seqLayer.m_flCycleBeganAt = 0.0f;

	SetSequenceLayers( &seqLayer, 1 );
	m_RootMDL.m_MDL.m_flPlaybackRate = GetSequenceFrameRate( nSequence );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event->GetTrackLookupSet() )
	{
		// Create lookup data
		for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
		{
			CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
			if ( !track )
				continue;

			if ( track->IsComboType() )
			{
				char name[512];
				V_strcpy_safe( name, "right_" );
				V_strcat_safe( name, track->GetFlexControllerName() );

				track->SetFlexControllerIndex( Max( FindFlexController( name ), LocalFlexController_t( 0 ) ), 0, 0 );

				V_strcpy_safe( name, "left_" );
				V_strcat_safe( name, track->GetFlexControllerName() );

				track->SetFlexControllerIndex( Max( FindFlexController( name ), LocalFlexController_t( 0 ) ), 0, 1 );
			}
			else
			{
				track->SetFlexControllerIndex( Max( FindFlexController( (char *)track->GetFlexControllerName() ), LocalFlexController_t( 0 ) ), 0 );
			}
		}

		event->SetTrackLookupSet( true );
	}

	float scenetime = scene->GetTime();

	float weight = event->GetIntensity( scenetime );

	// Compute intensity for each track in animation and apply
	// Iterate animation tracks
	for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				LocalFlexController_t controller = track->GetRawFlexControllerIndex( side );

				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scenetime, side );
				if ( controller >= LocalFlexController_t( 0 ) )
				{
					float orig = GetFlexWeight( controller );
					float value = orig * ( 1 - weight ) + flIntensity * weight;
					SetFlexWeight( controller, value );
				}
			}
		}
		else
		{
			LocalFlexController_t controller = track->GetRawFlexControllerIndex( 0 );

			// Get spline intensity for controller
			float flIntensity = track->GetIntensity( scenetime, 0 );
			if ( controller >= LocalFlexController_t( 0 ) )
			{
				float orig = GetFlexWeight( controller );
				float value = orig * ( 1 - weight ) + flIntensity * weight;
				SetFlexWeight( controller, value );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event )
{
	// Look up the actual strings
	const char *scenefile = event->GetParameters();
	const char *name = event->GetParameters2();

	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = (const flexsettinghdr_t *)g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			float scenetime = scene->GetTime();

			float scale = event->GetIntensity( scenetime );

			// Add the named expression
			AddFlexSetting( name, scale, pExpHdr );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expr - 
//			scale - 
//			*pSettinghdr - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	// Find the named setting in the base
	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !V_stricmp( name, expr ) )
			break;
	}

	if ( i >= pSettinghdr->numflexsettings )
	{
		return;
	}

	flexweight_t *pWeights = NULL;
	int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
	if ( !pWeights )
		return;

	for ( i = 0; i < truecount; i++, pWeights++ )
	{
		// Translate to local flex controller
		// this is translating from the settings's local index to the models local index
		int index = FlexControllerLocalToGlobal( pSettinghdr, pWeights->key );

		// blend scaled weighting in to total (post networking g_flexweight!!!!)
		float s = clamp( scale * pWeights->influence, 0.0f, 1.0f );
		m_RootMDL.m_MDL.m_pFlexControls[index] = m_RootMDL.m_MDL.m_pFlexControls[index] * ( 1.0f - s ) + pWeights->weight * s;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !V_stricmp( event->GetName(), "NULL" ) )
	{
		Scene_Printf( "%s : %8.2f:  ignored %s\n", scene->GetFilename(), currenttime, event->GetDescription() );
		return;
	}

	switch ( event->GetType() )
	{
	case CChoreoEvent::SPEAK:
	{
		float time_in_past = m_flCurrentTime - event->GetStartTime();
		float soundtime = gpGlobals->curtime - time_in_past;

		EmitSound_t es;
		es.m_nChannel = CHAN_VOICE;
		es.m_flVolume = 1.0f;
		es.m_SoundLevel = SNDLVL_TALKING;
		es.m_flSoundTime = soundtime;

		es.m_bEmitCloseCaption = false;
		es.m_pSoundName = event->GetParameters();

		C_RecipientFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, es );
		break;
	}
	case CChoreoEvent::LOOP:
		Assert( scene != NULL );
		Assert( event != NULL );
		
		ProcessLoop( scene, event );
		break;

	case CChoreoEvent::SEQUENCE:
		Assert( scene != NULL );
		Assert( event != NULL );

		ProcessSequence( scene, event );
		break;
	}

	event->m_flPrevTime = currenttime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::FLEXANIMATION:
		if ( m_bFlexEvents )
		{
			ProcessFlexAnimation( scene, event );
		}
		break;
	case CChoreoEvent::EXPRESSION:
		if ( !m_bFlexEvents )
		{
			ProcessFlexSettingSceneEvent( scene, event );
		}
		break;
	}

	event->m_flPrevTime = currenttime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerModelPanel::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector &CTDCPlayerModelPanel::GetModelTintColor( void )
{
	if ( m_bUseMercCvars )
	{
		static Vector vecColor;
		vecColor.x = tdc_merc_color_r.GetFloat() / 255.0f;
		vecColor.y = tdc_merc_color_g.GetFloat() / 255.0f;
		vecColor.z = tdc_merc_color_b.GetFloat() / 255.0f;
		return vecColor;
	}

	return m_vecModelTintColor;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::SetModelTintColor( const Vector &vecColor )
{
	m_vecModelTintColor = vecColor;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector &CTDCPlayerModelPanel::GetModelSkinToneColor( void )
{
	if (m_bUseMercCvars)
	{
		static Vector vecColor;
		const CUtlVector<SkinTone_t> &skinTones = g_TDCPlayerItems.GetSkinTones();
		if ( !skinTones.IsEmpty() )
		{
			int iToneIdx = tdc_merc_skintone.GetInt();

			iToneIdx %= skinTones.Count();

			vecColor = skinTones[iToneIdx].tone;
			vecColor /= 255.0f;
		}
		return vecColor;
	}
	return m_vecModelSkinToneColor;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::SetModelSkinToneColor( const Vector &vecColor )
{
	m_vecModelSkinToneColor = vecColor;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerModelPanel::EmitSpawnEffect( const char *pszEffect )
{
	if ( !pszEffect || pszEffect[0] == '\0' )
		return;

	m_bUseParticle = true;

	if ( m_pSpawnEffectData )
	{
		SafeDeleteParticleData( &m_pSpawnEffectData );
	}

	m_pSpawnEffectData = CreateParticleData( pszEffect );

	// We failed at creating that particle for whatever reason, bail (!)
	if ( !m_pSpawnEffectData )
		return;

	CUtlVector<int> vecAttachments;
	m_pSpawnEffectData->UpdateControlPoints( GetModelPtr(), &m_RootMDL.m_MDLToWorld, vecAttachments );

	if ( m_iClass == TDC_CLASS_GRUNT_NORMAL )
	{
		m_pSpawnEffectData->m_pParticleSystem->SetControlPoint( CUSTOM_COLOR_CP1, GetModelTintColor() );
	}
}
