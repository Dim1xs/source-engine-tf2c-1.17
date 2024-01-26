//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
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
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include "view.h"

#include "c_playerresource.h"
#include "tdc_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tdc_player.h"
#include "c_team.h"
#include "c_tdc_team.h"
#include "c_tdc_objective_resource.h"
#include "tdc_hud_objectivestatus.h"
#include "tdc_gamerules.h"
#include "tdc_hud_freezepanel.h"
#include "c_func_capture_zone.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTDCArrowPanel );
DECLARE_BUILD_FACTORY( CTDCFlagStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCArrowPanel::CTDCArrowPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_RedMaterial.Init( "hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterial.Init( "hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI ); 
	m_NeutralMaterial.Init( "hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI ); 

	m_RedMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTDCArrowPanel::GetAngleRotation( void )
{
	float flRetVal = 0.0f;

	C_TDCPlayer *pPlayer = ToTDCPlayer( C_BasePlayer::GetLocalPlayer() );
	C_BaseEntity *pEnt = m_hEntity.Get();

	if ( pPlayer && pEnt )
	{
		QAngle vangles = MainViewAngles();
		Vector eyeOrigin = MainViewOrigin();

		Vector vecFlag = pEnt->WorldSpaceCenter() - eyeOrigin;
		vecFlag.z = 0;
		vecFlag.NormalizeInPlace();

		Vector forward, right, up;
		AngleVectors( vangles, &forward, &right, &up );
		forward.z = 0;
		right.z = 0;
		forward.NormalizeInPlace();
		right.NormalizeInPlace();

		float dot = DotProduct( vecFlag, forward );
		float angleBetween = acos( clamp( dot, -1.0f, 1.0f ) );

		dot = DotProduct( vecFlag, right );

		if ( dot < 0.0f )
		{
			angleBetween *= -1;
		}

		flRetVal = RAD2DEG( angleBetween );
	}

	return flRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCArrowPanel::Paint()
{
	if ( !m_hEntity.Get() )
		return;

	C_TDCPlayer *pPlayer = GetLocalObservedPlayer( false );
	if ( !pPlayer )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	IMaterial *pMaterial = m_NeutralMaterial;

	// Don't draw the arrow if watching the player who's carrying the flag.
	bool bCarried = ( pPlayer->GetTheFlag() == pEnt );

	// figure out what material we need to use
	switch ( pEnt->GetTeamNumber() )
	{
	case TDC_TEAM_RED:
		pMaterial = bCarried ? m_RedMaterialNoArrow : m_RedMaterial;
		break;
	case TDC_TEAM_BLUE:
		pMaterial = bCarried ? m_BlueMaterialNoArrow : m_BlueMaterial;
		break;
	}

	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos( GetVPanel(), x, y );
	int nWidth = GetWide();
	int nHeight = GetTall();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix(); 

	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixBuildRotationAboutAxis( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
//	MatrixRotate( panelRotation, Vector( 1, 0, 0 ), 5 );
	panelRotation.SetTranslation( Vector( x + nWidth/2, y + nHeight/2, 0 ) );
	pRenderContext->LoadMatrix( panelRotation );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3f( -nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3f( nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3f( nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3f( -nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCArrowPanel::IsVisible( void )
{
	if ( !m_hEntity.Get() )
		return false;

	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCFlagStatus::CTDCFlagStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pArrow = NULL;
	m_pStatusIcon = NULL;
	m_pBriefcase = NULL;
	m_hEntity = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFlagStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/FlagStatus.res" );

	m_pArrow = dynamic_cast<CTDCArrowPanel *>( FindChildByName( "Arrow" ) );
	m_pStatusIcon = dynamic_cast<CTDCImagePanel *>( FindChildByName( "StatusIcon" ) );
	m_pBriefcase = dynamic_cast<CTDCImagePanel *>( FindChildByName( "Briefcase" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCFlagStatus::IsVisible( void )
{
	if ( !m_hEntity.Get() )
		return false;

	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCFlagStatus::UpdateStatus( void )
{
	if ( m_hEntity.Get() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );
	
		if ( pFlag )
		{
			const char *pszImage = "../hud/objectives_flagpanel_ico_flag_home";

			if ( pFlag->IsDropped() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
			}
			else if ( pFlag->IsStolen() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
			}

			if ( m_pStatusIcon )
			{
				m_pStatusIcon->SetImage( pszImage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTDCHudFlagObjectives::CTDCHudFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_pPlayingTo = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;
	m_bShowPlayingTo = false;
	m_iNumFlags = 0;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	ListenForGameEvent( "game_maploaded" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCHudFlagObjectives::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );

	m_iNumFlags = 2;
	m_bShowPlayingTo = true;

	if ( TDCGameRules() )
	{
		switch ( TDCGameRules()->GetGameType() )
		{
		case TDC_GAMETYPE_CTF:
		default:
			AddSubKeyNamed( pConditions, "if_ctf" );
			m_iNumFlags = 2;
			m_bShowPlayingTo = true;
			break;
		case TDC_GAMETYPE_ATTACK_DEFEND:
			AddSubKeyNamed( pConditions, "if_ad" );
			m_iNumFlags = 1;
			m_bShowPlayingTo = false;
			break;
		case TDC_GAMETYPE_INVADE:
			AddSubKeyNamed( pConditions, "if_invade" );
			m_iNumFlags = 1;
			m_bShowPlayingTo = true;
			break;
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveFlagPanel.res", NULL, NULL, pConditions );

	pConditions->deleteThis();

	m_pCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "CarriedImage" ) );
	m_pPlayingTo = dynamic_cast<CExLabel *>( FindChildByName( "PlayingTo" ) );
	m_pPlayingToBG = dynamic_cast<CTDCImagePanel *>( FindChildByName( "PlayingToBG" ) );
	m_pRedFlag = dynamic_cast<CTDCFlagStatus *>( FindChildByName( "RedFlag" ) );
	m_pBlueFlag = dynamic_cast<CTDCFlagStatus *>( FindChildByName( "BlueFlag" ) );

	m_pCapturePoint = dynamic_cast<CTDCArrowPanel *>( FindChildByName( "CaptureFlag" ) );

	m_pSpecCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "SpecCarriedImage" ) );

	// outline is always on, so we need to init the alpha to 0
	CTDCImagePanel *pOutline = dynamic_cast<CTDCImagePanel *>( FindChildByName( "OutlineImage" ) );
	if ( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );

	if ( m_pCarriedImage )
	{
		m_pCarriedImage->SetVisible( false );
	}

	if ( m_pBlueFlag )
	{
		m_pBlueFlag->SetVisible( true );
	}

	if ( m_pRedFlag )
	{
		m_pRedFlag->SetVisible( true );
	}

	if ( m_pSpecCarriedImage )
	{
		m_pSpecCarriedImage->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudFlagObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo && m_pPlayingToBG )
	{
		m_pPlayingTo->SetVisible( bVisible );
		m_pPlayingToBG->SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudFlagObjectives::OnTick()
{
	if ( !TDCGameRules() )
		return;

	if ( m_iNumFlags == 0 )
	{
		if ( m_pRedFlag )
		{
			m_pRedFlag->SetEntity( NULL );
		}
		if ( m_pBlueFlag )
		{
			m_pBlueFlag->SetEntity( NULL );
		}
	}
	else if ( m_iNumFlags == 1 )
	{
		// If there's only one flag, just set the red panel up to track it.
		for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
		{
			if ( pFlag->IsDisabled() && !pFlag->IsVisibleWhenDisabled() )
				continue;

			if ( m_pRedFlag )
			{
				m_pRedFlag->SetEntity( pFlag );
			}
			break;
		}

		if ( m_pBlueFlag )
		{
			m_pBlueFlag->SetEntity( NULL );
		}
	}
	else
	{
		// iterate through the flags to set their position in our HUD
		for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
		{
			if ( pFlag->IsDisabled() && !pFlag->IsVisibleWhenDisabled() )
				continue;

			if ( m_pRedFlag && pFlag->GetTeamNumber() == TDC_TEAM_RED )
			{
				m_pRedFlag->SetEntity( pFlag );
			}
			else if ( m_pBlueFlag && pFlag->GetTeamNumber() == TDC_TEAM_BLUE )
			{
				m_pBlueFlag->SetEntity( pFlag );
			}
		}
	}

	if ( m_bShowPlayingTo )
	{
		// are we playing captures for rounds?
		C_TDCTeam *pTeam = GetGlobalTFTeam( TDC_TEAM_BLUE );
		if ( pTeam )
		{
			SetDialogVariable( "bluescore", pTeam->GetRoundScore() );
		}

		pTeam = GetGlobalTFTeam( TDC_TEAM_RED );
		if ( pTeam )
		{
			SetDialogVariable( "redscore", pTeam->GetRoundScore() );
		}

		SetPlayingToLabelVisible( TDCGameRules()->GetScoreLimit() > 0 );
		SetDialogVariable( "rounds", TDCGameRules()->GetScoreLimit() );
	}

	// Update the carried flag status.
	UpdateStatus();

	// check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag
	bool bSpecCarriedImage = false;
	C_TDCPlayer *pPlayer = GetLocalObservedPlayer( true );

	if ( pPlayer && !pPlayer->IsLocalPlayer() )
	{
		CCaptureFlag *pPlayerFlag = pPlayer->GetTheFlag();

		if ( pPlayerFlag )
		{
			bSpecCarriedImage = true;

			if ( m_pSpecCarriedImage )
			{
				char szImage[MAX_PATH];
				pPlayerFlag->GetHudIcon( pPlayerFlag->GetTeamNumber(), szImage, MAX_PATH );
				m_pSpecCarriedImage->SetImage( szImage );
			}
		}
	}

	if ( bSpecCarriedImage )
	{
		if ( m_pSpecCarriedImage )
		{
			m_pSpecCarriedImage->SetVisible( true );
		}
	}
	else
	{
		if ( m_pSpecCarriedImage )
		{
			m_pSpecCarriedImage->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudFlagObjectives::UpdateStatus( void )
{
	C_TDCPlayer *pLocalPlayer = ToTDCPlayer( C_BasePlayer::GetLocalPlayer() );

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer )
	{
		pPlayerFlag = pLocalPlayer->GetTheFlag();
	}

	if ( pPlayerFlag )
	{
		m_bCarryingFlag = true;

		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if ( m_pCarriedImage && !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;

			// Set the correct flag image depending on the flag we're holding
			char szImage[MAX_PATH];
			pPlayerFlag->GetHudIcon( pPlayerFlag->GetTeamNumber(), szImage, MAX_PATH );
			m_pCarriedImage->SetImage( szImage );

			if ( m_pRedFlag )
			{
				m_pRedFlag->SetVisible( false );
			}

			if ( m_pBlueFlag )
			{
				m_pBlueFlag->SetVisible( false );
			}

			m_pCarriedImage->SetVisible( true );

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );

			if ( m_pCapturePoint )
			{
				m_pCapturePoint->SetVisible( true );

				if ( pLocalPlayer )
				{
					// go through all the capture zones and find ours
					for ( C_CaptureZone *pZone : C_CaptureZone::AutoList() )
					{
						if ( pZone->IsDisabled() )
							continue;

						if ( pZone->GetTeamNumber() == pLocalPlayer->GetTeamNumber() )
						{
							m_pCapturePoint->SetEntity( pZone );
						}
					}
				}
			}
		}
	}
	else
	{
		// were we carrying the flag?
		if ( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;
			m_bFlagAnimationPlayed = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		if ( m_pCarriedImage )
		{
			m_pCarriedImage->SetVisible( false );
		}

		if ( m_pCapturePoint )
		{
			m_pCapturePoint->SetVisible( false );
		}

		if ( m_pBlueFlag )
		{
			m_pBlueFlag->SetVisible( true );
			m_pBlueFlag->UpdateStatus();
		}

		if ( m_pRedFlag )
		{
			m_pRedFlag->SetVisible( true );
			m_pRedFlag->UpdateStatus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCHudFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( V_strcmp( eventName, "game_maploaded" ) == 0 )
	{
		InvalidateLayout( false, true );
	}
}
