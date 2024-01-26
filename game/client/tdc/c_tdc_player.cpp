//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tdc_player.h"
#include "c_user_message_register.h"
#include "view.h"
#include "iclientvehicle.h"
#include "ivieweffects.h"
#include "input.h"
#include "IEffects.h"
#include "fx.h"
#include "c_basetempentity.h"
#include "hud_macros.h"
#include "engine/ivdebugoverlay.h"
#include "smoke_fog_overlay.h"
#include "playerandobjectenumerator.h"
#include "bone_setup.h"
#include "in_buttons.h"
#include "r_efx.h"
#include "dlight.h"
#include "shake.h"
#include "cl_animevent.h"
#include "tdc_weaponbase.h"
#include "c_tdc_playerresource.h"
#include "toolframework/itoolframework.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "prediction.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tdc_fx_muzzleflash.h"
#include "tdc_gamerules.h"
#include "view_scene.h"
#include "toolframework_client.h"
#include "soundenvelope.h"
#include "voice_status.h"
#include "clienteffectprecachesystem.h"
#include "functionproxy.h"
#include "toolframework_client.h"
#include "choreoevent.h"
#include "vguicenterprint.h"
#include "eventlist.h"
#include "tdc_hud_statpanel.h"
#include "input.h"
#include "in_main.h"
#include "c_team.h"
#include "collisionutils.h"
// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "baseanimatedtextureproxy.h"
#include "animation.h"
#include "choreoscene.h"
#include "c_tdc_team.h"
#include "tdc_viewmodel.h"
#include "c_tdc_objective_resource.h"
#include "tdc_playermodelpanel.h"
#include "cam_thirdperson.h"
#include "tdc_hud_chat.h"
#include "iclientmode.h"
#include "steam/steam_api.h"
#include "tdc_wearable.h"

#if defined( CTDCPlayer )
#undef CTDCPlayer
#endif

#include "materialsystem/imesh.h"		//for materials->FindMaterial
#include "iviewrender.h"				//for view->

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_autorezoom( "cl_autorezoom", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, sniper rifle will re-zoom after firing a zoomed shot." );

ConVar tdc_muzzlelight("tdc_muzzlelight", "0", FCVAR_ARCHIVE, "Enable dynamic lights for muzzleflashes and the flamethrower");
ConVar tdc_dev_mark( "tdc_dev_mark", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar tdc_zoom_hold( "tdc_zoom_hold", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );

ConVar tdc_merc_color_r( "tdc_merc_color_r", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets merc color's red channel value", true, 0, true, 255 );
ConVar tdc_merc_color_g( "tdc_merc_color_g", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets merc color's green channel value", true, 0, true, 255 );
ConVar tdc_merc_color_b( "tdc_merc_color_b", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets merc color's blue channel value", true, 0, true, 255 );
ConVar tdc_merc_winanim( "tdc_merc_winanim", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets FFA win animation.", true, 1, false, 0 );

ConVar tdc_merc_skintone( "tdc_merc_skintone", "0", FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_HIDDEN, "Sets grunt's skin tone" );

void tdc_merc_color_callback( IConVar *var, const char *pOldValue, float flOldValue );
ConVar tdc_merc_color( "tdc_merc_color", "-1", FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_HIDDEN, "Sets grunt's outfit color", tdc_merc_color_callback );

#define BDAY_HAT_MODEL		"models/effects/bday_hat.mdl"
#define TDC_INVIS_MOVEMENT_SCALE  0.85f

IMaterial	*g_pHeadLabelMaterial[4] = { NULL, NULL }; 
void	SetupHeadLabelMaterials( void );

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
										   const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params );

const char *pszHeadLabelNames[] =
{
	"effects/speech_voice_red",
	"effects/speech_voice_blue",
	"effects/speech_voice_green",
	"effects/speech_voice_yellow"
};

#define TDC_PLAYER_HEAD_LABEL_RED 0
#define TDC_PLAYER_HEAD_LABEL_BLUE 1
#define TDC_PLAYER_HEAD_LABEL_GREEN 2
#define TDC_PLAYER_HEAD_LABEL_YELLOW 3


CLIENTEFFECT_REGISTER_BEGIN( PrecacheInvuln )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_blue.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_red.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_green.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_yellow.vmt" )
CLIENTEFFECT_REGISTER_END()

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		VPROF( "C_TEPlayerAnimEvent::PostDataUpdate" );

		// Create the effect.
		if ( m_iPlayerIndex == 0 )
			return;

		EHANDLE hPlayer = cl_entitylist->GetNetworkableHandle( m_iPlayerIndex );
		if ( !hPlayer )
			return;

		C_TDCPlayer *pPlayer = dynamic_cast< C_TDCPlayer* >( hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}	
	}

public:
	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

//-----------------------------------------------------------------------------
// Data tables and prediction tables.
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()


ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );
ConVar cl_ragdoll_fade_time( "cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_forcefade( "cl_ragdoll_forcefade", "0", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_pronecheck_distance( "cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL );

ConVar tdc_always_deathanim( "tdc_always_deathanim", "0", FCVAR_CHEAT, "Force death anims to always play." );

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_TDCRagdoll, DT_TDCRagdoll, CTDCRagdoll )
	RecvPropEHandle( RECVINFO( m_hOwnerEntity ) ),
	RecvPropVector( RECVINFO( m_vecRagdollOrigin ) ),
	RecvPropVector( RECVINFO( m_vecForce ) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) ),
	RecvPropInt( RECVINFO( m_nForceBone ) ),
	RecvPropInt( RECVINFO( m_nRagdollFlags ) ),
	RecvPropInt( RECVINFO( m_iDamageCustom ) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
	RecvPropInt( RECVINFO( m_iClass ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TDCRagdoll::C_TDCRagdoll()
{
	m_fDeathTime = -1;
	m_bFadingOut = false;
	m_nRagdollFlags = 0;
	m_iDamageCustom = TDC_DMG_CUSTOM_NONE;
	m_flBurnEffectStartTime = 0.0f;
	m_iTeam = -1;
	m_iClass = -1;
	m_nForceBone = -1;
	m_bPlayingDeathAnim = false;
	m_bFinishedDeathAnim = false;
	m_vecPlayerColor.Init();
	m_vecPlayerSkinTone.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TDCRagdoll::~C_TDCRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSourceEntity - 
//-----------------------------------------------------------------------------
void C_TDCRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for (int j = 0; j < pSrc->m_Entries.Count(); j++)
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if (!Q_strcmp(pSrcEntry->watcher->GetDebugName(), pszName))
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_TDCRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// While we're dying, we want to mimic the facial animation of the player. Once they respawn, we just stay as we are.
	C_TDCPlayer *pPlayer = GetOwningPlayer();

	if ( pPlayer && g_PR && !g_PR->IsAlive( pPlayer->entindex() ) )
	{
		pPlayer->SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
	else
	{
		BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTrace - 
//			iDamageType - 
//			*pCustomImpactName - 
//-----------------------------------------------------------------------------
void C_TDCRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "C_TDCRagdoll::ImpactTrace" );
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( !pPhysicsObject )
		return;

	Vector vecDir;
	VectorSubtract( pTrace->endpos, pTrace->startpos, vecDir );

	if ( iDamageType == DMG_BLAST )
	{
		// Adjust the impact strength and apply the force at the center of mass.
		vecDir *= 4000;
		pPhysicsObject->ApplyForceCenter( vecDir );
	}
	else
	{
		// Find the apporx. impact point.
		Vector vecHitPos;
		VectorMA( pTrace->startpos, pTrace->fraction, vecDir, vecHitPos );
		VectorNormalize( vecDir );

		// Adjust the impact strength and apply the force at the impact point..
		vecDir *= 4000;
		pPhysicsObject->ApplyForceOffset( vecDir, vecHitPos );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

// ---------------------------------------------------------------------------- -
// Purpose: 
// Input  : flInterval - 
// Output : float
//-----------------------------------------------------------------------------
float C_TDCRagdoll::FrameAdvance( float flInterval )
{
	float flRet = BaseClass::FrameAdvance( flInterval );

	// Turn into a ragdoll once animation is over.
	if ( !m_bFinishedDeathAnim && m_bPlayingDeathAnim && IsSequenceFinished() )
	{
		m_bFinishedDeathAnim = true;

		if ( cl_ragdoll_physics_enable.GetBool() )
		{
			// Make us a ragdoll.
			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.1f;

			GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, false );
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			EndFadeOut();
		}
	}

	return flRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void C_TDCRagdoll::CreateTFRagdoll(void)
{
	// Get the player.
	C_TDCPlayer *pPlayer = GetOwningPlayer();

	TDCPlayerClassData_t *pData = GetPlayerClassData( m_iClass );
	if ( pData )
	{
		int nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );
		SetModelIndex( nModelIndex );	
		m_nSkin = GetTeamSkin( m_iTeam );
	}

#ifdef _DEBUG
	DevMsg( 2, "CreateTFRagdoll %d %d\n", gpGlobals->framecount, pPlayer ? pPlayer->entindex() : 0 );
#endif
	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// Move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.		
		if ( !pPlayer->IsLocalPlayer() && pPlayer->IsInterpolationEnabled() )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( pPlayer->GetRenderOrigin() );
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( m_vecRagdollVelocity );

			// Hack! Find a neutral standing pose or use the idle.
			int iSeq = LookupSequence( "RagdollSpawn" );
			if ( iSeq == -1 )
			{
				Assert( false );
				iSeq = 0;
			}
			SetSequence( iSeq );
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}

		m_nBody = pPlayer->GetBody();
		pPlayer->MoveBoneAttachments( this );

		// Copies cosmetics to the ragdoll
		// Should probably check if the ragdoll model is different from player model
		for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		{
			C_TDCWearable *pWearable = pPlayer->m_hWearables[ i ];
			if ( pWearable )
				pWearable->CopyToRagdoll( this );
		}
	}
	else
	{
		// Overwrite network origin so later interpolation will use this position.
		SetNetworkOrigin( m_vecRagdollOrigin );
		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
	}

	// See if we should play a custom death animation.
	if ( pPlayer && !pPlayer->IsDormant() &&
		( m_nRagdollFlags & TDC_RAGDOLL_ONGROUND ) &&
		( tdc_always_deathanim.GetBool() || RandomInt( 0, 3 ) == 0 ) )
	{
		int iSeq = pPlayer->m_Shared.GetSequenceForDeath( this, m_iDamageCustom );
		if ( iSeq != -1 )
		{
			// Doing this here since the server doesn't send the value over.
			ForceClientSideAnimationOn();

			// Slam velocity when doing death animation.
			SetAbsOrigin( pPlayer->GetRenderOrigin() );
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( vec3_origin );
			m_vecForce = vec3_origin;

			SetSequence( iSeq );
			ResetSequenceInfo();
			m_bPlayingDeathAnim = true;
			
			CreateShadow();
		}
	}

	// Turn it into a ragdoll.
	if ( !m_bPlayingDeathAnim )
	{
		if ( cl_ragdoll_physics_enable.GetBool() )
		{
			// Make us a ragdoll..
			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.05f;

			// We have to make sure that we're initting this client ragdoll off of the same model.
			// GetRagdollInitBoneArrays uses the *player* Hdr, which may be a different model than
			// the ragdoll Hdr, if we try to create a ragdoll in the same frame that the player
			// changes their player model.
			CStudioHdr *pRagdollHdr = GetModelPtr();
			CStudioHdr *pPlayerHdr = NULL;
			if ( pPlayer )
				pPlayerHdr = pPlayer->GetModelPtr();

			bool bChangedModel = false;

			if ( pRagdollHdr && pPlayerHdr )
			{
				bChangedModel = pRagdollHdr->GetVirtualModel() != pPlayerHdr->GetVirtualModel();

				Assert( !bChangedModel && "C_TDCRagdoll::CreateTFRagdoll: Trying to create ragdoll with a different model than the player it's based on" );
			}

			if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel )
			{
				pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			}
			else
			{
				GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			}

			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			// Remove it immediately.
			EndFadeOut();
		}
	}

	if ( m_nRagdollFlags & TDC_RAGDOLL_BURNING )
	{
		m_flBurnEffectStartTime = gpGlobals->curtime;
		ParticleProp()->Create( "burningplayer_corpse", PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( pPlayer )
	{
		m_vecPlayerColor = pPlayer->m_vecPlayerColor;
		m_vecPlayerSkinTone = pPlayer->m_vecPlayerSkinTone;
	}

	// Fade out the ragdoll in a while
	StartFadeOut( cl_ragdoll_fade_time.GetFloat() );
	SetNextClientThink( gpGlobals->curtime + cl_ragdoll_fade_time.GetFloat() * 0.33f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCRagdoll::CreateTFGibs( void )
{
	SetAbsOrigin( m_vecRagdollOrigin );

	C_TDCPlayer *pPlayer = GetOwningPlayer();

	if ( pPlayer && ( pPlayer->m_hFirstGib == NULL ) )
	{
		Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
		VectorNormalize( vecVelocity );
		pPlayer->CreatePlayerGibs( m_vecRagdollOrigin, vecVelocity, m_vecForce.Length(), m_nRagdollFlags & TDC_RAGDOLL_BURNING );
	}

	EndFadeOut();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void C_TDCRagdoll::UpdateOnRemove( void )
{
	DestroyBoneAttachments();
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_TDCRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		bool bCreateRagdoll = true;

		// Get the player.
		C_TDCPlayer *pPlayer = GetOwningPlayer();

		if ( pPlayer )
		{
			// If we're getting the initial update for this player (e.g., after resetting entities after
			//  lots of packet loss, then don't create gibs, ragdolls if the player and it's gib/ragdoll
			//  both show up on same frame.
			if ( abs( pPlayer->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}
		else if ( C_BasePlayer::GetLocalPlayer() )
		{
			// Ditto for recreation of the local player
			if ( abs( C_BasePlayer::GetLocalPlayer()->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}

		if ( bCreateRagdoll )
		{
			if ( m_nRagdollFlags & TDC_RAGDOLL_GIB )
			{
				CreateTFGibs();
			}
			else
			{
				CreateTFRagdoll();
			}
		}
	}
	else 
	{
		if ( !cl_ragdoll_physics_enable.GetBool() )
		{
			// Don't let it set us back to a ragdoll with data from the server.
			m_nRenderFX = kRenderFxNone;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll* C_TDCRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TDCRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector(-1,-1,-1);	//WorldAlignMins();
	Vector vMaxs = Vector(1,1,1);	//WorldAlignMaxs();
		
	Vector origin = GetAbsOrigin();
	
	if( !engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) )
	{
		return false;
	}
	else if( engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		return false;
	}

	return true;
}

void C_TDCRagdoll::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	if ( m_bFadingOut == true )
	{
		int iAlpha = GetRenderColor().a;
		int iFadeSpeed = 600.0f;

		iAlpha = Max( iAlpha - (int)( iFadeSpeed * gpGlobals->frametime ), 0 );

		SetRenderMode( kRenderTransAlpha );
		SetRenderColorA( iAlpha );

		if ( iAlpha == 0 )
		{
			EndFadeOut(); // remove clientside ragdoll
		}

		return;
	}

	// if the player is looking at us, delay the fade
	if ( IsRagdollVisible() )
	{
		if ( cl_ragdoll_forcefade.GetBool() )
		{
			m_bFadingOut = true;
			float flDelay = cl_ragdoll_fade_time.GetFloat() * 0.33f;
			m_fDeathTime = gpGlobals->curtime + flDelay;

			// If we were just fully healed, remove all decals
			RemoveAllDecals();
		}

		StartFadeOut( cl_ragdoll_fade_time.GetFloat() * 0.33f );
		return;
	}

	if ( m_fDeathTime > gpGlobals->curtime )
		return;

	EndFadeOut(); // remove clientside ragdoll
}

void C_TDCRagdoll::StartFadeOut( float fDelay )
{
	if ( !cl_ragdoll_forcefade.GetBool() )
	{
		m_fDeathTime = gpGlobals->curtime + fDelay;
	}
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_TDCRagdoll::EndFadeOut()
{
	SetNextClientThink( CLIENT_THINK_NEVER );
	ClearRagdoll();
	SetRenderMode( kRenderNone );
	DestroyBoneAttachments();
	UpdateVisibility();
}

C_TDCPlayer *C_TDCRagdoll::GetOwningPlayer()
{
	return ToTDCPlayer( GetOwnerEntity() );
}

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyInvulnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 1.0f );
			return;
		}

		C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTDCPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.InCond( TDC_COND_INVULNERABLE_SPAWN_PROTECT ) &&
				pPlayer->m_Shared.GetConditionDuration( TDC_COND_INVULNERABLE_SPAWN_PROTECT ) > 1.0f )
			{
				m_pResult->SetFloatValue( 1.0f );
			}
			else
			{
				m_pResult->SetFloatValue( 0.0f );
			}
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyInvulnLevel, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for burning material on player models
//			Returns 0.0->1.0 for level of burn to show on player skin
//-----------------------------------------------------------------------------
class CProxyBurnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 0.0f );
			return;
		}

		// default to zero
		float flBurnStartTime = 0;
			
		C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( !pPlayer )
		{
			// See if it's a wearable.
			pPlayer = ToTDCPlayer( pEntity->GetMoveParent() );
			if ( !pPlayer )
			{
				// Must be attached to ragdoll.
				pEntity = pEntity->GetMoveParent();
			}
		}

		if ( pPlayer )
		{
			// is the player burning?
			if ( pPlayer->m_Shared.InCond( TDC_COND_BURNING ) )
			{
				flBurnStartTime = pPlayer->m_flBurnEffectStartTime;
			}
		}
		else
		{
			// is the ragdoll burning?
			C_TDCRagdoll *pRagDoll = dynamic_cast< C_TDCRagdoll* >( pEntity );
			if ( pRagDoll )
			{
				flBurnStartTime = pRagDoll->GetBurnStartTime();
			}
		}

		float flResult = 0.0;
		
		// if player/ragdoll is burning, set the burn level on the skin
		if ( flBurnStartTime > 0 )
		{
			float flBurnPeakTime = flBurnStartTime + 0.3;
			float flTempResult;
			if ( gpGlobals->curtime < flBurnPeakTime )
			{
				// fade in from 0->1 in 0.3 seconds
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnStartTime, flBurnPeakTime, 0.0, 1.0 );
			}
			else
			{
				// fade out from 1->0 in the remaining time until flame extinguished
				float flFadeStart = pPlayer ? Max( flBurnPeakTime, pPlayer->m_flBurnRenewTime ) : flBurnPeakTime;
				float flFadeEnd = pPlayer ? pPlayer->m_Shared.GetFlameRemoveTime() : flBurnStartTime + 10.0f;

				flTempResult = RemapValClamped( gpGlobals->curtime, flFadeStart, flFadeEnd, 1.0, 0.0 );
			}	

			// We have to do some more calc here instead of in materialvars.
			flResult = 1.0 - abs( flTempResult - 1.0 );
		}

		m_pResult->SetFloatValue( flResult );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyBurnLevel, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for Berserk Glow
//-----------------------------------------------------------------------------
class CProxyBerserkLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 1.0f );
			return;
		}

		C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTDCPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		if ( pPlayer && pPlayer->m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
		{
			m_pResult->SetFloatValue( 0.0f );
			return;
		}

		m_pResult->SetFloatValue( 1.0f );
	}
};

EXPOSE_INTERFACE( CProxyBerserkLevel, IMaterialProxy, "BerserkLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for the weapon glow color when critted
//-----------------------------------------------------------------------------
class CProxyModelGlowColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		Vector vecColor( 1, 1, 1 );

		C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity );

		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTDCPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}
		/*
		Live TF2 crit glow colors
		RED Crit: 94 8 5
		BLU Crit: 6 21 80
		RED Mini-Crit: 237 140 55
		BLU Mini-Crit: 28 168 112
		Hype Mode: 50 2 50
		*/

		if ( pPlayer && pPlayer->m_Shared.IsCritBoosted() )
		{
			if ( TDCGameRules()->IsTeamplay() )
			{
				switch ( pPlayer->GetTeamNumber() )
				{
				case TDC_TEAM_RED:
					vecColor.Init( 94, 8, 5 );
					break;
				case TDC_TEAM_BLUE:
					vecColor.Init( 6, 21, 80 );
					break;
				}
			}
			else
			{
				vecColor = pPlayer->m_vecPlayerColor * 75;
			}
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};

EXPOSE_INTERFACE( CProxyModelGlowColor, IMaterialProxy, "ModelGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Universal proxy from live tf2 used for spy invisiblity material
//			Its' purpose is to replace weapon_invis, vm_invis and spy_invis
//-----------------------------------------------------------------------------
class CInvisProxy : public IMaterialProxy
{
public:
	CInvisProxy( void );
	virtual				~CInvisProxy( void );
	virtual void		Release( void ) { delete this; }
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( void *pC_BaseEntity );
	virtual IMaterial *	GetMaterial();

	virtual void		HandlePlayerInvis( C_TDCPlayer *pPlayer );
	virtual void		HandleVMInvis( C_BaseEntity *pVM );
	virtual void		HandleWeaponInvis( C_BaseEntity *pC_BaseEntity );

private:

	IMaterialVar		*m_pPercentInvisible;
	IMaterialVar		*m_pCloakColorTint;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInvisProxy::CInvisProxy(void)
{
	m_pPercentInvisible = NULL;
	m_pCloakColorTint = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInvisProxy::~CInvisProxy(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	// if we have $cloakColorTint, it's spy_invis
	if ( bTint )
	{
		return ( bInvis && bTint );
	}

	return ( bTint );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::OnBind( void *pC_BaseEntity )
{
	IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
	C_BaseEntity *pEnt = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
	if ( !pEnt )
	{
		if ( m_pPercentInvisible )
		{
			m_pPercentInvisible->SetFloatValue( 0.0f );
		}
		return;
	}

	C_TDCPlayer *pPlayer = ToTDCPlayer( pEnt );
	if ( pPlayer )
	{
		HandlePlayerInvis( pPlayer );
		return;
	}

	C_BaseAnimating *pAnim = pEnt->GetBaseAnimating();
	if ( !pAnim )
		return;

	if ( pAnim->IsViewModel() )
	{
		HandleVMInvis( pAnim );
		return;
	}

	HandleWeaponInvis( pAnim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandlePlayerInvis( C_TDCPlayer *pPlayer )
{
	if ( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleVMInvis( C_BaseEntity *pVM )
{
	if ( !m_pPercentInvisible )
		return;

	IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pVM );
	if ( !pOwnerInterface )
		return;

	C_TDCPlayer *pPlayer = ToTDCPlayer( pOwnerInterface->GetOwnerViaInterface() );

	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	float flPercentInvisible = pPlayer->GetPercentInvisible();

	// remap from 0.22 to 0.5
	// but drop to 0.0 if we're not invis at all
	float flWeaponInvis = ( flPercentInvisible < 0.01 ) ?
		0.0 :
		RemapVal( flPercentInvisible, 0.0, 1.0, 0.22f, 0.5f );

	m_pPercentInvisible->SetFloatValue( flWeaponInvis );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleWeaponInvis( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	C_TDCPlayer *pPlayer = NULL;

	IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEnt );
	if ( pOwnerInterface )
	{
		pPlayer = ToTDCPlayer( pOwnerInterface->GetOwnerViaInterface() );
	}
	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
}

IMaterial *CInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

EXPOSE_INTERFACE( CInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Gets player's color in FFA.
//-----------------------------------------------------------------------------
class CPlayerTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

		// If not tied to an entity assume we're used on a model panel.
		if ( !pEntity )
		{
			if ( pC_BaseEntity )
			{
				CTDCPlayerModelPanel *pPanel = dynamic_cast<CTDCPlayerModelPanel *>( (IClientRenderable *)pC_BaseEntity );

				if ( pPanel )
				{
					m_pResult->SetVecValue( pPanel->GetModelTintColor().Base(), 3 );
					return;
				}
			}

			m_pResult->SetVecValue( 0, 0, 0 );
			return;
		}

		if ( !TDCGameRules()->IsTeamplay() )
		{		
			C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity ); 
			if ( pPlayer )
			{
				m_pResult->SetVecValue( pPlayer->m_vecPlayerColor.Base(), 3 );
				return;
			}
			else
			{
				C_TDCRagdoll *pRagdoll = dynamic_cast<C_TDCRagdoll *>( pEntity );
				if ( pRagdoll )
				{
					m_pResult->SetVecValue( pRagdoll->m_vecPlayerColor.Base(), 3 );
					return;
				}
				else if ( g_TDC_PR )
				{
					// Assume that's an entity owned by a player.
					pPlayer = ToTDCPlayer( pEntity->GetOwnerEntity() );
					if ( pPlayer )
					{
						m_pResult->SetVecValue( g_TDC_PR->GetPlayerColorVector( pPlayer->entindex() ).Base(), 3 );
						return;
					}
				}
			}
		}

		m_pResult->SetVecValue( 0, 0, 0 );
	}
};

EXPOSE_INTERFACE( CPlayerTintColor, IMaterialProxy, "PlayerTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Gets player's color in FFA.
//-----------------------------------------------------------------------------
class CPlayerSkinColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

		// If not tied to an entity assume we're used on a model panel.
		if ( !pEntity )
		{
			if ( pC_BaseEntity )
			{
				CTDCPlayerModelPanel *pPanel = dynamic_cast<CTDCPlayerModelPanel *>( (IClientRenderable *)pC_BaseEntity );

				if ( pPanel )
				{
					m_pResult->SetVecValue( pPanel->GetModelSkinToneColor().Base(), 3 );
					return;
				}
			}

			m_pResult->SetVecValue( 0, 0, 0 );
			return;
		}

		C_TDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( pPlayer )
		{
			m_pResult->SetVecValue( pPlayer->m_vecPlayerSkinTone.Base(), 3 );
			return;
		}
		else
		{
			C_TDCRagdoll *pRagdoll = dynamic_cast<C_TDCRagdoll *>( pEntity );
			if ( pRagdoll )
			{
				m_pResult->SetVecValue( pRagdoll->m_vecPlayerSkinTone.Base(), 3 );
				return;
			}
			else if ( g_TDC_PR )
			{
				// Assume that's an entity owned by a player.
				pPlayer = ToTDCPlayer( pEntity->GetOwnerEntity() );
				if ( pPlayer )
				{
					m_pResult->SetVecValue( g_TDC_PR->GetPlayerSkinTone( pPlayer->entindex() ).Base(), 3 );
					return;
				}
			}
		}

		m_pResult->SetVecValue( 0, 0, 0 );
	}
};

EXPOSE_INTERFACE(CPlayerSkinColor, IMaterialProxy, "PlayerSkinColor" IMATERIAL_PROXY_INTERFACE_VERSION );

// specific to the local player
BEGIN_RECV_TABLE_NOBASE( C_TDCPlayer, DT_TDCLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO( m_angEyeAngles ) ),

	RecvPropBool( RECVINFO( m_bArenaSpectator ) ),
	RecvPropInt( RECVINFO( m_nMoneyPacks ) ),
	RecvPropBool( RECVINFO( m_bWasHoldingJump ) ),
	RecvPropBool( RECVINFO( m_bIsPlayerADev ) ),

END_RECV_TABLE()

// all players except the local player
BEGIN_RECV_TABLE_NOBASE( C_TDCPlayer, DT_TDCNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

	RecvPropInt( RECVINFO( m_nActiveWpnClip ) ),
	RecvPropInt( RECVINFO( m_nActiveWpnAmmo ) ),

	RecvPropBool( RECVINFO( m_bTyping ) ),

END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_TDCPlayer, DT_TDCPlayer, CTDCPlayer )

	RecvPropBool( RECVINFO( m_bIsABot ) ),

	// This will create a race condition will the local player, but the data will be the same so.....
	RecvPropInt( RECVINFO( m_nWaterLevel ) ),
	RecvPropFloat( RECVINFO( m_flSprintPower ) ),
	RecvPropFloat( RECVINFO( m_flSprintPowerLastCheckTime ) ),
	RecvPropFloat( RECVINFO( m_flSprintRegenStartTime ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropDataTable( RECVINFO_DT( m_PlayerClass ), 0, &REFERENCE_RECV_TABLE( DT_TDCPlayerClassShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE( DT_TDCPlayerShared ) ),

	RecvPropEHandle( RECVINFO( m_hItem ) ),

	RecvPropVector( RECVINFO( m_vecPlayerColor ) ),
	RecvPropVector( RECVINFO( m_vecPlayerSkinTone ) ),

	RecvPropDataTable( "tflocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TDCLocalPlayerExclusive ) ),
	RecvPropDataTable( "tfnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TDCNonLocalPlayerExclusive ) ),

	RecvPropInt( RECVINFO( m_iSpawnCounter ) ),
	RecvPropTime( RECVINFO( m_flLastDamageTime ) ),
	RecvPropTime( RECVINFO( m_flHeadshotFadeTime ) ),
	RecvPropBool( RECVINFO( m_bFlipViewModel ) ),
	RecvPropFloat( RECVINFO( m_flViewModelFOV ) ),
	RecvPropVector( RECVINFO( m_vecViewModelOffset ) ),
	RecvPropArray3
	(
		RECVINFO_ARRAY( m_hWearables ),
		RecvPropEHandle ( RECVINFO( m_hWearables[0] ) )
	),

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_TDCPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_Shared, CTDCPlayerShared ),
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE  ),
	DEFINE_PRED_FIELD( m_hOffHandWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_angEyeAngles, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bWasHoldingJump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// ------------------------------------------------------------------------------------------ //
// C_TDCPlayer implementation.
// ------------------------------------------------------------------------------------------ //

C_TDCPlayer::C_TDCPlayer() : 
	m_iv_angEyeAngles( "C_TDCPlayer::m_iv_angEyeAngles" )
{
	m_PlayerAnimState = CreateTFPlayerAnimState( this );
	m_Shared.Init( this );

	m_iIDEntIndex = 0;

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_pTeleporterEffect = NULL;
	m_flBurnEffectStartTime = 0;
	m_flBurnRenewTime = 0;
	m_pTypingEffect = NULL;
	m_pVoiceEffect = NULL;
	m_pOverhealEffect = NULL;
	
	m_aGibs.Purge();

	m_hRagdoll.Set( NULL );

	m_pNemesisEffect = NULL;

	m_bWasTaunting = false;
	m_flTauntOffTime = 0.0f;

	m_flWaterEntryTime = 0;
	m_nOldWaterLevel = WL_NotInWater;
	m_bWaterExitEffectActive = false;

	m_iOldObserverMode = OBS_MODE_NONE;

	m_bUpdateAttachedModels = false;

	m_bTyping = false;

	ListenForGameEvent( "localplayer_changeteam" );

	// Load phonemes for MP3s.
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes.txt" );
	engine->AddPhonemeFile( nullptr ); // Optimization by Valve; nullptr tells the engine that we have loaded all phomeme files we want.
}

C_TDCPlayer::~C_TDCPlayer()
{
	ShowNemesisIcon( false );
	m_PlayerAnimState->Release();
}


C_TDCPlayer* C_TDCPlayer::GetLocalTDCPlayer()
{
	return ToTDCPlayer( C_BasePlayer::GetLocalPlayer() );
}

const QAngle& C_TDCPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateOnRemove( void )
{
	// Stop the taunt.
	if ( m_bWasTaunting )
	{
		TurnOffTauntCam();
	}

	// HACK!!! ChrisG needs to fix this in the particle system.
	ParticleProp()->OwnerSetDormantTo( true );
	ParticleProp()->StopParticlesInvolving( this );

	// Remove all conditions, this should also kill all effects and looping sounds.
	m_Shared.RemoveAllCond();

	if ( IsLocalPlayer() )
	{
		CTDCStatPanel *pStatPanel = GetStatPanel();
		pStatPanel->OnLocalPlayerRemove( this );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: returns max health for this player
//-----------------------------------------------------------------------------
int C_TDCPlayer::GetMaxHealth( void ) const
{	
	return g_TDC_PR ? g_TDC_PR->GetMaxHealth( entindex() ) : 1;
}

//-----------------------------------------------------------------------------
// Deal with recording
//-----------------------------------------------------------------------------
void C_TDCPlayer::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );
	BaseEntityRecordingState_t *pBaseEntityState = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );

	bool bDormant = IsDormant();
	bool bDead = !IsAlive();
	bool bSpectator = ( GetTeamNumber() == TEAM_SPECTATOR );
	bool bNoRender = ( GetRenderMode() == kRenderNone );
	bool bDeathCam = (GetObserverMode() == OBS_MODE_DEATHCAM);
	bool bNoDraw = IsEffectActive(EF_NODRAW);

	bool bVisible = 
		!bDormant && 
		!bDead && 
		!bSpectator &&
		!bNoRender &&
		!bDeathCam &&
		!bNoDraw;

	bool changed = m_bToolRecordingVisibility != bVisible;
	// Remember state
	m_bToolRecordingVisibility = bVisible;

	pBaseEntityState->m_bVisible = bVisible;
	if ( changed && !bVisible )
	{
		// If the entity becomes invisible this frame, we still want to record a final animation sample so that we have data to interpolate
		//  toward just before the logs return "false" for visiblity.  Otherwise the animation will freeze on the last frame while the model
		//  is still able to render for just a bit.
		pBaseEntityState->m_bRecordFinalVisibleSample = true;
	}
#endif
}


void C_TDCPlayer::UpdateClientSideAnimation()
{
	// Update the animation data.
	m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::SetDormant( bool bDormant )
{
	if ( !IsDormant() && bDormant )
	{
		ShowNemesisIcon( false );
	}

	if ( IsDormant() && !bDormant )
	{
		// Update client-side models attached to us.
		m_bUpdateAttachedModels = true;
	}

	m_Shared.UpdateLoopingSounds( bDormant );

	// Deliberately skip base player.
	C_BaseEntity::SetDormant( bDormant );

	// Kill speech bubbles.
	UpdateSpeechBubbles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldHealth = m_iHealth;
	m_iOldPlayerClass = m_PlayerClass.GetClassIndex();
	m_iOldSpawnCounter = m_iSpawnCounter;
	m_nOldWaterLevel = GetWaterLevel();

	m_iOldTeam = GetTeamNumber();
	m_hOldActiveWeapon.Set( GetActiveTFWeapon() );

	m_hOldObserverTarget = GetObserverTarget();
	m_iOldObserverMode = GetObserverMode();

	m_Shared.OnPreDataChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		InitInvulnerableMaterial();
	}
	else
	{
		if ( m_iOldTeam != GetTeamNumber() )
		{
			InitInvulnerableMaterial();
		}
	}

	if ( GetActiveTFWeapon() != m_hOldActiveWeapon.Get() )
	{
		if ( ShouldDrawThisPlayer() )
		{
			m_Shared.UpdateCritBoostEffect();
		}

		if ( !GetPredictable() )
		{
			m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
		}
	}

	// Check for full health and remove decals.
	if ( ( m_iHealth > m_iOldHealth && m_iHealth >= GetMaxHealth() ) || m_Shared.IsInvulnerable() )
	{
		// If we were just fully healed, remove all decals
		RemoveAllDecals();
	}

	// Detect class changes
	if ( m_iOldPlayerClass != m_PlayerClass.GetClassIndex() )
	{
		OnPlayerClassChange();
	}

	bool bJustSpawned = false;

	if ( m_iOldSpawnCounter != m_iSpawnCounter )
	{
		ClientPlayerRespawn();
		bJustSpawned = true;
	}

	UpdateSpeechBubbles();

	// See if we should show or hide nemesis icon for this player
	ShowNemesisIcon( ShouldShowNemesisIcon() );

	m_Shared.OnDataChanged();

	int nNewWaterLevel = GetWaterLevel();

	if ( nNewWaterLevel != m_nOldWaterLevel )
	{
		if ( ( m_nOldWaterLevel == WL_NotInWater ) && ( nNewWaterLevel > WL_NotInWater ) )
		{
			// Set when we do a transition to/from partially in water to completely out
			m_flWaterEntryTime = gpGlobals->curtime;
		}

		// If player is now up to his eyes in water and has entered the water very recently (not just bobbing eyes in and out), play a bubble effect.
		if ( ( nNewWaterLevel == WL_Eyes ) && ( gpGlobals->curtime - m_flWaterEntryTime ) < 0.5f ) 
		{
			CNewParticleEffect *pEffect = ParticleProp()->Create( "water_playerdive", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pEffect, 1, NULL, PATTACH_WORLDORIGIN, NULL, WorldSpaceCenter() );
		}		
		// If player was up to his eyes in water and is now out to waist level or less, play a water drip effect
		else if ( m_nOldWaterLevel == WL_Eyes && ( nNewWaterLevel < WL_Eyes ) && !bJustSpawned )
		{
			CNewParticleEffect *pWaterExitEffect = ParticleProp()->Create( "water_playeremerge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pWaterExitEffect, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			m_bWaterExitEffectActive = true;
		}
	}

	UpdateClientSideGlow();

	if ( m_iOldHealth != m_iHealth && HasTheFlag() && GetGlowObject() )
	{
		// Update the glow color on flag carrier according to their health.
		Vector vecColor;
		GetGlowEffectColor( &vecColor.x, &vecColor.y, &vecColor.z );

		GetGlowObject()->SetColor( vecColor );
	}

	if ( IsLocalPlayer() )
	{
		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetupHeadLabelMaterials();
			GetClientVoiceMgr()->SetHeadLabelOffset( 50 );
		}

		if ( m_iOldTeam != GetTeamNumber() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeteam" );
			if ( event )
			{
				gameeventmanager->FireEventClientSide( event );
			}
		}

		if ( !IsPlayerClass( m_iOldPlayerClass ) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeclass" );
			if ( event )
			{
				event->SetInt( "updateType", updateType );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		if ( GetObserverTarget() != m_hOldObserverTarget.Get() )
		{
			// Update effects on players when chaging targets.
			C_TDCPlayer *pOldPlayer = ToTDCPlayer( m_hOldObserverTarget.Get() );

			if ( pOldPlayer )
			{
				pOldPlayer->ThirdPersonSwitch( false );
			}

			C_TDCPlayer *pNewPlayer = ToTDCPlayer( GetObserverTarget() );

			if ( pNewPlayer )
			{
				pNewPlayer->ThirdPersonSwitch( GetObserverMode() == OBS_MODE_IN_EYE );
			}
		}
		else if ( GetObserverMode() != m_iOldObserverMode &&
			( GetObserverMode() == OBS_MODE_IN_EYE || m_iOldObserverMode == OBS_MODE_IN_EYE ) )
		{
			// Update effects on the spectated player when switching between first and third person view.
			C_TDCPlayer *pPlayer = ToTDCPlayer( GetObserverTarget() );
			
			if ( pPlayer )
			{
				pPlayer->ThirdPersonSwitch( GetObserverMode() != OBS_MODE_IN_EYE );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::InitInvulnerableMaterial( void )
{
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalPlayer )
		return;

	const char *pszMaterial = NULL;

	if ( TDCGameRules()->IsTeamplay() )
	{
		switch ( GetTeamNumber() )
		{
		case TDC_TEAM_RED:
			pszMaterial = "models/effects/invulnfx_red.vmt";
			break;
		case TDC_TEAM_BLUE:
			pszMaterial = "models/effects/invulnfx_blue.vmt";
			break;
		default:
			break;
		}	
	}
	else
	{
		pszMaterial = "models/effects/invulnfx_custom.vmt";
	}

	if ( pszMaterial )
	{
		m_InvulnerableMaterial.Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
	else
	{
		m_InvulnerableMaterial.Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateRecentlyTeleportedEffect( void )
{
	if ( m_Shared.ShouldShowRecentlyTeleported() )
	{
		if ( !m_pTeleporterEffect )
		{
			const char *pszEffect = ConstructTeamParticle( "player_recent_teleport_%s", GetTeamNumber(), true );
			m_pTeleporterEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
			m_Shared.SetParticleToMercColor( m_pTeleporterEffect );
		}
	}
	else
	{
		if ( m_pTeleporterEffect )
		{
			ParticleProp()->StopEmission( m_pTeleporterEffect );
			m_pTeleporterEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::OnPlayerClassChange( void )
{
	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	// Execute the class cfg
	if ( IsLocalPlayer() )
	{
		char szCommand[128];
		V_sprintf_safe( szCommand, "exec %s.cfg\n", GetPlayerClass()->GetName() );
		engine->ExecuteClientCmd( szCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::InitPhonemeMappings()
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		V_sprintf_safe( szExpressionName, "%s/phonemes/phonemes", szBasename );
		if ( FindSceneFile( szExpressionName ) )
		{
			SetupMappings( szExpressionName );	
		}
		else
		{
			BaseClass::InitPhonemeMappings();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::ResetFlexWeights( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr || pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	LocalFlexController_t iController;
	for ( iController = LocalFlexController_t(0); iController < pStudioHdr->numflexcontrollers(); ++iController )
	{
		SetFlexWeight( iController, 0.0f );
	}

	// Reset the prediction interpolation values.
	m_iv_flexWeight.Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_TDCPlayer::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Initialize the gibs.
	InitPlayerGibs();

	InitializePoseParams();

	// Init flexes, cancel any scenes we're playing
	ClearSceneEvents( NULL, false );

	// Reset the flex weights.
	ResetFlexWeights( hdr );

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}

	if ( hdr )
	{
		InitPhonemeMappings();
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: For checking how Spy's cloak and disguise effects should apply.
//-----------------------------------------------------------------------------
bool C_TDCPlayer::IsEnemyPlayer( void )
{
	int iTeam = GetTeamNumber();
	int iLocalTeam = GetLocalPlayerTeam();

	// Spectators are nobody's enemy.
	if ( iLocalTeam < FIRST_GAME_TEAM )
		return false;

	// In FFA everybody is an enemy. Except for ourselves.
	if ( !TDCGameRules()->IsTeamplay() )
		return !IsLocalPlayer();

	// Players from other teams are enemies.
	return ( iTeam != iLocalTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Displays a nemesis icon on this player to the local player
//-----------------------------------------------------------------------------
void C_TDCPlayer::ShowNemesisIcon( bool bShow )
{
	if ( bShow )
	{
		if ( !m_pNemesisEffect )
		{
			const char *pszEffect = ConstructTeamParticle( "particle_nemesis_%s", GetTeamNumber(), true );
			m_pNemesisEffect = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "head" );
			m_Shared.SetParticleToMercColor( m_pNemesisEffect );
		}
	}
	else
	{
		if ( m_pNemesisEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pNemesisEffect );
			m_pNemesisEffect = NULL;
		}
	}
}

extern ConVar tdc_tauntcam_dist;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::TurnOnTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	// Already in third person?
	if ( InThirdPersonShoulder() )
		return;

	m_flTauntOffTime = 0.0f;

	// If we're already in taunt cam just reset the distance.
	if ( m_bWasTaunting )
	{
		g_ThirdPersonManager.SetDesiredCameraOffset( Vector( tdc_tauntcam_dist.GetFloat(), 0.0f, 0.0f ) );
		return;
	}

	m_bWasTaunting = true;

	::input->CAM_ToThirdPerson();
	ThirdPersonSwitch( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::TurnOffTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	m_bWasTaunting = false;
	m_flTauntOffTime = 0.0f;

	::input->CAM_ToFirstPerson();

	// Force the feet to line up with the view direction post taunt.
	m_PlayerAnimState->m_bForceAimYaw = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::HandleTaunting( void )
{
	// Clear the taunt slot.
	if ( ( !m_bWasTaunting || m_flTauntOffTime != 0.0f ) &&
		( m_Shared.InCond( TDC_COND_TAUNTING ) ||
		m_Shared.IsLoser() ||
		m_Shared.InCond( TDC_COND_STUNNED ) ) )
	{
		// Handle the camera for the local player.
		TurnOnTauntCam();
	}

	if ( m_bWasTaunting && m_flTauntOffTime == 0.0f && (
		!m_Shared.InCond( TDC_COND_TAUNTING ) &&
		!m_Shared.IsLoser() &&
		!m_Shared.InCond( TDC_COND_STUNNED ) ) )
	{
		m_flTauntOffTime = gpGlobals->curtime;

		// Clear the vcd slot.
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	}

	TauntCamInterpolation();
}

//---------------------------------------------------------------------------- -
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::TauntCamInterpolation( void )
{
	if ( m_flTauntOffTime != 0.0f )
	{
		// Pull the camera back in over the course of half a second.
		float flDist = RemapValClamped( gpGlobals->curtime - m_flTauntOffTime, 0.0f, 0.5f, tdc_tauntcam_dist.GetFloat(), 0.0f );

		if ( flDist == 0.0f || !m_bWasTaunting || !IsAlive() )
		{
			// Snap the camera back into first person.
			TurnOffTauntCam();
		}
		else
		{
			g_ThirdPersonManager.SetDesiredCameraOffset( Vector( flDist, 0.0f, 0.0f ) );
		}
	}
}

extern ConVar tdc_thirdperson;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TDCPlayer::InThirdPersonShoulder( void )
{
	if ( !TDCGameRules()->AllowThirdPersonCamera() )
		return false;

	if ( IsObserver() )
		return false;

	if ( m_Shared.InCond( TDC_COND_ZOOMED ) )
		return false;

	if ( InTauntCam() )
		return false;

	return tdc_thirdperson.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	// Update any effects affected by camera mode.
	m_Shared.UpdateCritBoostEffect();

	// Notify the weapon so it can update effects, etc.
	C_TDCWeaponBase *pWeapon = GetActiveTFWeapon();

	if ( pWeapon )
	{
		pWeapon->ThirdPersonSwitch( bThirdPerson );
	}

	// Update visibility on everything attached to us.
	for ( C_BaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		pChild->UpdateVisibility();
		pChild->CreateShadow();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::PreThink( void )
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;
	if ( buttonsChanged & ( IN_SPEED | IN_DUCK ) || m_Shared.InCond( TDC_COND_SPRINT ) )
	{
		TeamFortress_SetSpeed();
	}

	BaseClass::PreThink();

	m_Shared.InvisibilityThink();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::PostThink( void )
{
	BaseClass::PostThink();

	m_angEyeAngles = EyeAngles();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::PhysicsSimulate( void )
{
#if !defined( NO_ENTITY_PREDICTION )
	VPROF( "C_TDCPlayer::PhysicsSimulate" );
	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( pMoveParent )
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if ( m_nSimulationTick == gpGlobals->tickcount )
		return;

	m_nSimulationTick = gpGlobals->tickcount;

	if ( !IsLocalPlayer() )
		return;

	C_CommandContext *ctx = GetCommandContext();
	Assert( ctx );
	Assert( ctx->needsprocessing );
	if ( !ctx->needsprocessing )
		return;

	ctx->needsprocessing = false;
	CUserCmd *ucmd = &ctx->cmd;

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
		//VectorCopy ( pl.v_angle, ucmd->viewangles );
	}
	else if ( m_Shared.IsMovementLocked() )
	{
		// Don't allow player to perform any actions while taunting or stunned.
		// Not preventing movement since some taunts have special movement which needs to be handled in CTDCGameMovement.
		// This is duplicated on server side in CTDCPlayer::RunPlayerCommand.
		ucmd->buttons = 0;
		ucmd->weaponselect = 0;
		ucmd->weaponsubtype = 0;

		// Don't allow the player to turn around.
		VectorCopy( EyeAngles(), ucmd->viewangles );
	}

	// Run the next command
	prediction->RunCommand( this, ucmd, MoveHelper() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::ClientThink()
{
	// Pass on through to the base class.
	BaseClass::ClientThink();

	UpdateIDTarget();

	UpdateLookAt();

	// If we stopped taunting but the animation is still active then kill it.
	if ( !m_Shared.InCond( TDC_COND_TAUNTING ) && m_PlayerAnimState->IsGestureSlotActive( GESTURE_SLOT_VCD ) )
	{
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	}

	if ( m_bWaterExitEffectActive && !IsAlive() )
	{
		ParticleProp()->StopParticlesNamed( "water_playeremerge", false );
		m_bWaterExitEffectActive = false;
	}

	if ( m_bUpdateAttachedModels )
	{
		m_bUpdateAttachedModels = false;
	}

	// Bad place for this
	if ( m_Shared.InCond( TDC_COND_SLIDE ) )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CLocalPlayerFilter filter;
		m_pSoundCur = controller.SoundCreate( filter, entindex(), "Player.SlideLoop" );
		controller.Play( m_pSoundCur , 1.0, 100);
	}
	else
	{
		if ( m_pSoundCur )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pSoundCur );
			m_pSoundCur = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateLookAt( void )
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	Vector vMyOrigin =  GetAbsOrigin();

	Vector vecLookAtTarget = vec3_origin;

	for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if ( !pEnt || !pEnt->IsPlayer() )
			continue;

		if ( !pEnt->IsAlive() )
			continue;

		if ( pEnt == this )
			continue;

		Vector vDir = pEnt->GetAbsOrigin() - vMyOrigin;

		if ( vDir.Length() > 300 ) 
			continue;

		VectorNormalize( vDir );

		if ( DotProduct( vForward, vDir ) < 0.0f )
			continue;

		vecLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if ( bFoundViewTarget == false )
	{
		// no target, look forward
		vecLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	// orient eyes
	m_viewtarget = vecLookAtTarget;

	// blinking
	if ( m_blinkTimer.IsElapsed() )
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
	}

	/*
	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = vecLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];

	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( -desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );

	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );

	m_flCurrentHeadPitch = ApproachAngle( -desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
	*/
}

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
void C_TDCPlayer::AvoidPlayers( CUserCmd *pCmd )
{
	// Turn off the avoid player code.
	if ( !tdc_avoidteammates_pushaway.GetBool() )
		return;

	// Don't test if the player doesn't exist or is dead.
	if ( IsAlive() == false )
		return;

	C_Team *pTeam = ( C_Team * )GetTeam();
	if ( !pTeam )
		return;

	// Up vector.
	static Vector vecUp( 0.0f, 0.0f, 1.0f );

	Vector vecTFPlayerCenter = GetAbsOrigin();
	Vector vecTFPlayerMin = GetPlayerMins();
	Vector vecTFPlayerMax = GetPlayerMaxs();
	float flZHeight = vecTFPlayerMax.z - vecTFPlayerMin.z;
	vecTFPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd( vecTFPlayerMin, vecTFPlayerCenter, vecTFPlayerMin );
	VectorAdd( vecTFPlayerMax, vecTFPlayerCenter, vecTFPlayerMax );

	// Find an intersecting player or object.
	int nAvoidPlayerCount = 0;
	C_TDCPlayer *pAvoidPlayerList[MAX_PLAYERS];

	C_TDCPlayer *pIntersectPlayer = NULL;
	float flAvoidRadius = 0.0f;

	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
	{
		C_TDCPlayer *pAvoidPlayer = static_cast< C_TDCPlayer * >( pTeam->GetPlayer( i ) );
		if ( pAvoidPlayer == NULL )
			continue;
		// Is the avoid player me?
		if ( pAvoidPlayer == this )
			continue;

		// Check to see if the avoid player is dormant.
		if ( pAvoidPlayer->IsDormant() )
			continue;

		// Is the avoid player solid?
		if ( pAvoidPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
			continue;

		// Save as list to check against for objects.
		pAvoidPlayerList[nAvoidPlayerCount] = pAvoidPlayer;
		++nAvoidPlayerCount;

		Vector t1, t2;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
		VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

		if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
		{
			// Need to avoid this player.
			if ( !pIntersectPlayer )
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}

	// Anything to avoid?
	if ( !pIntersectPlayer )
		return;

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player.
	if ( pIntersectPlayer )
	{
		VectorSubtract( pIntersectPlayer->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}

	float flPushStrength = RemapValClamped( vecDelta.Length(), flAvoidRadius, 0, 0, 256.0f ); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if ( flPushStrength < 0.01f )
		return;

	Vector vecPush;
	if ( GetAbsVelocity().Length2DSqr() > 0.1f )
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct( vecUp, vecVelocity, vecPush );
		VectorNormalize( vecPush );
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors( angView, NULL, &vecPush, NULL );
	}

	// Move away from the other player.
	Vector vecSeparationVelocity;
	if ( vecDelta.Dot( vecPush ) < 0 )
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flMaxPlayerSpeed = MaxSpeed();
	float flCropFraction = 1.33333333f;

	if ( ( GetFlags() & FL_DUCKING ) && ( GetGroundEntity() != NULL ) )
	{	
		flMaxPlayerSpeed *= flCropFraction;
	}	

	float flMaxPlayerSpeedSqr = flMaxPlayerSpeed * flMaxPlayerSpeed;

	if ( vecSeparationVelocity.LengthSqr() > flMaxPlayerSpeedSqr )
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale( vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity );
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize( vDirection );

	float fwd = currentdir.Dot( vDirection );
	float rt = rightdir.Dot( vDirection );

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	pCmd->forwardmove	+= forward;
	pCmd->sidemove		+= side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}

	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}

	float flScale = Min( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInputSampleTime - 
//			*pCmd - 
//-----------------------------------------------------------------------------
bool C_TDCPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{	
	// HACK: We're using an unused bit in buttons var to set the typing status based on whether player's chat panel is open.
	if ( GetTDCChatHud() && GetTDCChatHud()->GetMessageMode() != MM_NONE )
	{
		pCmd->buttons |= IN_TYPING;
	}

	if ( InThirdPersonShoulder() )
	{
		pCmd->buttons |= IN_THIRDPERSON;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd );

	AvoidPlayers( pCmd );

#if 0
	// If player is taunting and in first person lock the camera angles.
	if ( m_Shared.IsMovementLocked() && InFirstPersonView() )
	{
		pCmd->viewangles = EyeAngles();
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( !prediction->IsFirstTimePredicted() )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, nData );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector C_TDCPlayer::GetObserverCamOrigin( void )
{
	if ( !IsAlive() )
	{
		if ( m_hFirstGib )
		{
			IPhysicsObject *pPhysicsObject = m_hFirstGib->VPhysicsGetObject();
			if( pPhysicsObject )
			{
				Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
				Vector vecWorld;
				m_hFirstGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &vecWorld );
				return (vecWorld);
			}
			return m_hFirstGib->GetRenderOrigin();
		}

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
			return pRagdoll->GetRagdollOrigin();
	}

	return BaseClass::GetObserverCamOrigin();	
}

//-----------------------------------------------------------------------------
// Purpose: Consider the viewer and other factors when determining resulting
// invisibility
//-----------------------------------------------------------------------------
float C_TDCPlayer::GetEffectiveInvisibilityLevel( void )
{
	float flPercentInvisible = GetPercentInvisible();

	// If this is a teammate of the local player or viewer is observer,
	// dont go above a certain max invis
	if ( !IsEnemyPlayer() )
	{
		flPercentInvisible = Min( flPercentInvisible, 0.95f );
	}
	else
	{
		// If this player just killed me, show them slightly
		// less than full invis in the deathcam and freezecam

		C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pLocalPlayer )
		{
			int iObserverMode = pLocalPlayer->GetObserverMode();

			if ( ( iObserverMode == OBS_MODE_FREEZECAM || iObserverMode == OBS_MODE_DEATHCAM ) && 
				pLocalPlayer->GetObserverTarget() == this )
			{
				flPercentInvisible = Min( flPercentInvisible, 0.95f );
			}
		}
	}
	if (!m_Shared.InCond( TDC_COND_STEALTHED_BLINK ))
	{
		float fVelSqr = GetLocalVelocity().LengthSqr();
		if (fVelSqr > 0.0f)
		{
			const float fMaxVelSqr = 50.0f * 50.0f;

			fVelSqr = fMaxVelSqr / fVelSqr;
			fVelSqr = clamp(fVelSqr, 0.0f, 1.0f);
			fVelSqr *= 1.0f - TDC_INVIS_MOVEMENT_SCALE;

			flPercentInvisible *= TDC_INVIS_MOVEMENT_SCALE + fVelSqr;
		}
	}

	return flPercentInvisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDCPlayer::DrawModel( int flags )
{
	// If we're a dead player with a fresh ragdoll, don't draw
	if ( m_nRenderFX == kRenderFxRagdoll )
		return 0;

	// Don't draw the model at all if we're fully invisible
	if ( GetEffectiveInvisibilityLevel() >= 1.0f )
	{
		return 0;
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDCPlayer::InternalDrawModel( int flags )
{
	bool bUseInvulnMaterial = ( m_Shared.IsInvulnerable() );
	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *GetInvulnMaterialRef() );
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
bool C_TDCPlayer::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Consistent lighting origin for all players.
	pInfo->pLightingOrigin = &WorldSpaceCenter();

	return BaseClass::OnInternalDrawModel( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::ProcessMuzzleFlashEvent()
{
	CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// Reenable when the weapons have muzzle flash attachments in the right spot.
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( this == pLocalPlayer && !bInToolRecordingMode )
		return; // don't show own world muzzle flash for localplayer

	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		// also don't show in 1st person spec mode
		if ( pLocalPlayer->GetObserverTarget() == this )
			return;
	}

	C_TDCWeaponBase *pWeapon = m_Shared.GetActiveTFWeapon();
	if ( !pWeapon )
		return;

	pWeapon->ProcessMuzzleFlashEvent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDCPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::SetForcedIDTarget( int iTarget )
{
	m_iForcedIDTarget = iTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's targetid entity
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateIDTarget()
{
	if ( !IsLocalPlayer() )
		return;

	// don't show IDs if mp_fadetoblack is on
	if ( GetTeamNumber() > TEAM_SPECTATOR && mp_fadetoblack.GetBool() && !IsAlive() )
	{
		m_iIDEntIndex = 0;
		return;
	}

	if ( m_iForcedIDTarget )
	{
		m_iIDEntIndex = m_iForcedIDTarget;
		return;
	}

	// If we're spectating someone then ID them.
	if ( GetObserverMode() == OBS_MODE_DEATHCAM ||
		GetObserverMode() == OBS_MODE_IN_EYE ||
		GetObserverMode() == OBS_MODE_CHASE )
	{	
		if ( GetObserverTarget() && GetObserverTarget() != this )
		{
			m_iIDEntIndex = GetObserverTarget()->entindex();
			return;
		}
	}

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	trace_t tr;
	Vector vecOrigin, vecForward, vecStart, vecEnd;
	vecOrigin = MainViewOrigin();
	vecForward = MainViewForward();

	VectorMA( vecOrigin, MAX_TRACE_LENGTH, vecForward, vecEnd );
	VectorMA( vecOrigin, 10, vecForward, vecStart );

	// If we're in observer mode, ignore our observer target. Otherwise, ignore ourselves.
	if ( IsObserver() )
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, GetObserverTarget(), COLLISION_GROUP_NONE, &tr );
	}
	else
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	}

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if ( pEntity && ( pEntity != this ) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes, bool &bShowingAmmo )
{
	sDataString[0] = '\0';
	bShowingAmmo = false;
}

//-----------------------------------------------------------------------------
// Purpose: Display appropriate hints for the target we're looking at
//-----------------------------------------------------------------------------
void C_TDCPlayer::DisplaysHintsForTarget( C_BaseEntity *pTarget )
{
	// If the entity provides hints, ask them if they have one for this player
	ITargetIDProvidesHint *pHintInterface = dynamic_cast<ITargetIDProvidesHint*>(pTarget);
	if ( pHintInterface )
	{
		pHintInterface->DisplayHintTo( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDCPlayer::GetRenderTeamNumber( void )
{
	return m_nSkin;
}

static Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
static Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	CBaseEntity	* killer = GetObserverTarget();

	// Swing to face our killer within half the death anim time
	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / ( TDC_DEATH_ANIMATION_TIME * 0.5 );
	interpolation = clamp( interpolation, 0.0f, 1.0f );
	interpolation = SimpleSpline( interpolation );

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, CHASE_CAM_DISTANCE_MIN, CHASE_CAM_DISTANCE_MAX );

	QAngle aForward = eyeAngles = EyeAngles();
	Vector origin = EyePosition();

	C_TDCRagdoll *pRagdollEnt = static_cast<C_TDCRagdoll *>( m_hRagdoll.Get() );

	if ( pRagdollEnt )
	{
		if ( pRagdollEnt->IsPlayingDeathAnim() )
		{
			// Bring the camera up if playing death animation.
			origin.z += VEC_DEAD_VIEWHEIGHT.z * 4.0f;
		}
		else
		{
			IRagdoll *pRagdoll = pRagdollEnt->GetIRagdoll();
			if ( pRagdoll )
			{
				origin = pRagdoll->GetRagdollOrigin();
				origin.z += VEC_DEAD_VIEWHEIGHT.z; // look over ragdoll, not through
			}
		}
	}

	if ( killer && killer != this )
	{
		Vector vKiller = killer->EyePosition() - origin;
		QAngle aKiller; VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	}

	Vector vForward; AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );

	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // clip against world
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if ( trace.fraction < 1.0 )
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength( origin - eyeOrigin );
	}

	fov = GetFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void C_TDCPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

float C_TDCPlayer::GetMinFOV() const
{
	// Min FOV for Sniper Rifle
	return 20;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const QAngle& C_TDCPlayer::EyeAngles()
{
	// We cannot use the local camera angles when taunting since player cannot turn.
	if ( IsLocalPlayer() && !m_Shared.IsMovementLocked() )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bCopyEntity - 
// Output : C_BaseAnimating *
//-----------------------------------------------------------------------------
C_BaseAnimating *C_TDCPlayer::BecomeRagdollOnClient()
{
	// Let the C_TDCRagdoll take care of this.
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll* C_TDCPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_TDCRagdoll *pRagdoll = static_cast<C_TDCRagdoll*>( m_hRagdoll.Get() );
		if ( !pRagdoll )
			return NULL;

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::InitPlayerGibs( void )
{
	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			&vecImpactVelocity - 
//-----------------------------------------------------------------------------
void C_TDCPlayer::CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning )
{
	// Make sure we have Gibs to create.
	if ( m_aGibs.Count() == 0 )
		return;

	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

	Vector vecBreakVelocity = vecVelocity;
	vecBreakVelocity.z += 1.0f;
	VectorNormalize( vecBreakVelocity );
	vecBreakVelocity *= 500.0f;

	// Cap the impulse.
	float flSpeed = vecBreakVelocity.Length();
	if ( flSpeed > 400.0f )
	{
		VectorScale( vecBreakVelocity, 400.0f / flSpeed, vecBreakVelocity );
	}

	breakablepropparams_t breakParams( vecOrigin, GetRenderAngles(), vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;//

	// Break up the player.
	m_hSpawnedGibs.Purge();
	m_hFirstGib = CreateGibsFromList( m_aGibs, GetModelIndex(), NULL, breakParams, this, -1 , false, true, &m_hSpawnedGibs, bBurning );

	if ( g_TDC_PR )
	{
		// Gib skin numbers don't match player skin numbers so we gotta fix it up here.
		for ( int i = 0; i < m_hSpawnedGibs.Count(); i++ )
		{
			C_BaseAnimating *pGib = static_cast<C_BaseAnimating *>( m_hSpawnedGibs[i].Get() );
			pGib->m_nSkin = GetTeamSkin( g_TDC_PR->GetTeam( entindex() ) );
		}
	}
}

float C_TDCPlayer::GetPercentInvisible( void )
{
	return m_Shared.GetPercentInvisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TDCPlayer::GetSkin()
{
	int nSkin = GetTeamSkin( GetTeamNumber() );

	return nSkin;
}

extern ConVar tdc_player_restrict_class;

//-----------------------------------------------------------------------------
// Purpose: Check to see if the player's able to open the class menu.
//-----------------------------------------------------------------------------
bool C_TDCPlayer::CanShowClassMenu( void ) const
{
	if ( GetTeamNumber() < FIRST_GAME_TEAM )
		return false;

	if ( TDCGameRules()->IsInfectionMode() && GetTeamNumber() == TDC_TEAM_ZOMBIES )
		return false;

	if ( !FStrEq( tdc_player_restrict_class.GetString(), "" ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TDCPlayer::IsPlayerClass( int iClass ) const
{
	return m_PlayerClass.IsClass( iClass );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TDCPlayer::IsNormalClass( void ) const
{
	return ( m_PlayerClass.GetClassIndex() <= TDC_LAST_NORMAL_CLASS );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the player's a zombie! (Used in infection)
//-----------------------------------------------------------------------------
bool C_TDCPlayer::IsZombie( void ) const
{
	return ( m_PlayerClass.GetClassIndex() == TDC_CLASS_ZOMBIE );
}

//-----------------------------------------------------------------------------
// Purpose: Don't take damage decals while stealthed
//-----------------------------------------------------------------------------
void C_TDCPlayer::AddDecal( const Vector& rayStart, const Vector& rayEnd,
							const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal )
{
	if ( m_Shared.IsStealthed() )
	{
		return;
	}

	if ( m_Shared.IsInvulnerable() )
	{ 
		Vector vecDir = rayEnd - rayStart;
		VectorNormalize(vecDir);
		g_pEffects->Ricochet( rayEnd - (vecDir * 8), -vecDir );
		return;
	}

	// don't decal from inside the player
	if ( tr.startsolid )
	{
		return;
	}

	BaseClass::AddDecal( rayStart, rayEnd, decalCenter, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
}

//-----------------------------------------------------------------------------
// Called every time the player respawns
//-----------------------------------------------------------------------------
void C_TDCPlayer::ClientPlayerRespawn( void )
{
	if ( IsLocalPlayer() )
	{
		// Dod called these, not sure why
		//MoveToLastReceivedPosition( true );
		//ResetLatched();

		// Reset the camera.
		m_bWasTaunting = false;
		HandleTaunting();

		ResetToneMapping( 1.0f );

		// Release the duck toggle key
		KeyUp( &in_ducktoggle, NULL ); 

		C_TDCPlayerClass *pClass = GetPlayerClass();
		if ( pClass )
		{
			int iClass = pClass->GetClassIndex();
			for ( int iSlot = 0; iSlot < TDC_WEARABLE_COUNT; iSlot++ )
			{
				int iItemID = g_TDCPlayerItems.GetWearableInSlot( iClass, (ETDCWearableSlot)iSlot );
				engine->ClientCmd( VarArgs( "setitempreset %d %d %d", iClass, iSlot, iItemID ) );
			}
		}
	}
	else
	{
		SetViewOffset( GetClassEyeHeight() );
	}

	UpdateVisibility();
	DestroyBoneAttachments();

	m_hFirstGib = NULL;
	m_hSpawnedGibs.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TDCPlayer::IsOverridingViewmodel( void )
{
	C_TDCPlayer *pPlayer = GetObservedPlayer( true );

	if ( pPlayer->m_Shared.IsInvulnerable() )
		return true;

	return BaseClass::IsOverridingViewmodel();
}

//-----------------------------------------------------------------------------
// Purpose: Draw my viewmodel in some special way
//-----------------------------------------------------------------------------
int	C_TDCPlayer::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int ret = 0;

	C_TDCPlayer *pPlayer = GetObservedPlayer( true );

	if ( pPlayer->m_Shared.IsInvulnerable() )
	{
		// Force the invulnerable material
		modelrender->ForcedMaterialOverride( *pPlayer->GetInvulnMaterialRef() );

		ret = pViewmodel->DrawOverriddenViewmodel( flags );

		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::InitializePoseParams( void )
{
	/*
	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );
	*/

	CStudioHdr *hdr = GetModelPtr();
	Assert( hdr );
	if ( !hdr )
		return;

	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_TDCPlayer::GetChaseCamViewOffset( CBaseEntity *target )
{
	return BaseClass::GetChaseCamViewOffset( target );
}

//-----------------------------------------------------------------------------
// Purpose: Called from PostDataUpdate to update the model index
//-----------------------------------------------------------------------------
void C_TDCPlayer::ValidateModelIndex( void )
{
	C_TDCPlayerClass *pClass = GetPlayerClass();
	if ( pClass )
	{
		m_nModelIndex = modelinfo->GetModelIndex( pClass->GetModelName() );
	}

	BaseClass::ValidateModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_TDCPlayer::Simulate( void )
{
	//Frame updates
	if ( IsLocalPlayer() )
	{
		//Update the flashlight
		Flashlight();
	}

	// Update step sound for first person players.
	if ( !GetPredictable() && !ShouldDrawThisPlayer() )
	{
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
	}

	// TF doesn't do step sounds based on velocity, instead using anim events
	// So we deliberately skip over the base player simulate, which calls them.
	BaseClass::BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	switch ( event )
	{
	case TDC_AE_FOOTSTEP:
	{
		// Force a footstep sound.
		// Set this to -1 to indicate we're about to make a forced footstep sound.
		m_flStepSoundTime = -1.0f;
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
		break;
	}
	case AE_WPN_HIDE:
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( false );
		}
		break;
	}
	case AE_WPN_UNHIDE:
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( true );
		}
		break;
	}
	case AE_WPN_PLAYWPNSOUND:
	{
		C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon )
		{
			int iSnd = GetWeaponSoundFromString( options );
			if ( iSnd != -1 )
			{
				pWeapon->WeaponSound( (WeaponSound_t)iSnd );
			}
		}
		break;
	}
	case TDC_AE_CIGARETTE_THROW:
	{
		CEffectData data;
		int iAttach = LookupAttachment( options );
		GetAttachment( iAttach, data.m_vOrigin, data.m_vAngles );

		data.m_vAngles = GetRenderAngles();

		data.m_hEntity = ClientEntityList().EntIndexToHandle( entindex() );
		DispatchEffect( "TDC_ThrowCigarette", data );
		break;
	}
	case AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN:
	{
		// Pass it through to weapon.
		CTDCWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon )
		{
			pWeapon->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
		}
		break;
	}
	case TDC_AE_WPN_EJECTBRASS:
	{
		CTDCWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( !pWeapon || pWeapon->UsingViewModel() )
			return;

		pWeapon->EjectBrass( pWeapon, atoi( options ) );
		break;
	}
	case AE_CL_MAG_EJECT:
	{
		CTDCWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( !pWeapon )
			return;

		// Check if we're actually in the middle of reload anim and it wasn't just interrupted by firing.
		C_AnimationLayer *pAnimLayer = m_PlayerAnimState->GetGestureSlotLayer( GESTURE_SLOT_ATTACK_AND_RELOAD );
		if ( pAnimLayer->m_flCycle == 1.0f )
			break;

		Vector vecForce;
		UTIL_StringToVector( vecForce.Base(), options );

		pWeapon->EjectMagazine( pWeapon->GetBaseAnimating(), vecForce );
		break;
	}
	case AE_CL_MAG_EJECT_UNHIDE:
	{
		CTDCWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( !pWeapon )
			return;

		pWeapon->UnhideMagazine( pWeapon->GetBaseAnimating() );
		break;
	}
	case AE_CL_DUMP_BRASS:
	{
		CTDCWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( !pWeapon )
			return;

		// Check if we're actually in the middle of reload anim and it wasn't just interrupted by firing.
		C_AnimationLayer *pAnimLayer = m_PlayerAnimState->GetGestureSlotLayer( GESTURE_SLOT_ATTACK_AND_RELOAD );
		if ( pAnimLayer->m_flCycle == 1.0f )
			break;

		pWeapon->DumpBrass( pWeapon->GetBaseAnimating(), atoi( options ) );
		break;
	}
	default:
		BaseClass::FireEvent( origin, angles, event, options );
		break;
	}
}

// Shadows

ConVar cl_blobbyshadows( "cl_blobbyshadows", "0", FCVAR_CLIENTDLL );
extern ConVar tdc_disable_player_shadows;
ShadowType_t C_TDCPlayer::ShadowCastType( void ) 
{
	if ( tdc_disable_player_shadows.GetBool() )
		return SHADOWS_NONE;

	// Removed the GetPercentInvisible - should be taken care off in BindProxy now.
	if ( !IsVisible() /*|| GetPercentInvisible() > 0.0f*/ )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	// If in ragdoll mode.
	if ( m_nRenderFX == kRenderFxRagdoll )
		return SHADOWS_NONE;

	// if we're first person spectating this player
	if ( !ShouldDrawThisPlayer() )
	{
		return SHADOWS_NONE;		
	}

	if( cl_blobbyshadows.GetBool() )
		return SHADOWS_SIMPLE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

float g_flFattenAmt = 4;
void C_TDCPlayer::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Don't let the render bounds change when we're using blobby shadows, or else the shadow
		// will pop and stretch.
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
	}
	else
	{
		GetRenderBounds( mins, maxs );

		// We do this because the normal bbox calculations don't take pose params into account, and 
		// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
		// the shadow will get cut off as he rotates.
		//
		// Thus, we give it some padding here.
		mins -= Vector( g_flFattenAmt, g_flFattenAmt, 0 );
		maxs += Vector( g_flFattenAmt, g_flFattenAmt, 0 );
	}
}


void C_TDCPlayer::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	// TODO POSTSHIP - this hack/fix goes hand-in-hand with a fix in CalcSequenceBoundingBoxes in utils/studiomdl/simplify.cpp.
	// When we enable the fix in CalcSequenceBoundingBoxes, we can get rid of this.
	//
	// What we're doing right here is making sure it only uses the bbox for our lower-body sequences since,
	// with the current animations and the bug in CalcSequenceBoundingBoxes, are WAY bigger than they need to be.
	C_BaseAnimating::GetRenderBounds( theMins, theMaxs );
}


bool C_TDCPlayer::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{ 
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Blobby shadows should sit directly underneath us.
		pDirection->Init( 0, 0, -1 );
		return true;
	}
	else
	{
		return BaseClass::GetShadowCastDirection( pDirection, shadowType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether we should show the nemesis icon for this player
//-----------------------------------------------------------------------------
bool C_TDCPlayer::ShouldShowNemesisIcon()
{
	// we should show the nemesis effect on this player if he is the nemesis of the local player,
	// and is not dead or invisible
	if ( g_TDC_PR && g_TDC_PR->IsPlayerDominating( entindex() ) )
	{
		if ( IsAlive() && !m_Shared.IsStealthed() )
			return true;
	}
	return false;
}

bool C_TDCPlayer::IsWeaponLowered( void )
{
	CTDCWeaponBase *pWeapon = GetActiveTFWeapon();

	if ( !pWeapon )
		return false;

	CTDCGameRules *pRules = TDCGameRules();

	// Lower losing team's weapons in bonus round
	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
		return true;

	// Hide all view models after the game is over
	if ( pRules->State_Get() == GR_STATE_GAME_OVER )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TDCPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
		return StartGestureSceneEvent( info, scene, event, actor, pTarget );
	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TDCPlayer::StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	// Get the (gesture) sequence.
	info->m_nSequence = LookupSequence( event->GetParameters() );
	if ( info->m_nSequence < 0 )
		return false;

	float flCycle = 0.0f;

	// Fix up for looping taunts.
	bool looping = ( ( GetSequenceFlags( GetModelPtr(), info->m_nSequence ) & STUDIO_LOOPING ) != 0 );
	if ( !looping )
	{
		float dt = scene->GetTime() - event->GetStartTime();
		float seq_duration = SequenceDuration( info->m_nSequence );
		flCycle = clamp( dt / seq_duration, 0.0f, 1.0f );
	}

	// Player the (gesture) sequence.
	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	m_PlayerAnimState->AddVCDSequenceToGestureSlot( GESTURE_SLOT_VCD, info->m_nSequence, flCycle );

	return true;
}

bool C_TDCPlayer::IsAllowedToSwitchWeapons( void )
{
	if ( IsWeaponLowered() == true )
		return false;

	if ( m_Shared.InCond( TDC_COND_SPRINT ) )
		return false;

	return BaseClass::IsAllowedToSwitchWeapons();
}

IMaterial *C_TDCPlayer::GetHeadLabelMaterial( void )
{
	if ( g_pHeadLabelMaterial[0] == NULL )
		SetupHeadLabelMaterials();

	switch ( GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		return g_pHeadLabelMaterial[TDC_PLAYER_HEAD_LABEL_RED];
		break;

	case TDC_TEAM_BLUE:
		return g_pHeadLabelMaterial[TDC_PLAYER_HEAD_LABEL_BLUE];
		break;

	}

	return BaseClass::GetHeadLabelMaterial();
}

void SetupHeadLabelMaterials( void )
{
	for (int i = 0; i < ( TDC_TEAM_COUNT - 2 ); i++)
	{
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->DecrementReferenceCount();
			g_pHeadLabelMaterial[i] = NULL;
		}

		g_pHeadLabelMaterial[i] = materials->FindMaterial( pszHeadLabelNames[i], TEXTURE_GROUP_VGUI );
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->IncrementReferenceCount();
		}
	}
}

void C_TDCPlayer::ComputeFxBlend( void )
{
	BaseClass::ComputeFxBlend();

	float flInvisible = GetPercentInvisible();
	if ( flInvisible != 0.0f )
	{
		// Tell our shadow
		ClientShadowHandle_t hShadow = GetShadowHandle();
		if ( hShadow != CLIENTSHADOW_INVALID_HANDLE )
		{
			g_pClientShadowMgr->SetFalloffBias( hShadow, flInvisible * 255 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	HandleTaunting();

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
C_TDCTeam *C_TDCPlayer::GetTDCTeam( void ) const
{
	// Grumble: server-side CBaseEntity::GetTeam is const,
	// but client-side C_BaseEntity::GetTeam is non-const,
	// for absolutely no good reason whatsoever.
	return assert_cast<C_TDCTeam *>( const_cast<C_TDCPlayer *>( this )->GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the weapon passed in would occupy a slot already occupied by the carrier
// Input  : *pWeapon - weapon to test for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TDCPlayer::Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon == NULL )
		return false;

	//Check to see if there's a resident weapon already in this slot
	if ( Weapon_GetSlot( pWeapon->GetSlot() ) == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon (if any) in the requested slot
// Input  : slot - which slot to poll
//-----------------------------------------------------------------------------
CBaseCombatWeapon *C_TDCPlayer::Weapon_GetSlot( int slot ) const
{
	int	targetSlot = slot;

	// Check for that slot being occupied already
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( GetWeapon( i ) != NULL )
		{
			// If the slots match, it's already occupied
			if ( GetWeapon( i )->GetSlot() == targetSlot )
				return GetWeapon( i );
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		if ( !IsLocalPlayer() )
		{
			UpdateSpyStateChange();
		}
	}
	else
	{
		BaseClass::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update any effects affected by cloak and disguise.
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateSpyStateChange( void )
{
	m_Shared.UpdateCritBoostEffect();
	UpdateOverhealEffect();
	UpdateRecentlyTeleportedEffect();

	if ( !IsLocalPlayer() )
	{
		// Update the weapon model.
		C_TDCWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon )
		{
			pWeapon->SetModelIndex( pWeapon->GetWorldModelIndex() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_TDCPlayer::CanShowSpeechBubbles( void )
{
	if ( IsDormant() )
		return false;

	if ( !GameRules()->ShouldDrawHeadLabels() )
		return false;

	extern ConVar *sv_alltalk;

	if ( !sv_alltalk )
		sv_alltalk = cvar->FindVar( "sv_alltalk" );

	if ( !sv_alltalk || !sv_alltalk->GetBool() )
	{
		int iLocalTeam = GetLocalPlayerTeam();
		if ( iLocalTeam != GetTeamNumber() && iLocalTeam != TEAM_SPECTATOR )
			return false;
	}

	if ( !IsAlive() )
		return false;

	if ( m_Shared.IsStealthed() && IsEnemyPlayer() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateSpeechBubbles( void )
{
	// Don't show the bubble for local player since they don't need it.
	if ( IsLocalPlayer() )
		return;

	if ( m_bTyping && CanShowSpeechBubbles() )
	{
		if ( !m_pTypingEffect )
		{
			m_pTypingEffect = ParticleProp()->Create( "speech_typing", PATTACH_POINT_FOLLOW, "head" );
		}
	}
	else
	{
		if ( m_pTypingEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pTypingEffect );
			m_pTypingEffect = NULL;
		}
	}

	if ( GetClientVoiceMgr()->IsPlayerSpeaking( entindex() ) && CanShowSpeechBubbles() )
	{
		if ( !m_pVoiceEffect )
		{
			m_pVoiceEffect = ParticleProp()->Create( "speech_voice", PATTACH_POINT_FOLLOW, "head" );
		}
	}
	else
	{
		if ( m_pVoiceEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pVoiceEffect );
			m_pVoiceEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateOverhealEffect( void )
{
	bool bShouldShow = true;

	if ( !m_Shared.InCond( TDC_COND_HEALTH_OVERHEALED ) )
	{
		bShouldShow = false;
	}
	else if ( IsEnemyPlayer() )
	{
		if ( m_Shared.IsStealthed() )
		{
			// Don't give away invisible players
			bShouldShow = false;
		}
	}

	if ( bShouldShow )
	{
		if ( !m_pOverhealEffect )
		{
			const char *pszEffect = ConstructTeamParticle( "overhealedplayer_%s_pluses", GetTeamNumber(), true );
			m_pOverhealEffect = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
			m_Shared.SetParticleToMercColor( m_pOverhealEffect );
		}
	}
	else
	{
		if ( m_pOverhealEffect )
		{
			ParticleProp()->StopEmission( m_pOverhealEffect );
			m_pOverhealEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::UpdateClientSideGlow( void )
{
	bool bShouldGlow = false;

	// Never show glow on local player.
	if ( !IsLocalPlayer() )
	{
		bShouldGlow = ( HasTheFlag() && !IsEnemyPlayer() ) || m_Shared.InCond( TDC_COND_LASTSTANDING );
	}

	if ( bShouldGlow != IsClientSideGlowEnabled() )
	{
		SetClientSideGlowEnabled( bShouldGlow );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TDCPlayer::GetGlowEffectColor( float *r, float *g, float *b )
{
	// Set the glow color on flag carrier according to their health.
	if ( HasTheFlag() && !IsEnemyPlayer() )
	{
		*r = RemapValClamped( GetHealth(), GetMaxHealth() * 0.5f, GetMaxHealth(), 0.75f, 0.33f );
		*g = RemapValClamped( GetHealth(), 0.0f, GetMaxHealth() * 0.5f, 0.23f, 0.75f );
		*b = 0.23f;
	}
	else
	{
		TDCGameRules()->GetTeamGlowColor( GetTeamNumber(), *r, *g, *b );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float C_TDCPlayer::GetDesaturationAmount( void )
{
	if ( TDCGameRules()->State_Get() == GR_STATE_PREROUND && !TDCGameRules()->IsInWaitingForPlayers() )
	{
		// Do a fade in during freeze time.
		// Can't get time from gamerules so get it from timer entity instead.
		C_TeamRoundTimer *pTimer = TDCGameRules()->GetStalemateTimer();

		if ( pTimer )
		{
			float flTimeLeft = pTimer->GetTimeRemaining();
			return RemapValClamped( flTimeLeft, 3.0f, 0.0f, 1.0f, 0.0f );
		}
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TDCPlayer::CollectVisibleSteamUsers( CUtlVector<CSteamID> &userList )
{
	CSteamID steamID;

	// Add our spectator target.
	C_TDCPlayer *pTarget = NULL;

	if ( IsObserver() && GetObserverMode() != OBS_MODE_ROAMING )
	{
		pTarget = ToTDCPlayer( GetObserverTarget() );
		if ( pTarget && pTarget->GetSteamID( &steamID ) )
		{
			userList.AddToTail( steamID );
		}
	}

	// Add everyone we can see.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		C_TDCPlayer *pOther = ToTDCPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pOther ||
			pOther == this ||
			pOther->IsDormant() ||
			!pOther->IsAlive() ||
			pOther == pTarget ||
			pOther->m_Shared.IsStealthed() ||
			!pOther->GetSteamID( &steamID ) )
			continue;

		// Check that they are on the screen.
		int x, y;
		if ( GetVectorInScreenSpace( pOther->EyePosition(), x, y ) == false )
			continue;

		// Make sure there are no obstructions.
		trace_t tr;
		UTIL_TraceLine( MainViewOrigin(), pOther->EyePosition(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
		{
			userList.AddToTail( steamID );
		}
	}
}

vgui::IImage* GetDefaultAvatarImage( int iPlayerIndex )
{
	if ( g_PR && g_PR->IsConnected( iPlayerIndex ) && g_PR->GetTeam( iPlayerIndex ) >= FIRST_GAME_TEAM )
	{
		const char *pszImage = NULL;

		if ( TDCGameRules() && TDCGameRules()->IsTeamplay() )
		{
			pszImage = VarArgs( "../vgui/avatar_default_%s", g_aTeamLowerNames[g_PR->GetTeam( iPlayerIndex )] );
		}
		else
		{
			pszImage = "../vgui/avatar_default";
		}

		if ( pszImage )
		{
			return vgui::scheme()->GetImage( pszImage, true );
		}
	}

	return NULL;
}

C_TDCPlayer *GetLocalObservedPlayer( bool bFirstPerson )
{
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalPlayer )
		return NULL;

	return pLocalPlayer->GetObservedPlayer( bFirstPerson );
}

CON_COMMAND( tdc_spec_playerinfo, "Shows some information about the spectated player." )
{
	C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalPlayer || !pLocalPlayer->IsObserver() )
		return;

	C_TDCPlayer *pPlayer = ToTDCPlayer( pLocalPlayer->GetObserverTarget() );
	if ( !pPlayer )
		return;

	Msg( "Name: %s\nFOV: %d\nViewmodel FOV: %g\nViewmodel offset: %g, %g, %g\n",
		pPlayer->GetPlayerName(),
		pPlayer->GetDefaultFOV(),
		pPlayer->GetViewModelFOV(),
		pPlayer->GetViewModelOffset().x, pPlayer->GetViewModelOffset().y, pPlayer->GetViewModelOffset().z );
}

void tdc_merc_color_callback(IConVar *var, const char *pOldValue, float flOldValue)
{
	const CUtlVector<PlayerColor_t> &playerColors = g_TDCPlayerItems.GetPlayerColors();
	if ( !playerColors.IsEmpty() )
	{
		Vector curColor = playerColors[tdc_merc_color.GetInt() % playerColors.Count()].color;

		tdc_merc_color_r.SetValue( VarArgs( "%f", curColor.x ) );
		tdc_merc_color_g.SetValue( VarArgs( "%f", curColor.y ) );
		tdc_merc_color_b.SetValue( VarArgs( "%f", curColor.z ) );
	}
}
