//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_SHAREDDEFS_H
#define TDC_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "mp_shareddefs.h"

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
enum
{
	TDC_TEAM_RED = LAST_SHARED_TEAM+1,
	TDC_TEAM_BLUE,
	TDC_TEAM_COUNT
};

#define TDC_TEAM_AUTOASSIGN (TDC_TEAM_COUNT + 1 )

extern const char *g_aTeamNames[TDC_TEAM_COUNT];
extern const char *g_aTeamNamesShort[TDC_TEAM_COUNT];
extern const char *g_aTeamLowerNames[TDC_TEAM_COUNT];
extern Vector g_aTeamParticleColors[TDC_TEAM_COUNT];
extern color32 g_aTeamColors[TDC_TEAM_COUNT];
extern Color g_aTeamHUDColors[TDC_TEAM_COUNT];

template <typename Functor>
void ForEachTeamName( Functor &&func, bool bFFAEnabled = false, const char **pNames = g_aTeamLowerNames )
{
	for ( int i = FIRST_GAME_TEAM; i < TDC_TEAM_COUNT; i++ )
	{
		func( pNames[i] );
	}

	if ( bFFAEnabled )
	{
		func( "dm" );
	}
}

const char *GetTeamSuffix( int iTeam, bool bDeathmatchOverride = false, const char **pNames = g_aTeamLowerNames, const char *pszNeutralName = NULL );
const char *ConstructTeamParticle( const char *pszFormat, int iTeam, bool bDeathmatchOverride = false, const char **pNames = g_aTeamLowerNames );
void PrecacheTeamParticles( const char *pszFormat, bool bFFAEnabled = false, const char **pNames = g_aTeamLowerNames );
int GetTeamSkin( int iTeam, bool bInGame = true );

#define CONTENTS_REDTEAM	CONTENTS_TEAM1
#define CONTENTS_BLUETEAM	CONTENTS_TEAM2
			
// Team roles
#define TDC_TEAM_ATTACKERS	TDC_TEAM_RED
#define TDC_TEAM_DEFENDERS	TDC_TEAM_BLUE

//-----------------------------------------------------------------------------
// CVar replacements
//-----------------------------------------------------------------------------
#define TDC_DAMAGE_CRIT_MULTIPLIER			3.0f


//-----------------------------------------------------------------------------
// TF-specific viewport panels
//-----------------------------------------------------------------------------
#define PANEL_MAPINFO			"mapinfo"
#define PANEL_DEATHMATCHTEAMSELECT "deathmatchteamselect"

// file we'll save our list of viewed intro movies in
#define MOVIES_FILE				"viewed.res"

//-----------------------------------------------------------------------------
// Player Classes.
//-----------------------------------------------------------------------------

#define TDC_FIRST_NORMAL_CLASS ( TDC_CLASS_UNDEFINED + 1 )
#define TDC_LAST_NORMAL_CLASS ( TDC_CLASS_ZOMBIE - 2 )

#define TDC_CLASS_MENU_BUTTONS ( TDC_LAST_NORMAL_CLASS + 2 )

enum
{
	TDC_CLASS_UNDEFINED = 0,

	TDC_CLASS_GRUNT_NORMAL,
	TDC_CLASS_GRUNT_LIGHT,
	TDC_CLASS_GRUNT_HEAVY,
	TDC_CLASS_VIP,
	TDC_CLASS_ZOMBIE,
	TDC_CLASS_COUNT_ALL
};

extern const char *g_aPlayerClassNames[TDC_CLASS_COUNT_ALL];				// localized class names
extern const char *g_aPlayerClassNames_NonLocalized[TDC_CLASS_COUNT_ALL];	// non-localized class names

enum ETDCGameType
{
	TDC_GAMETYPE_UNDEFINED = 0,
	TDC_GAMETYPE_FFA,
	TDC_GAMETYPE_TDM,
	TDC_GAMETYPE_DUEL,
	TDC_GAMETYPE_BM,
	TDC_GAMETYPE_TBM,
	TDC_GAMETYPE_CTF,
	TDC_GAMETYPE_ATTACK_DEFEND,
	TDC_GAMETYPE_INVADE,
	TDC_GAMETYPE_INFECTION,
	TDC_GAMETYPE_VIP,
	TDC_GAMETYPE_COUNT
};

struct GameTypeInfo_t
{
	const char *name;
	const char *nonlocalized_name;
	const char *localized_name;
	bool teamplay;
	bool roundbased;
	bool end_at_timelimit;
	bool shared_goal;
};

extern GameTypeInfo_t g_aGameTypeInfo[TDC_GAMETYPE_COUNT];

#define TDC_TEAM_HUMANS TDC_TEAM_RED
#define TDC_TEAM_ZOMBIES TDC_TEAM_BLUE

//-----------------------------------------------------------------------------
// Items.
//-----------------------------------------------------------------------------
enum
{
	TDC_ITEM_UNDEFINED		= 0,
	TDC_ITEM_CAPTURE_FLAG,
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
enum
{
	TDC_AMMO_DUMMY = 0,	// Dummy index to make the CAmmoDef indices correct for the other ammo types.
	TDC_AMMO_PRIMARY,
	TDC_AMMO_SECONDARY,
	TDC_AMMO_GRENADES1,
	TDC_AMMO_GRENADES2,
	TDC_AMMO_SUPER,
	TDC_AMMO_COUNT
};

extern const char *g_aAmmoNames[];

enum EAmmoSource
{
	TDC_AMMO_SOURCE_AMMOPACK = 0, // Default, used for ammopacks
	TDC_AMMO_SOURCE_RESUPPLY, // Maybe?
	TDC_AMMO_SOURCE_DISPENSER,
	TDC_AMMO_SOURCE_COUNT
};

enum
{
	TDC_GIVEAMMO_MAX = -3,
	TDC_GIVEAMMO_INITIAL = -2,
	TDC_GIVEAMMO_NONE = -1,
};

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
#define TDC_PLAYER_WEAPON_COUNT		5
#define TDC_PLAYER_GRENADE_COUNT		2

#define TDC_WEAPON_PRIMARY_MODE		0
#define TDC_WEAPON_SECONDARY_MODE	1

enum ETDCWeaponID
{
	WEAPON_NONE = 0,
	WEAPON_CUBEMAP,
	WEAPON_CROWBAR,
	WEAPON_TIREIRON,
	WEAPON_LEADPIPE,
	WEAPON_UMBRELLA,
	WEAPON_HAMMERFISTS,
	WEAPON_SHOTGUN,
	WEAPON_PISTOL,
	WEAPON_ROCKETLAUNCHER,
	WEAPON_GRENADELAUNCHER,
	WEAPON_SUPERSHOTGUN,
	WEAPON_STENGUN,
	WEAPON_REVOLVER,
	WEAPON_CHAINSAW,
	WEAPON_ASSAULTRIFLE,
	WEAPON_DISPLACER,
	WEAPON_LEVERRIFLE,
	WEAPON_NAILGUN,
	WEAPON_SUPERNAILGUN,
	WEAPON_FLAMETHROWER,
	WEAPON_FLAREGUN,
	WEAPON_REMOTEBOMB,
	WEAPON_FLAG,
	WEAPON_HUNTINGSHOTGUN,
	WEAPON_CLAWS,

	WEAPON_COUNT
};

extern const char *g_aWeaponNames[];
extern int g_aWeaponDamageTypes[];

ETDCWeaponID GetWeaponId( const char *pszWeaponName );
#ifdef GAME_DLL
ETDCWeaponID GetWeaponFromDamage( const CTakeDamageInfo &info );
#endif

const char *WeaponIdToAlias( ETDCWeaponID iWeapon );
const char *WeaponIdToClassname( ETDCWeaponID iWeapon );

// There 3 projectile bases in TF2:
// 1) CTDCBaseRocket: flies in a straight line or with an arch, explodes on contact.
// 2) CTDCWeaponBaseGrenadeProj: physics object, explodes on timer.
// 3) CTDCBaseProjectile: used for nails, flies with an arch, does damage on contact.
// They all have different behavior and accessors so it's important to distinguish them easily.
enum
{
	TDC_PROJECTILE_BASE_ROCKET,
	TDC_PROJECTILE_BASE_GRENADE,
	TDC_PROJECTILE_BASE_NAIL,
};

enum ProjectileType_t
{
	TDC_PROJECTILE_NONE,
	TDC_PROJECTILE_BULLET,
	TDC_PROJECTILE_ROCKET,
	TDC_PROJECTILE_PIPEBOMB,
	TDC_PROJECTILE_NAIL,
	TDC_PROJECTILE_SUPERNAIL,
	TDC_PROJECTILE_FLARE,
	TDC_PROJECTILE_PLASMA,
	TDC_PROJECTILE_REMOTEBOMB,

	TDC_NUM_PROJECTILES
};

extern const char *g_szProjectileNames[];

enum ETDCWearableSlot
{
	TDC_WEARABLE_INVALID = -1,
	TDC_WEARABLE_HEAD = 0,
	TDC_WEARABLE_FACE,
	TDC_WEARABLE_TORSO,
	TDC_WEARABLE_MISC,
	TDC_WEARABLE_ARMS,
	TDC_WEARABLE_HANDS,

	TDC_WEARABLE_COUNT
};

extern const char *g_aWearableSlots[TDC_WEARABLE_COUNT];

enum ETDCDamageSourceType
{
	TDC_DMG_SOURCE_WEAPON = 0,
	TDC_DMG_SOURCE_ENVIRONMENT,
	TDC_DMG_SOURCE_PLAYER_SPREAD,

	TDC_DMG_SOURCE_COUNT
};

//-----------------------------------------------------------------------------
// TF Player Condition.
//-----------------------------------------------------------------------------
enum ETDCCond
{
	TDC_COND_INVALID = -1,
	TDC_COND_AIMING = 0,		// Sniper aiming, Heavy minigun.
	TDC_COND_ZOOMED,
	TDC_COND_TELEPORTED,
	TDC_COND_TAUNTING,
	TDC_COND_STEALTHED_BLINK,
	TDC_COND_SELECTED_TO_TELEPORT,
	TDC_COND_BURNING,
	TDC_COND_CRITBOOSTED_BONUS_TIME,
	TDC_COND_BLASTJUMPING,
	TDC_COND_HEALTH_OVERHEALED,
	TDC_COND_STUNNED,

	// Add TF2C conds here
	TDC_COND_AIRBLASTED,
	TDC_COND_INVULNERABLE_SPAWN_PROTECT,
	TDC_COND_LASTSTANDING,
	TDC_COND_CHARGING_POUNCE,
	TDC_COND_SOFTZOOM,
	TDC_COND_SLIDE,
	TDC_COND_SPRINT,

	// Powerup conditions
	TDC_COND_POWERUP_CRITDAMAGE,
	TDC_COND_POWERUP_SPEEDBOOST,
	TDC_COND_POWERUP_CLOAK,
	TDC_COND_POWERUP_RAGEMODE,

	TDC_COND_LAST
};

enum ETDCPowerupDropStyle
{
	TDC_POWERUP_DROP_INVALID = -1,
	TDC_POWERUP_DROP_FEET = 0,
	TDC_POWERUP_DROP_THROW,
	TDC_POWERUP_DROP_EXPLODE
};

struct PowerupData_t
{
	ETDCCond cond;
	const char *name;
	const char *hud_icon;
	const char *kill_bg;
	bool drop_on_death;
};

extern PowerupData_t g_aPowerups[];

#define TDC_POWERUP_SPEEDBOOST_FACTOR 1.3f
#define TDC_POWERUP_SPEEDBOOST_INV_FACTOR 0.7f

//-----------------------------------------------------------------------------
// TF Player State.
//-----------------------------------------------------------------------------
enum 
{
	TDC_STATE_ACTIVE = 0,		// Happily running around in the game.
	TDC_STATE_WELCOME,			// First entering the server (shows level intro screen).
	TDC_STATE_OBSERVER,			// Game observer mode.
	TDC_STATE_DYING,				// Player is dying.
	TDC_STATE_COUNT
};

//-----------------------------------------------------------------------------
// TF FlagInfo State.
//-----------------------------------------------------------------------------
enum
{
	TDC_FLAGINFO_NONE = 0,
	TDC_FLAGINFO_STOLEN,
	TDC_FLAGINFO_DROPPED,
};

enum ETDCFlagEventTypes
{
	TDC_FLAGEVENT_PICKUP = 1,
	TDC_FLAGEVENT_CAPTURE,
	TDC_FLAGEVENT_DEFEND,
	TDC_FLAGEVENT_DROPPED,
	TDC_FLAGEVENT_RETURN,
};

//-----------------------------------------------------------------------------
// Domination/nemesis constants
//-----------------------------------------------------------------------------
#define TDC_KILLS_DOMINATION				4		// # of unanswered kills to dominate another player

//--------------
// TF Specific damage flags
//--------------
//#define DMG_UNUSED					(DMG_LASTGENERICFLAG<<2)
// We can't add anymore dmg flags, because we'd be over the 32 bit limit.
// So lets re-use some of the old dmg flags in TF
#define DMG_USE_HITLOCATIONS	(DMG_AIRBOAT)
#define DMG_HALF_FALLOFF		(DMG_RADIATION)
#define DMG_CRITICAL			(DMG_ACID)
#define DMG_IGNITE				(DMG_PLASMA)
#define DMG_USEDISTANCEMOD		(DMG_SLOWBURN)		// NEED TO REMOVE CALTROPS
#define DMG_NOCLOSEDISTANCEMOD	(DMG_POISON)
#define DMG_TRAIN				(DMG_VEHICLE)
#define DMG_SAWBLADE			(DMG_NERVEGAS)

#define TDC_DMG_SENTINEL_VALUE	INT_MAX

// This can only ever be used on a TakeHealth call, since it re-uses a dmg flag that means something else
#define HEAL_IGNORE_MAXHEALTH	( 1 << 1 )
#define HEAL_NOTIFY				( 1 << 2 )
#define HEAL_MAXBUFFCAP			( 1 << 3 )

// Special Damage types
enum ETDCDmgCustom
{
	TDC_DMG_CUSTOM_NONE = 0,
	TDC_DMG_CUSTOM_HEADSHOT,
	TDC_DMG_CUSTOM_BACKSTAB,
	TDC_DMG_CUSTOM_BURNING,
	TDC_DMG_CUSTOM_SUICIDE,
	TDC_DMG_TELEFRAG,
	TDC_DMG_DISPLACER_BOMB,
	TDC_DMG_DISPLACER_TELEFRAG,
	TDC_DMG_CHAINSAW_BACKSTAB,
	TDC_DMG_REMOTEBOMB_IMPACT,
	TDC_DMG_STOMP,
	TDC_DMG_COUNT
};

#define TDC_JUMP_ROCKET		( 1 << 0 )
#define TDC_JUMP_GRENADE	( 1 << 1 )
#define TDC_JUMP_OTHER		( 1 << 2 )

enum
{
	TDC_COLLISIONGROUP_GRENADES = LAST_SHARED_COLLISION_GROUP,
	TDC_COLLISION_GROUP_OBJECT,
	TDC_COLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT,
	TDC_COLLISION_GROUP_COMBATOBJECT,
	TDC_COLLISION_GROUP_ROCKETS,		// Solid to players, but not player movement. ensures touch calls are originating from rocket
	TDC_COLLISION_GROUP_RESPAWNROOMS,
	TDC_COLLISION_GROUP_PUMPKINBOMBS,
	TDC_COLLISION_GROUP_ROCKETS_NOTSOLID, // Same as TDC_COLLISION_GROUP_ROCKETS but not solid to other rockets.
};

//--------------
// Scoring
//--------------

#define TDC_SCORE_KILL							1
#define TDC_SCORE_DEATH							0
#define TDC_SCORE_CAPTURE						2
#define TDC_SCORE_DEFEND							1
#define TDC_SCORE_DESTROY_BUILDING				1
#define TDC_SCORE_HEADSHOT						1
#define TDC_SCORE_BACKSTAB						1
#define TDC_SCORE_INVULN							1
#define TDC_SCORE_REVENGE						1
#define TDC_SCORE_KILL_ASSISTS_PER_POINT			2
#define TDC_SCORE_TELEPORTS_PER_POINT			2	
#define TDC_SCORE_HEAL_HEALTHUNITS_PER_POINT		600
#define TDC_SCORE_DAMAGE_PER_POINT				600
#define TDC_SCORE_BONUS_PER_POINT				1

// Custom animation events
#define TDC_AE_CIGARETTE_THROW			7000
#define TDC_AE_FOOTSTEP					7001
#define TDC_AE_WPN_EJECTBRASS			6002

// Win panel styles
enum
{
	WINPANEL_BASIC = 0,
};

#define TDC_DEATH_ANIMATION_TIME 2.0

typedef enum
{
	HUD_NOTIFY_YOUR_FLAG_TAKEN,
	HUD_NOTIFY_YOUR_FLAG_DROPPED,
	HUD_NOTIFY_YOUR_FLAG_RETURNED,
	HUD_NOTIFY_YOUR_FLAG_CAPTURED,

	HUD_NOTIFY_ENEMY_FLAG_TAKEN,
	HUD_NOTIFY_ENEMY_FLAG_DROPPED,
	HUD_NOTIFY_ENEMY_FLAG_RETURNED,
	HUD_NOTIFY_ENEMY_FLAG_CAPTURED,

	HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP,

	HUD_NOTIFY_NO_INVULN_WITH_FLAG,
	HUD_NOTIFY_NO_TELE_WITH_FLAG,

	HUD_NOTIFY_SPECIAL,

	HUD_NOTIFY_GOLDEN_WRENCH,

	HUD_NOTIFY_RD_ROBOT_ATTACKED,

	HUD_NOTIFY_HOW_TO_CONTROL_GHOST,
	HUD_NOTIFY_HOW_TO_CONTROL_KART,

	HUD_NOTIFY_PASSTIME_HOWTO,
	HUD_NOTIFY_PASSTIME_BALL_BASKET,
	HUD_NOTIFY_PASSTIME_BALL_ENDZONE,
	HUD_NOTIFY_PASSTIME_SCORE,
	HUD_NOTIFY_PASSTIME_FRIENDLY_SCORE,
	HUD_NOTIFY_PASSTIME_ENEMY_SCORE,
	HUD_NOTIFY_PASSTIME_NO_TELE,
	HUD_NOTIFY_PASSTIME_NO_CARRY,
	HUD_NOTIFY_PASSTIME_NO_INVULN,
	HUD_NOTIFY_PASSTIME_NO_DISGUISE,
	HUD_NOTIFY_PASSTIME_NO_CLOAK,
	HUD_NOTIFY_PASSTIME_NO_OOB,
	HUD_NOTIFY_PASSTIME_NO_HOLSTER,
	HUD_NOTIFY_PASSTIME_NO_TAUNT,

	NUM_STOCK_NOTIFICATIONS
} HudNotification_t;

#define TDC_TRAIN_MAX_HILLS			5
#define TDC_TRAIN_FLOATS_PER_HILL	2
#define TDC_TRAIN_HILLS_ARRAY_SIZE	TDC_TEAM_COUNT * TDC_TRAIN_MAX_HILLS * TDC_TRAIN_FLOATS_PER_HILL

#define TDC_DEATH_DOMINATION				( 1 << 0 )	// killer is dominating victim
#define TDC_DEATH_ASSISTER_DOMINATION	( 1 << 1 )	// assister is dominating victim
#define TDC_DEATH_REVENGE				( 1 << 2 )	// killer got revenge on victim
#define TDC_DEATH_ASSISTER_REVENGE		( 1 << 3 )	// assister got revenge on victim

// Unused death flags
#define TDC_DEATH_FIRST_BLOOD			( 1 << 4 )
#define TDC_DEATH_FEIGN_DEATH			( 1 << 5 )
#define TDC_DEATH_UNKNOWN				( 1 << 6 ) // Seemingly unused in live TF2.
#define TDC_DEATH_GIB					( 1 << 7 )
#define TDC_DEATH_PURGATORY				( 1 << 8 )
#define TDC_DEATH_MINIBOSS				( 1 << 9 )
#define TDC_DEATH_AUSTRALIUM				( 1 << 10 )

// Ragdolls flags
#define TDC_RAGDOLL_GIB		( 1 << 0 )
#define TDC_RAGDOLL_BURNING	( 1 << 1 )
#define TDC_RAGDOLL_ONGROUND	( 1 << 2 )

#define HUD_ALERT_SCRAMBLE_TEAMS 0

// Custom mask used for bullet tracing.
#define MASK_TFSHOT ( MASK_SOLID | CONTENTS_HITBOX )

enum
{
	TDC_SCRAMBLEMODE_SCORETIME_RATIO = 0,
	TDC_SCRAMBLEMODE_KILLDEATH_RATIO,
	TDC_SCRAMBLEMODE_SCORE,
	TDC_SCRAMBLEMODE_CLASS,
	TDC_SCRAMBLEMODE_RANDOM,
	TDC_SCRAMBLEMODE_COUNT
};

#define TDC_CAMERA_DIST 80
#define TDC_CAMERA_DIST_RIGHT 20
#define TDC_CAMERA_DIST_UP 0

#define IN_THIRDPERSON ( 1 << 30 )
#define IN_TYPING ( 1 << 31 )

#endif // TDC_SHAREDDEFS_H
