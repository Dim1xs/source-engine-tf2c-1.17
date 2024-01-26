//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Create a muzzle flash temp ent
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basetempentity.h"
#include "coordsize.h"
#include "tdc_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTETDCBlood : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTETDCBlood, CBaseTempEntity );

	DECLARE_SERVERCLASS();

	CTETDCBlood( const char *name );

	virtual void Test( const Vector& current_origin, const QAngle& current_angles ) {}

public:
	Vector m_vecOrigin;
	Vector m_vecNormal;
	int m_nEntIndex;
	ETDCDmgCustom m_iDamageCustom;
};

// Singleton to fire TEMuzzleFlash objects
static CTETDCBlood g_TETDCBlood( "TFBlood" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETDCBlood::CTETDCBlood( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_nEntIndex = 0;
	m_iDamageCustom = TDC_DMG_CUSTOM_NONE;
}

IMPLEMENT_SERVERCLASS_ST( CTETDCBlood, DT_TETDCBlood )
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropVector( SENDINFO_NOCHECK( m_vecNormal ), 6, 0, -1.0f, 1.0f ),
	SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NOCHECK( m_iDamageCustom ) ),
END_SEND_TABLE()

void TE_TFBlood( IRecipientFilter& filter, float delay,
				const Vector &origin, const Vector &normal, int nEntIndex, ETDCDmgCustom iDamageCustom )
{
	g_TETDCBlood.m_vecOrigin		= origin;
	g_TETDCBlood.m_vecNormal		= normal;
	g_TETDCBlood.m_nEntIndex		= nEntIndex;
	g_TETDCBlood.m_iDamageCustom = iDamageCustom;

	// Send it over the wire
	g_TETDCBlood.Create( filter, delay );
}