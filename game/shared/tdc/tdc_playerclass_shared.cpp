//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#include "cbase.h"
#include "KeyValues.h"
#include "tdc_playerclass_shared.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier2/tier2.h"
#include "baseplayer_shared.h"
#include "tdc_gamerules.h"

#ifdef CLIENT_DLL
bool UseHWMorphModels();
#endif

TDCPlayerClassData_t s_aTFPlayerClassData[TDC_CLASS_COUNT_ALL];

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TDCPlayerClassData_t::TDCPlayerClassData_t()
{
	m_iClass = TDC_CLASS_UNDEFINED;
	m_szClassName[0] = '\0';
	m_szModelName[0] = '\0';
	m_szHWMModelName[0] = '\0';
	m_szModelHandsName[0] = '\0';
	m_szLocalizableName[0] = '\0';
	m_flMaxSpeed = 0.0f;
	m_flFlagSpeed = 0.0f;
	m_nMaxHealth = 0;
	m_flSelfDamageScale = 0.0f;
	m_flDamageForceScale = 0.0f;
	m_flStompDamageMult = 0.0f;
	m_iMeleeWeapon = WEAPON_NONE;

#ifdef GAME_DLL
	m_szDeathSound[0] = '\0';
	m_szCritDeathSound[0] = '\0';
	m_szMeleeDeathSound[0] = '\0';
	m_szExplosionDeathSound[0] = '\0';
#endif

	m_szJumpSound[0] = '\0';

	for ( int iAmmo = 0; iAmmo < TDC_AMMO_COUNT; ++iAmmo )
	{
		m_aAmmoMax[iAmmo] = TDC_AMMO_DUMMY;
	}

	m_bParsed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TDCPlayerClassData_t::Parse( int iClass )
{
	// Have we parsed this file already?
	if ( m_bParsed )
		return;

	// No filesystem at this point????  Hmmmm......

	// Parse class file.
	m_iClass = iClass;

	const unsigned char *pKey = NULL;

	if ( g_pGameRules )
	{
		pKey = g_pGameRules->GetEncryptionKey();
	}

	char szClassFile[MAX_PATH];
	V_sprintf_safe( szClassFile, "scripts/playerclasses/%s", g_aPlayerClassNames_NonLocalized[iClass] );
	KeyValues *pKV = ReadEncryptedKVFile( filesystem, szClassFile, pKey );
	if ( pKV )
	{
		ParseData( pKV );
		pKV->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TDCPlayerClassData_t::GetModelName() const
{
#ifdef CLIENT_DLL
	if ( UseHWMorphModels() )
	{
		if ( m_szHWMModelName[0] != '\0' )
		{
			return m_szHWMModelName;
		}
	}

	return m_szModelName;
#else
	return m_szModelName;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TDCPlayerClassData_t::ParseData( KeyValues *pKeyValuesData )
{
	// Attributes.
	V_strcpy_safe( m_szClassName, pKeyValuesData->GetString( "name" ) );

	// Load the high res model or the lower res model.
	V_strcpy_safe( m_szHWMModelName, pKeyValuesData->GetString( "model_hwm" ) );
	V_strcpy_safe( m_szModelName, pKeyValuesData->GetString( "model" ) );
	V_strcpy_safe( m_szModelHandsName, pKeyValuesData->GetString("model_hands") );
	V_strcpy_safe( m_szLocalizableName, pKeyValuesData->GetString( "localize_name" ) );

	m_flMaxSpeed = pKeyValuesData->GetFloat( "speed_max" );
	m_flFlagSpeed = pKeyValuesData->GetFloat( "flagspeed_max", m_flMaxSpeed );
	m_nMaxHealth = pKeyValuesData->GetInt( "health_max" );
	m_flSelfDamageScale = pKeyValuesData->GetFloat( "selfdamage_scale", 1.0f );
	m_flDamageForceScale = pKeyValuesData->GetFloat( "damageforce_scale", 1.0f );
	m_flViewHeight = pKeyValuesData->GetFloat( "view_height", 72.0f );
	m_flStompDamageMult = pKeyValuesData->GetFloat( "stompdamage_scale", 10.0f );
	m_iMeleeWeapon = GetWeaponId( pKeyValuesData->GetString( "melee_weapon" ) );

	// Ammo Max.
	KeyValues *pAmmoKeyValuesData = pKeyValuesData->FindKey( "AmmoMax" );
	if ( pAmmoKeyValuesData )
	{
		for ( int iAmmo = 1; iAmmo < TDC_AMMO_COUNT; ++iAmmo )
		{
			m_aAmmoMax[iAmmo] = pAmmoKeyValuesData->GetInt( g_aAmmoNames[iAmmo], 0 );
		}
	}

#ifdef GAME_DLL		// right now we only emit these sounds from server. if that changes we can do this in both dlls

	// Death Sounds
	V_strcpy_safe( m_szDeathSound, pKeyValuesData->GetString( "sound_death", "Player.Death" ) );
	V_strcpy_safe( m_szCritDeathSound, pKeyValuesData->GetString( "sound_crit_death", "Player.CritDeath" ) );
	V_strcpy_safe( m_szMeleeDeathSound, pKeyValuesData->GetString( "sound_melee_death", "Player.MeleeDeath" ) );
	V_strcpy_safe( m_szExplosionDeathSound, pKeyValuesData->GetString( "sound_explosion_death", "Player.ExplosionDeath" ) );

#endif

	V_strcpy_safe( m_szJumpSound, pKeyValuesData->GetString( "sound_jump", "Player.Jump" ) );

	// The file has been parsed.
	m_bParsed = true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class data (keep a cache).
//-----------------------------------------------------------------------------
void InitPlayerClasses( void )
{
	// Special case the undefined class.
	TDCPlayerClassData_t *pClassData = &s_aTFPlayerClassData[TDC_CLASS_UNDEFINED];
	Assert( pClassData );
	V_strcpy_safe( pClassData->m_szClassName, "undefined" );
	V_strcpy_safe( pClassData->m_szModelName, "models/player/grunt_m.mdl" );	// Undefined players still need a model
	V_strcpy_safe( pClassData->m_szModelHandsName, "models/weapons/c_models/c_merc_arms.mdl" );
	V_strcpy_safe( pClassData->m_szLocalizableName, "undefined" );

	// Initialize the classes.
	for ( int iClass = 1; iClass < TDC_CLASS_COUNT_ALL; ++iClass )
	{
		TDCPlayerClassData_t *pClassData = &s_aTFPlayerClassData[iClass];
		Assert( pClassData );
		pClassData->Parse( iClass );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to get player class data.
//-----------------------------------------------------------------------------
TDCPlayerClassData_t *GetPlayerClassData( int iClass )
{
	Assert ( ( iClass >= 0 ) && ( iClass < TDC_CLASS_COUNT_ALL ) );
	return &s_aTFPlayerClassData[iClass];
}

//=============================================================================
//
// Shared player class data.
//

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTDCPlayerClassShared, DT_TDCPlayerClassShared )
	RecvPropInt( RECVINFO( m_iClass ) ),
END_RECV_TABLE()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTDCPlayerClassShared, DT_TDCPlayerClassShared )
	SendPropInt( SENDINFO( m_iClass ), Q_log2( TDC_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
END_SEND_TABLE()

#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCPlayerClassShared::CTDCPlayerClassShared()
{
	m_iClass.Set( TDC_CLASS_UNDEFINED );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class.
//-----------------------------------------------------------------------------
bool CTDCPlayerClassShared::Init( int iClass )
{
	Assert ( ( iClass >= TDC_FIRST_NORMAL_CLASS ) && ( iClass < TDC_CLASS_COUNT_ALL ) );
	m_iClass = iClass;
	return true;
}
