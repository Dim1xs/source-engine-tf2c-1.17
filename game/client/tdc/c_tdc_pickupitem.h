//=============================================================================//
//
// Purpose: CTDC AmmoPack.
//
//=============================================================================//
#ifndef C_TDC_POWERUP_H
#define C_TDC_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TDCPickupItem : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_TDCPickupItem, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_TDCPickupItem();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual int DrawModel( int flags );

	virtual bool ShouldShowRespawnTimer( void ) { return false; }
	virtual float GetRespawnTimerSize( void ) { return 20.0f; }
	virtual Color GetRespawnTimerFullColor( void ) { return COLOR_WHITE; }
	virtual Color GetRespawnTimerEmptyColor( void ) { return COLOR_WHITE; }

	virtual CMaterialReference GetReturnProgressMaterial_Empty( void ) { return m_ReturnProgressMaterial_Empty; }
	virtual CMaterialReference GetReturnProgressMaterial_Full( void ) { return m_ReturnProgressMaterial_Full; }
	virtual void SetReturnProgressMaterial_Empty( const char* pMaterialName ) { m_ReturnProgressMaterial_Empty.Init( pMaterialName, TEXTURE_GROUP_VGUI ); }
	virtual void SetReturnProgressMaterial_Full( const char* pMaterialName ) { m_ReturnProgressMaterial_Full.Init( pMaterialName, TEXTURE_GROUP_VGUI ); }

	ShadowType_t ShadowCastType( void ) ;

protected:
	bool m_bDisabled;
	bool m_bRespawning;
	float m_flRespawnStartTime;
	float m_flRespawnTime;

private:
	CMaterialReference m_ReturnProgressMaterial_Empty;
	CMaterialReference m_ReturnProgressMaterial_Full;
};

#endif // C_TDC_POWERUP_H
