//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tdc_hud_target_id.h"
#include "c_tdc_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "tdc_gamerules.h"
#include "tdc_hud_statpanel.h"
#include "functionproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CMainTargetID );
DECLARE_HUDELEMENT( CSpectatorTargetID );
DECLARE_HUDELEMENT( CSecondaryTargetID );

using namespace vgui;

vgui::IImage* GetDefaultAvatarImage( int iPlayerIndex );
extern ConVar tdc_coloredhud;

ConVar tdc_hud_target_id_alpha( "tdc_hud_target_id_alpha", "100", FCVAR_ARCHIVE , "Alpha value of target id background, default 100" );
ConVar tdc_hud_target_id_show_avatars( "tdc_hud_target_id_show_avatars", "2", FCVAR_ARCHIVE, "Display Steam avatars on TargetID. 1 = everyone, 2 = friends only." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pTargetNameLabel = NULL;
	m_pTargetDataLabel = NULL;
	m_pAvatar = NULL;
	m_pBGPanel = NULL;
	m_pTargetHealth = new CTDCSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_pAmmoIcon = NULL;
	m_pMoveableSubPanel = NULL;
	m_bLayoutOnUpdate = false;

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	m_iRenderPriority = 5;
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Reset( void )
{
	m_pTargetHealth->Reset();
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	LoadControlSettings( "resource/UI/TargetID.res" );

	BaseClass::ApplySchemeSettings( scheme );

	m_pTargetNameLabel = dynamic_cast<Label *>( FindChildByName( "TargetNameLabel" ) );
	m_pTargetDataLabel = dynamic_cast<Label *>( FindChildByName( "TargetDataLabel" ) );
	m_pAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( "AvatarImage" ) );
	m_pBGPanel = dynamic_cast<CTDCImagePanel *>( FindChildByName( "TargetIDBG" ) );
	m_pAmmoIcon = dynamic_cast<ImagePanel *>( FindChildByName( "AmmoIcon" ) );
	m_pMoveableSubPanel = dynamic_cast<EditablePanel *>( FindChildByName( "MoveableSubPanel" ) );
	m_hFont = scheme->GetFont( "TargetID", true );

	// Hide it for now until it's implemented.
	if ( m_pMoveableSubPanel )
	{
		m_pMoveableSubPanel->SetVisible( false );
	}

	//SetPaintBackgroundEnabled( true );
	//SetBgColor( Color( 0, 0, 0, 90 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_iRenderPriority = inResourceData->GetInt( "priority" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTargetID::GetRenderGroupPriority( void )
{
	return m_iRenderPriority;
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTargetID::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_TDCPlayer *pLocalTFPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalTFPlayer )
		return false;

	// Get our target's ent index
	m_iTargetEntIndex = CalculateTargetIndex(pLocalTFPlayer);
	if ( !m_iTargetEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && ( gpGlobals->curtime > m_flLastChangeTime ) )
		{
			m_flLastChangeTime = 0;
			m_iLastEntIndex = 0;
		}
		else
		{
			// Keep re-using the old one
			m_iTargetEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
	}

	if ( m_iTargetEntIndex )
	{
		C_TDCPlayer *pPlayer = ToTDCPlayer( ClientEntityList().GetBaseEntity( m_iTargetEntIndex ) );
		if ( pPlayer && !pPlayer->IsEnemyPlayer() )
		{
			if ( !IsVisible() || ( m_iTargetEntIndex != m_iLastEntIndex ) )
			{
				m_iLastEntIndex = m_iTargetEntIndex;
				m_bLayoutOnUpdate = true;
			}

			UpdateID();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::PerformLayout( void )
{
	int iWidth = m_pTargetHealth->GetXPos() + m_pTargetHealth->GetWide() + YRES( 5 );
	int iBaseTextPos = iWidth;

	if ( m_pAvatar && m_pAvatar->IsVisible() )
	{
		// Push the name further off to the right if avatar is on.
		m_pAvatar->SetPos( iBaseTextPos, m_pAvatar->GetYPos() );
		int iAvatarOffset = m_pAvatar->GetWide() + YRES( 2 );
		m_pTargetNameLabel->SetPos( iBaseTextPos + iAvatarOffset, m_pTargetNameLabel->GetYPos() );
		iWidth += iAvatarOffset;
	}
	else
	{
		// Place the name off to the right of health.
		m_pTargetNameLabel->SetPos( iBaseTextPos, m_pTargetNameLabel->GetYPos() );
	}

	m_pTargetDataLabel->SetPos( iBaseTextPos, m_pTargetDataLabel->GetYPos() );

	if ( m_pAmmoIcon )
	{
		m_pAmmoIcon->SetPos( iBaseTextPos, m_pAmmoIcon->GetYPos() );
	}

	int iTextW, iTextH;
	int iDataW, iDataH;
	m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
	m_pTargetDataLabel->GetContentSize( iDataW, iDataH );
	iWidth += Max( iTextW, iDataW ) + YRES( 10 );

	SetWide( iWidth );
	SetPos( ( ScreenWidth() - iWidth ) * 0.5, GetYPos() );

	if ( m_pBGPanel )
	{
		m_pBGPanel->SetSize( iWidth, GetTall() );
		m_pBGPanel->SetAlpha( tdc_hud_target_id_alpha.GetFloat() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTargetID::CalculateTargetIndex( C_TDCPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = pLocalTFPlayer->GetIDTarget(); 

	// If our target entity is already in our secondary ID, don't show it in primary.
	CSecondaryTargetID *pSecondaryID = GET_HUDELEMENT( CSecondaryTargetID );
	if ( pSecondaryID && pSecondaryID != this && pSecondaryID->GetTargetIndex() == iIndex )
	{
		iIndex = 0;
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::UpdateID( void )
{
	wchar_t sIDString[ MAX_ID_STRING ] = L"";
	wchar_t sDataString[ MAX_ID_STRING ] = L"";

	C_TDCPlayer *pLocalTFPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalTFPlayer )
		return;

	if ( !g_PR )
		return;

	// Get our target's ent index
	Assert( m_iTargetEntIndex );

	// Is this an entindex sent by the server?
	if ( !m_iTargetEntIndex )
		return;

	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( m_iTargetEntIndex );
	if ( !pEnt )
		return;

	int iHealth = 0;
	int iMaxHealth = 1;
	int iMaxBuffedHealth = 0;
	int iColorNum = TEAM_UNASSIGNED;
	bool bShowAmmo = false;
	int iAvatarPlayer = 0;

	// Some entities we always want to check, cause the text may change
	// even while we're looking at it
	// Is it a player?
	if ( IsPlayerIndex( m_iTargetEntIndex ) )
	{
		C_TDCPlayer *pPlayer = static_cast<C_TDCPlayer*>( pEnt );
		if ( !pPlayer )
			return;

		const char *printFormatString = NULL;
		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];

		// Get their name, team color and avatar.
		g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof( wszPlayerName ) );

		if ( TDCGameRules()->IsTeamplay() )
		{
			iColorNum = pPlayer->GetTeamNumber();
		}
		else
		{
			// Use paintable material in FFA.
			iColorNum = tdc_coloredhud.GetBool() ? TEAM_UNASSIGNED : TEAM_SPECTATOR;
		}

		iAvatarPlayer = pPlayer->entindex();

		pPlayer->GetTargetIDDataString( sDataString, sizeof( sDataString ), bShowAmmo );

		if ( !pPlayer->IsEnemyPlayer() )
		{
			printFormatString = "#TDC_playerid_sameteam";
		}
		else
		{
			printFormatString = "#TDC_playerid_diffteam";
		}

		iHealth = pPlayer->GetHealth();
		iMaxHealth = g_TDC_PR->GetMaxHealth( m_iTargetEntIndex );
		iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();

		wchar_t *pszPrepend = GetPrepend();
		if ( !pszPrepend || !pszPrepend[0] )
		{
			pszPrepend = L"";
		}
		g_pVGuiLocalize->ConstructString( sIDString, sizeof( sIDString ), g_pVGuiLocalize->Find( printFormatString ), 2, pszPrepend, wszPlayerName );
	}

	// Setup health icon
	if ( !pEnt->IsAlive() )
	{
		iHealth = 0;	// fixup for health being 1 when dead
	}

	m_pTargetHealth->SetHealth( iHealth, iMaxHealth, iMaxBuffedHealth );
	m_pTargetHealth->SetVisible( true );

	m_pBGPanel->SetBGImage( iColorNum );

	if ( m_pAvatar )
	{
		bool bShowAvatar = ( tdc_hud_target_id_show_avatars.GetBool() && iAvatarPlayer );

		if ( bShowAvatar && tdc_hud_target_id_show_avatars.GetInt() == 2 )
		{
			CSteamID steamID;
			if ( g_TDC_PR->GetSteamID( iAvatarPlayer, &steamID ) )
			{
				bShowAvatar = ( steamapicontext->SteamFriends()->GetFriendRelationship( steamID ) == k_EFriendRelationshipFriend );
			}
			else
			{
				bShowAvatar = false;
			}
		}

		if ( m_pAvatar->IsVisible() != bShowAvatar )
		{
			m_pAvatar->SetVisible( bShowAvatar );
			m_bLayoutOnUpdate = true;
		}

		// Only update avatar if we're switching to a different player.
		if ( m_pAvatar->IsVisible() && m_bLayoutOnUpdate )
		{
			m_pAvatar->SetDefaultAvatar( GetDefaultAvatarImage( iAvatarPlayer ) );
			m_pAvatar->SetPlayer( iAvatarPlayer );
			m_pAvatar->SetShouldDrawFriendIcon( false );
		}
	}

	if ( m_pAmmoIcon )
	{
		m_pAmmoIcon->SetVisible( bShowAmmo );
	}

	int iNameW, iDataW, iIgnored;
	m_pTargetNameLabel->GetContentSize( iNameW, iIgnored );
	m_pTargetDataLabel->GetContentSize( iDataW, iIgnored );

	// Target name
	if ( sIDString[0] )
	{
		sIDString[ARRAYSIZE( sIDString ) - 1] = '\0';
		m_pTargetNameLabel->SetVisible( true );

		// TODO: Support	if( hud_centerid.GetInt() == 0 )
		SetDialogVariable( "targetname", sIDString );
	}
	else
	{
		m_pTargetNameLabel->SetVisible( false );
		m_pTargetNameLabel->SetText( "" );
	}

	// Extra target data
	if ( sDataString[0] )
	{
		sDataString[ARRAYSIZE( sDataString ) - 1] = '\0';
		m_pTargetDataLabel->SetVisible( true );
		SetDialogVariable( "targetdata", sDataString );
	}
	else
	{
		m_pTargetDataLabel->SetVisible( false );
		m_pTargetDataLabel->SetText( "" );
	}

	int iPostNameW, iPostDataW;
	m_pTargetNameLabel->GetContentSize( iPostNameW, iIgnored );
	m_pTargetDataLabel->GetContentSize( iPostDataW, iIgnored );

	if ( m_bLayoutOnUpdate || ( iPostDataW != iDataW ) || ( iPostNameW != iNameW ) )
	{
		InvalidateLayout( true );
		m_bLayoutOnUpdate = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSecondaryTargetID::CSecondaryTargetID( const char *pElementName ) : CTargetID( pElementName )
{
	m_wszPrepend[0] = '\0';

	RegisterForRenderGroup( "mid" );

	m_bWasHidingLowerElements = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSecondaryTargetID::ShouldDraw( void )
{
	bool bDraw = BaseClass::ShouldDraw();

	if ( bDraw )
	{
		if ( !m_bWasHidingLowerElements )
		{
			HideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = true;
		}
	}
	else 
	{
		if ( m_bWasHidingLowerElements )
		{
			UnhideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = false;
		}
	}

	return bDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSecondaryTargetID::CalculateTargetIndex( C_TDCPlayer *pLocalTFPlayer )
{
	return 0;
}

// Separately declared versions of the hud element for alive and dead so they
// can have different positions

bool CMainTargetID::ShouldDraw( void )
{
	C_TDCPlayer *pLocalTFPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() > OBS_MODE_NONE )
		return false;

	return BaseClass::ShouldDraw();
}

void CSpectatorTargetID::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_iOriginalY = GetYPos();
}

bool CSpectatorTargetID::ShouldDraw( void )
{
	C_TDCPlayer *pLocalTFPlayer = C_TDCPlayer::GetLocalTDCPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() <= OBS_MODE_FREEZECAM )
		return false;

	return BaseClass::ShouldDraw();
}

int	CSpectatorTargetID::CalculateTargetIndex( C_TDCPlayer *pLocalTFPlayer ) 
{ 
	return BaseClass::CalculateTargetIndex( pLocalTFPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Gets custom color of Target ID entity.
//-----------------------------------------------------------------------------
class CTargetIDTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		C_TDCPlayer *pPlayer = C_TDCPlayer::GetLocalTDCPlayer();

		if ( pPlayer )
		{
			C_TDCPlayer *pTarget = ToTDCPlayer( UTIL_PlayerByIndex( pPlayer->GetIDTarget() ) );

			if ( pTarget )
			{
				m_pResult->SetVecValue( pTarget->m_vecPlayerColor.Base(), 3 );
				return;
			}
		}
	}
};

EXPOSE_INTERFACE( CTargetIDTintColor, IMaterialProxy, "TargetIDTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );
