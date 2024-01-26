//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include <engine/IEngineSound.h>

#include "hud_numericdisplay.h"
#include "c_team.h"
#include "c_tdc_player.h"
#include "tdc_shareddefs.h"
#include "tdc_hud_playerstatus.h"
#include "tdc_hud_target_id.h"
#include "tdc_gamerules.h"

using namespace vgui;

ConVar tdc_low_health_sound( "tdc_low_health_sound", "0", FCVAR_ARCHIVE, "Play a warning sound when player's health drops below the percentage set by tdc_low_health_jingle_threshold." );
ConVar tdc_low_health_sound_threshold( "tdc_low_health_sound_threshold", "0.49", FCVAR_ARCHIVE, "Low health warning threshold percentage.", true, 0.0f, true, 1.0f );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHealthPanel::CTDCHealthPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_flHealth = 1.0f;

	m_iMaterialIndex = -1;
	m_iDeadMaterialIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHealthPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *pszImage = inResourceData->GetString( "healthImage", "hud/health_color" );
	m_iMaterialIndex = surface()->DrawGetTextureId( pszImage );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();
	}
	surface()->DrawSetTextureFile( m_iMaterialIndex, pszImage, true, false );

	const char *pszDeadImage = inResourceData->GetString( "deadImage", "hud/health_dead" );
	m_iDeadMaterialIndex = surface()->DrawGetTextureId( pszDeadImage );
	if ( m_iDeadMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iDeadMaterialIndex = surface()->CreateNewTextureID();
	}
	surface()->DrawSetTextureFile( m_iDeadMaterialIndex, pszDeadImage, true, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHealthPanel::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];

	if ( m_flHealth <= 0 )
	{
		// Draw the dead material
		surface()->DrawSetTexture( m_iDeadMaterialIndex );
		
		vert[0].Init( Vector2D( 0, 0 ), Vector2D( 0.0f, 0.0f ) );
		vert[1].Init( Vector2D( w, 0 ), Vector2D( 1.0f, 0.0f ) );
		vert[2].Init( Vector2D( w, h ), Vector2D( 1.0f, 1.0f ) );				
		vert[3].Init( Vector2D( 0, h ), Vector2D( 0.0f, 1.0f ) );

		surface()->DrawSetColor( Color(255,255,255,255) );
	}
	else if ( m_bLeftToRight )
	{
		float flDamageX = w * m_flHealth;

		// blend in the red "damage" part
		surface()->DrawSetTexture( m_iMaterialIndex );

		vert[0].Init( Vector2D( 0, 0 ), Vector2D( 0.0f, 0.0f ) );
		vert[1].Init( Vector2D( flDamageX, 0 ), Vector2D( m_flHealth, 0.0f ) );
		vert[2].Init( Vector2D( flDamageX, h ), Vector2D( m_flHealth, 1.0f ) );
		vert[3].Init( Vector2D( 0, h ), Vector2D( 0.0f, 1.0f ) );

		surface()->DrawSetColor( GetFgColor() );
	}
	else
	{
		float flDamageY = h * ( 1.0f - m_flHealth );

		// blend in the red "damage" part
		surface()->DrawSetTexture( m_iMaterialIndex );

		vert[0].Init( Vector2D( 0, flDamageY ), Vector2D( 0.0f, 1.0f - m_flHealth ) );
		vert[1].Init( Vector2D( w, flDamageY ), Vector2D( 1.0f, 1.0f - m_flHealth ) );
		vert[2].Init( Vector2D( w, h ), Vector2D( 1.0f, 1.0f ) );
		vert[3].Init( Vector2D( 0, h ), Vector2D( 0.0f, 1.0f ) );

		surface()->DrawSetColor( GetFgColor() );
	}

	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudPlayerHealth::CTDCHudPlayerHealth( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pHealthImage = new CTDCHealthPanel( this, "PlayerStatusHealthImage" );	
	m_pHealthImageBG = new ImagePanel( this, "PlayerStatusHealthImageBG" );
	m_pHealthBonusImage = new ImagePanel( this, "PlayerStatusHealthBonusImage" );

	m_flNextThink = 0.0f;
	m_iClass = TDC_CLASS_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerHealth::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_nHealth = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerHealth::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	KeyValues *pConditions = new KeyValues( "conditions" );
	AddSubKeyNamed( pConditions, g_aPlayerClassNames_NonLocalized[m_iClass] );
	LoadControlSettings( GetResFilename(), NULL, NULL, pConditions );
	pConditions->deleteThis();

	m_pHealthBonusImage->GetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
	m_flNextThink = 0.0f;

	// Gah, why did you have to put in so many status icons?!
	static const char *aStatusIcons[] =
	{
		"PlayerStatusBleedImage",
		"PlayerStatusHookBleedImage",
		"PlayerStatusMilkImage",
		"PlayerStatusMarkedForDeathImage",
		"PlayerStatusMarkedForDeathSilentImage",
		"PlayerStatus_MedicUberBulletResistImage",
		"PlayerStatus_MedicUberBlastResistImage",
		"PlayerStatus_MedicUberFireResistImage",
		"PlayerStatus_MedicSmallBulletResistImage",
		"PlayerStatus_MedicSmallBlastResistImage",
		"PlayerStatus_MedicSmallFireResistImage",
		"PlayerStatus_WheelOfDoom",
		"PlayerStatus_SoldierOffenseBuff",
		"PlayerStatus_SoldierDefenseBuff",
		"PlayerStatus_SoldierHealOnHitBuff",
		"PlayerStatus_SpyMarked",
		"PlayerStatus_Parachute",
		"PlayerStatus_RuneStrength",
		"PlayerStatus_RuneHaste",
		"PlayerStatus_RuneRegen",
		"PlayerStatus_RuneResist",
		"PlayerStatus_RuneVampire",
		"PlayerStatus_RuneReflect",
		"PlayerStatus_RunePrecision",
		"PlayerStatus_RuneAgility",
		"PlayerStatus_RuneKnockout",
		"PlayerStatus_RuneKing",
		"PlayerStatus_RunePlague",
		"PlayerStatus_RuneSupernova",
	};

	for ( int i = 0; i < ARRAYSIZE( aStatusIcons ); i++ )
	{
		Panel *pChild = FindChildByName( aStatusIcons[i] );
		if ( pChild )
		{
			pChild->SetVisible( false );
		}
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerHealth::SetHealth( int iNewHealth, int iMaxHealth, int	iMaxBuffedHealth )
{
	int nPrevHealth = m_nHealth;

	// set our health
	m_nHealth = iNewHealth;
	m_nMaxHealth = iMaxHealth;
	m_pHealthImage->SetHealth( (float)(m_nHealth) / (float)(m_nMaxHealth) );
	m_pHealthImage->SetFgColor( Color( 255, 255, 255, 255 ) );

	if ( m_nHealth <= 0 )
	{
		m_pHealthImageBG->SetVisible( false );
		HideHealthBonusImage();
	}
	else
	{
		m_pHealthImageBG->SetVisible( true );

		// are we getting a health bonus?
		if ( m_nHealth > m_nMaxHealth )
		{
			if ( !m_pHealthBonusImage->IsVisible() )
			{
				m_pHealthBonusImage->SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulse" );
			}

			m_pHealthBonusImage->SetDrawColor( Color( 255, 255, 255, 255 ) );

			// scale the flashing image based on how much health bonus we currently have
			float flBoostMaxAmount = (iMaxBuffedHealth)-m_nMaxHealth;
			float flPercent = Min( 1.0f, ( m_nHealth - m_nMaxHealth ) / flBoostMaxAmount ); // clamped to 1 to not cut off for values above 150%

			int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
			int nSizeAdj = 2 * nPosAdj;

			m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj,
				m_nBonusHealthOrigY - nPosAdj,
				m_nBonusHealthOrigW + nSizeAdj,
				m_nBonusHealthOrigH + nSizeAdj );
		}
		// are we close to dying?
		else if ( m_nHealth < m_nMaxHealth * m_flHealthDeathWarning )
		{
			if ( !m_pHealthBonusImage->IsVisible() )
			{
				m_pHealthBonusImage->SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulse" );
			}

			m_pHealthBonusImage->SetDrawColor( m_clrHealthDeathWarningColor );

			// scale the flashing image based on how much health bonus we currently have
			float flBoostMaxAmount = m_nMaxHealth * m_flHealthDeathWarning;
			float flPercent = ( flBoostMaxAmount - m_nHealth ) / flBoostMaxAmount;

			int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
			int nSizeAdj = 2 * nPosAdj;

			m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX - nPosAdj,
				m_nBonusHealthOrigY - nPosAdj,
				m_nBonusHealthOrigW + nSizeAdj,
				m_nBonusHealthOrigH + nSizeAdj );

			m_pHealthImage->SetFgColor( m_clrHealthDeathWarningColor );
		}
		// turn it off
		else
		{
			HideHealthBonusImage();
		}
	}

	// set our health display value
	if ( nPrevHealth != m_nHealth )
	{
		if ( m_nHealth > 0 )
		{
			SetDialogVariable( "Health", m_nHealth );
		}
		else
		{
			SetDialogVariable( "Health", "" );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerHealth::HideHealthBonusImage( void )
{
	if ( m_pHealthBonusImage->IsVisible() )
	{
		m_pHealthBonusImage->SetBounds( m_nBonusHealthOrigX, m_nBonusHealthOrigY, m_nBonusHealthOrigW, m_nBonusHealthOrigH );
		m_pHealthBonusImage->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulseStop" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulseStop" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerHealth::SetClass( int iClass )
{
	m_iClass = iClass;
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerHealth::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pPlayer )
		{
			int iOldHealth = m_nHealth;
			SetHealth( pPlayer->GetHealth(), pPlayer->GetMaxHealth(), pPlayer->m_Shared.GetMaxBuffedHealth() );

			if ( tdc_low_health_sound.GetBool() && pPlayer->IsAlive() )
			{
				float flWarningHealth = m_nMaxHealth * tdc_low_health_sound_threshold.GetFloat();

				if ( (float)iOldHealth >= flWarningHealth && (float)m_nHealth < flWarningHealth )
				{
					CLocalPlayerFilter filter;
					C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Player.LowHealth" );
				}
			}

			if ( !pPlayer->IsPlayerClass( m_iClass ) )
			{
				SetClass( pPlayer->GetPlayerClass()->GetClassIndex() );
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}


DECLARE_HUDELEMENT( CTDCHudPlayerStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudPlayerStatus::CTDCHudPlayerStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPlayerStatus" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudPlayerHealth = new CTDCHudPlayerHealth( this, "HudPlayerHealth" );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudPlayerStatus::Reset()
{
	if ( m_pHudPlayerHealth )
	{
		m_pHudPlayerHealth->Reset();
	}
}
