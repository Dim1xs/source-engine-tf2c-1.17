//=============================================================================
//
// Purpose:
//
//=============================================================================
#include "cbase.h"
#include "tdc_extra_map_entity.h"
#include "tdc_player.h"
#include "tdc_projectile_rocket.h"
#include "tdc_projectile_pipebomb.h"
#include "tdc_projectile_arrow.h"
#include "particle_parse.h"

BEGIN_DATADESC( CTDCPointWeaponMimic )
	DEFINE_KEYFIELD( m_iWeaponType, FIELD_INTEGER, "WeaponType" ),
	DEFINE_KEYFIELD( m_iszFireSound, FIELD_STRING, "FireSound" ),
	DEFINE_KEYFIELD( m_iszFireEffect, FIELD_STRING, "ParticleEffect" ),
	DEFINE_KEYFIELD( m_iszModelOverride, FIELD_STRING, "ModelOverride" ),
	DEFINE_KEYFIELD( m_flModelScale, FIELD_FLOAT, "ModelScale" ),
	DEFINE_KEYFIELD( m_flMinSpeed, FIELD_FLOAT, "SpeedMin" ),
	DEFINE_KEYFIELD( m_flMaxSpeed, FIELD_FLOAT, "SpeedMax" ),
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "SplashRadius" ),
	DEFINE_KEYFIELD( m_flSpread, FIELD_FLOAT, "SpreadAngle" ),
	DEFINE_KEYFIELD( m_flTimer, FIELD_FLOAT, "DetonationTimer" ),
	DEFINE_KEYFIELD( m_bCritical, FIELD_BOOLEAN, "Crits" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "FireOnce", InputFire ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "FireMultiple", InputFireMultiple ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( point_weapon_mimic, CTDCPointWeaponMimic );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	ChangeTeam( TDC_TEAM_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::Precache( void )
{
	BaseClass::Precache();

	if ( m_iszFireSound != NULL_STRING )
		PrecacheScriptSound( STRING( m_iszFireSound ) );

	if ( m_iszFireEffect != NULL_STRING )
		PrecacheParticleSystem( STRING( m_iszFireEffect ) );

	if ( m_iszModelOverride != NULL_STRING )
		PrecacheModel( STRING( m_iszModelOverride ) );

	PrecacheScriptSound( "Weapon_StickyBombLauncher.ModeSwitch" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::GetFiringAngles( QAngle &angShoot )
{
	if ( m_flSpread == 0.0f )
	{
		angShoot = GetAbsAngles();
		return;
	}

	Vector vecForward, vecRight, vecUp;
	AngleVectors( GetAbsAngles(), &vecForward, &vecRight, &vecUp );

	float flSpreadMult = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
	VMatrix mat1 = SetupMatrixAxisRot( vecForward, RandomFloat( -180.0f, 180.0f ) );
	VMatrix mat2 = SetupMatrixAxisRot( vecUp, m_flSpread * flSpreadMult );

	VMatrix matResult;
	MatrixMultiply( mat1, mat2, matResult );
	MatrixToAngles( matResult, angShoot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::Fire( void )
{
	switch( m_iWeaponType )
	{
	case TDC_WEAPON_MIMIC_ROCKET:
		FireRocket();
		break;
	case TDC_WEAPON_MIMIC_GRENADE:
		FireGrenade();
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::DoFireEffects( void )
{
	if ( m_iszFireSound != NULL_STRING )
		EmitSound( STRING( m_iszFireSound ) );

	if ( m_iszFireEffect != NULL_STRING )
		DispatchParticleEffect( STRING( m_iszFireEffect ), GetAbsOrigin(), GetAbsAngles() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::FireRocket( void )
{
	QAngle angForward;
	GetFiringAngles( angForward );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( angForward, &vecForward );
	float flSpeed = RandomFloat( m_flMinSpeed, m_flMaxSpeed );
	Vector vecVelocity = vecForward * flSpeed;

	CTDCBaseRocket *pRocket = CProjectile_Rocket::Create( GetAbsOrigin(), vecVelocity, m_flDamage, m_flRadius, m_bCritical, this, NULL );

	if ( pRocket )
	{
		if ( m_iszModelOverride != NULL_STRING )
			pRocket->SetModel( STRING( m_iszModelOverride ) );

		pRocket->SetModelScale( m_flModelScale );
		pRocket->SetCollisionGroup( TDC_COLLISION_GROUP_ROCKETS_NOTSOLID );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::FireGrenade( void )
{
	QAngle angForward;
	GetFiringAngles( angForward );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( angForward, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = ( vecForward * RandomFloat( m_flMinSpeed, m_flMaxSpeed ) ) +
		( vecUp * 200.0f ) +
		( RandomFloat( -10.0f, 10.0f ) * vecRight ) +
		( RandomFloat( -10.0f, 10.0f ) * vecUp );

	AngularImpulse angVelocity( 600, RandomInt( -1200, 1200 ), 0 );

	CTDCBaseGrenade *pProjectile = CProjectile_Pipebomb::Create( GetAbsOrigin(), angForward,
		vecVelocity, angVelocity, m_flDamage, m_flRadius, m_bCritical, m_flTimer,
		this, NULL );

	if ( pProjectile )
	{
		if ( m_iszModelOverride != NULL_STRING )
		{
			pProjectile->VPhysicsDestroyObject();
			pProjectile->SetModel( STRING( m_iszModelOverride ) );
			pProjectile->VPhysicsInitNormal( SOLID_BBOX, FSOLID_TRIGGER, false );
		}

		pProjectile->SetModelScale( m_flModelScale );
		pProjectile->SetCollisionGroup( TDC_COLLISION_GROUP_ROCKETS );

		pProjectile->SetDamage( m_flDamage );
		pProjectile->SetCritical( m_bCritical );
		pProjectile->SetDamageRadius( m_flRadius );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::InputFire( inputdata_t &inputdata )
{
	Fire();
	DoFireEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPointWeaponMimic::InputFireMultiple( inputdata_t &inputdata )
{
	int count = inputdata.value.Int();
	for ( int i = 0; i < count; i++ )
	{
		Fire();
	}

	DoFireEffects();
}

BEGIN_DATADESC( CFuncJumpPad )
	DEFINE_KEYFIELD( m_angPush, FIELD_VECTOR, "impulse_dir" ),
	DEFINE_KEYFIELD( m_flPushForce, FIELD_FLOAT, "force" ),
	DEFINE_KEYFIELD( m_flClampSpeed, FIELD_FLOAT, "clampspeed" ),
	DEFINE_KEYFIELD( m_iszJumpSound, FIELD_SOUNDNAME, "jumpsound" ),
	
	DEFINE_OUTPUT( m_OnJump, "OnJump" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_jumppad, CFuncJumpPad );

CFuncJumpPad::CFuncJumpPad()
{
	m_angPush.Init();
	m_flPushForce = 0.0f;
	m_flClampSpeed = 0.0f;
	m_iszJumpSound = NULL_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncJumpPad::Precache( void )
{
	BaseClass::Precache();

	if ( m_iszJumpSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszJumpSound ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncJumpPad::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	// Convert push angle to vector and transform it into entity space.
	Vector vecDir;
	AngleVectors( m_angPush, &vecDir );
	VectorIRotate( vecDir, EntityToWorldTransform(), m_vecPushDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncJumpPad::StartTouch( CBaseEntity *pOther )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( pOther );
	if ( !pPlayer )
		return;

	// Transform the vector back into world space.
	Vector vecAbsDir, vecVelocity, vecForce;
	VectorRotate( m_vecPushDir, EntityToWorldTransform(), vecAbsDir );
	vecForce = vecAbsDir * m_flPushForce;

	// Figure out how much velocity to keep.
	vecVelocity = pPlayer->GetAbsVelocity();
	vecVelocity.z = 0.0f;

	if ( m_flClampSpeed == 0.0f )
	{
		vecVelocity = vec3_origin;
	}
	else if ( m_flClampSpeed != -1.0f )
	{
		float flSpeed = VectorLength( vecVelocity );
		if ( flSpeed > m_flClampSpeed )
		{
			vecVelocity *= m_flClampSpeed / flSpeed;
		}
	}

	vecVelocity += vecForce;
	pPlayer->SetAbsVelocity( vecVelocity );

	if ( m_iszJumpSound != NULL_STRING )
	{
		EmitSound( STRING( m_iszJumpSound ) );
	}

	m_OnJump.FireOutput( pPlayer, this );
}
