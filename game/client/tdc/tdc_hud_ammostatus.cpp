//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tdc_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>

#include "tdc_controls.h"
#include "in_buttons.h"
#include "tdc_imagepanel.h"
#include "c_team.h"
#include "c_tdc_player.h"
#include "ihudlcd.h"
#include "tdc_hud_ammostatus.h"
#include "tdc_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCAmmoBar::CTDCAmmoBar( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_flAmmo = 1.0f;

	m_iMaterialIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCAmmoBar::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *pszImage = inResourceData->GetString( "ammoImage", "hud/health_color" );
	m_iMaterialIndex = surface()->DrawGetTextureId( pszImage );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();
	}
	surface()->DrawSetTextureFile( m_iMaterialIndex, pszImage, true, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCAmmoBar::Paint()
{
	BaseClass::Paint();

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );

	float barWide = wide * m_flAmmo;

	// blend in the red "damage" part
	surface()->DrawSetTexture( m_iMaterialIndex );

	Vertex_t vert[4];
	vert[0].Init( Vector2D( wide - barWide, 0.0f ), Vector2D( 1.0f - m_flAmmo, 0.0f ) );
	vert[1].Init( Vector2D( wide, 0.0f ), Vector2D( 1.0f, 0.0f ) );
	vert[2].Init( Vector2D( wide, tall ), Vector2D( 1.0f, 1.0f ) );
	vert[3].Init( Vector2D( wide - barWide, tall ), Vector2D( 1.0f - m_flAmmo, 1.0f ) );

	surface()->DrawSetColor( m_flAmmo < m_flAmmoWarning ? m_clrAmmoWarningColor : COLOR_WHITE );

	surface()->DrawTexturedPolygon( 4, vert );
}

DECLARE_HUDELEMENT( CTDCHudWeaponAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCHudWeaponAmmo::CTDCHudWeaponAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudWeaponAmmo" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(ammo_primary)", "0" );
	hudlcd->SetGlobalStat( "(ammo_secondary)", "0" );
	hudlcd->SetGlobalStat( "(weapon_print_name)", "" );
	hudlcd->SetGlobalStat( "(weapon_name)", "" );

	m_pInClip = new CExLabel( this, "AmmoInClip", "" );
	m_pInClipShadow = new CExLabel( this, "AmmoInClipShadow", "" );
	m_pInReserve = new CExLabel( this, "AmmoInReserve", "" );
	m_pInReserveShadow = new CExLabel( this, "AmmoInReserveShadow", "" );
	m_pNoClip = new CExLabel( this, "AmmoNoClip", "" );
	m_pNoClipShadow = new CExLabel( this, "AmmoNoClipShadow", "" );
	m_pLowAmmoImage = new ImagePanel( this, "HudWeaponLowAmmoImage" );
	m_pAmmoBar = new CTDCAmmoBar( this, "AmmoBar" );
	m_pAmmoIcon = new ImagePanel( this, "IconAmmoImage" );

	m_iLowAmmoX = m_iLowAmmoY = m_iLowAmmoWide = m_iLowAmmoTall = 0;

	m_nAmmo = -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;
	m_iClass = TDC_CLASS_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudWeaponAmmo::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudWeaponAmmo::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	KeyValues *pConditions = new KeyValues( "conditions" );
	AddSubKeyNamed( pConditions, g_aPlayerClassNames_NonLocalized[m_iClass] );
	LoadControlSettings( "resource/UI/HudAmmoWeapons.res" );
	pConditions->deleteThis();

	m_pLowAmmoImage->GetBounds( m_iLowAmmoX, m_iLowAmmoY, m_iLowAmmoWide, m_iLowAmmoTall );

	m_nAmmo = -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;

	UpdateAmmoLabels( false, false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCHudWeaponAmmo::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

	if ( !pPlayer )
	{
		return false;
	}

	C_TDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if ( !pWeapon )
	{
		return false;
	}

	// Hide the panel if the weapon uses no ammo.
	if ( !pWeapon->UsesPrimaryAmmo() )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudWeaponAmmo::UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip )
{
	if ( m_pInClip && m_pInClipShadow )
	{
		if ( m_pInClip->IsVisible() != bPrimary )
		{
			m_pInClip->SetVisible( bPrimary );
			m_pInClipShadow->SetVisible( bPrimary );
		}
	}

	if ( m_pInReserve && m_pInReserveShadow )
	{
		if ( m_pInReserve->IsVisible() != bReserve )
		{
			m_pInReserve->SetVisible( bReserve );
			m_pInReserveShadow->SetVisible( bReserve );
		}
	}

	if ( m_pNoClip && m_pNoClipShadow )
	{
		if ( m_pNoClip->IsVisible() != bNoClip )
		{
			m_pNoClip->SetVisible( bNoClip );
			m_pNoClipShadow->SetVisible( bNoClip );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTDCHudWeaponAmmo::OnThink()
{
	// Get the player and active weapon.
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsPlayerClass( m_iClass ) )
	{
		m_iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		InvalidateLayout( true, true );
	}

	C_TDCWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if ( m_flNextThink < gpGlobals->curtime )
	{
		hudlcd->SetGlobalStat( "(weapon_print_name)", pWeapon ? pWeapon->GetPrintName() : " " );
		hudlcd->SetGlobalStat( "(weapon_name)", pWeapon ? pWeapon->GetName() : " " );

		if ( !m_pAmmoIcon->IsVisible() )
		{
			m_pAmmoIcon->SetVisible( true );
		}

		if ( pWeapon && pWeapon->GetTDCWpnData().m_szAmmoIcon[0] )
		{
			m_pAmmoIcon->SetImage( pWeapon->GetTDCWpnData().m_szAmmoIcon );
		}
		else
		{
			m_pAmmoIcon->SetImage( "../hud/icon_ammo_unknown" );
		}

		if ( !pWeapon || !pWeapon->UsesPrimaryAmmo() )
		{
			hudlcd->SetGlobalStat( "(ammo_primary)", "n/a" );
			hudlcd->SetGlobalStat( "(ammo_secondary)", "n/a" );

			// turn off our ammo counts
			UpdateAmmoLabels( false, false, false );

			m_nAmmo = -1;
			m_nAmmo2 = -1;

			m_hCurrentActiveWeapon = pWeapon;

			if ( m_pLowAmmoImage && m_pLowAmmoImage->IsVisible() )
			{
				m_pLowAmmoImage->SetVisible( false );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudLowAmmoPulseStop" );
			}

			m_pAmmoBar->SetAmmo( 1.0f );
		}
		else
		{
			// Get the ammo in our clip.
			int nAmmo1 = pWeapon->Clip1();
			int nAmmo2 = 0;
			// Clip ammo not used, get total ammo count.
			if ( nAmmo1 < 0 )
			{
				nAmmo1 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}
			// Clip ammo, so the second ammo is the total ammo.
			else
			{
				nAmmo2 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}

			hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", nAmmo1 ) );
			hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", nAmmo2 ) );

			if ( m_nAmmo != nAmmo1 || m_nAmmo2 != nAmmo2 || m_hCurrentActiveWeapon.Get() != pWeapon )
			{
				m_nAmmo = nAmmo1;
				m_nAmmo2 = nAmmo2;
				m_hCurrentActiveWeapon = pWeapon;

				if ( !pPlayer->HasInfiniteAmmo() )
				{
					if ( m_hCurrentActiveWeapon.Get()->UsesClipsForAmmo1() )
					{
						UpdateAmmoLabels( true, true, false );

						SetDialogVariable( "Ammo", m_nAmmo );
						SetDialogVariable( "AmmoInReserve", m_nAmmo2 );
					}
					else
					{
						UpdateAmmoLabels( false, false, true );
						SetDialogVariable( "Ammo", m_nAmmo );
					}
				}
				else
				{
					if ( m_hCurrentActiveWeapon.Get()->UsesClipsForAmmo1() )
					{
						UpdateAmmoLabels( true, true, false );

						SetDialogVariable( "Ammo", m_nAmmo );
						SetDialogVariable( "AmmoInReserve", L"\u221E" );
					}
					else
					{
						UpdateAmmoLabels( true, false, false );
						SetDialogVariable( "Ammo", L"\u221E" );
					}
				}

				// Check low ammo warning pulse.
				// We want to include both clip and max ammo in calculation.
				int iTotalAmmo = m_nAmmo + m_nAmmo2;
				int iMaxAmmo = pPlayer->GetMaxAmmo( pWeapon->GetPrimaryAmmoType() );

				if ( pWeapon->UsesClipsForAmmo1() )
				{
					iMaxAmmo += pWeapon->GetMaxClip1();
				}

				float flRatio = (float)iTotalAmmo / (float)iMaxAmmo;

				if ( flRatio < m_flAmmoWarning )
				{
					if ( !m_pLowAmmoImage->IsVisible() )
					{
						m_pLowAmmoImage->SetBounds( m_iLowAmmoX, m_iLowAmmoY, m_iLowAmmoWide, m_iLowAmmoTall );
						m_pLowAmmoImage->SetVisible( true );
						m_pLowAmmoImage->SetFgColor( m_clrAmmoWarningColor );
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudLowAmmoPulse" );
					}

					// Rescale the flashing border based on ammo count.
					int iScaleFactor = RemapValClamped( flRatio, m_flAmmoWarning, 0, 0, m_nAmmoWarningPosAdj );
					m_pLowAmmoImage->SetBounds(
						m_iLowAmmoX - iScaleFactor,
						m_iLowAmmoY - iScaleFactor,
						m_iLowAmmoWide + iScaleFactor * 2,
						m_iLowAmmoTall + iScaleFactor * 2 );
				}
				else
				{
					if ( m_pLowAmmoImage->IsVisible() )
					{
						m_pLowAmmoImage->SetVisible( false );
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudLowAmmoPulseStop" );
					}
				}

				m_pAmmoBar->SetAmmo( flRatio );
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
