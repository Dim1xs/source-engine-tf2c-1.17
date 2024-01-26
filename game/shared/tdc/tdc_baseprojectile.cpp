//=============================================================================//
//
// Purpose: TF2 projectile base.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_baseprojectile.h"

#ifdef CLIENT_DLL
#include "tdc_gamerules.h"
#include "particles_new.h"
#include "c_te_effect_dispatch.h"
#include "c_tdc_player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TDCBaseProjectile, DT_TDCBaseProjectile );
BEGIN_NETWORK_TABLE( CTDCBaseProjectile, DT_TDCBaseProjectile )
END_NETWORK_TABLE()

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ETDCWeaponID CTDCBaseProjectile::GetWeaponID( void ) const
{
	// Derived classes should override this.
	Assert( false );
	return WEAPON_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseProjectile::Spawn( void )
{
	BaseClass::Spawn();
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTDCBaseProjectile::GetSkin( void )
{
	if ( HasTeamSkins() )
	{
		return GetTeamSkin( GetTeamNumber() );
	}

	return BaseClass::GetSkin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCBaseProjectile::SetParticleColor( CNewParticleEffect *pEffect )
{
	C_TDCPlayer *pOwner = ToTDCPlayer( GetOwnerEntity() );

	if ( pEffect && pOwner && !TDCGameRules()->IsTeamplay() )
	{
		pEffect->SetControlPoint( CUSTOM_COLOR_CP1, pOwner->m_vecPlayerColor );
	}
}
#endif
