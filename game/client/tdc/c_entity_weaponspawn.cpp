//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawning entity.
//
//=============================================================================//
#include "cbase.h"
#include "glow_outline_effect.h"
#include "collisionutils.h"
#include "c_tdc_pickupitem.h"
#include "c_tdc_player.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tdc_weapon_respawn_timer( "tdc_weapon_respawn_timer", "1", FCVAR_REPLICATED, "Show visual respawn timers for weapons in Deathmatch." );

class C_WeaponSpawner : public C_TDCPickupItem
{
public:
	DECLARE_CLASS( C_WeaponSpawner, C_TDCPickupItem );
	DECLARE_CLIENTCLASS();

	C_WeaponSpawner();
	~C_WeaponSpawner();

	virtual void OnDataChanged( DataUpdateType_t type );

	void	Spawn( void );
	void	ClientThink( void );
	void	UpdateGlowEffect( void );

	virtual bool ShouldShowRespawnTimer( void ) { return tdc_weapon_respawn_timer.GetBool(); }
	virtual float GetRespawnTimerSize( void ) { return 20.0f; }
	virtual Color GetRespawnTimerFullColor( void );
	virtual Color GetRespawnTimerEmptyColor( void );

private:
	QAngle				m_qAngle;
	CGlowObject		   *m_pGlowEffect;
	char				m_iszWeaponName[64];
	bool				m_bShouldGlow;
	bool				m_bShouldFlash;
	float				m_flAlpha;
	bool				m_bAlphaRising;

	bool				m_bTouchingPlayer;
	bool				m_bStaticSpawner;
	bool				m_bOutlineDisabled;
	bool				m_bSpecialGlow;
};

IMPLEMENT_CLIENTCLASS_DT( C_WeaponSpawner, DT_WeaponSpawner, CWeaponSpawner )
	RecvPropBool( RECVINFO( m_bStaticSpawner ) ),	// Mapper var that disables the hovering weapon rotation
	RecvPropBool( RECVINFO( m_bOutlineDisabled ) ), // Mapper var that disables the weapon outlines
	RecvPropBool( RECVINFO( m_bSpecialGlow ) ),
	RecvPropString( RECVINFO( m_iszWeaponName ) )
END_RECV_TABLE()

C_WeaponSpawner::C_WeaponSpawner()
{
	m_qAngle = vec3_angle;

	m_pGlowEffect = NULL;
	m_bShouldGlow = false;
	m_bTouchingPlayer = false;
	m_bSpecialGlow = false;
	m_flAlpha = 0.0f;
	m_bAlphaRising = false;
}

C_WeaponSpawner::~C_WeaponSpawner()
{
	delete m_pGlowEffect;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_WeaponSpawner::Spawn( void )
{
	BaseClass::Spawn();
	m_qAngle = GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_WeaponSpawner::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged(type);

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		const char *pszWeaponName = m_iszWeaponName;

		// Handle temp translations.
		if ( V_strcmp( pszWeaponName, "WEAPON_TOMMYGUN" ) == 0 )
		{
			pszWeaponName = "WEAPON_REMOTEBOMB";
		}
		else if ( V_strcmp( pszWeaponName, "WEAPON_SIXSHOOTER" ) == 0 )
		{
			pszWeaponName = "WEAPON_REVOLVER";
		}
		else if ( V_strcmp( pszWeaponName, "WEAPON_SNIPERRIFLE" ) == 0 )
		{
			pszWeaponName = "WEAPON_LEVERRIFLE";
		}

		ETDCWeaponID m_iWeaponID = GetWeaponId( pszWeaponName );

		if ( m_iWeaponID == WEAPON_NONE )
		{
			return;
		}

		CTDCWeaponInfo *m_pWeaponInfo = GetTDCWeaponInfo( m_iWeaponID );
		Assert( m_pWeaponInfo );

		if ( m_pWeaponInfo->m_szTimerIconFull[0] != '\0' )
			SetReturnProgressMaterial_Full( m_pWeaponInfo->m_szTimerIconFull );

		if ( m_pWeaponInfo->m_szTimerIconEmpty[0] != '\0' )
			SetReturnProgressMaterial_Empty( m_pWeaponInfo->m_szTimerIconEmpty );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_WeaponSpawner::ClientThink( void )
{
	// The mapper disabled the rotating effect of this spawner
	if ( !m_bStaticSpawner )
	{
		m_qAngle.y += 90 * gpGlobals->frametime;
		if ( m_qAngle.y >= 360 )
			m_qAngle.y -= 360;

		SetAbsAngles( m_qAngle );
	}

	// The mapper has disabled glows for this specific weapon spawner
	if ( m_bOutlineDisabled )
		return;

	C_TDCPlayer *pPlayer = ToTDCPlayer( C_BasePlayer::GetLocalPlayer() );

	bool bShouldGlow = false;
	bool bTouchingPlayer = false;
	bool bShouldFlash = false;
	bool bIsZombie = pPlayer->IsZombie();

	if ( pPlayer && !m_bDisabled && !bIsZombie )
	{
		Vector vecPlayerOrigin = pPlayer->GetAbsOrigin();
		Vector vecPlayerMins = vecPlayerOrigin + pPlayer->GetPlayerMins();
		Vector vecPlayerMaxs = vecPlayerOrigin + pPlayer->GetPlayerMaxs();

		bTouchingPlayer = IsBoxIntersectingBox( GetAbsOrigin() + WorldAlignMins(), GetAbsOrigin() + WorldAlignMaxs(), vecPlayerMins, vecPlayerMaxs );\

		// Disable the outline if the weapon has been picked up.
		if ( m_bSpecialGlow )
		{
			// Super weapons glow through walls.
			if ( !m_bRespawning )
			{
				bShouldGlow = true;
				bShouldFlash = false;
			}
			else if ( m_flRespawnTime - gpGlobals->curtime < 15.0f )
			{
				// Start flashing 15 sseconds before respawning.
				bShouldGlow = true;
				bShouldFlash = true;
			}
		}
		else if ( !m_bRespawning )
		{
			// Temp crutch for Occluded\Unoccluded glow parameters not working.
			trace_t tr;
			UTIL_TraceLine( GetAbsOrigin(), MainViewOrigin(), MASK_VISIBLE, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction == 1.0f )
			{
				bShouldGlow = true;
			}
		}
	}

	if ( m_bShouldGlow != bShouldGlow ||
		m_bTouchingPlayer != bTouchingPlayer ||
		bShouldFlash ||
		m_bShouldFlash != bShouldFlash )
	{
		m_bShouldGlow = bShouldGlow;
		m_bTouchingPlayer = bTouchingPlayer;
		m_bShouldFlash = bShouldFlash;
		UpdateGlowEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_WeaponSpawner::UpdateGlowEffect( void )
{
	if ( !m_pGlowEffect )
	{
		m_pGlowEffect = new CGlowObject( this, Vector( 0.6f, 0.6f, 1.0f ), 1.0f, true, true );
	}

	if ( !m_bShouldGlow )
	{
		m_flAlpha = 0.0f;
		m_pGlowEffect->SetAlpha( m_flAlpha );
	}
	else
	{
		Vector vecColor;

		if ( m_bTouchingPlayer && !m_bRespawning )
		{
			// White glow.
			vecColor.Init( 0.76f, 0.76f, 0.76f );
		}
		else
		{
			// Blue glow.
			vecColor.Init( 0.6f, 0.6f, 1.0f );
		}

		m_pGlowEffect->SetColor( vecColor );

		if ( m_bShouldFlash )
		{
			if ( m_bAlphaRising )
			{
				if ( m_flAlpha == 1.0f )
				{
					m_bAlphaRising = !m_bAlphaRising;
				}
			}
			else
			{
				if ( m_flAlpha <= 0.5f )
				{
					m_bAlphaRising = !m_bAlphaRising;
				}
			}

			m_flAlpha = Approach( m_bAlphaRising ? 1.0f : 0.5f, m_flAlpha, 0.5f * gpGlobals->frametime );
		}
		else
		{
			m_flAlpha = 1.0f;
		}

		m_pGlowEffect->SetAlpha( m_flAlpha );
	}
}

Color C_WeaponSpawner::GetRespawnTimerFullColor()
{
	Color cColor = COLOR_WHITE;

	const char *pszWeaponName = m_iszWeaponName;

	// Handle temp translations.
	if ( V_strcmp( pszWeaponName, "WEAPON_TOMMYGUN" ) == 0 )
	{
		pszWeaponName = "WEAPON_REMOTEBOMB";
	}
	else if ( V_strcmp( pszWeaponName, "WEAPON_SIXSHOOTER" ) == 0 )
	{
		pszWeaponName = "WEAPON_REVOLVER";
	}
	else if ( V_strcmp( pszWeaponName, "WEAPON_SNIPERRIFLE" ) == 0 )
	{
		pszWeaponName = "WEAPON_LEVERRIFLE";
	}

	ETDCWeaponID m_iWeaponID = GetWeaponId( pszWeaponName );
	if ( m_iWeaponID == WEAPON_NONE )
	{
		return cColor;
	}

	CTDCWeaponInfo *m_pWeaponInfo = GetTDCWeaponInfo( m_iWeaponID );
	cColor = m_pWeaponInfo->m_cTimerIconFullColor;

	return cColor;
}

Color C_WeaponSpawner::GetRespawnTimerEmptyColor()
{
	Color cColor = COLOR_WHITE;

	const char *pszWeaponName = m_iszWeaponName;

	// Handle temp translations.
	if ( V_strcmp( pszWeaponName, "WEAPON_TOMMYGUN" ) == 0 )
	{
		pszWeaponName = "WEAPON_REMOTEBOMB";
	}
	else if ( V_strcmp( pszWeaponName, "WEAPON_SIXSHOOTER" ) == 0 )
	{
		pszWeaponName = "WEAPON_REVOLVER";
	}
	else if ( V_strcmp( pszWeaponName, "WEAPON_SNIPERRIFLE" ) == 0 )
	{
		pszWeaponName = "WEAPON_LEVERRIFLE";
	}

	ETDCWeaponID m_iWeaponID = GetWeaponId( pszWeaponName );
	if ( m_iWeaponID == WEAPON_NONE )
	{
		return cColor;
	}

	CTDCWeaponInfo *m_pWeaponInfo = GetTDCWeaponInfo( m_iWeaponID );
	cColor = m_pWeaponInfo->m_cTimerIconEmptyColor;

	return cColor;
}