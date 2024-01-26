//=============================================================================
//
//	Ejected magazine.
//
//=============================================================================
#include "cbase.h"
#include "c_droppedmagazine.h"
#include "gamestringpool.h"

ConVar tdc_ejectmag_fade_mintime( "tdc_ejectmag_fade_mintime", "5", FCVAR_ARCHIVE );
ConVar tdc_ejectmag_fade_maxtime( "tdc_ejectmag_fade_maxtime", "10", FCVAR_ARCHIVE );
ConVar tdc_ejectmag_max_count( "tdc_ejectmag_max_count", "3", FCVAR_ARCHIVE );
ConVar tdc_ejectmag_collide( "tdc_ejectmag_collide", "0", FCVAR_ARCHIVE );

static int MagsSort( C_DroppedMagazine * const *pEnt1, C_DroppedMagazine * const *pEnt2 )
{
	return ( ( *pEnt2 )->GetCreateTime() - ( *pEnt1 )->GetCreateTime() );
}

C_DroppedMagazine *C_DroppedMagazine::Create( const char *pszModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, C_BaseCombatWeapon *pWeapon )
{
	C_DroppedMagazine *pNewMag = new C_DroppedMagazine();

	pNewMag->SetModelName( AllocPooledString( pszModel ) );
	pNewMag->SetLocalOrigin( vecOrigin );
	pNewMag->SetLocalAngles( vecAngles );
	pNewMag->SetOwnerEntity( pWeapon );
	pNewMag->SetPhysicsMode( PHYSICS_MULTIPLAYER_CLIENTSIDE );
	pNewMag->SetCreateTime( gpGlobals->curtime );

	if ( !pNewMag->Initialize() )
	{
		pNewMag->Release();
		return NULL;
	}

	// Control the number of dropped mags per player as they are spammable.
	CUtlVector<C_DroppedMagazine *> aMags;

	for ( C_DroppedMagazine *pMag : C_DroppedMagazine::AutoList() )
	{
		if ( pMag->GetOwnerEntity() == pWeapon )
		{
			aMags.AddToTail( pMag );
		}
	}

	aMags.Sort( MagsSort );

	// If we're over the limit remove the oldest mag.
	for ( int i = aMags.Count() - 1; i >= tdc_ejectmag_max_count.GetInt(); i-- )
	{
		aMags[i]->Release();
		aMags.Remove( i );
	}

	pNewMag->m_nSkin = pWeapon->GetSkin();
	pNewMag->m_iHealth = 1;
	pNewMag->SetCollisionGroup( tdc_ejectmag_collide.GetBool() ? COLLISION_GROUP_NONE : COLLISION_GROUP_DEBRIS );

	// Make it empty or not based on clip size.
	pNewMag->SetBodygroup( 0, ( pWeapon->Clip1() != 0 ) ? 0 : 1 );

	pNewMag->StartFadeOut( RandomFloat( tdc_ejectmag_fade_mintime.GetFloat(), tdc_ejectmag_fade_maxtime.GetFloat() ) );

	IPhysicsObject *pPhysicsObject = pNewMag->VPhysicsGetObject();

	if ( pPhysicsObject )
	{
		// randomize velocity by 5%
		Vector rndVel = vecVelocity + vecVelocity * RandomFloat( -0.025, 0.025 );
		pPhysicsObject->AddVelocity( &rndVel, &vec3_origin );
	}

	return pNewMag;
}
