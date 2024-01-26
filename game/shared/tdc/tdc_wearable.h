//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TDC_WEARABLE_H
#define TDC_WEARABLE_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_merc_customizations.h"

class CTDCPlayer;

#ifdef CLIENT_DLL
#define CTDCWearable C_TDCWearable
class C_TDCRagdoll;
#endif

class CTDCWearable : public CBaseAnimating, public IHasOwner
{
public:
	DECLARE_CLASS( CTDCWearable, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CTDCWearable();

	int GetItemID( void ) { return m_iItemID; }
	WearableDef_t *GetStaticData( void );
	CBaseEntity *GetOwnerViaInterface( void ) { return GetOwnerEntity(); }

#ifdef GAME_DLL
	static CTDCWearable *Create( int iItemID, CTDCPlayer *pPlayer );
	virtual int UpdateTransmitState( void );
	virtual void UpdateOnRemove( void );

	void UpdatePlayerBodygroups( void );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual int	InternalDrawModel( int flags );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual int GetSkin( void );
	virtual ShadowType_t ShadowCastType( void );
	virtual bool ShouldDraw( void );

	void CopyToRagdoll( C_TDCRagdoll *pTFRagdoll );
#endif

private:
	CNetworkVar( int, m_iItemID );

	CTDCWearable( const CTDCWearable & );
};

#endif // TDC_WEARABLE_H
