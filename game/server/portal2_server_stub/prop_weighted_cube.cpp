// $
#include "cbase.h"
#include "props.h"

// ses
#include "tier0/memdbgon.h"

class CPropWeightedCube : public CPhysicsProp
{
public:
	DECLARE_CLASS( CPropWeightedCube, CPhysicsProp );
	DECLARE_DATADESC();

	void Spawn()
	{
		SetModel( "models/props/metal_box.mdl" );
		SetSolid( SOLID_VPHYSICS );
		m_nSkin = ( m_iSkin != 0 ? m_iSkin : 0 );
	};

	void Precache()
	{
		PrecacheModel( "models/props/metal_box.mdl" );
	};

	virtual int     ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION; };
	virtual int     OnTakeDamage( const CTakeDamageInfo &info )
	{
		return BaseClass::OnTakeDamage( info );
	};

	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
	{
		BaseClass::VPhysicsCollision( index, pEvent );
	};
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
	{
	        BaseClass::OnPhysGunPickup( pPhysGunUser, reason );

	        if ( reason == PUNTED_BY_CANNON )
	        {
	                Vector vForward;
	                AngleVectors( pPhysGunUser->EyeAngles(), &vForward, NULL, NULL );
	                Vector vForce = Pickup_PhysGunLaunchVelocity( this, vForward, PHYSGUN_FORCE_PUNTED );
	                AngularImpulse angular = AngularImpulse( 0, 0, 0 );

	                IPhysicsObject *pPhysics = VPhysicsGetObject();

	                if ( pPhysics )
	                {
	                        pPhysics->AddVelocity( &vForce, &angular );
	                }

	                TakeDamage( CTakeDamageInfo( pPhysGunUser, pPhysGunUser, GetHealth(), DMG_GENERIC ) );
	        }
	};

private:
	int	m_iSkin;
};

BEGIN_DATADESC( CPropWeightedCube )
	DEFINE_KEYFIELD( m_iSkin, FIELD_INTEGER, "skin" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_weighted_cube, CPropWeightedCube );
