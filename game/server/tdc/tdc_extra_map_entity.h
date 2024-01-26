//=============================================================================
//
// Purpose:
//
//=============================================================================
#ifndef TDC_EXTRA_MAP_ENTITY_H
#define TDC_EXTRA_MAP_ENTITY_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "tdc_shareddefs.h"

class CTDCPointWeaponMimic : public CPointEntity
{
public:
	DECLARE_CLASS( CTDCPointWeaponMimic, CPointEntity );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );

	void GetFiringAngles( QAngle &angShoot );
	void Fire( void );
	void DoFireEffects( void );
	void FireRocket( void );
	void FireGrenade( void );

	void InputFire( inputdata_t &inputdata );
	void InputFireMultiple( inputdata_t &inputdata );

private:
	enum
	{
		TDC_WEAPON_MIMIC_ROCKET,
		TDC_WEAPON_MIMIC_GRENADE,
	};

	int m_iWeaponType;

	string_t m_iszFireSound;
	string_t m_iszFireEffect;
	string_t m_iszModelOverride;
	float m_flModelScale;

	float m_flMinSpeed;
	float m_flMaxSpeed;
	float m_flDamage;
	float m_flRadius;
	float m_flSpread;
	float m_flTimer;
	bool m_bCritical;
};

class CFuncJumpPad : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFuncJumpPad, CBaseTrigger );
	DECLARE_DATADESC();

	CFuncJumpPad();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );

private:
	QAngle m_angPush;
	Vector m_vecPushDir;
	float m_flPushForce;
	float m_flClampSpeed;
	string_t m_iszJumpSound;

	COutputEvent m_OnJump;
};

#endif // TDC_EXTRA_MAP_ENTITY_H
