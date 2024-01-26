//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_HUD_FLAGSTATUS_H
#define TDC_HUD_FLAGSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tdc_controls.h"
#include "tdc_imagepanel.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose:  Draws the rotated arrow panels
//-----------------------------------------------------------------------------
class CTDCArrowPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCArrowPanel, vgui::Panel );

	CTDCArrowPanel( vgui::Panel *parent, const char *name );
	virtual void Paint();
	virtual bool IsVisible( void );
	void SetEntity( EHANDLE hEntity ){ m_hEntity = hEntity; }
	float GetAngleRotation( void );

private:

	EHANDLE				m_hEntity;	

	CMaterialReference	m_RedMaterial;
	CMaterialReference	m_BlueMaterial;
	CMaterialReference	m_NeutralMaterial;

	CMaterialReference	m_RedMaterialNoArrow;
	CMaterialReference	m_BlueMaterialNoArrow;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCFlagStatus : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCFlagStatus, vgui::EditablePanel );

	CTDCFlagStatus( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	void UpdateStatus( void );

	void SetEntity( EHANDLE hEntity )
	{ 
		m_hEntity = hEntity;

		if ( m_pArrow )
		{
			m_pArrow->SetEntity( hEntity );
		}
	}

private:

	EHANDLE			m_hEntity;

	CTDCArrowPanel	*m_pArrow;
	CTDCImagePanel	*m_pStatusIcon;
	CTDCImagePanel	*m_pBriefcase;
};


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudFlagObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTDCHudFlagObjectives, vgui::EditablePanel );

public:

	CTDCHudFlagObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void Reset();
	void OnTick();

public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:
	
	void UpdateStatus( void );
	void SetPlayingToLabelVisible( bool bVisible );

private:

	vgui::ImagePanel		*m_pCarriedImage;

	CExLabel				*m_pPlayingTo;
	CTDCImagePanel			*m_pPlayingToBG;

	CTDCFlagStatus			*m_pRedFlag;
	CTDCFlagStatus			*m_pBlueFlag;
	CTDCArrowPanel			*m_pCapturePoint;

	bool					m_bFlagAnimationPlayed;
	bool					m_bCarryingFlag;
	bool					m_bShowPlayingTo;
	int						m_iNumFlags;

	vgui::ImagePanel		*m_pSpecCarriedImage;
};

#endif	// TDC_HUD_FLAGSTATUS_H
