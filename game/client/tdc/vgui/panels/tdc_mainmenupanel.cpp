#include "cbase.h"
#include "tdc_mainmenupanel.h"
#include "controls/tdc_advbutton.h"
#include "controls/tdc_advslider.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/ImagePanel.h"
#include "engine/IEngineSound.h"
#include "vgui_avatarimage.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tdc_mainmenu_music( "tdc_mainmenu_music", "1", FCVAR_ARCHIVE, "Toggle music in the main menu" );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCMainMenuPanel::CTDCMainMenuPanel( Panel* parent, const char *panelName ) : CTDCMenuPanelBase( parent, panelName )
{
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	m_pProfileAvatar = new CAvatarImagePanel( this, "AvatarImage" );
	m_pFakeBGImage = new ImagePanel( this, "FakeBGImage" );

	m_psMusicStatus = MUSIC_FIND;
	m_nSongGuid = 0;

	if ( steamapicontext->SteamUser() )
	{
		m_SteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	m_iShowFakeIntro = 4;

	ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCMainMenuPanel::~CTDCMainMenuPanel()
{

}

void CTDCMainMenuPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/MainMenuPanel.res" );
}

void CTDCMainMenuPanel::PerformLayout()
{
	m_pProfileAvatar->SetPlayer( m_SteamID, k_EAvatarSize64x64 );
	m_pProfileAvatar->SetShouldDrawFriendIcon( false );

	char szNickname[128];
	const char *pszNickname = steamapicontext->SteamFriends() ? steamapicontext->SteamFriends()->GetPersonaName() : "Unknown";
	V_strcpy_safe( szNickname, pszNickname );
	V_strupr( szNickname );

	SetDialogVariable( "nickname", szNickname );

	if ( m_iShowFakeIntro > 0 )
	{
		char szBGName[128];
		engine->GetMainMenuBackgroundName( szBGName, sizeof( szBGName ) );
		char szImage[128];
		V_sprintf_safe( szImage, "../console/%s", szBGName );
		int width, height;
		surface()->GetScreenSize( width, height );
		float fRatio = (float)width / (float)height;
		bool bWidescreen = ( fRatio < 1.5 ? false : true );
		if ( bWidescreen )
			V_strcat_safe( szImage, "_widescreen" );
		m_pFakeBGImage->SetImage( szImage );
		m_pFakeBGImage->SetVisible( true );
		m_pFakeBGImage->SetAlpha( 255 );
	}
}

CON_COMMAND( tdc_menu_newquit, "" )
{
	guiroot->ShowPanel( QUIT_MENU );
};

CON_COMMAND( tdc_menu_newoptionsdialog, "" )
{
	guiroot->ShowPanel( OPTIONSDIALOG_MENU );
};

CON_COMMAND( tdc_menu_newloadout, "" )
{
	guiroot->ShowPanel( LOADOUT_MENU );
};

CON_COMMAND( tdc_menu_newcreateserver, "" )
{
	guiroot->ShowPanel( CREATESERVER_MENU );
};

void CTDCMainMenuPanel::OnCommand( const char* command )
{
	if ( !V_stricmp( command, "newquit" ) )
	{
		guiroot->ShowPanel( QUIT_MENU );
	}
	else if ( !V_stricmp( command, "newoptionsdialog" ) )
	{
		guiroot->ShowPanel( OPTIONSDIALOG_MENU );
	}
	else if ( !V_stricmp( command, "newloadout" ) )
	{
		guiroot->ShowPanel( LOADOUT_MENU );
	}
	else if ( !V_stricmp( command, "newcreateserver" ) )
	{
		guiroot->ShowPanel( CREATESERVER_MENU );
	}
	else if ( !V_stricmp( command, "newstats" ) )
	{
		//guiroot->ShowPanel( STATSUMMARY_MENU );
	}
	else if ( !V_stricmp( command, "randommusic" ) )
	{
		enginesound->StopSoundByGuid( m_nSongGuid );
	}
	else if ( V_stristr( command, "gamemenucommand " ) )
	{
		engine->ClientCmd( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTDCMainMenuPanel::OnTick()
{
	if ( tdc_mainmenu_music.GetBool() && !guiroot->IsInLevel() )
	{
		if ( ( m_psMusicStatus == MUSIC_FIND || m_psMusicStatus == MUSIC_STOP_FIND ) && !enginesound->IsSoundStillPlaying( m_nSongGuid ) )
		{
			m_psMusicStatus = MUSIC_PLAY;
		}
		else if ( ( m_psMusicStatus == MUSIC_PLAY || m_psMusicStatus == MUSIC_STOP_PLAY ) )
		{
			enginesound->StopSoundByGuid( m_nSongGuid );

			C_RecipientFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, "music.gamestartup" );
			m_nSongGuid = enginesound->GetGuidForLastSoundEmitted();
			m_psMusicStatus = MUSIC_FIND;
		}
	}
	else if ( m_psMusicStatus == MUSIC_FIND )
	{
		enginesound->StopSoundByGuid( m_nSongGuid );
		m_psMusicStatus = ( m_nSongGuid == 0 ? MUSIC_STOP_FIND : MUSIC_STOP_PLAY );
	}
}

void CTDCMainMenuPanel::OnThink()
{
	if ( m_iShowFakeIntro > 0 )
	{
		m_iShowFakeIntro--;
		if ( m_iShowFakeIntro == 0 )
		{
			GetAnimationController()->RunAnimationCommand( m_pFakeBGImage, "Alpha", 0, 1.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE );
		}
	}

	if ( m_pFakeBGImage->IsVisible() && m_pFakeBGImage->GetAlpha() == 0 )
	{
		m_pFakeBGImage->SetVisible( false );
	}
}

void CTDCMainMenuPanel::Show()
{
	BaseClass::Show();

//	RequestFocus();
}

void CTDCMainMenuPanel::Hide()
{
	BaseClass::Hide();
}

void CTDCMainMenuPanel::HideFakeIntro( void )
{
	m_iShowFakeIntro = 0;
	m_pFakeBGImage->SetVisible( false );
}
