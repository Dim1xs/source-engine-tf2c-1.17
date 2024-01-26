//=============================================================================
//
// Purpose: RC Bomb
//
//=============================================================================
#ifndef TDC_PROJECTILE_REMOTEBOMB_H
#define TDC_PROJECTILE_REMOTEBOMB_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_weaponbase_grenadeproj.h"

#ifdef CLIENT_DLL
#define CProjectile_RemoteBomb C_Projectile_RemoteBomb
#endif

#define TDC_REMOTEBOMB_LIVETIME 0.8f

class CProjectile_RemoteBomb : public CTDCBaseGrenade
{
public:
	DECLARE_CLASS( CProjectile_RemoteBomb, CTDCBaseGrenade );
	DECLARE_NETWORKCLASS();

	CProjectile_RemoteBomb();
	~CProjectile_RemoteBomb();

	// Unique identifier.
	virtual ETDCWeaponID	GetWeaponID( void ) const;

#ifdef GAME_DLL
	static CProjectile_RemoteBomb *Create( const Vector &position, const QAngle &angles,
		const Vector &velocity, const AngularImpulse &angVelocity,
		float flDamage, float flRadius, bool bCritical, float flLiveDelay,
		CBaseEntity *pOwner, CBaseEntity *pWeapon );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int UpdateTransmitState( void );
	virtual int ShouldTransmit( CCheckTransmitInfo *pInfo );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void Detonate( void );
	virtual void UpdateOnRemove( void );
	
	float GetImpactDamage() { return 10.0f; }

	void Fizzle( void );

	void Hit( CBaseEntity *pOther );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void Simulate( void );

	void CreateTrails( void );
#endif

private:
	CNetworkVar( float, m_flLiveTime );

#ifdef GAME_DLL
	bool m_bTouched;
	bool m_bFizzle;
#else
	bool m_bPulsed;
#endif
};

#endif // TDC_PROJECTILE_REMOTEBOMB_H
