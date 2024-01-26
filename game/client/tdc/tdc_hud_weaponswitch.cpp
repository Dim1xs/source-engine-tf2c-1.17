//========= Copyright  1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include "controls/tdc_advbuttonbase.h"

#include "tdc_hud_weaponswitch.h"
#include "c_tdc_player.h"
#include "tdc_hud_freezepanel.h"

#include "tdc_gamerules.h"
#include "tdc_weapon_parse.h"
#include "c_tdc_playerresource.h"

using namespace vgui;


DECLARE_BUILD_FACTORY(CTDCWeaponSwitchIcon);

CTDCWeaponSwitchIcon::CTDCWeaponSwitchIcon( Panel *parent, const char *name ) : Panel( parent, name )
{
	m_icon = NULL;
}

void CTDCWeaponSwitchIcon::SetIcon( const CHudTexture *pIcon )
{
	m_icon = pIcon;
}

void CTDCWeaponSwitchIcon::PaintBackground( void )
{
	if ( m_icon )
	{
		m_icon->DrawSelf( 0, 0, GetWide(), GetTall(), COLOR_WHITE );
	}
}


DECLARE_HUDELEMENT( CTDCHudWeaponSwitch );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CItemModelPanel::CItemModelPanel( Panel *parent, const char* name ) : EditablePanel( parent, name )
{
	m_pWeaponIcon = new CTDCWeaponSwitchIcon( this, "WeaponIcon" );
	m_iBorderStyle = -1;
	m_bShowQuality = false;
	m_bModelOnly = false;
	m_iSlot = 0;
}

void CItemModelPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pDefaultBorder = pScheme->GetBorder( "TFFatLineBorder" );
	m_pSelectedRedBorder = pScheme->GetBorder( "TFFatLineBorderRedBG" );
	m_pSelectedBlueBorder = pScheme->GetBorder( "TFFatLineBorderBlueBG" );

	SetPaintBorderEnabled( false );
}

void CItemModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_bModelOnly = inResourceData->GetBool( "model_only" );
}

void CItemModelPanel::PerformLayout( void )
{
	// Set border.
	if ( m_iBorderStyle == -1 )
	{
		SetPaintBorderEnabled( false );
	}
	else if ( m_iBorderStyle == 0 )
	{
		SetPaintBorderEnabled( true );
		SetBorder( m_pDefaultBorder );
	}
	else if ( m_iBorderStyle == 1 )
	{
		int iTeam = GetLocalPlayerTeam();

		if ( iTeam == TDC_TEAM_RED )
		{
			SetBorder( m_pSelectedRedBorder );
		}
		else
		{
			SetBorder( m_pSelectedBlueBorder );
		}
	}

	// Primary icons use different size.
	if ( m_iSlot == 0 )
	{
		m_pWeaponIcon->SetBounds( 0, 0, GetWide(), GetTall() );
	}
	else
	{
		m_pWeaponIcon->SetBounds( GetWide() / 2 - GetTall() / 2, 0, GetTall(), GetTall() );
	}
}

void CItemModelPanel::SetWeapon( C_BaseCombatWeapon *pWeapon, int iBorderStyle, int ID )
{
	m_hWeapon = pWeapon;
	m_iBorderStyle = iBorderStyle;

	m_pWeaponIcon->SetIcon( m_hWeapon->GetSpriteInactive() );

	InvalidateLayout();
}

void CItemModelPanel::SetWeapon( ETDCWeaponID iWeapon, int iBorderStyle, int ID )
{
	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( iWeapon );
	if ( !pWeaponInfo )
		return;

	m_hWeapon = NULL;
	m_iBorderStyle = iBorderStyle;
	m_iSlot = ID;

	m_pWeaponIcon->SetIcon( pWeaponInfo->iconInactive );

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudWeaponSwitch::CTDCHudWeaponSwitch( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudWeaponSwitch" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pWeaponFrom = new CItemModelPanel( this, "WeaponFrom" );
	m_pWeaponTo = new CItemModelPanel( this, "WeaponTo" );

	ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCHudWeaponSwitch::ShouldDraw( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return false;

	ETDCWeaponID iWeaponTo = pPlayer->m_Shared.GetDesiredWeaponIndex();
	if ( iWeaponTo == WEAPON_NONE )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudWeaponSwitch::SetVisible( bool bVisible )
{
	if ( bVisible )
	{
		const char *key = engine->Key_LookupBinding( "+use" );
		if ( !key )
		{
			key = "< not bound >";
		}
		SetDialogVariable( "use", key );
	}

	BaseClass::SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudWeaponSwitch::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudWeaponSwitch.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudWeaponSwitch::OnTick()
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	m_iWeaponTo = pPlayer->m_Shared.GetDesiredWeaponIndex();
	if ( m_iWeaponTo == WEAPON_NONE )
		return;


	CTDCWeaponInfo *pWeaponInfo = GetTDCWeaponInfo( m_iWeaponTo );
	if ( !pWeaponInfo )
		return;

	if (m_iWeaponTo != m_iWeaponFrom)
	{
		m_pWeaponTo->SetWeapon(m_iWeaponTo, -1, pWeaponInfo->iSlot );
	}

	C_TDCWeaponBase *pWeapon = pPlayer->GetTDCWeaponBySlot( pWeaponInfo->iSlot );
	if ( !pWeapon )
		return;

	if ( pWeapon->GetWeaponID() != m_iWeaponFrom )
	{
		m_iWeaponFrom = pWeapon->GetWeaponID();
		m_pWeaponFrom->SetWeapon( m_iWeaponFrom, -1, pWeaponInfo->iSlot );
	}
}
