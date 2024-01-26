//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "IGameUIFuncs.h" // for key bindings

#ifdef _WIN32
#include "winerror.h"
#endif
#include "ixboxsystem.h"
#include "tdc_gamerules.h"
#include "tdc_controls.h"
#include "tdc_shareddefs.h"
#include "tdc_mapinfomenu.h"
#include "tdc_viewport.h"
#include "GameEventListener.h"

using namespace vgui;

const char *GetMapDisplayName( const char *mapName );
const char *GetMapAuthor( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCMapInfoMenu::CTDCMapInfoMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_MAPINFO )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );

	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pContinue = new CExButton( this, "MapInfoContinue", "#TDC_Continue" );
	m_pNameLabel = new Label( this, "NameLabel", "" );
	m_pGoalLabel = new Label( this, "GoalLabel", "" );
	m_pGoalImage = new CTDCImagePanel( this, "GoalImage" );

	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "game_maploaded" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCMapInfoMenu::~CTDCMapInfoMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/MapInfoMenu.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( FStrEq( type, "server_spawn" ) )
	{
		// Set the level name after the server spawn
		const char *pszMapName = GetMapDisplayName( event->GetString( "mapname" ) );
		SetDialogVariable( "mapname", pszMapName );
	}
	if ( FStrEq( type, "game_maploaded" ) )
	{
		if ( TDCGameRules() )
		{
			GameTypeInfo_t *pInfo = TDCGameRules()->GetGameTypeInfo();

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

			// Set the description. Use team neutral description since we have not joined a team yet.
			m_pGoalLabel->SetText( VarArgs( "#GameSubTypeGoal_%s", pInfo->name ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	m_KeyRepeat.Reset();

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		CheckBackContinueButtons();

		m_pContinue->RequestFocus();
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::CheckBackContinueButtons()
{
	if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
	{
		m_pContinue->SetText( "#TDC_Continue" );
	}
	else
	{
		m_pContinue->SetText( "#TDC_Close" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::OnCommand( const char *command )
{
	m_KeyRepeat.Reset();

	if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			GetTDCViewPort()->ShowTeamMenu( true );
		}

		UTIL_IncrementMapKey( "viewed" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( code == KEY_XBUTTON_A )
	{
		OnCommand( "continue" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCMapInfoMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	BaseClass::OnThink();
}

// TODO: Move this into a manifest.
struct s_MapInfo
{
	const char	*pDiskName;
	const char	*pDisplayName;
	const char	*pAuthor;
};

static s_MapInfo s_Maps[] =
{
	{ "dm_crossfire",		"Crossfire",		"EonDynamo" },
	{ "dm_lumberyard",		"Lumberyard",		"Valve, EonDynamo" },
};

struct s_MapTypeInfo
{
	const char	*pDiskPrefix;
	int			iLength;
};

static s_MapTypeInfo s_MapTypes[] =
{
	{ "dm_", 3 },
	{ "ctf_", 4 },
	{ "duel_", 5 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapDisplayName( const char *mapName )
{
	if ( !mapName )
		return "";

	static char szDisplayName[256];

	// check our lookup table
	V_strcpy_safe( szDisplayName, mapName );
	V_strlower( szDisplayName );

#if 0 //no reason for this hacky crap //DAN_H
	for ( int i = 0; i < ARRAYSIZE( s_Maps ); ++i )
	{
		if ( !V_strcmp( s_Maps[i].pDiskName, szDisplayName ) )
		{
			return s_Maps[i].pDisplayName;
		}
	}
#endif

	// we haven't found a "friendly" map name, so let's just clean up what we have
	char *pszSrc = szDisplayName;

	// Remove the prefix.
	for ( int i = 0; i < ARRAYSIZE( s_MapTypes ); ++i )
	{
		if ( !V_strncmp( mapName, s_MapTypes[i].pDiskPrefix, s_MapTypes[i].iLength ) )
		{
			pszSrc = szDisplayName + s_MapTypes[i].iLength;
			break;
		}
	}

	// Replace underscores with spaces.
	char *pChar = pszSrc;
	while ( ( pChar = strchr( pChar, '_' ) ) != NULL )
		*pChar = ' ';

	V_strupr( pszSrc );

	return pszSrc;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapAuthor( const char *mapName )
{
	if ( mapName )
	{
		// Have we got a registered map named that?
		for ( int i = 0; i < ARRAYSIZE( s_Maps ); ++i )
		{
			if ( !Q_stricmp( s_Maps[i].pDiskName, mapName ) )
			{
				// If so, return the registered author
				return s_Maps[i].pAuthor;
			}
		}
	}

	return ""; // Otherwise, return NULL
}