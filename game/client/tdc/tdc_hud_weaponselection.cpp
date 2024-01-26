//========= Copyright © 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include <vgui_controls/EditablePanel.h>
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include "iinput.h"
#include "tdc_hud_ammostatus.h"
#include "c_tdc_player.h"

#define SELECTION_TIMEOUT_THRESHOLD		2.5f	// Seconds
#define SELECTION_FADEOUT_TIME			3.0f

#define FASTSWITCH_DISPLAY_TIMEOUT		0.5f
#define FASTSWITCH_FADEOUT_TIME			0.5f

#define NUM_WEAPON_ICONS 3

class CTDCWeaponIcon : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCWeaponIcon, vgui::EditablePanel );

	CTDCWeaponIcon( vgui::Panel *pParent, const char *pszName ) : BaseClass( pParent, pszName )
	{
		m_pAmmoBar = new CTDCAmmoBar( this, "AmmoBar" );
		m_Icon = NULL;
	}

	void SetWeaponIcon( const CHudTexture *pIcon ) { m_Icon = pIcon; }
	const CHudTexture *GetWeaponIcon( void ) { return m_Icon; }
	void SetAmmo( float flRatio ) { m_pAmmoBar->SetAmmo( flRatio ); }
	void PaintBackground( void )
	{
		if ( m_Icon )
		{
			int w, h;
			GetSize( w, h );
			m_Icon->DrawSelf( 0, 0, w, h, COLOR_WHITE );
		}
	}

private:
	const CHudTexture *m_Icon;
	CTDCAmmoBar *m_pAmmoBar;
};

//-----------------------------------------------------------------------------
// Purpose: tf weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection( const char *pElementName );

	virtual bool ShouldDraw();
	virtual void SetVisible( bool bVisible );
	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );
	virtual void SwitchToLastWeapon( void );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos );
	virtual void SelectWeaponSlot( int iSlot );
	virtual void SelectWeapon( void );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void );

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void Init();
	virtual void LevelInit();

	virtual void FireGameEvent( IGameEvent *event );

	virtual void Reset( void )
	{
		CBaseHudWeaponSelection::Reset();

		// selection time is a little farther back so we don't show it when we spawn
		m_flSelectionTime = gpGlobals->curtime - ( FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME + 0.1 );
	}

protected:
	virtual void OnTick();
	virtual void OnThink();
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual bool IsWeaponSelectable()
	{
		if ( IsInSelectionMode() )
			return true;

		return false;
	}
	void SetSelectedSlot( int iSlot ) { m_iSelectedSlot = iSlot; }

private:
	C_BaseCombatWeapon * FindNextWeaponInWeaponSelection( int iCurrentSlot, int iCurrentPosition );
	C_BaseCombatWeapon *FindPrevWeaponInWeaponSelection( int iCurrentSlot, int iCurrentPosition );

	void FastWeaponSwitch( int iWeaponSlot );
	int GetNumVisibleSlots();

	virtual	void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon )
	{
		m_hSelectedWeapon = pWeapon;
		InvalidateLayout();
	}

	vgui::EditablePanel *m_pWeaponPanels[NUM_WEAPON_ICONS];
	CTDCWeaponIcon *m_pWeaponIcons[NUM_WEAPON_ICONS];
	vgui::ImagePanel *m_pActiveImages[NUM_WEAPON_ICONS];

	int m_iActiveSlot;
	int m_iSelectedSlot;
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection( pElementName ), EditablePanel( NULL, "HudWeaponSelection" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	for ( int i = 0; i < NUM_WEAPON_ICONS; i++ )
	{
		m_pWeaponPanels[i] = new EditablePanel( this, VarArgs( "Weapon%d", i ) );
		m_pWeaponIcons[i] = new CTDCWeaponIcon( m_pWeaponPanels[i], "WeaponIcon" );
		m_pActiveImages[i] = new ImagePanel( m_pWeaponPanels[i], "ActiveBorder" );
	}

	m_iActiveSlot = -1;
	m_iSelectedSlot = -1;

	ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: sets up display for showing weapon pickup
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnWeaponPickup( C_BaseCombatWeapon *pWeapon )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnTick()
{
	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	for ( int i = 0; i < NUM_WEAPON_ICONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, 0 );
		if ( !pWeapon )
			continue;

		if ( pWeapon == pPlayer->GetActiveWeapon() && i != m_iActiveSlot )
		{
			m_iActiveSlot = i;
			InvalidateLayout();
		}

		if ( pWeapon->GetSpriteInactive() != m_pWeaponIcons[i]->GetWeaponIcon() )
		{
			// If this isn't updated the player will get stuck in the menu
			if ( i == m_iSelectedSlot )
			{
				SetSelectedWeapon( pWeapon );
			}

			m_pWeaponIcons[i]->SetWeaponIcon( pWeapon->GetSpriteInactive() ); 
			InvalidateLayout();
		}

		int iTotalAmmo = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
		int iMaxAmmo = pPlayer->GetMaxAmmo( pWeapon->GetPrimaryAmmoType() );

		if ( pWeapon->UsesClipsForAmmo1() )
		{
			iTotalAmmo += pWeapon->Clip1();
			iMaxAmmo += pWeapon->GetMaxClip1();
		}

		m_pWeaponIcons[i]->SetAmmo( (float)iTotalAmmo / (float)iMaxAmmo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink()
{
	float flSelectionTimeout = SELECTION_TIMEOUT_THRESHOLD;
	float flSelectionFadeoutTime = SELECTION_FADEOUT_TIME;
	if ( hud_fastswitch.GetBool() )
	{
		flSelectionTimeout = FASTSWITCH_DISPLAY_TIMEOUT;
		flSelectionFadeoutTime = FASTSWITCH_FADEOUT_TIME;
	}

	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > flSelectionTimeout )
	{
		// close
		if ( gpGlobals->curtime - m_flSelectionTime > flSelectionTimeout + flSelectionFadeoutTime )
		{
			HideSelection();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
		{
			HideSelection();
		}
		return false;
	}

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return false;

	if ( pPlayer->IsAlive() == false )
		return false;

	bool bret = CBaseHudWeaponSelection::ShouldDraw();
	if ( !bret )
		return false;

	// draw weapon selection a little longer if in fastswitch so we can see what we've selected
	if ( hud_fastswitch.GetBool() && ( gpGlobals->curtime - m_flSelectionTime ) < ( FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME ) )
		return true;

	return IsInSelectionMode();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SetVisible( bool bVisible )
{
	if ( bVisible != IsVisible() )
	{
		if ( bVisible )
		{
			InvalidateLayout();
		}
	}

	BaseClass::SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::Init()
{
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();
}

//-------------------------------------------------------------------------
// Purpose: Calculates how many weapons slots need to be displayed
//-------------------------------------------------------------------------
int CHudWeaponSelection::GetNumVisibleSlots()
{
	int nCount = 0;

	// iterate over all the weapon slots
	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		if ( GetFirstPos( i ) )
		{
			nCount++;
		}
	}

	return nCount;
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetPaintBackgroundEnabled( false );

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos( x, y );
	GetHudSize( screenWide, screenTall );
	SetBounds( 0, 0, screenWide, screenTall );

	// load control settings...
	LoadControlSettings( "resource/UI/HudWeaponSelection.res" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudWeaponSelection::PerformLayout( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	for ( int i = 0; i < NUM_WEAPON_ICONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, 0 );

		if ( pWeapon )
		{
			m_pWeaponPanels[i]->SetVisible( true );
			m_pWeaponIcons[i]->SetWeaponIcon( pWeapon->GetSpriteInactive() );
			m_pActiveImages[i]->SetVisible( hud_fastswitch.GetBool() ? i == m_iActiveSlot : pWeapon == GetSelectedWeapon() );
		}
		else
		{
			m_pWeaponPanels[i]->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert( !IsInSelectionMode() );

	CBaseHudWeaponSelection::OpenSelection();
	//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "OpenWeaponSelectionMenu" );
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	m_flSelectionTime = 0;
	CBaseHudWeaponSelection::HideSelection();
	m_iSelectedSlot = -1;
	//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "CloseWeaponSelectionMenu" );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindNextWeaponInWeaponSelection( int iCurrentSlot, int iCurrentPosition )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestNextSlot = MAX_WEAPON_SLOTS;
	int iLowestNextPosition = MAX_WEAPON_POSITIONS;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot > iCurrentSlot || ( weaponSlot == iCurrentSlot && weaponPosition > iCurrentPosition ) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot < iLowestNextSlot || ( weaponSlot == iLowestNextSlot && weaponPosition < iLowestNextPosition ) )
				{
					iLowestNextSlot = weaponSlot;
					iLowestNextPosition = weaponPosition;
					pNextWeapon = pWeapon;
				}
			}
		}
	}

	return pNextWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the prior available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindPrevWeaponInWeaponSelection( int iCurrentSlot, int iCurrentPosition )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pPrevWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestPrevSlot = -1;
	int iLowestPrevPosition = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot < iCurrentSlot || ( weaponSlot == iCurrentSlot && weaponPosition < iCurrentPosition ) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot > iLowestPrevSlot || ( weaponSlot == iLowestPrevSlot && weaponPosition > iLowestPrevPosition ) )
				{
					iLowestPrevSlot = weaponSlot;
					iLowestPrevPosition = weaponPosition;
					pPrevWeapon = pWeapon;
				}
			}
		}
	}

	return pPrevWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindNextWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindNextWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to start
		pNextWeapon = FindNextWeaponInWeaponSelection( -1, -1 );
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );
		SetSelectedSlot( pNextWeapon->GetSlot() );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindPrevWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindPrevWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to end of weapon list
		pNextWeapon = FindPrevWeaponInWeaponSelection( MAX_WEAPON_SLOTS, MAX_WEAPON_POSITIONS );
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );
		SetSelectedSlot( pNextWeapon->GetSlot() );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// Play the "cycle to next weapon" sound
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Switches the last weapon the player was using
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SwitchToLastWeapon( void )
{
	// Get the player's last weapon
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatWeapon *pWeapon = player->GetLastWeapon();
	if ( !player || !player->IsAllowedToSwitchWeapons() || pWeapon == player->GetActiveWeapon() )
	{
		player->EmitSound( "Player.DenyWeaponSelection" );
		return;
	}

	player->EmitSound( "Player.WeaponFastSelect" );
	::input->MakeWeaponSelection( pWeapon );
	UpdateSelectionTime();
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon( i );

		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos )
			return pWeapon;
	}

	return NULL;
}

C_BaseCombatWeapon *CHudWeaponSelection::GetSelectedWeapon( void )
{
	return m_hSelectedWeapon;
}

void CHudWeaponSelection::FireGameEvent( IGameEvent *event )
{
	CHudElement::FireGameEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::FastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// see where we should start selection
	int iPosition = -1;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	if ( pActiveWeapon && pActiveWeapon->GetSlot() == iWeaponSlot )
	{
		// start after this weapon
		iPosition = pActiveWeapon->GetPosition();
	}

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search for the weapon after the current one
	pNextWeapon = FindNextWeaponInWeaponSelection( iWeaponSlot, iPosition );
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iWeaponSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = FindNextWeaponInWeaponSelection( iWeaponSlot, -1 );
	}

	// see if we found a weapon that's different from the current and in the selected slot
	if ( pNextWeapon && pNextWeapon != pActiveWeapon && pNextWeapon->GetSlot() == iWeaponSlot )
	{
		// select the new weapon
		::input->MakeWeaponSelection( pNextWeapon );

		pPlayer->EmitSound( "Player.WeaponFastSelect" );
	}
	else
	{
		// error sound
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;

	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_FASTSWITCH:
	{
		FastWeaponSwitch( iSlot );
		return;
	}

	case HUDTYPE_BUCKETS:
	{
		int slotPos = 0;
		C_BaseCombatWeapon *pActiveWeapon = GetSelectedWeapon();

		// start later in the list
		if ( IsInSelectionMode() && pActiveWeapon && pActiveWeapon->GetSlot() == iSlot )
		{
			slotPos = pActiveWeapon->GetPosition() + 1;
		}

		// find the weapon in this slot
		pActiveWeapon = GetNextActivePos( iSlot, slotPos );
		if ( !pActiveWeapon )
		{
			pActiveWeapon = GetNextActivePos( iSlot, 0 );
		}

		if ( pActiveWeapon != NULL )
		{
			if ( !IsInSelectionMode() )
			{
				// open the weapon selection
				OpenSelection();
			}

			// Mark the change
			SetSelectedWeapon( pActiveWeapon );
			SetSelectedSlot( pActiveWeapon->GetSlot() );

			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
		}
		else
		{
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
		}
		return;
	}
	break;

	default:
		break;
	}

	pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}

//-----------------------------------------------------------------------------
// Purpose: Player has chosen to draw the currently selected weapon
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeapon( void )
{
	if ( !GetSelectedWeapon() )
	{
		HideSelection();
		return;
	}

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	// Don't allow selections of weapons that can't be selected (out of ammo, etc)
	if ( !GetSelectedWeapon()->CanBeSelected() )
	{
		player->EmitSound( "Player.DenyWeaponSelection" );
	}
	else
	{
		SetWeaponSelected();

		m_hSelectedWeapon = NULL;

		HideSelection();

		// Play the "weapon selected" sound
		switch ( hud_fastswitch.GetInt() )
		{
			case HUDTYPE_FASTSWITCH:
				player->EmitSound( "Player.WeaponFastSelect" );
				break;
			default:
				player->EmitSound( "Player.WeaponSelected" );
				break;
		}
	}
}
