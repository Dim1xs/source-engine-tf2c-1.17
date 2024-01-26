//======= Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTDC Regenerate Zone.
//
//=============================================================================//
#ifndef FUNC_REGENERATE_ZONE_H
#define FUNC_REGENERATE_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "props.h"

//=============================================================================
//
// CTDC Regenerate Zone class.
//
class CRestockZone : public CBaseTrigger, public TAutoList<CRestockZone>
{
public:
	DECLARE_CLASS( CRestockZone, CBaseTrigger );
	DECLARE_DATADESC();

	CRestockZone();

	void	Spawn( void );
	void	Precache( void );
	void	Activate( void );
	void	Touch( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );
	void	Regenerate( CTDCPlayer *pPlayer );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

protected:
	CHandle<CDynamicProp>	m_hAssociatedModel;

private:
	bool					m_bDisabled;
	bool					m_bRestoreHealth;
	bool					m_bRestoreAmmo;
	string_t				m_iszAssociatedModel;
};

#endif // FUNC_REGENERATE_ZONE_H
