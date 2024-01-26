//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tdc_shareddefs.h"
#include "KeyValues.h"
#include "takedamageinfo.h"
#include "tdc_gamerules.h"

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames[TDC_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue",
};

// These two arrays should always be used when constructing team specific names (e.g. particle names).
const char *g_aTeamNamesShort[TDC_TEAM_COUNT] =
{
	"unassigned",
	"spectator",
	"red",
	"blu",
};

const char *g_aTeamLowerNames[TDC_TEAM_COUNT] =
{
	"unassigned",
	"spectator",
	"red",
	"blue",
};

Vector g_aTeamParticleColors[TDC_TEAM_COUNT] =
{
	Vector( 1, 1, 1 ),
	Vector( 1, 1, 1 ),
	Vector( 1.0f, 0.2f, 0.15f ),
	Vector( 0.2f, 0.4f, 1.0f ),
};

const char *GetTeamSuffix( int iTeam, bool bDeathmatchOverride /*= false*/, const char **pNames /*= g_aTeamLowerNames*/, const char *pszNeutralName /*= NULL*/ )
{
	if ( bDeathmatchOverride && TDCGameRules() && !TDCGameRules()->IsTeamplay() )
	{
		return "dm";
	}

	// Most effects and textures don't have versions for Unassigned and Spectator teams for obvious reasons so we need a fallback.
	// This is also used for HUD since texture artists can't seem to agree on what suffix should be used for unassigned CPs and such.
	if ( iTeam < FIRST_GAME_TEAM && pszNeutralName )
	{
		return pszNeutralName;
	}

	return pNames[iTeam];
}

const char *ConstructTeamParticle( const char *pszFormat, int iTeam, bool bDeathmatchOverride /*= false*/, const char **pNames /*= g_aTeamLowerNames*/ )
{
	static char szParticleName[128];

	// "red" by default because people will spawn unassigned buildings through console and then complain about missing particles.
	V_sprintf_safe( szParticleName, pszFormat, GetTeamSuffix( iTeam, bDeathmatchOverride, pNames, "red" ) );
	return szParticleName;
}

void PrecacheTeamParticles( const char *pszFormat, bool bFFAEnabled /*= false*/, const char **pNames /*= g_aTeamLowerNames*/ )
{
	ForEachTeamName( [=]( const char *pszTeam )
	{
		char szParticle[128];
		V_snprintf( szParticle, sizeof( szParticle ), pszFormat, pszTeam );
		PrecacheParticleSystem( szParticle );
	}, bFFAEnabled, pNames );
}

int GetTeamSkin( int iTeam, bool bInGame /*= true*/ )
{
	if ( bInGame )
	{
		if ( TDCGameRules() && !TDCGameRules()->IsTeamplay() )
			return 2;
	}
	else
	{
		if ( iTeam == TEAM_UNASSIGNED )
			return 2;
	}

	switch ( iTeam )
	{
	case TDC_TEAM_RED:
		return 0;
	case TDC_TEAM_BLUE:
		return 1;
	}

	return 0;
}

color32 g_aTeamColors[TDC_TEAM_COUNT] = 
{
	{ 0, 0, 0, 0 }, // Unassigned
	{ 0, 0, 0, 0 }, // Spectator
	{ 255, 0, 0, 0 }, // Red
	{ 0, 0, 255, 0 }, // Blue
};

Color g_aTeamHUDColors[TDC_TEAM_COUNT] =
{
	Color( 245, 229, 196, 255 ),
	Color( 245, 229, 196, 255 ),
	Color( 191, 55, 55, 255 ),
	Color( 79, 117, 143, 255 ),
};

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

const char *g_aPlayerClassNames[TDC_CLASS_COUNT_ALL] =
{
	"#TDC_Class_Name_Undefined",
	"#TDC_Class_Name_Grunt_Normal",
	"#TDC_Class_Name_Grunt_Light",
	"#TDC_Class_Name_Grunt_Heavy",
	"#TDC_Class_Name_VIP",
	"#TDC_Class_Name_Zombie",
};

const char *g_aPlayerClassNames_NonLocalized[TDC_CLASS_COUNT_ALL] =
{
	"undefined",
	"grunt_normal",
	"grunt_light",
	"grunt_heavy",
	"vip",
	"zombie",
};

GameTypeInfo_t g_aGameTypeInfo[TDC_GAMETYPE_COUNT] =
{
	// name			nonlocalized_name		localized_name					teamplay	roundbased	end_at_timelimit	shared_goal
	{ "none",		"Undefined",			"Undefined",					true,		true,		true,				true		},
	{ "ffa",		"Free for All",			"#GameSubType_FFA",				false,		false,		true,				true		},
	{ "tdm",		"Team Deathmatch",		"#GameSubType_TDM",				true,		false,		true,				true		},
	{ "duel",		"Duel",					"#GameSubType_Duel",			false,		true,		false,				true		},
	{ "bm",			"Blood Money",			"#GameSubType_BM",				false,		false,		true,				true		},
	{ "tbm",		"Team Blood Money",		"#GameSubType_TBM",				true,		false,		true,				true		},
	{ "ctf",		"Capture the Flag",		"#GameSubType_CTF",				true,		true,		true,				true		},
	{ "ad",			"Attack / Defense",		"#GameSubType_AttackDefend",	true,		true,		false,				false		},
	{ "invade",		"Invade",				"#GameSubType_Invade",			true,		true,		true,				true		},
	{ "infection",	"Infection",			"#GameSubType_Infection",		true,		true,		false,				true		},
	{ "vip",		"VIP Escort",			"#GameSubType_VIP",				true,		true,		false,				false		},
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
const char *g_aAmmoNames[] =
{
	"DUMMY AMMO",
	"AMMO_PRIMARY",
	"AMMO_SECONDARY",
	"AMMO_GRENADES1",
	"AMMO_GRENADES2",
	"AMMO_SUPER",
};

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
const char *g_aWeaponNames[] =
{
	"WEAPON_NONE",
	"WEAPON_CUBEMAP",
	"WEAPON_CROWBAR",
	"WEAPON_TIREIRON",
	"WEAPON_LEADPIPE",
	"WEAPON_UMBRELLA",
	"WEAPON_HAMMERFISTS",
	"WEAPON_SHOTGUN",
	"WEAPON_PISTOL",
	"WEAPON_ROCKETLAUNCHER",
	"WEAPON_GRENADELAUNCHER",
	"WEAPON_SUPERSHOTGUN",
	"WEAPON_STENGUN",
	"WEAPON_REVOLVER",
	"WEAPON_CHAINSAW",
	"WEAPON_ASSAULTRIFLE",
	"WEAPON_DISPLACER",
	"WEAPON_LEVERRIFLE",
	"WEAPON_NAILGUN",
	"WEAPON_SUPERNAILGUN",
	"WEAPON_FLAMETHROWER",
	"WEAPON_FLAREGUN",
	"WEAPON_REMOTEBOMB",
	"WEAPON_FLAG",
	"WEAPON_HUNTINGSHOTGUN",
	"WEAPON_CLAWS",

	"WEAPON_COUNT",	// end marker, do not add below here
};

int g_aWeaponDamageTypes[] =
{
	DMG_GENERIC,	// WEAPON_NONE
	DMG_GENERIC,	// WEAPON_CUBEMAP
	DMG_CLUB,	// WEAPON_CROWBAR
	DMG_CLUB,	// WEAPON_TIREIRON
	DMG_CLUB,	// WEAPON_LEADPIPE
	DMG_CLUB,	// WEAPON_UMBRELLA
	DMG_CLUB | DMG_ALWAYSGIB,	// WEAPON_HAMMERFISTS
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD, // WEAPON_SHOTGUN
	DMG_BULLET | DMG_USEDISTANCEMOD, // WEAPON_PISTOL
	DMG_BLAST | DMG_USEDISTANCEMOD, // WEAPON_ROCKETLAUNCHER
	DMG_BLAST, // WEAPON_GRENADELAUNCHER
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// WEAPON_SUPERSHOTGUN
	DMG_BULLET | DMG_USEDISTANCEMOD,	// WEAPON_STENGUN
	DMG_BULLET | DMG_USEDISTANCEMOD,	// WEAPON_REVOLVER
	DMG_SLASH | DMG_ALWAYSGIB,	// WEAPON_CHAINSAW
	DMG_BULLET | DMG_USEDISTANCEMOD,	// WEAPON_ASSAULTRIFLE
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,	// WEAPON_DISPLACER
	DMG_BULLET,	// WEAPON_LEVERRIFLE
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,	// WEAPON_NAILGUN
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,	// WEAPON_SUPERNAILGUN
	DMG_IGNITE,	// WEAPON_FLAMETHROWER
	DMG_IGNITE, // WEAPON_FLAREGUN
	DMG_BLAST, // WEAPON_REMOTEBOMB
	DMG_CLUB,	// WEAPON_FLAG
	DMG_BULLET | DMG_BUCKSHOT | DMG_USEDISTANCEMOD, // WEAPON_HUNTINGSHOTGUN
	DMG_SLASH, // WEAPON_CLAWS

	// This is a special entry that must match with WEAPON_COUNT
	// to protect against updating the weapon list without updating this list
	TDC_DMG_SENTINEL_VALUE
};

const char *g_szProjectileNames[] =
{
	"",
	"projectile_bullet",
	"projectile_rocket",
	"projectile_pipe",
	"projectile_nail",
	"projectile_supernail",
	"projectile_flare",
	"projectile_plasma",
	"projectile_remotebomb",
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ETDCWeaponID GetWeaponId( const char *pszWeaponName )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( WEAPON_COUNT + 1 ) );

	for ( int iWeapon = 0; iWeapon < ARRAYSIZE( g_aWeaponNames ); ++iWeapon )
	{
		if ( !Q_stricmp( pszWeaponName, g_aWeaponNames[iWeapon] ) )
			return (ETDCWeaponID)iWeapon;
	}

	return WEAPON_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToAlias( ETDCWeaponID iWeapon )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( WEAPON_COUNT + 1 ) );

	if ( ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) ) || ( iWeapon < 0 ) )
		return NULL;

	return g_aWeaponNames[iWeapon];
}

//-----------------------------------------------------------------------------
// Purpose: Entity classnames need to be in lower case. Use this whenever
// you're spawning a weapon.
//-----------------------------------------------------------------------------
const char *WeaponIdToClassname( ETDCWeaponID iWeapon )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( WEAPON_COUNT + 1 ) );

	if ( ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) ) || ( iWeapon < 0 ) )
		return NULL;

	static char szEntName[256];
	V_strcpy_safe( szEntName, g_aWeaponNames[iWeapon] );
	V_strlower( szEntName );

	return szEntName;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ETDCWeaponID GetWeaponFromDamage( const CTakeDamageInfo &info )
{
	KillingWeaponData_t weaponData;
	TDCGameRules()->GetKillingWeaponName( info, NULL, weaponData );
	return weaponData.iWeaponID;
}

#endif

const char *g_aWearableSlots[TDC_WEARABLE_COUNT] =
{
	"Head",
	"Face",
	"Torso",
	"Misc",
	"Arms",
	"Hands",
};

PowerupData_t g_aPowerups[] =
{
	{
		TDC_COND_POWERUP_CRITDAMAGE,
		"item_powerup_critdamage",
		"hud/powerup_critdamage",
		"d_kill_bg_critdamage",
		true,
	},
	{
		TDC_COND_POWERUP_SPEEDBOOST,
		"item_powerup_speedboost",
		"hud/powerup_speedboost",
		"d_kill_bg_speedboost",
		true,
	},
	{
		TDC_COND_POWERUP_CLOAK,
		"item_powerup_cloak",
		"hud/powerup_cloak",
		"d_kill_bg_cloak",
		true,
	},
	{
		TDC_COND_POWERUP_RAGEMODE,
		"item_powerup_ragemode",
		"hud/powerup_ragemode",
		"",
		true,
	},
	{
		TDC_COND_LAST,
	}
};

#ifdef STAGING_ONLY
uint64 groupcheckmask = 0xDEC2423BBBB352DA;
uint64 groupcheck = 103582791461573007 ^ groupcheckmask;
#endif
