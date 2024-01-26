#include "cbase.h"
#include "tdc_backgroundpanel.h"
#include "tdc_mainmenupanel.h"
#include "tdc_mainmenu.h"
#include <vgui/ISurface.h>
#include <filesystem.h>

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_RATIO_WIDE 1920.0 / 1080.0
#define DEFAULT_RATIO 1024.0 / 768.0

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCBackgroundPanel::CTDCBackgroundPanel( vgui::Panel* parent, const char *panelName ) : CTDCMenuPanelBase( parent, panelName )
{
	m_pVideo = new CTDCVideoPanel( this, "BackgroundVideo" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCBackgroundPanel::~CTDCBackgroundPanel()
{

}

void CTDCBackgroundPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/BackgroundPanel.res" );

	int width, height;
	surface()->GetScreenSize( width, height );

	float fRatio = (float)width / (float)height;
	bool bWidescreen = ( fRatio < 1.5 ? false : true );

	GetRandomVideo( m_szVideoFile, sizeof( m_szVideoFile ), bWidescreen );

	float flRatio = ( bWidescreen ? DEFAULT_RATIO_WIDE : DEFAULT_RATIO );
	int iWide = (float)height * flRatio + 4;
	m_pVideo->SetBounds( -1, -1, iWide, iWide );
}

void CTDCBackgroundPanel::SetVisible( bool bVisible )
{
	BaseClass::SetVisible( bVisible );

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	VideoReplay();
}

void CTDCBackgroundPanel::VideoReplay()
{
	if ( IsVisible() && m_szVideoFile[0] != '\0' )
	{
		m_pVideo->Activate();
		m_pVideo->BeginPlaybackNoAudio( m_szVideoFile );
	}
	else
	{
		m_pVideo->Shutdown();
	}
}

void CTDCBackgroundPanel::GetRandomVideo( char *pszBuf, int iBufLength, bool bWidescreen )
{
	pszBuf[0] = '\0';

	KeyValues *pVideoKeys = new KeyValues( "Videos" );
	if ( pVideoKeys->LoadFromFile( filesystem, "media/menubackgrounds.txt", "MOD" ) == false )
		return;

	KeyValues *pGroupKey = pVideoKeys->FindKey( bWidescreen ? "widescreen" : "normal" );
	if ( !pGroupKey )
		return;

	CUtlVector<const char *> fileList;
	for ( KeyValues *pSubData = pGroupKey->GetFirstSubKey(); pSubData; pSubData = pSubData->GetNextKey() )
	{
		fileList.AddToTail( pSubData->GetString() );
	}

	if ( fileList.Count() == 0 )
		return;

	V_strncpy( pszBuf, fileList.Random(), iBufLength );
}
