//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tdc_projectile_rocket.h"
#include "particles_new.h"
#include "c_tdc_player.h"
#include "tdc_gamerules.h"

IMPLEMENT_NETWORKCLASS_ALIASED( Projectile_Rocket, DT_Projectile_Rocket )

BEGIN_NETWORK_TABLE( C_Projectile_Rocket, DT_Projectile_Rocket )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Projectile_Rocket::C_Projectile_Rocket( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Projectile_Rocket::~C_Projectile_Rocket( void )
{
	ParticleProp()->StopEmission();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Projectile_Rocket::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateRocketTrails();
	}
	// Watch owner changes and change trail accordingly.
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateRocketTrails();
	}
}

float UTIL_WaterLevel( const Vector &position, float minz, float maxz );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Projectile_Rocket::CreateRocketTrails( void )
{
	if ( IsDormant() )
		return;

	if ( UTIL_PointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		CNewParticleEffect *pParticle = ParticleProp()->Create( "rockettrail_underwater", PATTACH_POINT_FOLLOW, "trail" );

		if ( pParticle )
		{
			Vector vecSurface = GetAbsOrigin();
			vecSurface.z = UTIL_WaterLevel( GetAbsOrigin(), GetAbsOrigin().z, GetAbsOrigin().z + 256 );
			pParticle->SetControlPoint( 1, vecSurface );
		}
	}
	else
	{
		CNewParticleEffect *pParticle = ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, "trail" );
		SetParticleColor( pParticle );
	}

	if ( m_bCritical )
	{
		const char *pszEffectName = ConstructTeamParticle( "critical_rocket_%s", GetTeamNumber(), true );
		CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
		SetParticleColor( pEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_Projectile_Rocket::GetTrailParticleName( void )
{
	if ( !TDCGameRules()->IsTeamplay() )
	{
		return "rockettrail_dm";
	}

	return "rockettrail";
}
