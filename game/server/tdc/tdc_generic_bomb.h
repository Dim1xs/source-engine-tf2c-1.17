//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#ifndef TDC_GENERIC_BOMB_H
#define TDC_GENERIC_BOMB_H
#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// TF Generic Bomb Class
//

class CTDCGenericBomb : public CBaseAnimating, public TAutoList<CTDCGenericBomb>
{
public:
	DECLARE_CLASS( CTDCGenericBomb, CBaseAnimating );
	DECLARE_DATADESC();

	CTDCGenericBomb();

	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	void				InputDetonate( inputdata_t &inputdata );

protected:
	float				m_flDamage;
	float				m_flRadius;
	string_t			m_iszParticleName;
	string_t			m_iszExplodeSound;
	bool				m_bFriendlyFire;

	COutputEvent		m_OnDetonate;
};


#endif // TDC_GENERIC_BOMB_H
