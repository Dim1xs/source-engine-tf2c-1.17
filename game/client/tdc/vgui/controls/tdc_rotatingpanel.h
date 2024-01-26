#ifndef TDC_ROTATINGPANEL_H
#define TDC_ROTATINGPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"

//-----------------------------------------------------------------------------
// Purpose:  Draws the rotated arrow panels
//-----------------------------------------------------------------------------
class CTDCRotatingImagePanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTDCRotatingImagePanel, vgui::EditablePanel );

	CTDCRotatingImagePanel( vgui::Panel *parent, const char *name );
	virtual void Paint();
	virtual void ApplySettings( KeyValues *inResourceData );
	float GetAngleRotation( void );

private:
	float				m_flAngle;
	CMaterialReference	m_Material;
};

#endif // TDC_ROTATINGPANEL_H
