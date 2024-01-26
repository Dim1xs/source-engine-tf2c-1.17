//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Dropped DM weapon
//
//=============================================================================//

#include "cbase.h"
#include "glow_outline_effect.h"
#include "c_tdc_player.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_TDCDroppedWeapon : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_TDCDroppedWeapon, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_TDCDroppedWeapon();
	~C_TDCDroppedWeapon();

	virtual void	OnDataChanged( DataUpdateType_t type );

	void	Spawn( void );
	void	ClientThink();
	void	UpdateGlowEffect();

private:
	CGlowObject *m_pGlowEffect;
	bool m_bShouldGlow;

	int m_iAmmo;
	int m_iMaxAmmo;
	bool m_bDissolving;
};

IMPLEMENT_CLIENTCLASS_DT( C_TDCDroppedWeapon, DT_TDCDroppedWeapon, CTDCDroppedWeapon )
	RecvPropInt( RECVINFO( m_iAmmo ) ),
	RecvPropInt( RECVINFO( m_iMaxAmmo ) ),
	RecvPropBool( RECVINFO( m_bDissolving ) ),
END_RECV_TABLE()


C_TDCDroppedWeapon::C_TDCDroppedWeapon()
{
	m_pGlowEffect = NULL;
	m_bShouldGlow = false;
}


C_TDCDroppedWeapon::~C_TDCDroppedWeapon()
{
	delete m_pGlowEffect;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function 
//-----------------------------------------------------------------------------
void C_TDCDroppedWeapon::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Start thinking
//-----------------------------------------------------------------------------
void C_TDCDroppedWeapon::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

void C_TDCDroppedWeapon::ClientThink()
{
	C_TDCPlayer *pPlayer = ToTDCPlayer( C_BasePlayer::GetLocalPlayer() );

	bool bShouldGlow = false;
	bool bIsZombie = pPlayer->IsZombie();

	if ( pPlayer && !m_bDissolving && !bIsZombie )
	{
		// Temp crutch for Occluded\Unoccluded glow parameters not working.
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), MainViewOrigin(), MASK_VISIBLE, this, COLLISION_GROUP_NONE, &tr );		
		bShouldGlow = ( tr.fraction == 1.0f );
	}

	if ( m_bShouldGlow != bShouldGlow )
	{
		m_bShouldGlow = bShouldGlow;
		UpdateGlowEffect();
	}
}

void C_TDCDroppedWeapon::UpdateGlowEffect()
{
	if ( !m_pGlowEffect )
	{
		Vector vecColor;

		if ( m_iMaxAmmo == 0 )
		{
			vecColor = Vector( 0.15f, 0.75f, 0.15f );
		}
		else
		{
			vecColor.x = RemapValClamped( m_iAmmo, m_iMaxAmmo / 2, m_iMaxAmmo, 0.75f, 0.15f );
			vecColor.y = RemapValClamped( m_iAmmo, 0, m_iMaxAmmo / 2, 0.15f, 0.75f );
			vecColor.z = 0.15f;
		}

		m_pGlowEffect = new CGlowObject( this, vecColor, 1.0f, true, true );
	}

	if ( m_bShouldGlow )
	{
		m_pGlowEffect->SetAlpha( 1.0f );
	}
	else
	{
		m_pGlowEffect->SetAlpha( 0.0f );
	}
}
