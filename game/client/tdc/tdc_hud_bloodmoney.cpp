//=============================================================================
//
// Purpose:
//
//=============================================================================
#include "cbase.h"
#include "tdc_hud_bloodmoney.h"
#include "iclientmode.h"
#include "c_tdc_objective_resource.h"
#include "c_tdc_bloodmoney.h"
#include <vgui/IVGui.h>
#include "view.h"
#include "tdc_gamerules.h"
#include "tdc_hud_freezepanel.h"
#include <vgui/ISurface.h>
#include "c_tdc_team.h"
#include "c_tdc_playerresource.h"
#include "clientsteamcontext.h"
#include "tdc_announcer.h"
#include "tdc_hud_objectivestatus.h"

using namespace vgui;

vgui::IImage* GetDefaultAvatarImage( int iPlayerIndex );

CHudBloodMoney::CHudBloodMoney( vgui::Panel *pParent, const char *pszName ) : BaseClass( pParent, pszName )
{
	m_pLocalPlayerPanel = new EditablePanel( this, "LocalPlayerPanel" );
	m_pBestPlayerPanel = new EditablePanel( this, "BestPlayerPanel" );
	m_pPlayerAvatar = new CAvatarImagePanel( m_pLocalPlayerPanel, "PlayerAvatar" );
	m_pRivalAvatar = new CAvatarImagePanel( m_pBestPlayerPanel, "PlayerAvatar" );
	m_pLocalProgress = new CBloodMoneyProgressBar( m_pLocalPlayerPanel, "ProgressBar" );
	m_pRivalProgress = new CBloodMoneyProgressBar( m_pBestPlayerPanel, "ProgressBar" );
	m_pLocalProgressCarried = new CBloodMoneyProgressBar( m_pLocalPlayerPanel, "ProgressBarCarried" );
	m_pRivalProgressCarried = new CBloodMoneyProgressBar( m_pBestPlayerPanel, "ProgressBarCarried" );
	m_pZoneProgressBar = new CTDCProgressBar( this, "ZoneProgressBar" );
	m_pRadar = new CHudBloodMoneyRadar( this, "BloodMoneyRadar" );

	m_pPlayingTo = new CExLabel( this, "PlayingTo", "0" );
	m_pPlayingToBG = new ScalableImagePanel( this, "PlayingToBG" );

	m_iRivalPlayer = 0;
	m_iLeadStatus = DM_STATUS_NONE;

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoney::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/HudObjectiveBloodMoney.res" );

	m_pPlayerAvatar->SetShouldDrawFriendIcon( false );
	m_pRivalAvatar->SetShouldDrawFriendIcon( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBloodMoney::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	if ( !TDCGameRules() || TDCGameRules()->IsInWaitingForPlayers() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoney::LevelInit( void )
{
	m_iRivalPlayer = 0;
	m_iLeadStatus = DM_STATUS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoney::Reset()
{
	m_pPlayerAvatar->SetPlayer( ClientSteamContext().GetLocalPlayerSteamID(), k_EAvatarSize32x32 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoney::OnTick( void )
{
	if ( !IsVisible() || !TDCGameRules() || !g_TDC_PR )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	int iFragLimit = TDCGameRules()->GetScoreLimit();

	if ( iFragLimit > 0 )
	{
		SetPlayingToLabelVisible( true );
		SetDialogVariable( "fraglimit", iFragLimit );
	}
	else
	{
		SetPlayingToLabelVisible( false );
	}

	// Get the local player's score.
	int iLocalIndex = pPlayer->entindex();
	int iLocalScore = g_TDC_PR->GetTotalScore( iLocalIndex );

	m_pLocalPlayerPanel->SetDialogVariable( "score", iLocalScore );
	m_pLocalPlayerPanel->SetDialogVariable( "carried", pPlayer->GetNumMoneyPacks() );
	m_pLocalProgress->SetDrawColor( g_TDC_PR->GetPlayerColor( iLocalIndex ) );
	m_pLocalProgressCarried->SetDrawColor( g_TDC_PR->GetPlayerColor( iLocalIndex ) );

	if ( iFragLimit > 0 )
	{
		float flProgress = (float)iLocalScore / (float)iFragLimit;
		float flCarried = (float)pPlayer->GetNumMoneyPacks() / (float)iFragLimit;
		m_pLocalProgress->SetProgress( flProgress );
		m_pLocalProgressCarried->SetProgress( flProgress + flCarried );
	}
	else
	{
		m_pLocalProgress->SetProgress( 1.0f );
		m_pLocalProgressCarried->SetProgress( 0.0f );
	}

	// Figure out who's the leading player.
	int iBestIndex = 0;
	int iBestScore = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( g_TDC_PR->IsConnected( i ) && g_PR->GetTeam( i ) >= FIRST_GAME_TEAM )
		{
			int iScore = g_TDC_PR->GetTotalScore( i );

			if ( i != iLocalIndex && iScore >= iBestScore )
			{
				iBestScore = iScore;
				iBestIndex = i;
			}
		}
	}

	// Set their score and avatar.
	if ( iBestIndex )
	{
		m_pBestPlayerPanel->SetDialogVariable( "score", iBestScore );
		m_pBestPlayerPanel->SetDialogVariable( "carried", g_TDC_PR->GetNumMoneyPacks( iBestIndex ) );
		m_pRivalProgress->SetDrawColor( g_TDC_PR->GetPlayerColor( iBestIndex ) );
		m_pRivalProgressCarried->SetDrawColor( g_TDC_PR->GetPlayerColor( iBestIndex ) );
	}
	else
	{
		m_pBestPlayerPanel->SetDialogVariable( "score", "" );
		m_pBestPlayerPanel->SetDialogVariable( "carried", "" );
		m_pRivalProgress->SetDrawColor( COLOR_BLACK );
	}

	if ( iFragLimit > 0 )
	{
		float flProgress = (float)iBestScore / (float)iFragLimit;
		float flCarried = (float)g_TDC_PR->GetNumMoneyPacks( iBestIndex ) / (float)iFragLimit;
		m_pRivalProgress->SetProgress( flProgress );
		m_pRivalProgressCarried->SetProgress( flProgress + flCarried );
	}
	else
	{
		m_pRivalProgress->SetProgress( 1.0f );
		m_pRivalProgressCarried->SetProgress( 0.0f );
	}

	if ( iBestIndex != m_iRivalPlayer )
	{
		m_iRivalPlayer = iBestIndex;

		m_pRivalAvatar->SetPlayer( iBestIndex, k_EAvatarSize32x32 );
		m_pRivalAvatar->SetDefaultAvatar( GetDefaultAvatarImage( iBestIndex ) );
	}

	// Show the leading player on the top.
	if ( iLocalScore >= iBestScore )
	{
		m_pLocalPlayerPanel->SetZPos( 2 );
		m_pBestPlayerPanel->SetZPos( 1 );
	}
	else
	{
		m_pLocalPlayerPanel->SetZPos( 1 );
		m_pBestPlayerPanel->SetZPos( 2 );
	}

	if ( GetLocalPlayerTeam() >= FIRST_GAME_TEAM )
	{
		m_pLocalPlayerPanel->SetVisible( true );
		m_pBestPlayerPanel->SetVisible( true );
		SetPlayingToLabelVisible( true );
	}
	else
	{
		m_pLocalPlayerPanel->SetVisible( false );
		m_pBestPlayerPanel->SetVisible( false );
		SetPlayingToLabelVisible( false );
	}

	// Check for lead announcements.
	int iStatus;

	if ( iBestIndex == 0 || pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
	{
		iStatus = DM_STATUS_NONE;
	}
	else if ( iLocalScore > iBestScore )
	{
		iStatus = DM_STATUS_LEADTAKEN;
	}
	else if ( iLocalScore == iBestScore )
	{
		iStatus = DM_STATUS_LEADTIED;
	}
	else
	{
		iStatus = DM_STATUS_LEADLOST;
	}

	if ( m_iLeadStatus != iStatus )
	{
		if ( m_iLeadStatus != DM_STATUS_NONE )
		{
			switch ( iStatus )
			{
			case DM_STATUS_LEADTAKEN:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTAKEN );
				break;
			case DM_STATUS_LEADTIED:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTIED );
				break;
			case DM_STATUS_LEADLOST:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADLOST );
				break;
			}
		}

		m_iLeadStatus = iStatus;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoney::OnThink( void )
{
	// Update zone switch timer.
	if ( ObjectiveResource() )
	{
		float flTimeTotal = ObjectiveResource()->GetMoneyZoneDuration();
		float flTimeRemaining = Max( 0.0f, ObjectiveResource()->GetMoneyZoneSwitchTime() - gpGlobals->curtime );
		SetDialogVariable( "zonetime", (int)ceil( flTimeRemaining ) );
		m_pZoneProgressBar->SetPercentage( ( flTimeTotal - flTimeRemaining ) / flTimeTotal ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoney::SetPlayingToLabelVisible( bool bVisible )
{
	m_pPlayingTo->SetVisible( bVisible );
	m_pPlayingToBG->SetVisible( bVisible );
}


CHudTeamBloodMoney::CHudTeamBloodMoney( vgui::Panel *pParent, const char *pszName ) : BaseClass( pParent, pszName )
{
	m_pRedPanel = new EditablePanel( this, "RedPanel" );
	m_pBluePanel = new EditablePanel( this, "BluePanel" );
	m_pRedProgress = new CBloodMoneyProgressBar( m_pRedPanel, "ProgressBar" );
	m_pBlueProgress = new CBloodMoneyProgressBar( m_pBluePanel, "ProgressBar" );
	m_pRedProgressCarried = new CBloodMoneyProgressBar( m_pRedPanel, "ProgressBarCarried" );
	m_pBlueProgressCarried = new CBloodMoneyProgressBar( m_pBluePanel, "ProgressBarCarried" );
	m_pZoneProgressBar = new CTDCProgressBar( this, "ZoneProgressBar" );
	m_pRadar = new CHudBloodMoneyRadar( this, "BloodMoneyRadar" );

	m_pPlayingTo = new CExLabel( this, "PlayingTo", "0" );
	m_pPlayingToBG = new ScalableImagePanel( this, "PlayingToBG" );

	m_iLeadStatus = DM_STATUS_NONE;

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamBloodMoney::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/HudObjectiveTeamBloodMoney.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTeamBloodMoney::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	if ( !TDCGameRules() || TDCGameRules()->IsInWaitingForPlayers() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamBloodMoney::LevelInit( void )
{
	m_iLeadStatus = DM_STATUS_NONE;
}

static int TeamsScoreSort( C_TDCTeam* const *p1, C_TDCTeam* const *p2 )
{
	return ( ( *p1 )->GetRoundScore() - ( *p2 )->GetRoundScore() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamBloodMoney::OnTick( void )
{
	if ( !IsVisible() || !TDCGameRules() || !g_TDC_PR )
		return;

	C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pPlayer )
		return;

	int iFragLimit = TDCGameRules()->GetScoreLimit();

	if ( iFragLimit > 0 )
	{
		SetPlayingToLabelVisible( true );
		SetDialogVariable( "fraglimit", iFragLimit );
	}
	else
	{
		SetPlayingToLabelVisible( false );
	}

	// Update zone switch timer.
	if ( ObjectiveResource() )
	{
		float flUnlockTime = Max( 0.0f, ObjectiveResource()->GetMoneyZoneSwitchTime() - gpGlobals->curtime );
		SetDialogVariable( "zonetime", (int)ceil( flUnlockTime ) );
	}

	CUtlVector<C_TDCTeam *> aTeams;
	int iLocalTeam = pPlayer->GetTeamNumber();
	int iLocalScore = -1;
	int iBestScore = -1;

	for ( int i = FIRST_GAME_TEAM; i < TDC_TEAM_COUNT; i++ )
	{
		C_TDCTeam *pTeam = GetGlobalTFTeam( i );
		if ( !pTeam )
			continue;

		EditablePanel *pPanel = GetTeamPanel( i );
		int iScore = pTeam->GetRoundScore();
		int iCarried = g_TDC_PR->GetTeamMoneyPacks( i );
		pPanel->SetDialogVariable( "score", iScore );
		pPanel->SetDialogVariable( "carried", iCarried );

		if ( iFragLimit > 0 )
		{
			float flProgress = (float)iScore / (float)iFragLimit;
			float flCarried = (float)iCarried / (float)iFragLimit;
			GetProgressBar( i, false )->SetProgress( flProgress );
			GetProgressBar( i, true )->SetProgress( flProgress + flCarried );
		}
		else
		{
			GetProgressBar( i, false )->SetProgress( 1.0f );
			GetProgressBar( i, true )->SetProgress( 0.0f );
		}

		if ( i == iLocalTeam )
		{
			iLocalScore = iScore;
		}
		else if ( iScore >= iBestScore )
		{
			iBestScore = iScore;
		}

		aTeams.AddToTail( pTeam );
	}

	if ( aTeams.Count() )
	{
		aTeams.Sort( TeamsScoreSort );

		// Show the leading team on the top.
		for ( int i = 0; i < aTeams.Count(); i++ )
		{
			EditablePanel *pPanel = GetTeamPanel( aTeams[i]->GetTeamNumber() );
			pPanel->SetZPos( i + 1 );
		}
	}

	// Check for lead announcements.
	int iStatus;

	if ( iLocalScore == -1 || iBestScore == -1 )
	{
		iStatus = DM_STATUS_NONE;
	}
	else if ( iLocalScore > iBestScore )
	{
		iStatus = DM_STATUS_LEADTAKEN;
	}
	else if ( iLocalScore == iBestScore )
	{
		iStatus = DM_STATUS_LEADTIED;
	}
	else
	{
		iStatus = DM_STATUS_LEADLOST;
	}

	if ( m_iLeadStatus != iStatus )
	{
		if ( m_iLeadStatus != DM_STATUS_NONE )
		{
			switch ( iStatus )
			{
			case DM_STATUS_LEADTAKEN:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTAKEN );
				break;
			case DM_STATUS_LEADTIED:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADTIED );
				break;
			case DM_STATUS_LEADLOST:
				g_TFAnnouncer.Speak( TDC_ANNOUNCER_DM_LEADLOST );
				break;
			}
		}

		m_iLeadStatus = iStatus;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamBloodMoney::OnThink( void )
{
	// Update zone switch timer.
	if ( ObjectiveResource() )
	{
		float flTimeTotal = ObjectiveResource()->GetMoneyZoneDuration();
		float flTimeRemaining = Max( 0.0f, ObjectiveResource()->GetMoneyZoneSwitchTime() - gpGlobals->curtime );
		SetDialogVariable( "zonetime", (int)ceil( flTimeRemaining ) );
		m_pZoneProgressBar->SetPercentage( ( flTimeTotal - flTimeRemaining ) / flTimeTotal );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamBloodMoney::SetPlayingToLabelVisible( bool bVisible )
{
	m_pPlayingTo->SetVisible( bVisible );
	m_pPlayingToBG->SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EditablePanel *CHudTeamBloodMoney::GetTeamPanel( int iTeam )
{
	switch ( iTeam )
	{
	case TDC_TEAM_RED:
		return m_pRedPanel;
	case TDC_TEAM_BLUE:
		return m_pBluePanel;
	default:
		Assert( false );
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBloodMoneyProgressBar* CHudTeamBloodMoney::GetProgressBar( int iTeam, bool bCarried )
{
	switch ( iTeam )
	{
	case TDC_TEAM_RED:
		return bCarried ? m_pRedProgressCarried : m_pRedProgress;
	case TDC_TEAM_BLUE:
		return bCarried ? m_pBlueProgressCarried : m_pBlueProgress;
	default:
		Assert( false );
		return NULL;
	}
}


CBloodMoneyProgressBar::CBloodMoneyProgressBar( vgui::Panel *pParent, const char *pszName ) : BaseClass( pParent, pszName )
{
	m_iTextureId = -1;
	m_bLeftToRight = false;
	m_DrawColor = COLOR_WHITE;
	m_flProgress = 0.0f;
}

static void UTIL_StringToColor( Color &outColor, const char *pszString )
{
	int r = 0, g = 0, b = 0, a = 255;

	if ( pszString[0] && sscanf( pszString, "%d %d %d %d", &r, &g, &b, &a ) >= 3 )
	{
		outColor.SetColor( r, g, b, a );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBloodMoneyProgressBar::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *pszImage = inResourceData->GetString( "image" );
	m_iTextureId = surface()->DrawGetTextureId( pszImage );
	if ( m_iTextureId == -1 )
	{
		m_iTextureId = surface()->CreateNewTextureID( false );
		surface()->DrawSetTextureFile( m_iTextureId, pszImage, true, false );
	}

	m_bLeftToRight = inResourceData->GetBool( "left_to_right" );
	UTIL_StringToColor( m_DrawColor, inResourceData->GetString( "drawcolor" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBloodMoneyProgressBar::Paint( void )
{
	if ( m_flProgress == 0.0f )
		return;

	surface()->DrawSetTexture( m_iTextureId );

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );
	float barWide = wide * m_flProgress;

	// Draw the bar.
	Vertex_t vert[4];

	if ( m_bLeftToRight )
	{
		vert[0].Init( Vector2D( 0.0f, 0.0f ), Vector2D( 0.0f, 0.0f ) );
		vert[1].Init( Vector2D( barWide, 0.0f ), Vector2D( m_flProgress, 0.0f ) );
		vert[2].Init( Vector2D( barWide, tall ), Vector2D( m_flProgress, 1.0f ) );
		vert[3].Init( Vector2D( 0.0f, tall ), Vector2D( 0.0f, 1.0f ) );
	}
	else
	{
		vert[0].Init( Vector2D( wide - barWide, 0.0f ), Vector2D( 1.0f - m_flProgress, 0.0f ) );
		vert[1].Init( Vector2D( wide, 0.0f ), Vector2D( 1.0f, 0.0f ) );
		vert[2].Init( Vector2D( wide, tall ), Vector2D( 1.0f, 1.0f ) );
		vert[3].Init( Vector2D( wide - barWide, tall ), Vector2D( 1.0f - m_flProgress, 1.0f ) );
	}

	surface()->DrawSetColor( m_DrawColor );
	surface()->DrawTexturedPolygon( 4, vert );
}


CHudBloodMoneyRadar::CHudBloodMoneyRadar( vgui::Panel *pParent, const char *pszName ) : BaseClass( pParent, pszName )
{
	m_pZoneIcon = new ImagePanel( this, "ZoneIcon" );

	m_bOnScreen = false;
	m_flRotation = 0.0f;
	m_iRadarAlpha = 255;

	ivgui()->AddTickSignal( GetVPanel() );
	SetPostChildPaintEnabled( true );
}

CHudBloodMoneyRadar::~CHudBloodMoneyRadar()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoneyRadar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/HudObjectiveBloodMoneyRadar.res" );
	m_ArrowMaterial.Init( "hud/bloodmoney_radar_arrow", TEXTURE_GROUP_VGUI );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoneyRadar::OnTick( void )
{
	// Update money icon.
	m_pZoneIcon->SetVisible( ObjectiveResource() && ObjectiveResource()->GetActiveMoneyZone() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoneyRadar::PaintBackground( void )
{
	if ( !ObjectiveResource() || !ObjectiveResource()->GetActiveMoneyZone() )
		return;

	// Position the zone icon to point at the active money zone.
	C_MoneyDeliveryZone *pZone = dynamic_cast<C_MoneyDeliveryZone *>( ClientEntityList().GetBaseEntity( ObjectiveResource()->GetActiveMoneyZone() ) );
	if ( !pZone )
		return;

	int iX, iY;
	Vector vecTarget = pZone->GetIconPosition();
	Vector vecDelta = vecTarget - MainViewOrigin();
	float flDist = VectorNormalize( vecDelta );
	bool bOnScreen = GetVectorInScreenSpace( vecTarget, iX, iY );

	int halfWidth = m_pZoneIcon->GetWide() / 2;
	if ( !bOnScreen || iX < halfWidth || iX > ScreenWidth() - halfWidth )
	{
		// It's off the screen. Position the callout.
		float xpos, ypos;
		float flRadius = YRES( 100 );
		GetCallerPosition( vecDelta, flRadius, &xpos, &ypos, &m_flRotation );

		iX = xpos;
		iY = ypos;

		// Move the icon there
		m_pZoneIcon->SetPos( iX - halfWidth, iY - ( m_pZoneIcon->GetTall() / 2 ) );
		m_bOnScreen = false;
	}
	else
	{
		// On screen
		m_pZoneIcon->SetPos( iX - halfWidth, iY - ( m_pZoneIcon->GetTall() / 2 ) );
		m_bOnScreen = true;
	}

	// Fade out the icon as player gets closer.
	m_iRadarAlpha = RemapValClamped( flDist, 1024, 256, 255, 0 );
	m_pZoneIcon->SetAlpha( m_iRadarAlpha );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoneyRadar::PostChildPaint( void )
{
	if ( !m_pZoneIcon->IsVisible() || m_bOnScreen )
		return;

	int x, y, wide, tall;
	m_pZoneIcon->GetBounds( x, y, wide, tall );

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();

	VMatrix panelRotation;
	MatrixSetIdentity( panelRotation );
	MatrixBuildRotationAboutAxis( panelRotation, Vector( 0, 0, -1 ), m_flRotation );
	panelRotation.SetTranslation( Vector( x + wide / 2, y + tall / 2, 0 ) );

	pRenderContext->LoadMatrix( panelRotation );
	pRenderContext->Bind( m_ArrowMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( -wide / 2, -tall / 2, 0.0f );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Color4ub( 255, 255, 255, m_iRadarAlpha );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( wide / 2, -tall / 2, 0.0f );
	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Color4ub( 255, 255, 255, m_iRadarAlpha );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( wide / 2, tall / 2, 0.0f );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Color4ub( 255, 255, 255, m_iRadarAlpha );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( -wide / 2, tall / 2, 0.0f );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Color4ub( 255, 255, 255, m_iRadarAlpha );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBloodMoneyRadar::GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up( 0, 0, 1 );
	AngleVectors( playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize( forward );
	CrossProduct( up, forward, right );
	float front = DotProduct( vecDelta, forward );
	float side = DotProduct( vecDelta, right );
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2( *xpos, *ypos ) + M_PI_F;
	*flRotation *= 180 / M_PI_F;

	float yawRadians = -DEG2RAD( *flRotation );
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the ellipse
	*xpos = (int)( ( ScreenWidth() / 2 ) + ( flRadius * sa * 3.0f ) );
	*ypos = (int)( ( ScreenHeight() / 2 ) - ( flRadius * ca ) );
}
