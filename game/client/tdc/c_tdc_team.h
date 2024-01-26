//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Client side CTDCTeam class
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TDC_TEAM_H
#define C_TDC_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "shareddefs.h"

class C_BaseEntity;
class C_BaseObject;
class CBaseTechnology;

//-----------------------------------------------------------------------------
// Purpose: TF's Team manager
//-----------------------------------------------------------------------------
class C_TDCTeam : public C_Team
{
	DECLARE_CLASS( C_TDCTeam, C_Team );
	DECLARE_CLIENTCLASS();

public:
					C_TDCTeam();
	virtual			~C_TDCTeam();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );

	int				GetRoundScore( void ) { return m_iRoundScore; }
	void			UpdateTeamName( void );
	const wchar_t	*GetTeamName( void ) { return m_wszLocalizedTeamName; }

private:
	int		m_iRoundScore;
	wchar_t	m_wszLocalizedTeamName[128];
};

C_TDCTeam *GetGlobalTFTeam( int iTeamNumber );
const wchar_t *GetLocalizedTeamName( int iTeamNumber );

#endif // C_TDC_TEAM_H
