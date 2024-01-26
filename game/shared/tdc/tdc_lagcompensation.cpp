//=============================================================================
//
// Purpose: RAII class to make sure we exit lag compensation when going out of scope.
//
//=============================================================================
#include "cbase.h"
#include "tdc_lagcompensation.h"

#ifdef GAME_DLL
#include "ilagcompensationmanager.h"

CEnableLagCompensation::CEnableLagCompensation( CBasePlayer *pPlayer, CUserCmd *cmd, bool bEnable /*= true*/ )
{
	m_bEnabled = bEnable;

	if ( m_bEnabled )
	{
		m_pPlayer = pPlayer;
		lagcompensation->StartLagCompensation( m_pPlayer, cmd );
	}
}

CEnableLagCompensation::~CEnableLagCompensation()
{
	if ( lagcompensation->IsCurrentlyDoingLagCompensation() )
	{
		AssertMsg( false, "Did not finish lag compensation properly!" );
		Finish();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnableLagCompensation::Finish( void )
{
	if ( m_bEnabled )
	{
		lagcompensation->FinishLagCompensation( m_pPlayer );
	}
}

#endif
