//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tdc_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tdc_imagepanel.h"
#include "tdc_gamerules.h"
#include "c_tdc_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudGameType : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudGameType, EditablePanel );

public:
	CHudGameType( const char *pElementName );

	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	void			SetupPanel( ETDCGameType iType );

private:
	Label			*m_pNameLabel;
	Label			*m_pGoalLabel;
	CTDCImagePanel	*m_pGoalImage;

	float			m_flHideAt;
	int				m_oldPlayerTeam;
	int				m_oldPlayerState;
};

DECLARE_HUDELEMENT( CHudGameType );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudGameType::CHudGameType( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudGameType" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pNameLabel = new Label( this, "NameLabel", "" );
	m_pGoalLabel = new Label( this, "GoalLabel", "" );
	m_pGoalImage = new CTDCImagePanel( this, "GoalImage" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flHideAt = 0;

	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGameType::LevelInit( void )
{
	m_flHideAt = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGameType::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "Resource/UI/HudGameType.res" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudGameType::ShouldDraw( void )
{
	bool bCouldSee = ( TDCGameRules() && TDCGameRules()->ShouldShowTeamGoal() );

	if ( TDCGameRules() && bCouldSee && m_oldPlayerState != TDCGameRules()->State_Get() )
	{
		// Show for 8 seconds
		m_flHideAt = gpGlobals->curtime + 8.0;
		m_oldPlayerState = TDCGameRules()->State_Get();
	}

	//we've just switched teams Show it again regardless of where we are in the round
	if ( m_oldPlayerTeam != GetLocalPlayerTeam() )
	{
		// Show for 8 seconds
		m_flHideAt = gpGlobals->curtime + 8.0;
		m_oldPlayerTeam = GetLocalPlayerTeam();
	}

	if ( m_flHideAt && m_flHideAt < gpGlobals->curtime )
	{
		if ( !bCouldSee )
		{
			m_flHideAt = 0;
		}
		return false;
	}

	if ( bCouldSee )
	{
		C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			if ( CHudElement::ShouldDraw() )
			{
				if ( !IsVisible() )
				{
					SetupPanel( (ETDCGameType)TDCGameRules()->GetGameType() );

					// Show for 8 seconds
					m_flHideAt = gpGlobals->curtime + 8.0;
					m_oldPlayerState = TDCGameRules()->State_Get();
					m_oldPlayerTeam = GetLocalPlayerTeam();
				}

				// Don't appear if the team switch alert is there
				CHudElement *pHudSwitch = gHUD.FindElement( "CHudTeamSwitch" );
				if ( pHudSwitch && pHudSwitch->ShouldDraw() )
					return false;

				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGameType::SetupPanel( ETDCGameType iType )
{
	GameTypeInfo_t *pInfo = &g_aGameTypeInfo[iType];

	// Set the gamemode name.
	m_pNameLabel->SetText( pInfo->localized_name );

	// Set the image.
	const char *pszIcon = VarArgs( "hud/scoreboard_gametype_%s", g_aGameTypeInfo[TDCGameRules()->GetGameType()].name );
	IMaterial *pMaterial = materials->FindMaterial( pszIcon, TEXTURE_GROUP_VGUI, false );
	if ( pMaterial->IsErrorMaterial() )
	{
		pszIcon = "hud/scoreboard_gametype_unknown";
	}

	char szIconProper[MAX_PATH];
	V_sprintf_safe( szIconProper, "../%s", pszIcon );
	m_pGoalImage->SetImage( szIconProper );

	// Set the description.
	if ( pInfo->shared_goal )
	{
		// This is a symmetrical map.
		m_pGoalLabel->SetText( VarArgs( "#GameSubTypeGoal_%s", pInfo->name ) );
	}
	else
	{
		// Attack/Defense.
		const char *pszRole = ( GetLocalPlayerTeam() == TDC_TEAM_DEFENDERS ) ? "Defense" : "Offense";
		m_pGoalLabel->SetText( VarArgs( "#GameSubTypeGoal_%s_%s", pInfo->name, pszRole ) );
	}
}
