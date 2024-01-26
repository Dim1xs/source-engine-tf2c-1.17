//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tdc_hud_freezepanel.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "c_tdc_player.h"
#include "c_tdc_playerresource.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "fmtstr.h"
#include "tdc_gamerules.h"
#include "tdc_hud_statpanel.h"
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "functionproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tdc_freezecammodel( "tdc_freezecammodel", "0", FCVAR_ARCHIVE );

vgui::IImage* GetDefaultAvatarImage( int iPlayerIndex );
extern ConVar hud_freezecamhide;

DECLARE_HUDELEMENT_DEPTH( CTDCFreezePanel, 1 );

#define CALLOUT_WIDE		(XRES(100))
#define CALLOUT_TALL		(XRES(50))

extern float g_flFreezeFlash;

#define FREEZECAM_SCREENSHOT_STRING "is looking good!"

bool IsTakingAFreezecamScreenshot( void )
{
	// Don't draw in freezecam, or when the game's not running
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	bool bInFreezeCam = ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

	if ( bInFreezeCam == true && engine->IsTakingScreenshot() )
		return true;

	CTDCFreezePanel *pPanel = GET_HUDELEMENT( CTDCFreezePanel );
	if ( pPanel )
	{
		if ( pPanel->IsHoldingAfterScreenShot() )
			return true;
	}

	return false;
}

DECLARE_BUILD_FACTORY( CTDCFreezePanelHealth );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCFreezePanel::CTDCFreezePanel( const char *pElementName )
	: EditablePanel( NULL, "FreezePanel" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	SetScheme( "ClientScheme" );

	m_pBasePanel = new EditablePanel( this, "FreezePanelBase" );
	m_pFreezePanelBG = new CTDCImagePanel( m_pBasePanel, "FreezePanelBG" );
	m_pFreezeLabel = new Label( m_pBasePanel, "FreezeLabel", "" );
	m_pFreezeLabelKiller = new Label( m_pBasePanel, "FreezeLabelKiller", "" );
	m_pAvatar = new CAvatarImagePanel( m_pBasePanel, "AvatarImage" );
	m_pNemesisSubPanel = new EditablePanel( m_pBasePanel, "NemesisSubPanel" );
	m_pKillerHealth = new CTDCFreezePanelHealth( m_pBasePanel, "FreezePanelHealth" );

	m_pScreenshotPanel = new EditablePanel( this, "ScreenshotPanel" );
	m_pModelBG = new CModelPanel( this, "FreezePanelModelBG" );

	m_iKillerIndex = 0;
	m_iShowNemesisPanel = SHOW_NO_NEMESIS;
	m_iYBase = -1;
	m_flShowCalloutsAt = 0;

	m_iBasePanelOriginalX = -1;
	m_iBasePanelOriginalY = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::Reset()
{
	Hide();

	if ( m_pKillerHealth )
	{
		m_pKillerHealth->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::Init()
{
	// listen for events
	ListenForGameEvent( "show_freezepanel" );
	ListenForGameEvent( "hide_freezepanel" );
	ListenForGameEvent( "freezecam_started" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "teamplay_win_panel" );

	Hide();

	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTDCFreezePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/FreezePanel_Basic.res" );

	// Move killer panels when the win panel is up
	int xp, yp;
	GetPos( xp, yp );
	m_iYBase = yp;

	int w, h;
	m_pBasePanel->GetBounds( m_iBasePanelOriginalX, m_iBasePanelOriginalY, w, h );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "player_death", pEventName ) == 0 )
	{
		// see if the local player died
		int iPlayerIndexVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();
		if ( pLocalPlayer && iPlayerIndexVictim == pLocalPlayer->entindex() )
		{
			// the local player is dead, see if this is a new nemesis or a revenge
			int nDeathFlags = event->GetInt( "death_flags" );

			if ( nDeathFlags & TDC_DEATH_DOMINATION )
			{
				m_iShowNemesisPanel = SHOW_NEW_NEMESIS;
			}
			else if ( nDeathFlags & TDC_DEATH_REVENGE )
			{
				m_iShowNemesisPanel = SHOW_REVENGE;
			}
			else
			{
				m_iShowNemesisPanel = SHOW_NO_NEMESIS;
			}
		}
	}
	else if ( Q_strcmp( "hide_freezepanel", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "freezecam_started", pEventName ) == 0 )
	{
		ShowCalloutsIn( 1.0 );
		ShowSnapshotPanelIn( 1.25 );

		C_TDCPlayer *pLocalPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pLocalPlayer )
		{
			// Collect everyone visible on the screenshot.
			pLocalPlayer->CollectVisibleSteamUsers( m_UsersInFreezecam );
		}
	}
	else if ( Q_strcmp( "teamplay_win_panel", pEventName ) == 0 )
	{
		Hide();
	}
	else if ( Q_strcmp( "show_freezepanel", pEventName ) == 0 )
	{
		if ( !g_PR )
		{
			m_pNemesisSubPanel->SetDialogVariable( "nemesisname", NULL_STRING ); // SanyaSho: 64bit compile error
			return;
		}

		Show();

		m_pBasePanel->SetVisible( true );
		m_pModelBG->SetVisible( false );
		ShowSnapshotPanel( false );
		m_bHoldingAfterScreenshot = false;

		if ( m_iBasePanelOriginalX > -1 && m_iBasePanelOriginalY > -1 )
		{
			m_pBasePanel->SetPos( m_iBasePanelOriginalX, m_iBasePanelOriginalY );
		}

		// Get the entity who killed us
		m_iKillerIndex = event->GetInt( "killer" );
		C_BaseEntity *pKiller = ClientEntityList().GetBaseEntity( m_iKillerIndex );

		int xp, yp;
		GetPos( xp, yp );
		if ( TDCGameRules()->RoundHasBeenWon() )
		{
			SetPos( xp, m_iYBase - YRES( 50 ) );
		}
		else
		{
			SetPos( xp, m_iYBase );
		}

		if ( pKiller )
		{
			CTDCPlayer *pPlayer = ToTDCPlayer( pKiller );
			int iMaxBuffedHealth = 0;

			if ( pPlayer )
			{
				iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();
			}

			int iKillerHealth = pKiller->GetHealth();
			if ( !pKiller->IsAlive() )
			{
				iKillerHealth = 0;
			}
			m_pKillerHealth->SetHealth( iKillerHealth, pKiller->GetMaxHealth(), iMaxBuffedHealth );

			int iKillerTeam = TEAM_UNASSIGNED;

			if ( pKiller->IsPlayer() )
			{
				iKillerTeam = g_PR->GetTeam( m_iKillerIndex );

				//If this was just a regular kill but this guy is our nemesis then just show it.
				if ( g_TDC_PR->IsPlayerDominating( m_iKillerIndex ) )
				{
					if ( !pKiller->IsAlive() )
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Nemesis_Dead" );
					}
					else
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Nemesis" );
					}
				}
				else
				{
					if ( !pKiller->IsAlive() )
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Killer_Dead" );
					}
					else
					{
						m_pFreezeLabel->SetText( "#FreezePanel_Killer" );
					}
				}

				m_pBasePanel->SetDialogVariable( "killername", g_PR->GetPlayerName( m_iKillerIndex ) );

				m_pAvatar->SetDefaultAvatar( GetDefaultAvatarImage( m_iKillerIndex ) );
				m_pAvatar->SetPlayer( (C_BasePlayer*)pKiller );
			}
			else
			{
				if ( !pKiller->IsAlive() )
				{
					m_pFreezeLabel->SetText( "#FreezePanel_Killer_Dead" );
				}
				else
				{
					m_pFreezeLabel->SetText( "#FreezePanel_Killer" );
				}
			}

			if ( TDCGameRules()->IsTeamplay() )
			{
				// Set the BG according to the team they're on
				m_pFreezePanelBG->SetImage( VarArgs( "../hud/color_panel_%s", GetTeamSuffix( iKillerTeam, false, g_aTeamNamesShort, "brown" ) ) );
			}
			else
			{
				// Use a paintable image in FFA.
				m_pFreezePanelBG->SetImage( "../hud/color_panel_custom_freezepanel" );
			}
			m_pAvatar->SetShouldDrawFriendIcon( false );

			// Position the killer name to the right of the avatar.
			// Live TF2 has it where the avatar is for bots since they don't have avatars but they do in TF2C.
			m_pFreezeLabelKiller->SetPos( m_pAvatar->GetXPos() + m_pAvatar->GetWide() + YRES( 2 ), m_pFreezeLabelKiller->GetYPos() );
		}

		// see if we should show nemesis panel
		const wchar_t *pchNemesisText = NULL;

		if ( IsPlayerIndex( m_iKillerIndex ) )
		{
			switch ( m_iShowNemesisPanel )
			{
			case SHOW_NO_NEMESIS:
				//If this was just a regular kill but this guy is our nemesis then just show it.
				if ( g_TDC_PR->IsPlayerDominating( m_iKillerIndex ) )
				{
					pchNemesisText = g_pVGuiLocalize->Find( "#TDC_FreezeNemesis" );
				}

				break;
			case SHOW_NEW_NEMESIS:
				// check to see if killer is still the nemesis of victim; victim may have managed to kill him after victim's
				// death by grenade or some such, extracting revenge and clearing nemesis condition
				if ( g_TDC_PR->IsPlayerDominating( m_iKillerIndex ) )
				{
					pchNemesisText = g_pVGuiLocalize->Find( "#TDC_NewNemesis" );
				}

				break;
			case SHOW_REVENGE:
				pchNemesisText = g_pVGuiLocalize->Find( "#TDC_GotRevenge" );
				break;
			default:
				Assert( false );	// invalid value
				break;
			}
		}

		m_pNemesisSubPanel->SetDialogVariable( "nemesisname", pchNemesisText );

		ShowNemesisPanel( NULL != pchNemesisText );
		m_iShowNemesisPanel = SHOW_NO_NEMESIS;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::ShowCalloutsIn( float flTime )
{
	m_flShowCalloutsAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCFreezePanelCallout *CTDCFreezePanel::TestAndAddCallout( Vector &origin, Vector &vMins, Vector &vMaxs, CUtlVector<Vector> *vecCalloutsTL,
	CUtlVector<Vector> *vecCalloutsBR, Vector &vecFreezeTL, Vector &vecFreezeBR, Vector &vecStatTL, Vector &vecStatBR, int *iX, int *iY )
{
	// This is the offset from the topleft of the callout to the arrow tip
	const int iXOffset = XRES( 25 );
	const int iYOffset = YRES( 50 );

	//if ( engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) && !engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		if ( GetVectorInScreenSpace( origin, *iX, *iY ) )
		{
			*iX -= iXOffset;
			*iY -= iYOffset;
			int iRight = *iX + CALLOUT_WIDE;
			int iBottom = *iY + CALLOUT_TALL;
			if ( *iX > 0 && *iY > 0 && ( iRight < ScreenWidth() ) && ( iBottom < ( ScreenHeight() - YRES( 40 ) ) ) )
			{
				// Make sure it wouldn't be over the top of the freezepanel or statpanel
				Vector vecCalloutTL( *iX, *iY, 0 );
				Vector vecCalloutBR( iRight, iBottom, 1 );
				if ( !QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecFreezeTL, vecFreezeBR ) &&
					!QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecStatTL, vecStatBR ) )
				{
					// Make sure it doesn't intersect any other callouts
					bool bClear = true;
					for ( int iCall = 0; iCall < vecCalloutsTL->Count(); iCall++ )
					{
						if ( QuickBoxIntersectTest( vecCalloutTL, vecCalloutBR, vecCalloutsTL->Element( iCall ), vecCalloutsBR->Element( iCall ) ) )
						{
							bClear = false;
							break;
						}
					}

					if ( bClear )
					{
						// Verify that we have LOS to the gib
						trace_t	tr;
						UTIL_TraceLine( origin, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
						bClear = ( tr.fraction >= 1.0f );
					}

					if ( bClear )
					{
						CTDCFreezePanelCallout *pCallout = new CTDCFreezePanelCallout( g_pClientMode->GetViewport(), "FreezePanelCallout" );
						m_pCalloutPanels.AddToTail( vgui::SETUP_PANEL( pCallout ) );
						vecCalloutsTL->AddToTail( vecCalloutTL );
						vecCalloutsBR->AddToTail( vecCalloutBR );
						pCallout->SetVisible( true );
						pCallout->SetBounds( *iX, *iY, CALLOUT_WIDE, CALLOUT_TALL );
						return pCallout;
					}
				}
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::UpdateCallout( void )
{
	CTDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	// Abort early if we have no gibs or ragdoll
	CUtlVector<EHANDLE>	*pGibList = pPlayer->GetSpawnedGibs();
	IRagdoll *pRagdoll = pPlayer->GetRepresentativeRagdoll();
	if ( ( !pGibList || pGibList->Count() == 0 ) && !pRagdoll )
		return;

	if ( m_pFreezePanelBG == NULL )
		return;

	// Precalc the vectors of the freezepanel & statpanel
	int iX, iY;
	m_pFreezePanelBG->GetPos( iX, iY );
	Vector vecFreezeTL( iX, iY, 0 );
	Vector vecFreezeBR( iX + m_pFreezePanelBG->GetWide(), iY + m_pFreezePanelBG->GetTall(), 1 );

	CUtlVector<Vector> vecCalloutsTL;
	CUtlVector<Vector> vecCalloutsBR;

	Vector vecStatTL( 0, 0, 0 );
	Vector vecStatBR( 0, 0, 1 );
	CTDCStatPanel *pStatPanel = GET_HUDELEMENT( CTDCStatPanel );
	if ( pStatPanel && pStatPanel->IsVisible() )
	{
		pStatPanel->GetPos( iX, iY );
		vecStatTL.x = iX;
		vecStatTL.y = iY;
		vecStatBR.x = vecStatTL.x + pStatPanel->GetWide();
		vecStatBR.y = vecStatTL.y + pStatPanel->GetTall();
	}

	Vector vMins, vMaxs;

	// Check gibs
	if ( pGibList && pGibList->Count() )
	{
		int iCount = 0;
		for ( int i = 0; i < pGibList->Count(); i++ )
		{
			CBaseEntity *pGib = pGibList->Element( i );
			if ( pGib )
			{
				Vector origin = pGib->GetRenderOrigin();
				IPhysicsObject *pPhysicsObject = pGib->VPhysicsGetObject();
				if ( pPhysicsObject )
				{
					Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
					pGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &origin );
				}
				pGib->GetRenderBounds( vMins, vMaxs );

				// Try and add the callout
				CTDCFreezePanelCallout *pCallout = TestAndAddCallout( origin, vMins, vMaxs, &vecCalloutsTL, &vecCalloutsBR,
					vecFreezeTL, vecFreezeBR, vecStatTL, vecStatBR, &iX, &iY );
				if ( pCallout )
				{
					pCallout->UpdateForGib( i, iCount );
					iCount++;
				}
			}
		}
	}
	else if ( pRagdoll )
	{
		Vector origin = pRagdoll->GetRagdollOrigin();
		pRagdoll->GetRagdollBounds( vMins, vMaxs );

		// Try and add the callout
		CTDCFreezePanelCallout *pCallout = TestAndAddCallout( origin, vMins, vMaxs, &vecCalloutsTL, &vecCalloutsBR,
			vecFreezeTL, vecFreezeBR, vecStatTL, vecStatBR, &iX, &iY );
		if ( pCallout )
		{
			pCallout->UpdateForRagdoll();
		}

		// even if the callout failed, check that our ragdoll is onscreen and our killer is taunting us (for an achievement)
		if ( GetVectorInScreenSpace( origin, iX, iY ) )
		{
			C_TDCPlayer *pKiller = ToTDCPlayer( UTIL_PlayerByIndex( GetSpectatorTarget() ) );
			if ( pKiller && pKiller->m_Shared.InCond( TDC_COND_TAUNTING ) )
			{
				// tell the server our ragdoll just got taunted during our freezecam
				KeyValues *pKeys = new KeyValues( "FreezeCamTaunt" );
				pKeys->SetInt( "achiever", GetSpectatorTarget() );

				engine->ServerCmdKeyValues( pKeys );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::Show()
{
	m_flShowCalloutsAt = 0;
	SetVisible( true );
	m_UsersInFreezecam.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::Hide()
{
	SetVisible( false );
	m_bHoldingAfterScreenshot = false;

	// Delete all our callout panels
	for ( int i = m_pCalloutPanels.Count() - 1; i >= 0; i-- )
	{
		m_pCalloutPanels[i]->MarkForDeletion();
	}
	m_pCalloutPanels.RemoveAll();
	m_UsersInFreezecam.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCFreezePanel::ShouldDraw( void )
{
	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( m_flShowCalloutsAt && m_flShowCalloutsAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			UpdateCallout();
		}
		m_flShowCalloutsAt = 0;
	}

	if ( m_flShowSnapshotReminderAt && m_flShowSnapshotReminderAt < gpGlobals->curtime )
	{
		if ( ShouldDraw() )
		{
			ShowSnapshotPanel( true );
		}
		m_flShowSnapshotReminderAt = 0;
	}

	if ( m_bHoldingAfterScreenshot )
	{
		if ( tdc_freezecammodel.GetBool() && !m_pModelBG->IsVisible() && !engine->IsTakingScreenshot() )
		{
			m_pModelBG->SetVisible( true );

			KeyValues *kvParms = new KeyValues( "SetAnimation" );
			kvParms->SetString( "animation", VarArgs( "drop%02d", RandomInt( 1, 4 ) ) );
			PostMessage( m_pModelBG, kvParms );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::ShowSnapshotPanelIn( float flTime )
{
#if defined (_X360 )
	return;
#endif

	m_flShowSnapshotReminderAt = gpGlobals->curtime + flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanel::ShowSnapshotPanel( bool bShow )
{
	if ( !m_pScreenshotPanel )
		return;

	const char *key = engine->Key_LookupBinding( "screenshot" );

	if ( key == NULL || FStrEq( key, "(null)" ) )
	{
		bShow = false;
		key = " ";
	}

	if ( bShow )
	{
		char szKey[16];
		V_sprintf_safe( szKey, "%s", key );
		wchar_t wKey[16];
		wchar_t wLabel[256];

		g_pVGuiLocalize->ConvertANSIToUnicode( szKey, wKey, sizeof( wKey ) );
		g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find( "#TDC_freezecam_snapshot" ), 1, wKey );

		m_pScreenshotPanel->SetDialogVariable( "text", wLabel );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudSnapShotReminderIn" );
	}

	m_pScreenshotPanel->SetVisible( bShow );
}

int	CTDCFreezePanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( ShouldDraw() && pszCurrentBinding )
	{
		if ( FStrEq( pszCurrentBinding, "screenshot" ) || FStrEq( pszCurrentBinding, "jpeg" ) )
		{
			// If we already took a screenshot, do nothing.
			if ( m_bHoldingAfterScreenshot )
				return 0;

			// move the target id to the corner
			if ( m_pBasePanel )
			{
				int w, h;
				m_pBasePanel->GetSize( w, h );
				m_pBasePanel->SetPos( ScreenWidth() - w, ScreenHeight() - h );

				if ( hud_freezecamhide.GetBool() )
				{
					m_pBasePanel->SetVisible( false );
				}
			}

			// Get the local player.
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pPlayer )
			{
				//Do effects
				g_flFreezeFlash = gpGlobals->curtime + 0.75f;
				pPlayer->EmitSound( "Camera.SnapShot" );

				//Extend Freezecam by a couple more seconds.
				engine->ClientCmd( "extendfreeze" );
				view->FreezeFrame( 3.0f );

				//Hide the reminder panel
				m_flShowSnapshotReminderAt = 0;
				ShowSnapshotPanel( false );

				m_bHoldingAfterScreenshot = true;

				//Set the screenshot name
				if ( m_iKillerIndex <= MAX_PLAYERS )
				{
					const char *pszKillerName = g_PR->GetPlayerName( m_iKillerIndex );

					if ( pszKillerName )
					{
						ConVarRef cl_screenshotname( "cl_screenshotname" );

						if ( cl_screenshotname.IsValid() )
						{
							char szScreenShotName[512];

							V_sprintf_safe( szScreenShotName, "%s %s", pszKillerName, FREEZECAM_SCREENSHOT_STRING );

							cl_screenshotname.SetValue( szScreenShotName );
						}
					}
				}
			}
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shows or hides the nemesis part of the panel
//-----------------------------------------------------------------------------
void CTDCFreezePanel::ShowNemesisPanel( bool bShow )
{
	m_pNemesisSubPanel->SetVisible( bShow );

#ifndef _X360
	if ( bShow )
	{
		vgui::Label *pLabel = dynamic_cast<vgui::Label *>( m_pNemesisSubPanel->FindChildByName( "NemesisLabel" ) );
		vgui::ImagePanel *pBG = dynamic_cast<vgui::ImagePanel *>( m_pNemesisSubPanel->FindChildByName( "NemesisPanelBG" ) );
		vgui::ImagePanel *pIcon = dynamic_cast<vgui::ImagePanel *>( m_pNemesisSubPanel->FindChildByName( "NemesisIcon" ) );

		// check that our Nemesis panel and resize it to the length of the string (the right side is pinned and doesn't move)
		if ( pLabel && pBG && pIcon )
		{
			int wide, tall;
			pLabel->GetContentSize( wide, tall );

			int nDiff = wide - pLabel->GetWide();

			if ( nDiff != 0 )
			{
				int x, y, w, t;

				// move the icon
				pIcon->GetBounds( x, y, w, t );
				pIcon->SetBounds( x - nDiff, y, w, t );

				// move/resize the label
				pLabel->GetBounds( x, y, w, t );
				pLabel->SetBounds( x - nDiff, y, w + nDiff, t );

				// move/resize the background
				pBG->GetBounds( x, y, w, t );
				pBG->SetBounds( x - nDiff, y, w + nDiff, t );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCFreezePanelCallout::CTDCFreezePanelCallout( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pGibLabel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTDCFreezePanelCallout::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/FreezePanelCallout.res" );

	m_pGibLabel = dynamic_cast<Label *>( FindChildByName( "CalloutLabel" ) );
	m_pCalloutBG = dynamic_cast<CTDCImagePanel *>( FindChildByName( "CalloutBG" ) );
}

const char *pszCalloutGibNames[] =
{
	"#Callout_Head",
	"#Callout_Foot",
	"#Callout_Hand",
	"#Callout_Torso",
	NULL,	// Random
};
const char *pszCalloutRandomGibNames[] =
{
	"#Callout_Organ2",
	"#Callout_Organ3",
	"#Callout_Organ4",
	"#Callout_Organ5",
	"#Callout_Organ6",
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanelCallout::UpdateForGib( int iGib, int iCount )
{
	if ( !m_pGibLabel )
		return;

	if ( iGib < ARRAYSIZE( pszCalloutGibNames ) )
	{
		if ( pszCalloutGibNames[iGib] )
		{
			m_pGibLabel->SetText( pszCalloutGibNames[iGib] );
		}
		else
		{
			m_pGibLabel->SetText( pszCalloutRandomGibNames[RandomInt( 0, ARRAYSIZE( pszCalloutRandomGibNames ) - 1 )] );
		}
	}
	else
	{
		if ( iCount > 1 )
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout3" );
		}
		else if ( iCount == 1 )
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout2" );
		}
		else
		{
			m_pGibLabel->SetText( "#FreezePanel_Callout" );
		}
	}

#ifndef _X360
	int wide, tall;
	m_pGibLabel->GetContentSize( wide, tall );

	// is the text wider than the label?
	if ( wide > m_pGibLabel->GetWide() )
	{
		int nDiff = wide - m_pGibLabel->GetWide();
		int x, y, w, t;

		// make the label wider
		m_pGibLabel->GetBounds( x, y, w, t );
		m_pGibLabel->SetBounds( x, y, w + nDiff, t );

		CTDCImagePanel *pBackground = dynamic_cast<CTDCImagePanel *>( FindChildByName( "CalloutBG" ) );
		if ( pBackground )
		{
			// also adjust the background image
			pBackground->GetBounds( x, y, w, t );
			pBackground->SetBounds( x, y, w + nDiff, t );
		}

		// make ourselves bigger to accommodate the wider children
		GetBounds( x, y, w, t );
		SetBounds( x, y, w + nDiff, t );

		// check that we haven't run off the right side of the screen
		if ( x + GetWide() > ScreenWidth() )
		{
			// push ourselves to the left to fit on the screen
			nDiff = ( x + GetWide() ) - ScreenWidth();
			SetPos( x - nDiff, y );

			// push the arrow to the right to offset moving ourselves to the left
			vgui::ImagePanel *pArrow = dynamic_cast<ImagePanel *>( FindChildByName( "ArrowIcon" ) );
			if ( pArrow )
			{
				pArrow->GetBounds( x, y, w, t );
				pArrow->SetBounds( x + nDiff, y, w, t );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanelCallout::UpdateForRagdoll( void )
{
	if ( !m_pGibLabel )
		return;

	m_pGibLabel->SetText( "#Callout_Ragdoll" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFreezePanelCallout::SetTeam( int iTeam )
{
	if ( m_pCalloutBG )
	{
		m_pCalloutBG->SetBGImage( iTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets custom color of the local player's observer target.
//-----------------------------------------------------------------------------
class CObserverTargetTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pPlayer )
		{
			C_TDCPlayer *pTarget = ToTDCPlayer( pPlayer->GetObserverTarget() );

			if ( pTarget )
			{
				m_pResult->SetVecValue( pTarget->m_vecPlayerColor.Base(), 3 );
				return;
			}
		}

		m_pResult->SetVecValue( 0, 0, 0 );
	}
};

EXPOSE_INTERFACE( CObserverTargetTintColor, IMaterialProxy, "ObserverTargetTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );
