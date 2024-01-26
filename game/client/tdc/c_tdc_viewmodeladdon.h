//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_TDC_VIEWMODELENT_H
#define C_TDC_VIEWMODELENT_H

class C_TDCViewModel;

class C_ViewmodelAttachmentModel : public C_BaseAnimating, public IHasOwner
{
	DECLARE_CLASS( C_ViewmodelAttachmentModel, C_BaseAnimating );
public:

	virtual bool InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup );

	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual int GetSkin( void );
	virtual bool ShouldReceiveProjectedTextures( int flags ) { return false; }

	void SetViewmodel( C_TDCViewModel *vm );

	virtual void			FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld );
	virtual void			UncorrectViewModelAttachment( Vector &vOrigin );
	virtual bool			IsViewModel() const { return true; }
	virtual RenderGroup_t	GetRenderGroup( void ) { return RENDER_GROUP_VIEW_MODEL_OPAQUE; }
	virtual ShadowType_t	ShadowCastType() { return SHADOWS_NONE; }

	virtual C_BaseEntity	*GetOwnerViaInterface( void );

private:
	CHandle<C_TDCViewModel> m_viewmodel;
};

#endif
