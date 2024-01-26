//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include <KeyValues.h>
#include "tdc_weapon_parse.h"
#include "tdc_shareddefs.h"
#include "tdc_playerclass_shared.h"
#include "activitylist.h"
#include "tdc_gamerules.h"

const char *g_aExplosionNames[TDC_EXPLOSION_COUNT] =
{
	"ExplosionEffect",
	"ExplosionPlayerEffect",
	"ExplosionWaterEffect",
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CTDCWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTDCWeaponInfo::CTDCWeaponInfo()
{
	m_WeaponData[0].Init();
	m_WeaponData[1].Init();

	m_flDamageRadius = 0.0f;
	m_flPrimerTime = 0.0f;

	m_szMuzzleFlashParticleEffect[0] = '\0';

	m_szTracerEffect[0] = '\0';
	m_iTracerFreq = 0;

	m_szBrassModel[0] = '\0';
	m_bDoInstantEjectBrass = true;

	m_szMagazineModel[0] = '\0';

	m_szExplosionSound[0] = '\0';
	memset( m_szExplosionEffects, 0, sizeof( m_szExplosionEffects ) );
	m_bHasTeamColoredExplosions = false;
	m_bHasCritExplosions = false;

	m_iMaxAmmo = 0;
	m_iSpawnAmmo = 0;
	m_bAlwaysDrop = false;
	m_bUseHands = true;
	m_bCanHeadshot = false;
	m_bUseNewActTable = false;

	m_szAmmoIcon[0] = '\0';
	m_szTimerIconFull[0] = '\0';
	m_szTimerIconEmpty[0] = '\0';
	m_cTimerIconFullColor = Color( 255, 255, 255, 255 );
	m_cTimerIconEmptyColor = Color( 255, 255, 255, 255 );
	m_flOptimalRange = 0.0f;

	m_szViewModelHeavy[0] = '\0';
	m_szViewModelLight[0] = '\0';
}

CTDCWeaponInfo::~CTDCWeaponInfo()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{

	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// Primary fire mode.
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_nDamage					= pKeyValuesData->GetInt( "Damage", 0 );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flRange					= pKeyValuesData->GetFloat( "Range", 8192.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_nBulletsPerShot			= pKeyValuesData->GetInt( "BulletsPerShot", 0 );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_nBurstSize				= pKeyValuesData->GetInt( "BurstSize", 0 );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flBurstDelay				= pKeyValuesData->GetFloat( "BurstDelay", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flSpread					= pKeyValuesData->GetFloat( "Spread", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flPunchAngle				= pKeyValuesData->GetFloat( "PunchAngle", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeFireDelay			= pKeyValuesData->GetFloat( "TimeFireDelay", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeIdle				= pKeyValuesData->GetFloat( "TimeIdle", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty			= pKeyValuesData->GetFloat( "TimeIdleEmpy", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeReloadStart		= pKeyValuesData->GetFloat( "TimeReloadStart", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeReload				= pKeyValuesData->GetFloat( "TimeReload", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeReloadRefill		= pKeyValuesData->GetFloat( "TimeReloadRefill", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_bDrawCrosshair			= pKeyValuesData->GetInt( "DrawCrosshair", 1 ) > 0;
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_iAmmoPerShot				= pKeyValuesData->GetInt( "AmmoPerShot", 1 );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flProjectileSpeed		= pKeyValuesData->GetFloat( "ProjectileSpeed", 0.0f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flSmackDelay				= pKeyValuesData->GetFloat( "SmackDelay", 0.2f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flDeployTime				= pKeyValuesData->GetFloat( "DeployTime", 0.5f );
	m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_bDropMagOnReload				= pKeyValuesData->GetBool( "DropMagOnReload", false );

	m_bDoInstantEjectBrass = ( pKeyValuesData->GetInt( "DoInstantEjectBrass", 1 ) != 0 );
	const char *pszBrassModel = pKeyValuesData->GetString( "BrassModel", NULL );
	if ( pszBrassModel )
	{
		V_strcpy_safe( m_szBrassModel, pszBrassModel );
	}

	// Secondary fire mode.
	// Inherit from primary fire mode
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_nDamage				= pKeyValuesData->GetInt( "Secondary_Damage", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_nDamage );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flRange				= pKeyValuesData->GetFloat( "Secondary_Range", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flRange );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_nBulletsPerShot		= pKeyValuesData->GetInt( "Secondary_BulletsPerShot", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_nBulletsPerShot );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flSpread				= pKeyValuesData->GetFloat( "Secondary_Spread", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flSpread );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flPunchAngle			= pKeyValuesData->GetFloat( "Secondary_PunchAngle", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flPunchAngle );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flTimeFireDelay		= pKeyValuesData->GetFloat( "Secondary_TimeFireDelay", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeFireDelay );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flTimeIdle				= pKeyValuesData->GetFloat( "Secondary_TimeIdle", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeIdle );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flTimeIdleEmpty		= pKeyValuesData->GetFloat( "Secondary_TimeIdleEmpy", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flTimeReloadStart		= pKeyValuesData->GetFloat( "Secondary_TimeReloadStart", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeReloadStart );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flTimeReload			= pKeyValuesData->GetFloat( "Secondary_TimeReload", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_flTimeReload );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flTimeReloadRefill		= pKeyValuesData->GetFloat( "Secondary_TimeReloadRefill", 0.0f );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_bDrawCrosshair			= pKeyValuesData->GetInt( "Secondary_DrawCrosshair", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_bDrawCrosshair ) > 0;
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_iAmmoPerShot			= pKeyValuesData->GetInt( "Secondary_AmmoPerShot", m_WeaponData[TDC_WEAPON_PRIMARY_MODE].m_iAmmoPerShot );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flProjectileSpeed		= pKeyValuesData->GetFloat( "Secondary_ProjectileSpeed", 0.0f );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flSmackDelay			= pKeyValuesData->GetFloat( "Secondary_SmackDelay", 0.2f );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_flDeployTime			= pKeyValuesData->GetFloat( "Secondary_DeployTime", 0.5f );
	m_WeaponData[TDC_WEAPON_SECONDARY_MODE].m_bDropMagOnReload		= pKeyValuesData->GetBool( "Secondary_DropMagOnReload", false );

	// Grenade data.
	m_flDamageRadius		= pKeyValuesData->GetFloat( "DamageRadius", 0.0f );
	m_flPrimerTime			= pKeyValuesData->GetFloat( "PrimerTime", 0.0f );

	// Model muzzleflash
	const char *pszMuzzleFlashParticleEffect = pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL );

	if ( pszMuzzleFlashParticleEffect )
	{
		V_strcpy_safe( m_szMuzzleFlashParticleEffect, pszMuzzleFlashParticleEffect );
	}

	// Tracer particle effect
	const char *pszTracerEffect = pKeyValuesData->GetString( "TracerEffect", NULL );

	if ( pszTracerEffect )
	{
		V_strcpy_safe( m_szTracerEffect, pszTracerEffect );
	}

	m_iTracerFreq = pKeyValuesData->GetInt( "TracerFreq", 2 );

	const char *pszMagModel = pKeyValuesData->GetString( "MagazineModel", NULL );

	if ( pszMagModel )
	{
		V_strcpy_safe( m_szMagazineModel, pszMagModel );
	}

	// Icon used for weapon spawner circle loading thing
	const char *pszTimerIconFull = pKeyValuesData->GetString( "TimerImageFull", NULL );

	if ( pszTimerIconFull )
	{
		V_strcpy_safe( m_szTimerIconFull, pszTimerIconFull );
	}

	const char *pszTimerIconEmpty = pKeyValuesData->GetString( "TimerImageEmpty", NULL );

	if ( pszTimerIconEmpty )
	{
		V_strcpy_safe( m_szTimerIconEmpty, pszTimerIconEmpty );
	}

	if ( !pKeyValuesData->IsEmpty( "TimerImageFullColor" ) )
	{
		m_cTimerIconFullColor = pKeyValuesData->GetColor( "TimerImageFullColor" );
		//m_cTimerIconEmptyColor = m_cTimerIconFullColor;
	}

	if ( !pKeyValuesData->IsEmpty( "TimerImageEmptyColor" ) )
	{
		m_cTimerIconEmptyColor = pKeyValuesData->GetColor( "TimerImageEmptyColor" );
	}

	// Weapon icon used for the HUD
	const char *pszAmmoIcon = pKeyValuesData->GetString( "AmmoIcon", NULL );

	if ( pszAmmoIcon )
	{
		V_strcpy_safe( m_szAmmoIcon, pszAmmoIcon );
	}


	// Explosion effects (used for grenades)
	const char *pszSound = pKeyValuesData->GetString( "ExplosionSound", NULL );
	if ( pszSound )
	{
		V_strcpy_safe( m_szExplosionSound, pszSound );
	}

	for ( int i = 0; i < TDC_EXPLOSION_COUNT; i++ )
	{
		const char *pszEffect = pKeyValuesData->GetString( g_aExplosionNames[i], NULL );
		if ( pszEffect )
		{
			V_strcpy_safe( m_szExplosionEffects[i], pszEffect );
		}
	}

	m_bHasTeamColoredExplosions = pKeyValuesData->GetBool( "HasTeamColoredExplosions" );
	m_bHasCritExplosions = pKeyValuesData->GetBool( "HasCritExplosions" );

	m_bDontDrop = ( pKeyValuesData->GetInt( "DontDrop", 0 ) > 0 );

	m_iMaxAmmo = pKeyValuesData->GetInt( "MaxAmmo", 0 );
	m_iSpawnAmmo = pKeyValuesData->GetInt( "SpawnAmmo", 0 );
	m_bAlwaysDrop = pKeyValuesData->GetBool( "AlwaysDrop" );
	m_bUseHands = pKeyValuesData->GetBool( "UseHands", true );
	m_bCanHeadshot = pKeyValuesData->GetBool( "CanHeadshot", false );
	m_bUseNewActTable = pKeyValuesData->GetBool( "UseNewActTable", false );
	m_flOptimalRange = pKeyValuesData->GetFloat( "OptimalRange", 1024.0f );

	const char *pszViewModelHeavy = pKeyValuesData->GetString( "viewmodel_heavy", NULL );
	if ( pszViewModelHeavy )
	{
		V_strcpy_safe( m_szViewModelHeavy, pszViewModelHeavy );
	}

	const char *pszViewModelLight = pKeyValuesData->GetString( "viewmodel_light", NULL );
	if ( pszViewModelLight )
	{
		V_strcpy_safe( m_szViewModelLight, pszViewModelLight );
	}
}
