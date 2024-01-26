//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TDC_HUD_BETASTAMP_H
#define TDC_HUD_BETASTAMP_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tdc_controls.h"
#include "tdc_imagepanel.h"
#include "GameEventListener.h"


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTDCHudBetaStamp : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTDCHudBetaStamp, vgui::EditablePanel );

public:

	CTDCHudBetaStamp( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual bool ShouldDraw( void );
	void OnThink( void );

private:
	vgui::ImagePanel *m_pBetaImage;

	bool			m_bGlowing;
	bool			m_bAnimationIn;
	float			m_flAnimationThink;
};

#endif	// TDC_HUD_BETASTAMP_H
