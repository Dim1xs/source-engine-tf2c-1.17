//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose:
//
//=============================================================================
#ifndef TDC_PLAYERCLASS_SHARED_H
#define TDC_PLAYERCLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"

#define TDC_NAME_LENGTH		128

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TDCPlayerClassShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TDCPlayerClassShared );

#endif


//-----------------------------------------------------------------------------
// Cache structure for the TF player class data (includes citizen). 
//-----------------------------------------------------------------------------

#define MAX_PLAYERCLASS_SOUND_LENGTH	128

struct TDCPlayerClassData_t
{
	char		m_szClassName[TDC_NAME_LENGTH];
	char		m_szModelName[TDC_NAME_LENGTH];
	char		m_szHWMModelName[TDC_NAME_LENGTH];
	char		m_szModelHandsName[TDC_NAME_LENGTH];
	char		m_szLocalizableName[TDC_NAME_LENGTH];
	float		m_flMaxSpeed;
	float		m_flFlagSpeed;
	int			m_nMaxHealth;
	float		m_flSelfDamageScale;
	float		m_flDamageForceScale;
	float		m_flViewHeight;
	float		m_flStompDamageMult;
	int			m_aAmmoMax[TDC_AMMO_COUNT];
	ETDCWeaponID	m_iMeleeWeapon;

	bool		m_bParsed;

#ifdef GAME_DLL
	// sounds
	char		m_szDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szCritDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szMeleeDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szExplosionDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
#endif

	char		m_szJumpSound[MAX_PLAYERCLASS_SOUND_LENGTH];

	TDCPlayerClassData_t();
	const char *GetModelName() const;
	void Parse( int iClass );

private:

	// Parser for the class data.
	void ParseData( KeyValues *pKeyValuesData );

	int m_iClass;
};

void InitPlayerClasses( void );
TDCPlayerClassData_t *GetPlayerClassData( int iClass );

//-----------------------------------------------------------------------------
// TF Player Class Shared
//-----------------------------------------------------------------------------
class CTDCPlayerClassShared
{
public:

	CTDCPlayerClassShared();

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTDCPlayerClassShared );

	bool		Init( int iClass );
	bool		IsClass( int iClass ) const						{ return ( m_iClass == iClass ); }
	int			GetClassIndex( void ) const						{ return m_iClass; }

	const char	*GetName( void ) const							{ return GetPlayerClassData( m_iClass )->m_szClassName; }
	const char	*GetModelName( void ) const						{ return GetPlayerClassData( m_iClass )->GetModelName(); }	
	const char	*GetHandModelName( void ) const					{ return GetPlayerClassData( m_iClass )->m_szModelHandsName; }		
	float		GetMaxSpeed( void ) const						{ return GetPlayerClassData( m_iClass )->m_flMaxSpeed; }
	int			GetMaxHealth( void ) const						{ return GetPlayerClassData( m_iClass )->m_nMaxHealth; }

	TDCPlayerClassData_t  *GetData( void ) const					{ return GetPlayerClassData( m_iClass ); }

protected:

	CNetworkVar( int,	m_iClass );
};

#endif // TDC_PLAYERCLASS_SHARED_H