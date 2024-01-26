//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "clientmode.h"
#include "c_tdc_player.h"
#include "tdc_hud_crosshair.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "mathlib/mathlib.h"
#include "basecombatweapon_shared.h"
#include "view.h"

ConVar cl_crosshair_red( "cl_crosshair_red", "255", FCVAR_ARCHIVE );
ConVar cl_crosshair_green( "cl_crosshair_green", "255", FCVAR_ARCHIVE );
ConVar cl_crosshair_blue( "cl_crosshair_blue", "255", FCVAR_ARCHIVE );
ConVar cl_crosshair_enemy_red( "cl_crosshair_enemy_red", "255", FCVAR_ARCHIVE );
ConVar cl_crosshair_enemy_green( "cl_crosshair_enemy_green", "0", FCVAR_ARCHIVE );
ConVar cl_crosshair_enemy_blue( "cl_crosshair_enemy_blue", "0", FCVAR_ARCHIVE );

ConVar cl_crosshair_file( "cl_crosshair_file", "", FCVAR_ARCHIVE );

ConVar cl_crosshair_scale( "cl_crosshair_scale", "32.0", FCVAR_ARCHIVE );
ConVar cl_crosshair_approach_speed( "cl_crosshair_approach_speed", "0.015" );

ConVar cl_dynamic_crosshair( "cl_dynamic_crosshair", "1", FCVAR_ARCHIVE );

using namespace vgui;

DECLARE_HUDELEMENT(CHudTFCrosshair);

CHudTFCrosshair::CHudTFCrosshair(const char *pElementName) :
	CHudCrosshair("CHudCrosshair")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pCrosshair = 0;
	m_szPreviousCrosshair[0] = '\0';

	m_pFrameVar = NULL;
	m_flAccuracy = 0.1;

	m_clrCrosshair = Color(0, 0, 0, 0);

	m_vecCrossHairOffsetAngle.Init();

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR);
}

void CHudTFCrosshair::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings( scheme );

	SetSize( ScreenWidth(), ScreenHeight() );
}

void CHudTFCrosshair::LevelShutdown(void)
{
	// forces m_pFrameVar to recreate next map
	m_szPreviousCrosshair[0] = '\0';

	if (m_pCrosshairOverride)
	{
		delete m_pCrosshairOverride;
		m_pCrosshairOverride = NULL;
	}

	if ( m_pFrameVar )
	{
		delete m_pFrameVar;
		m_pFrameVar = NULL;
	}
}

void CHudTFCrosshair::Init()
{
	m_iCrosshairTextureID = vgui::surface()->CreateNewTextureID();
}

void CHudTFCrosshair::SetCrosshair(CHudTexture *texture, Color& clr)
{
	m_pCrosshair = texture;
	m_clrCrosshair = clr;
}

bool CHudTFCrosshair::ShouldDraw()
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( !pPlayer )
		return false;

	CTDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if ( !pWeapon )
		return false;

	if ( pPlayer->m_Shared.InCond( TDC_COND_TAUNTING ) || pPlayer->m_Shared.IsLoser() )
		return false;

	return pWeapon->ShouldDrawCrosshair();
}

void CHudTFCrosshair::Paint()
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( !pPlayer )
		return;

	const char *crosshairfile = cl_crosshair_file.GetString();

	if ( crosshairfile[0] == '\0' )
	{
		CTDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

		if ( !pWeapon )
			return;

		m_pCrosshair = pWeapon->GetWpnData().iconCrosshair;
		BaseClass::Paint();
		return;
	}
	else
	{
		if ( Q_stricmp( m_szPreviousCrosshair, crosshairfile ) != 0 )
		{
			char buf[256];
			V_sprintf_safe( buf, "vgui/crosshairs/%s", crosshairfile );

			vgui::surface()->DrawSetTextureFile( m_iCrosshairTextureID, buf, true, false );

			if ( m_pCrosshairOverride )
			{
				delete m_pCrosshairOverride;
			}

			m_pCrosshairOverride = vgui::surface()->DrawGetTextureMatInfoFactory( m_iCrosshairTextureID );

			if ( !m_pCrosshairOverride )
				return;

			if ( m_pFrameVar )
			{
				delete m_pFrameVar;
			}

			bool bFound = false;
			m_pFrameVar = m_pCrosshairOverride->FindVarFactory( "$frame", &bFound );
			Assert( bFound );

			m_nNumFrames = m_pCrosshairOverride->GetNumAnimationFrames();

			// save the name to compare with the cvar in the future
			V_strcpy_safe( m_szPreviousCrosshair, crosshairfile );
		}

		bool bEnemyInSights = false;
		C_TDCPlayer *pPlayer = GetLocalObservedPlayer( false );
		if ( pPlayer )
		{
			// Trace forward to see if we're aiming at an enemy.
			C_TDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
			float flRange = pWeapon ? pWeapon->GetTDCWpnData().m_flOptimalRange : MAX_TRACE_LENGTH;

			Vector vecStart, vecEnd;
			vecStart = pPlayer->EyePosition();
			vecEnd = vecStart + MainViewForward() * flRange;
			trace_t tr;
			UTIL_TraceLine( vecStart, vecEnd, MASK_TFSHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

			C_TDCPlayer *pOther = ToTDCPlayer( tr.m_pEnt );

			if ( pOther && !pOther->m_Shared.IsStealthed() && pOther->IsEnemy( pPlayer ) )
			{
				bEnemyInSights = true;
			}
		}

		Color clr;
		if ( bEnemyInSights )
		{
			clr.SetColor( cl_crosshair_enemy_red.GetInt(), cl_crosshair_enemy_green.GetInt(), cl_crosshair_enemy_blue.GetInt(), 255 );
		}
		else
		{
			clr.SetColor( cl_crosshair_red.GetInt(), cl_crosshair_green.GetInt(), cl_crosshair_blue.GetInt(), 255 );
		}

		float x, y;
		bool bBehindCamera;
		GetDrawPosition( &x, &y, &bBehindCamera, m_vecCrossHairOffsetAngle );

		if ( bBehindCamera )
			return;

		int screenWide, screenTall;
		GetHudSize( screenWide, screenTall );

		int iWidth, iHeight;

		iWidth = iHeight = (int)( cl_crosshair_scale.GetFloat() + 0.5f );
		int iX = (int)( x + 0.5f );
		int iY = (int)( y + 0.5f );

		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawSetTexture( m_iCrosshairTextureID );
		vgui::surface()->DrawTexturedRect( iX - iWidth, iY - iHeight, iX + iWidth, iY + iHeight );
		vgui::surface()->DrawSetTexture( 0 );
	}

	/*
	if (m_pFrameVar)
	{
		if (cl_dynamic_crosshair.GetBool() == false)
		{
			m_pFrameVar->SetIntValue(0);
		}
		else
		{
			CTDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

			if (!pWeapon)
				return;

			float accuracy = 5;//pWeapon->GetWeaponAccuracy(pPlayer->GetAbsVelocity().Length2D());

			float flMin = 0.02;

			float flMax = 0.125;

			accuracy = clamp(accuracy, flMin, flMax);

			// approach this accuracy from our current accuracy
			m_flAccuracy = Approach(accuracy, m_flAccuracy, cl_crosshair_approach_speed.GetFloat());

			float flFrame = RemapVal(m_flAccuracy, flMin, flMax, 0, m_nNumFrames - 1);

			m_pFrameVar->SetIntValue((int)flFrame);
		}
	}

*/
}

