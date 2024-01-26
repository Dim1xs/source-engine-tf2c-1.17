//=============================================================================//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tdc_projectile_arrow.h"

#ifdef GAME_DLL
#include "props_shared.h"
#include "tdc_player.h"
#include "debugoverlay_shared.h"
#include "te_effect_dispatch.h"
#include "tdc_fx.h"
#include "tdc_gamerules.h"
#include "tdc_obj.h"
#include "decals.h"
#endif

#ifdef GAME_DLL
ConVar tf_debug_arrows( "tf_debug_arrows", "0", FCVAR_CHEAT );
#endif

enum
{
	TDC_ARROW_MODEL_NORMAL,
	TDC_ARROW_MODEL_HEALING,
	//TDC_ARROW_MODEL_REPAIR,
	//TDC_ARROW_MODEL_FESTIVE,
	TDC_ARROW_MODEL_COUNT,
};

const char *g_pszArrowModels[TDC_ARROW_MODEL_COUNT] =
{
	"models/weapons/w_models/w_arrow.mdl",
	"models/weapons/w_models/w_syringe_proj.mdl",
	//"models/weapons/w_models/w_repair_claw.mdl",
	//"models/weapons/w_models/w_arrow_xmas.mdl",
};

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Arrow, DT_TDCProjectile_Arrow )
BEGIN_NETWORK_TABLE( CTDCProjectile_Arrow, DT_TDCProjectile_Arrow )
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTDCProjectile_Arrow )
	DEFINE_ENTITYFUNC( ArrowTouch )
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_projectile_arrow, CTDCProjectile_Arrow );
PRECACHE_REGISTER( tf_projectile_arrow );

CTDCProjectile_Arrow::CTDCProjectile_Arrow()
{
}

CTDCProjectile_Arrow::~CTDCProjectile_Arrow()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

#ifdef GAME_DLL

CTDCProjectile_Arrow *CTDCProjectile_Arrow::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, float flSpeed, float flGravity, CBaseEntity *pOwner, ProjectileType_t iType )
{
	CTDCProjectile_Arrow *pArrow = static_cast<CTDCProjectile_Arrow *>( CTDCBaseRocket::Create( pWeapon, "tf_projectile_arrow", vecOrigin, vecAngles, pOwner, iType ) );

	if ( pArrow )
	{
		// Overriding speed.
		Vector vecForward;
		AngleVectors( vecAngles, &vecForward );

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flSpeed, mult_projectile_speed );

		Vector vecVelocity = vecForward * flSpeed;
		pArrow->SetAbsVelocity( vecVelocity );
		pArrow->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		pArrow->SetGravity( flGravity );

		pArrow->CreateTrail();
	}

	return pArrow;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ETDCWeaponID CTDCProjectile_Arrow::GetWeaponID( void ) const
{
	if ( m_iType == TDC_PROJECTILE_BUILDING_REPAIR_BOLT )
		return TDC_WEAPON_SHOTGUN_BUILDING_RESCUE;

	return TDC_WEAPON_COMPOUND_BOW;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::Precache( void )
{
	// Precache all arrow models we have.
	for ( int i = 0; i < TDC_ARROW_MODEL_COUNT; i++ )
	{
		int iIndex = PrecacheModel( g_pszArrowModels[i] );
		PrecacheGibsForModel( iIndex );
	}

	for ( int i = FIRST_GAME_TEAM; i < TDC_TEAM_COUNT; i++ )
	{
		PrecacheModel( UTIL_VarArgs( "effects/arrowtrail_%s.vmt", g_aTeamNamesShort[i] ) );
		PrecacheModel( UTIL_VarArgs( "effects/healingtrail_%s.vmt", g_aTeamNamesShort[i] ) );
		PrecacheModel( UTIL_VarArgs( "effects/repair_claw_trail_%s.vmt", g_aTeamLowerNames[i] ) );
	}

	PrecacheModel( "effects/arrowtrail_dm.vmt" );

	PrecacheScriptSound( "Weapon_Arrow.ImpactFlesh" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactMetal" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactWood" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactConcrete" );
	PrecacheScriptSound( "Weapon_Arrow.Nearmiss" );

	PrecacheTeamParticles( "critical_rocket_%s", true );
	PrecacheTeamParticles( "repair_claw_heal_%s" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::Spawn( void )
{
	switch ( m_iType )
	{
	case TDC_PROJECTILE_HEALING_BOLT:
	case TDC_PROJECTILE_FESTIVE_HEALING_BOLT:
		SetModel( g_pszArrowModels[TDC_ARROW_MODEL_HEALING] );
		break;
	//case TDC_PROJECTILE_BUILDING_REPAIR_BOLT:
	//	SetModel( g_pszArrowModels[TDC_ARROW_MODEL_REPAIR] );
	//	break;
	default:
		SetModel( g_pszArrowModels[TDC_ARROW_MODEL_NORMAL] );
		break;
	}

	BaseClass::Spawn();

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetGravity( 0.3f );

	SetTouch( &CTDCProjectile_Arrow::ArrowTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::ArrowTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t trHit;
	trHit = CBaseEntity::GetTouchTrace();
	if ( trHit.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Invisible.
	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();

	Vector vecOrigin = GetAbsOrigin();
	Vector vecDir = GetAbsVelocity();
	VectorNormalize( vecDir );

	CTDCPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{
#if 0
		CStudioHdr *pStudioHdr = pPlayer->GetModelPtr();
		if ( !pStudioHdr )
			return;

		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pPlayer->GetHitboxSet() );
		if ( !set )
			return;

		Vector vecDir = GetAbsVelocity();
		VectorNormalize( vecDir );

		// Oh boy... we gotta figure out the closest hitbox on player model to land a hit on.
		// Trace a bit ahead, to get closer to player's body.
		trace_t trFly;
		UTIL_TraceLine( vecOrigin, vecOrigin + vecDir * 16.0f, MASK_SHOT, this, COLLISION_GROUP_NONE, &trFly );

		QAngle angHit;
		trace_t trHit;
		float flClosest = FLT_MAX;
		for ( int i = 0; i < set->numhitboxes; i++ )
		{
			mstudiobbox_t *pBox = set->pHitbox( i );

			Vector boxPosition;
			QAngle boxAngles;
			pPlayer->GetBonePosition( pBox->bone, boxPosition, boxAngles );

			trace_t tr;
			UTIL_TraceLine( trFly.endpos, boxPosition, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			float flLengthSqr = ( tr.endpos - trFly.endpos ).LengthSqr();

			if ( flLengthSqr < flClosest )
			{
				flClosest = flLengthSqr;
				trHit = tr;
			}
		}

		if ( tf_debug_arrows.GetBool() )
		{
			NDebugOverlay::Line( trHit.endpos, trFly.endpos, 0, 255, 0, true, 5.0f );
		}

		// Place arrow at hitbox.
		SetAbsOrigin( trHit.endpos );

		Vector vecHitDir = trHit.plane.normal * -1.0f;
		AngleVectors( angHit, &vecHitDir );
		SetAbsAngles( angHit );
#endif

		EmitSound( "Weapon_Arrow.ImpactFlesh" );
	}
	else if ( pOther->IsBaseObject() )
	{
		EmitSound( "Weapon_Arrow.ImpactMetal" );
	}
	else
	{
		surfacedata_t *pData = physprops->GetSurfaceData( trHit.surface.surfaceProps );
		if ( pData )
		{
			int iMaterial = pData->game.material;
			switch ( iMaterial )
			{
			case CHAR_TEX_METAL:
			case CHAR_TEX_VENT:
				EmitSound( "Weapon_Arrow.ImpactMetal" );
				break;
			case CHAR_TEX_WOOD:
				EmitSound( "Weapon_Arrow.ImpactWood" );
				break;
			case CHAR_TEX_CONCRETE:
			case CHAR_TEX_TILE:
			default:
				EmitSound( "Weapon_Arrow.ImpactConcrete" );
				break;
			}
		}
	}

	// don't decal your teammates or objects on your team
	if ( pOther->GetTeamNumber() != GetTeamNumber() )
	{
		UTIL_ImpactTrace( &trHit, DMG_BULLET );
	}

	// HACK: Make players get knocked back in arrow's flying direction rather than away from the arrow.
	if ( pPlayer )
	{
		// Ignore Z.
		Vector vecPushOrigin = pPlayer->WorldSpaceCenter() - vecDir * 25.0f;
		vecPushOrigin.z = pPlayer->WorldSpaceCenter().z + 10.0f;
		SetAbsOrigin( vecPushOrigin );
	}

	if ( tf_debug_arrows.GetBool() )
	{
		NDebugOverlay::Line( trHit.endpos, trHit.endpos - vecDir * 20.0f, 0, 255, 0, true, 5.0f );
		NDebugOverlay::Cross3D( GetAbsOrigin(), 10.0f, 255, 0, 0, true, 5.0f );
	}

	// Repair bolts heal friendly buildings on impact.
	if ( m_iType == TDC_PROJECTILE_BUILDING_REPAIR_BOLT &&
		pOther->IsBaseObject() &&
		pOther->GetTeamNumber() == GetTeamNumber() )
	{
		CBaseObject *pObject = static_cast<CBaseObject *>( pOther );

		float flAmount = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOriginalLauncher(), flAmount, arrow_heals_buildings );
		pObject->BoltHeal( flAmount, ToTFPlayer( pAttacker ) );

		const char *pszEffect = ConstructTeamParticle( "repair_claw_heal_%s", GetTeamNumber() );
		DispatchParticleEffect( pszEffect, vecOrigin, vec3_angle );
	}
	else
	{
		// Do damage.
		CTakeDamageInfo info( this, pAttacker, GetOriginalLauncher(), GetDamage(), GetDamageType() );
		info.SetReportedPosition( pAttacker ? pAttacker->GetAbsOrigin() : vec3_origin );
		CalculateBulletDamageForce( &info, TDC_AMMO_PRIMARY, vecDir, trHit.endpos );

		pOther->DispatchTraceAttack( info, vecDir, &trHit );
		ApplyMultiDamage();
	}

	// Now restore the original position.
	if ( pPlayer )
	{
		SetAbsOrigin( vecOrigin );
	}

	FX_TFBoltImpact( GetWeaponID(), m_iType, trHit, vecDir, GetModelIndex() );

	// Remove after 1 second, we want sprite trail to hang around.
	SetTouch( NULL );
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
unsigned int CTDCProjectile_Arrow::PhysicsSolidMaskForEntity( void ) const
{
	unsigned int nMask = BaseClass::PhysicsSolidMaskForEntity();
	
	if ( CanHeadshot() )
	{
		nMask |= CONTENTS_HITBOX;
	}

	return nMask;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BaseClass::Deflected( pDeflectedBy, vecDir );

	// Change trail color.
	if ( m_hSpriteTrail.Get() )
	{
		UTIL_Remove( m_hSpriteTrail.Get() );
	}

	CreateTrail();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCProjectile_Arrow::CanHeadshot( void ) const
{
	return ( ( GetDamageType() & DMG_USE_HITLOCATIONS ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTDCProjectile_Arrow::GetTrailParticleName( void )
{
	const char *pszFormat = NULL;
	const char **pNames = g_aTeamNamesShort;

	switch( m_iType )
	{
	case TDC_PROJECTILE_BUILDING_REPAIR_BOLT:
		pszFormat = "effects/repair_claw_trail_%s.vmt";
		pNames = g_aTeamLowerNames;
		break;
	case TDC_PROJECTILE_HEALING_BOLT:
	case TDC_PROJECTILE_FESTIVE_HEALING_BOLT:
		pszFormat = "effects/healingtrail_%s.vmt";
		break;
	default:
		pszFormat = "effects/arrowtrail_%s.vmt";
		break;
	}

	return ConstructTeamParticle( pszFormat, GetTeamNumber(), true, pNames );
}

// ---------------------------------------------------------------------------- -
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::CreateTrail( void )
{
	CSpriteTrail *pTrail = CSpriteTrail::SpriteTrailCreate( GetTrailParticleName(), GetAbsOrigin(), true );

	if ( pTrail )
	{
		pTrail->FollowEntity( this );
		pTrail->SetOwnerEntity( GetOwnerEntity() );

		// Paint the trail with player's color in FFA.
		CTDCPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );

		if ( TFGameRules()->IsFreeForAll() && pPlayer )
		{
			Vector vecColor = pPlayer->m_vecPlayerColor * 255.0f;
			pTrail->SetTransparency( kRenderTransColor, vecColor.x, vecColor.y, vecColor.z, 255, kRenderFxNone );
		}
		else
		{
			pTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 255, kRenderFxNone );
		}

		pTrail->SetStartWidth( m_iType == TDC_PROJECTILE_BUILDING_REPAIR_BOLT ? 5.0f : 3.0f );
		pTrail->SetTextureResolution( 0.01f );
		pTrail->SetLifeTime( 0.3f );
		pTrail->TurnOn();

		pTrail->SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 3.0f, "RemoveThink" );

		m_hSpriteTrail.Set( pTrail );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::UpdateOnRemove( void )
{
	UTIL_Remove( m_hSpriteTrail.Get() );

	BaseClass::UpdateOnRemove();
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateCritTrail();
	}
	// Watch owner changes and change trail accordingly.
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		ParticleProp()->StopEmission();
		CreateCritTrail();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCProjectile_Arrow::CreateCritTrail( void )
{
	if ( IsDormant() || !m_bCritical )
		return;

	const char *pszEffectName = ConstructTeamParticle( "critical_rocket_%s", GetTeamNumber(), true );
	CNewParticleEffect *pParticle = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	SetParticleColor( pParticle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCProjectile_Arrow::HasTeamSkins( void )
{
	return ( m_iType == TDC_PROJECTILE_BUILDING_REPAIR_BOLT );
}

#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TDCStickyArrow *C_TDCStickyArrow::Create( C_BaseAnimating *pAttachTo, int nModelIndex, int iBone, const Vector &vecPos, const QAngle &vecAngles )
{
	const model_t *pModel = modelinfo->GetModel( nModelIndex );
	if ( !pModel )
		return NULL;

	const char *pszModelName = modelinfo->GetModelName( pModel );

	C_TDCStickyArrow *pArrow = new C_TDCStickyArrow();
	if ( pArrow->InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		pArrow->Release();
		return NULL;
	}

	pArrow->AttachEntityToBone( pAttachTo, iBone, vecPos, vecAngles );

	return pArrow;
}

#endif
