//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tdc_shareddefs.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_FuncForceField : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FuncForceField, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	virtual int DrawModel( int flags );
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncForceField, DT_FuncForceField, CFuncForceField )
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Don't draw for friendly players
//-----------------------------------------------------------------------------
int C_FuncForceField::DrawModel( int flags )
{
	// Don't draw for anyone in endround
	if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		return 1;
	}

	// Don't draw for teammates.
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetTeamNumber() == GetTeamNumber() )
	{
		return 1;
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: Enemy players collide with us, except in endround
//-----------------------------------------------------------------------------
bool C_FuncForceField::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	// Always open in post round time.
	if ( TDCGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		return false;

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		switch ( GetTeamNumber() )
		{
		case TDC_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;

		case TDC_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;
		}

		return true;
	}

	return false;
}