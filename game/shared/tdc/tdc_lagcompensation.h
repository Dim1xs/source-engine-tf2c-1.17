//=============================================================================
//
// Purpose: RAII class to make sure we exit lag compensation when going out of scope.
//
//=============================================================================
#ifndef TDC_LAGCOMPENSATION_H
#define TDC_LAGCOMPENSATION_H

#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL

#define START_LAG_COMPENSATION( player, cmd ) CEnableLagCompensation lagcompenabler( player, cmd )
#define FINISH_LAG_COMPENSATION() lagcompenabler.Finish()
#define START_LAG_COMPENSATION_CONDITIONAL( player, cmd, condition ) CEnableLagCompensation lagcompenabler( player, cmd, condition )

class CEnableLagCompensation
{
public:
	CEnableLagCompensation( CBasePlayer *pPlayer, CUserCmd *cmd, bool bEnable = true );
	~CEnableLagCompensation();

	void Finish();

private:
	CBasePlayer *m_pPlayer;
	bool m_bEnabled;
};

#else

#define START_LAG_COMPENSATION( player, cmd ) (void(0))
#define FINISH_LAG_COMPENSATION() (void(0))
#define START_LAG_COMPENSATION_CONDITIONAL( player, cmd, condition ) (void(0))

#endif

#endif // TDC_LAGCOMPENSATION_H
