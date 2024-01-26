//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TDC_HUD_TARGET_ID_H
#define TDC_HUD_TARGET_ID_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include "tdc_imagepanel.h"
#include "tdc_hud_spectator.h"
#include "c_tdc_player.h"
#include "vgui_avatarimage.h"

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)
#define MAX_ID_STRING			256
#define MAX_PREPEND_STRING		32

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetID : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTargetID, vgui::EditablePanel );
public:
	CTargetID( const char *pElementName );
	void			Reset( void );
	void			VidInit( void );
	virtual bool	ShouldDraw( void );
	virtual void	PerformLayout( void );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );

	void			UpdateID( void );

	virtual int		CalculateTargetIndex( C_TDCPlayer *pLocalTFPlayer );
	virtual wchar_t	*GetPrepend( void ) { return NULL; }

	int				GetTargetIndex( void ) { return m_iTargetEntIndex; }

	virtual int		GetRenderGroupPriority( void );

protected:
	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	float			m_flLastChangeTime;
	int				m_iTargetEntIndex;
	bool			m_bLayoutOnUpdate;

	vgui::Label				*m_pTargetNameLabel;
	vgui::Label				*m_pTargetDataLabel;
	CAvatarImagePanel		*m_pAvatar;
	CTDCImagePanel			*m_pBGPanel;
	CTDCSpectatorGUIHealth	*m_pTargetHealth;
	vgui::ImagePanel		*m_pAmmoIcon;
	vgui::EditablePanel		*m_pMoveableSubPanel;

	int m_iRenderPriority;
};

class CMainTargetID : public CTargetID
{
	DECLARE_CLASS_SIMPLE( CMainTargetID, CTargetID );
public:
	CMainTargetID( const char *pElementName ) : CTargetID( pElementName ) {}

	virtual bool ShouldDraw( void );
};

class CSpectatorTargetID : public CTargetID
{
	DECLARE_CLASS_SIMPLE( CSpectatorTargetID, CTargetID );
public:
	CSpectatorTargetID( const char *pElementName ) : CTargetID( pElementName ) { m_iOriginalY = 0; }

	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool ShouldDraw( void );
	virtual int	CalculateTargetIndex( C_TDCPlayer *pLocalTFPlayer );

private:
	CPanelAnimationVarAliasType( int, m_iXOffset, "x_offset", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iYOffset, "y_offset", "0", "proportional_int" );
	int m_iOriginalY;
};

//-----------------------------------------------------------------------------
// Purpose: Second target ID that's used for displaying a second target below the primary
//-----------------------------------------------------------------------------
class CSecondaryTargetID : public CTargetID
{
	DECLARE_CLASS_SIMPLE( CSecondaryTargetID, CTargetID );
public:
	CSecondaryTargetID( const char *pElementName );

	virtual bool	ShouldDraw( void );
	virtual int		CalculateTargetIndex( C_TDCPlayer *pLocalTFPlayer );
	virtual wchar_t	*GetPrepend( void ) { return m_wszPrepend; }

private:
	wchar_t		m_wszPrepend[ MAX_PREPEND_STRING ];

	bool m_bWasHidingLowerElements;
};

#endif // TDC_HUD_TARGET_ID_H
