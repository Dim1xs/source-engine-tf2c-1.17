//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TDC_FX_MUZZLEFLASH_H
#define TDC_FX_MUZZLEFLASH_H

// Model versions of muzzle flashes
class C_MuzzleFlashModel : public C_BaseAnimating
{
	DECLARE_CLASS( C_MuzzleFlashModel, C_BaseAnimating );
public:
	static C_MuzzleFlashModel *CreateMuzzleFlashModel( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime = 0.2 );
	bool	InitializeMuzzleFlash( const char *pszModelName, C_BaseEntity *pParent, int iAttachment, float flLifetime );
	void	ClientThink( void );

	void	SetLifetime( float flLifetime );

private:
	float	m_flExpiresAt;
	float	m_flRotateAt;
};

#endif //TDC_FX_MUZZLEFLASH_H