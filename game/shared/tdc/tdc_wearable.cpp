//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tdc_wearable.h"
#include "tdc_viewmodel.h"

#ifdef GAME_DLL
#include "tdc_player.h"
#else
#include "c_tdc_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TDCWearable, DT_TDCWearable );
BEGIN_NETWORK_TABLE( CTDCWearable, DT_TDCWearable )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iItemID ) ),
#else
	SendPropInt( SENDINFO( m_iItemID ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( item_wearable, CTDCWearable );

CTDCWearable::CTDCWearable()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
WearableDef_t *CTDCWearable::GetStaticData( void )
{
	return g_TDCPlayerItems.GetWearable( m_iItemID );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCWearable *CTDCWearable::Create( int iItemID, CTDCPlayer *pPlayer )
{
	WearableDef_t *pItemDef = g_TDCPlayerItems.GetWearable( iItemID );
	if ( !pItemDef )
		return NULL;

	if ( pItemDef->devOnly && !pPlayer->PlayerHasPowerplay() )
		return NULL;

	CTDCWearable *pWearable = static_cast<CTDCWearable *>( CBaseEntity::CreateNoSpawn( "item_wearable", pPlayer->GetAbsOrigin(), vec3_angle, pPlayer ) );
	if ( pWearable )
	{
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

		pWearable->m_iItemID = iItemID;
		pWearable->SetModel( pItemDef->visuals[iClass].model );
		DispatchSpawn( pWearable );

		pPlayer->AddWearable( pWearable );
		pWearable->FollowEntity( pPlayer, true );
		pWearable->ChangeTeam( pPlayer->GetTeamNumber() );
	}

	return pWearable;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCWearable::UpdateTransmitState( void )
{
	// Always transmit cosmetics even if they are not visible since they can affect players in other ways.
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWearable::UpdatePlayerBodygroups( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return;

	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	WearableDef_t *pItemDef = g_TDCPlayerItems.GetWearable( m_iItemID );
	for ( auto i = pItemDef->visuals[iClass].bodygroups.First(); i != pItemDef->visuals[iClass].bodygroups.InvalidIndex(); i = pItemDef->visuals[iClass].bodygroups.Next( i ) )
	{
		const char *pszName = pItemDef->visuals[iClass].bodygroups.GetElementName( i );
		int iState = pItemDef->visuals[iClass].bodygroups.Element( i );

		int iGroup = pPlayer->FindBodygroupByName( pszName );
		if ( iGroup != -1 )
		{
			pPlayer->SetBodygroup( iGroup, iState );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWearable::UpdateOnRemove( void )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
		pPlayer->RemoveWearable( this );
		StopFollowingEntity();
		SetOwnerEntity( NULL );
	}

	BaseClass::UpdateOnRemove();
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCWearable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCWearable::InternalDrawModel( int flags )
{
	C_TDCPlayer *pOwner = ToTDCPlayer( GetOwnerEntity() );
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
bool CTDCWearable::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
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
//-----------------------------------------------------------------------------
int CTDCWearable::GetSkin( void )
{
	return GetTeamSkin( GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ShadowType_t CTDCWearable::ShadowCastType( void )
{
	if ( ShouldDraw() )
		return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;

	return SHADOWS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCWearable::ShouldDraw( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	if ( !pOwner->ShouldDrawThisPlayer() )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Make a dummy copy of the cosmetic for player's ragdoll.
//-----------------------------------------------------------------------------
void CTDCWearable::CopyToRagdoll( C_TDCRagdoll *pTFRagdoll )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return;

	WearableDef_t *pItemDef = GetStaticData();
	if ( !pItemDef )
		return;

	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	C_BaseAnimating *pWearable = new C_BaseAnimating();
	if ( !pWearable->InitializeAsClientEntity( pItemDef->visuals[iClass].model, RENDER_GROUP_OPAQUE_ENTITY ) )
	{
		pWearable->Release();
		return;
	}

	if ( pWearable )
	{
		pWearable->AttachEntityToBone( pTFRagdoll );
		pWearable->m_nSkin = GetSkin();
		pWearable->m_nBody = GetBody();
		pWearable->ChangeTeam( GetTeamNumber() );
		pWearable->SetOwnerEntity( GetOwnerEntity() );
	}
}
#endif
