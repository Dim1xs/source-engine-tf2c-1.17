//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_tdc_viewmodeladdon.h"
#include "tdc_viewmodel.h"
#include "tdc_gamerules.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_ViewmodelAttachmentModel::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	if ( BaseClass::InitializeAsClientEntity( pszModelName, renderGroup ) )
	{
		// EF_NODRAW so it won't get drawn directly. We want to draw it from the viewmodel.
		AddEffects( EF_BONEMERGE | EF_BONEMERGE_FASTCULL | EF_NODRAW );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ViewmodelAttachmentModel::SetViewmodel( C_TDCViewModel *vm )
{
	m_viewmodel.Set( vm );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_ViewmodelAttachmentModel::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Use camera position for lighting origin.
	pInfo->pLightingOrigin = &MainViewOrigin();
	return BaseClass::OnInternalDrawModel( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_ViewmodelAttachmentModel::GetSkin( void )
{
	return m_viewmodel->GetArmsSkin();
}

void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ViewmodelAttachmentModel::FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld )
{
	Vector vecOrigin;
	MatrixPosition( attachmentToWorld, vecOrigin );
	::FormatViewModelAttachment( vecOrigin, false );
	PositionMatrix( vecOrigin, attachmentToWorld );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ViewmodelAttachmentModel::UncorrectViewModelAttachment( Vector &vOrigin )
{
	// Unformat the attachment.
	::FormatViewModelAttachment( vOrigin, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseEntity *C_ViewmodelAttachmentModel::GetOwnerViaInterface( void )
{
	return m_viewmodel->GetOwnerViaInterface();
}
