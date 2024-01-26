//=============================================================================//
//
// Purpose: Payload pushable physics ported from live TF2
//
//=============================================================================//
#include "cbase.h"
#include "pushentity.h"
#include "tdc_gamerules.h"
#include "collisionutils.h"
#include "func_respawnroom.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTDCPhysicsPushEntities : public CPhysicsPushedEntities
{
public:
	DECLARE_CLASS( CTDCPhysicsPushEntities, CPhysicsPushedEntities );

protected:
	virtual bool SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot );
	virtual bool SpeculativelyCheckLinearPush( const Vector &vecAbsPush );
	virtual void FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove );

	bool UsingNewPushPhysics( void );
	bool RotationPushTFPlayer( CPhysicsPushedEntities::PhysicsPushedInfo_t &info, const Vector &vecAbsPush, const CPhysicsPushedEntities::RotatingPushMove_t &rotPushMove );
	bool LinearPushTFPlayer( CPhysicsPushedEntities::PhysicsPushedInfo_t &info, const Vector &vecAbsPush );
	void RotationCheckPush( CPhysicsPushedEntities::PhysicsPushedInfo_t &info );
	void LinearCheckPush( CPhysicsPushedEntities::PhysicsPushedInfo_t &info );
	void MovePlayer( CBaseEntity *pPlayer, CPhysicsPushedEntities::PhysicsPushedInfo_t &info, float flAmount, bool bParentIsTrain );
	void FindNewPushDirection( const Vector &vecDir, const Vector &vecNormal, Vector &vecNewDir );
	bool IsPlayerAABBIntersetingPusherOBB( CBaseEntity *pPlayer, CBaseEntity *pPusher );

	float m_flPushDistance;
	Vector m_vecPushDirection;
};

CTDCPhysicsPushEntities s_TFPushedEntities;
CPhysicsPushedEntities *g_pPushedEntities = &s_TFPushedEntities;

//-----------------------------------------------------------------------------
// Purpose: Speculatively checks to see if all entities in this list can be pushed
//-----------------------------------------------------------------------------
bool CTDCPhysicsPushEntities::SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot )
{
	if ( UsingNewPushPhysics() )
	{
		Vector vecAbsPush;
		m_nBlocker = -1;

		for ( int i = m_rgMoved.Count() - 1; i >= 0; i-- )
		{
			if ( m_rgMoved[i].m_pEntity->IsPlayer() )
			{
				RotationPushTFPlayer( m_rgMoved[i], vecAbsPush, rotPushMove );
			}
			else
			{
				ComputeRotationalPushDirection( m_rgMoved[i].m_pEntity, rotPushMove, &vecAbsPush, pRoot );
				if ( !SpeculativelyCheckPush( m_rgMoved[i], vecAbsPush, true ) )
				{
					m_nBlocker = i;
					return false;
				}
			}
		}

		return true;
	}
	else
	{
		return BaseClass::SpeculativelyCheckRotPush( rotPushMove, pRoot );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Speculatively checks to see if all entities in this list can be pushed
//-----------------------------------------------------------------------------
bool CTDCPhysicsPushEntities::SpeculativelyCheckLinearPush( const Vector &vecAbsPush )
{
	if ( UsingNewPushPhysics() )
	{
		m_nBlocker = -1;

		for ( int i = m_rgMoved.Count() - 1; i >= 0; i-- )
		{
			if ( m_rgMoved[i].m_pEntity->IsPlayer() )
			{
				LinearPushTFPlayer( m_rgMoved[i], vecAbsPush );
			}
			else
			{
				if ( !SpeculativelyCheckPush( m_rgMoved[i], vecAbsPush, false ) )
				{
					m_nBlocker = i;
					return false;
				}
			}
		}

		return true;
	}
	else
	{
		return BaseClass::SpeculativelyCheckLinearPush( vecAbsPush );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Some fixup for objects pushed by rotating objects
//-----------------------------------------------------------------------------
void CTDCPhysicsPushEntities::FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove )
{
	if ( UsingNewPushPhysics() )
	{
		// Make players view not rotate when riding the cart
		if ( !pPushedEntity->IsPlayer() )
		{
			QAngle angles = pPushedEntity->GetAbsAngles();

			angles[YAW] += rotPushMove.amove[YAW];
			pPushedEntity->SetAbsAngles( angles );
		}
	}
	else
	{
		BaseClass::FinishRotPushedEntity( pPushedEntity, rotPushMove );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPhysicsPushEntities::UsingNewPushPhysics( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPhysicsPushEntities::RotationPushTFPlayer( CPhysicsPushedEntities::PhysicsPushedInfo_t &info, const Vector &vecAbsPush, const CPhysicsPushedEntities::RotatingPushMove_t &rotPushMove )
{
	memset( &info.m_Trace, 0, sizeof( trace_t ) );
	CBaseEntity *pBlocker = info.m_pEntity;
	info.m_vecStartAbsOrigin = pBlocker->GetAbsOrigin();
	CBaseEntity *pRootHighestParent = m_rgPusher[0].m_pEntity->GetRootMoveParent();

	if ( IsPlayerAABBIntersetingPusherOBB( pBlocker, pRootHighestParent ) )
	{
		if ( pBlocker->GetGroundEntity() == pRootHighestParent )
		{
			m_vecPushDirection.Init( 0.0f, 0.0f, 1.0f );

			// Push the player up if they're standing on the train and it rotates up/down.
			if ( rotPushMove.amove[PITCH] == 0.0f )
			{
				m_flPushDistance = 0.0f;
			}
			else
			{
				float flTanPitch = fabs( tan( DEG2RAD( rotPushMove.amove[PITCH] ) ) );
				float flRadius = pRootHighestParent->BoundingRadius();

				m_flPushDistance = flTanPitch * flRadius * 1.1f;
			}
		}
		else
		{
			// Push the player away from the train when it rotates.
			float flTotalRadius = pBlocker->BoundingRadius() + pRootHighestParent->BoundingRadius();

			m_vecPushDirection = pBlocker->CollisionProp()->GetCollisionOrigin() - pRootHighestParent->CollisionProp()->GetCollisionOrigin();
			m_flPushDistance = ( flTotalRadius - VectorNormalize( m_vecPushDirection ) ) * 1.1f;
		}

		RotationCheckPush( info );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPhysicsPushEntities::LinearPushTFPlayer( CPhysicsPushedEntities::PhysicsPushedInfo_t &info, const Vector &vecAbsPush )
{
	memset( &info.m_Trace, 0, sizeof( trace_t ) );
	CBaseEntity *pBlocker = info.m_pEntity;
	info.m_vecStartAbsOrigin = pBlocker->GetAbsOrigin();
	CBaseEntity *pRootHighestParent = m_rgPusher[0].m_pEntity->GetRootMoveParent();

	if ( IsPlayerAABBIntersetingPusherOBB( pBlocker, pRootHighestParent ) )
	{
		if ( pBlocker->GetGroundEntity() == pRootHighestParent )
		{
			m_vecPushDirection = vecAbsPush;
			m_flPushDistance = VectorNormalize( m_vecPushDirection );
		}
		else
		{
			// Don't apply vertical push if they're not standing on the train.
			m_vecPushDirection = vecAbsPush;
			m_vecPushDirection.z = 0.0f;
			m_flPushDistance = VectorNormalize( m_vecPushDirection );
		}

		LinearCheckPush( info );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPhysicsPushEntities::RotationCheckPush( CPhysicsPushedEntities::PhysicsPushedInfo_t &info )
{
	CBaseEntity *pBlocker = info.m_pEntity;
	CBaseEntity *pRootHighestParent = m_rgPusher[0].m_pEntity->GetRootMoveParent();

	int *pPusherHandles = new int[m_rgPusher.Count()];
	UnlinkPusherList( pPusherHandles );

	for ( int i = 0; i < 2; i++ )
	{
		MovePlayer( pBlocker, info, 0.35f, pRootHighestParent->IsBaseTrain() );
		if ( !IsPlayerAABBIntersetingPusherOBB( pBlocker, pRootHighestParent ) )
			break;
	}

	RelinkPusherList( pPusherHandles );

	info.m_bPusherIsGround = false;
	if ( pBlocker->GetGroundEntity() && pBlocker->GetGroundEntity()->GetRootMoveParent() == m_rgPusher[0].m_pEntity )
	{
		info.m_bPusherIsGround = true;
	}

	if ( IsPlayerAABBIntersetingPusherOBB( pBlocker, pRootHighestParent ) )
	{
		UnlinkPusherList( pPusherHandles );
		MovePlayer( pBlocker, info, 1.0f, pRootHighestParent->IsBaseTrain() );
		RelinkPusherList( pPusherHandles );
	}

	info.m_bBlocked = false;
	delete[] pPusherHandles;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPhysicsPushEntities::LinearCheckPush( CPhysicsPushedEntities::PhysicsPushedInfo_t &info )
{
	CBaseEntity *pBlocker = info.m_pEntity;
	CBaseEntity *pRootHighestParent = m_rgPusher[0].m_pEntity->GetRootMoveParent();

	int *pPusherHandles = new int[m_rgPusher.Count()];
	UnlinkPusherList( pPusherHandles );
	MovePlayer( pBlocker, info, 1.0, pRootHighestParent->IsBaseTrain() );
	RelinkPusherList( pPusherHandles );

	info.m_bPusherIsGround = false;
	if ( pBlocker->GetGroundEntity() && pBlocker->GetGroundEntity()->GetRootMoveParent() == m_rgPusher[0].m_pEntity )
	{
		info.m_bPusherIsGround = true;
	}
	else if ( !IsPushedPositionValid( pBlocker ) )
	{
		UnlinkPusherList( pPusherHandles );
		MovePlayer( pBlocker, info, 1.0, pRootHighestParent->IsBaseTrain() );
		RelinkPusherList( pPusherHandles );
	}

	info.m_bBlocked = false;
	delete[] pPusherHandles;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPhysicsPushEntities::MovePlayer( CBaseEntity *pEntity, CPhysicsPushedEntities::PhysicsPushedInfo_t &info, float flFraction, bool bRidingTrain )
{
	// Where the player started.
	Vector vecOriginalPos = pEntity->GetAbsOrigin();
	// Raise a little off the ground
	vecOriginalPos.z += 4.0f;

	Vector vecCurrentPushDir = m_vecPushDirection;
	float flPushDistanceLeft = m_flPushDistance * flFraction;

	// Iteratively compute the deflection
	for ( int i = 0; i < 4; i++ )
	{
		Vector vecTraceEnd = pEntity->GetAbsOrigin() + vecCurrentPushDir * flPushDistanceLeft;
		UTIL_TraceEntity( pEntity, vecOriginalPos, vecTraceEnd, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &info.m_Trace );

		// If you're move-parented to the train you don't wanna ignore the respawn room visualizer
		if ( bRidingTrain && pEntity->IsPlayer() && PointsCrossRespawnRoomVisualizer( vecOriginalPos, info.m_Trace.endpos, pEntity->GetTeamNumber() ) )
		{
			break;
		}

		// Can move at all? Move!
		if ( info.m_Trace.fraction != 0.0f )
		{
			pEntity->SetAbsOrigin( info.m_Trace.endpos );
		}

		if ( info.m_Trace.fraction == 1.0f )
		{
			// Not stopped? We're done here.
			return;
		}

		// Hit something? Deflect!
		Vector vecNewDir;
		FindNewPushDirection( vecCurrentPushDir, info.m_Trace.plane.normal, vecNewDir );

		float flDeflect = DotProduct( vecCurrentPushDir, info.m_Trace.plane.normal );
		flPushDistanceLeft *= ( 2.0f - flDeflect ) * ( 1.0f - info.m_Trace.fraction );
		vecCurrentPushDir = vecNewDir;
	}

	// We can't solve the collision, so the player feels the consequences for 
	// putting us in this stupid situation
	CBasePlayer *pPlayer = ToBasePlayer( pEntity );
	if ( pPlayer )
	{
		pPlayer->CommitSuicide( true, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPhysicsPushEntities::FindNewPushDirection( const Vector &vecDir, const Vector &vecNormal, Vector &vecNewDir )
{
	// Determine how far along plane to slide based on incoming direction.
	float flDeflectionAmount = DotProduct( vecDir, vecNormal );
	vecNewDir = vecDir - vecNormal * flDeflectionAmount;

	// iterate once to make sure we aren't still moving through the plane
	flDeflectionAmount = DotProduct( vecNewDir, vecNormal );
	if ( flDeflectionAmount < 0.0f )
	{
		vecNewDir -= vecNormal * flDeflectionAmount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check whether or not the bounding boxes of player and pusher intersect (spelling mistake is accurate with live TF2)
//-----------------------------------------------------------------------------
bool CTDCPhysicsPushEntities::IsPlayerAABBIntersetingPusherOBB( CBaseEntity *pPlayer, CBaseEntity *pPusher )
{
	if ( !pPlayer )
		return false;

	if ( !pPlayer->IsPlayer() )
		return false;

	return IsOBBIntersectingOBB(
		pPlayer->CollisionProp()->GetCollisionOrigin(), pPlayer->CollisionProp()->GetCollisionAngles(), pPlayer->CollisionProp()->OBBMins(), pPlayer->CollisionProp()->OBBMaxs(),
		pPusher->CollisionProp()->GetCollisionOrigin(), pPusher->CollisionProp()->GetCollisionAngles(), pPusher->CollisionProp()->OBBMins(), pPusher->CollisionProp()->OBBMaxs() );
}
