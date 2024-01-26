//=============================================================================//
//
// Purpose: HUD for drawing screen overlays.
//
//=============================================================================//
#include "cbase.h"
#include "tdc_hud_screenoverlays.h"
#include "iclientmode.h"
#include "c_tdc_player.h"
#include "view_scene.h"
#include "functionproxy.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CHudScreenOverlays g_ScreenOverlayManager;

struct ScreenOverlayData_t
{
	const char *pszName;
	bool bTeamColored;
};

ScreenOverlayData_t g_aScreenOverlays[TDC_OVERLAY_COUNT] =
{
	{ "effects/berserk_overlay", false },
	{ "effects/dodge_overlay", false },
	{ "effects/imcookin", false },
	{ "effects/invuln_overlay_%s", true },
	{ "effects/headshot_overlay", false },
	{ "effects/softzoom_overlay_dm", false },
	{ "dev/desaturate", false },
};

CHudScreenOverlays::CHudScreenOverlays()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScreenOverlays::LevelInitPreEntity( void )
{
	for ( int i = 0; i < TDC_OVERLAY_COUNT; i++ )
	{
		if ( g_aScreenOverlays[i].bTeamColored )
		{
			// Precache all team colored versions including FFA.
			for ( int j = FIRST_GAME_TEAM; j < TDC_TEAM_COUNT; j++ )
			{
				m_ScreenOverlays[i][j].Init( VarArgs( g_aScreenOverlays[i].pszName, g_aTeamLowerNames[j] ), TEXTURE_GROUP_CLIENT_EFFECTS );
			}

			m_ScreenOverlays[i][TEAM_SPECTATOR].Init( VarArgs( g_aScreenOverlays[i].pszName, "dm" ), TEXTURE_GROUP_CLIENT_EFFECTS );
		}
		else
		{
			m_ScreenOverlays[i][TEAM_UNASSIGNED].Init( g_aScreenOverlays[i].pszName, TEXTURE_GROUP_CLIENT_EFFECTS );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScreenOverlays::LevelShutdownPostEntity( void )
{
	for ( int i = 0; i < TDC_OVERLAY_COUNT; i++ )
	{
		for ( int j = 0; j < TDC_TEAM_COUNT; j++ )
		{
			m_ScreenOverlays[i][j].Shutdown();
		}
	}
}

#define MAKEFLAG(x)	( 1 << x )
extern ConVar tdc_headshoteffect_fadetime;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudScreenOverlays::DrawOverlays( const CViewSetup &view )
{
	C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( !pPlayer )
		return;

	// Check which overlays we should draw.
	int nOverlaysToDraw = 0;

	if ( pPlayer->IsAlive() )
	{
		if ( pPlayer->m_Shared.InCond( TDC_COND_BURNING ) )
		{
			nOverlaysToDraw |= MAKEFLAG( TDC_OVERLAY_BURNING );
		}

		if ( pPlayer->m_Shared.InCond( TDC_COND_POWERUP_RAGEMODE ) )
		{
			nOverlaysToDraw |= MAKEFLAG( TDC_OVERLAY_BERSERK );
		}

		if ( pPlayer->m_Shared.InCond( TDC_COND_POWERUP_SPEEDBOOST ) )
		{
			nOverlaysToDraw |= MAKEFLAG( TDC_OVERLAY_HASTE );
		}

		if ( gpGlobals->curtime - pPlayer->m_flHeadshotFadeTime < tdc_headshoteffect_fadetime.GetFloat() )
		{
			nOverlaysToDraw |= MAKEFLAG( TDC_OVERLAY_HEADSHOT );
		}

		if ( pPlayer->GetFOV() != pPlayer->GetDefaultFOV() && !pPlayer->m_Shared.InCond( TDC_COND_ZOOMED ) )
		{
			nOverlaysToDraw |= MAKEFLAG( TDC_OVERLAY_ZOOM );
		}

		if ( pPlayer->GetDesaturationAmount() != 0.0f )
		{
			nOverlaysToDraw |= MAKEFLAG( TDC_OVERLAY_DESAT );
		}
	}

	int iTeam = pPlayer->GetTeamNumber();

	// Draw overlays, the order is important.
	for ( int i = 0; i < TDC_OVERLAY_COUNT; i++ )
	{
		IMaterial *pMaterial;
		if ( g_aScreenOverlays[i].bTeamColored )
		{
			pMaterial = TDCGameRules()->IsTeamplay() ? m_ScreenOverlays[i][iTeam] : m_ScreenOverlays[i][TEAM_SPECTATOR];
		}
		else
		{
			pMaterial = m_ScreenOverlays[i][TEAM_UNASSIGNED];
		}

		if ( ( nOverlaysToDraw & MAKEFLAG( i ) ) && pMaterial )
		{
			int x = view.x;
			int y = view.y;
			int w = view.width;
			int h = view.height;

			if ( pMaterial->NeedsFullFrameBufferTexture() )
			{
				// FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
				DrawScreenEffectMaterial( pMaterial, x, y, w, h );
			}
			else if ( pMaterial->NeedsPowerOfTwoFrameBufferTexture() )
			{
				// First copy the FB off to the offscreen texture
				UpdateRefractTexture( x, y, w, h, true );

				// Now draw the entire screen using the material...
				CMatRenderContextPtr pRenderContext( materials );
				ITexture *pTexture = GetPowerOfTwoFrameBufferTexture();
				int sw = pTexture->GetActualWidth();
				int sh = pTexture->GetActualHeight();
				// Note - don't offset by x,y - already done by the viewport.
				pRenderContext->DrawScreenSpaceRectangle( pMaterial, 0, 0, w, h,
					0, 0, sw - 1, sh - 1, sw, sh );
			}
			else
			{
				byte color[4] = { 255, 255, 255, 255 };
				render->ViewDrawFade( color, pMaterial );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Proxy for damage received from headshots
//-----------------------------------------------------------------------------
class CHeadshotDamageProxy : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );
		if ( pPlayer )
		{
			float flScale = RemapValClamped( gpGlobals->curtime - pPlayer->m_flHeadshotFadeTime,
				0.0f, tdc_headshoteffect_fadetime.GetFloat(),
				1.0f, 0.4f );

			// We're just sort of plugging in the condition duration
			// Which we're interpretting as some sort of opacity value
			// This also assumes the value is between 0.0 and 1.0
			SetFloatResult( flScale );
		}
	}
};

EXPOSE_INTERFACE( CHeadshotDamageProxy, IMaterialProxy, "HeadshotDamage" IMATERIAL_PROXY_INTERFACE_VERSION );

extern ConVar tdc_zoom_fov;

//-----------------------------------------------------------------------------
// Purpose: Proxy for softzoom
//-----------------------------------------------------------------------------
class CSoftZoomScaleProxy : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );
		if ( pPlayer )
		{
			float flScale = RemapValClamped( pPlayer->GetFOV(),
				tdc_zoom_fov.GetFloat(), pPlayer->GetDefaultFOV(),
				1.0f, 0.4f );

			SetFloatResult( flScale );
		}
	}
};

EXPOSE_INTERFACE( CSoftZoomScaleProxy, IMaterialProxy, "SoftZoomScale" IMATERIAL_PROXY_INTERFACE_VERSION );

class CDesaturationProxy : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );
		if ( pPlayer )
		{
			m_pResult->SetFloatValue( pPlayer->GetDesaturationAmount() );
		}
	}
};

EXPOSE_INTERFACE( CDesaturationProxy, IMaterialProxy, "DesaturationAmount" IMATERIAL_PROXY_INTERFACE_VERSION );
