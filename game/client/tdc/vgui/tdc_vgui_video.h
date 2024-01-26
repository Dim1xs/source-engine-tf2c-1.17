//====== Copyright © 1996-2007, Valve Corporation, All rights reserved. =======
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#ifndef TDC_VGUI_VIDEO_H
#define TDC_VGUI_VIDEO_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_video.h"

class CTDCVideoPanel : public VideoPanel
{
	DECLARE_CLASS_SIMPLE( CTDCVideoPanel, VideoPanel );
public:

	CTDCVideoPanel( vgui::Panel *parent, const char *panelName );
	~CTDCVideoPanel();

	virtual void OnClose();
	virtual void OnKeyCodePressed( vgui::KeyCode code ){}
	virtual void RequestFocus( int direction = 0 ) {}
	virtual void ApplySettings( KeyValues *inResourceData );
	
	virtual void GetPanelPos( int &xpos, int &ypos );
	virtual void Shutdown();

	float GetStartDelay(){ return m_flStartAnimDelay; }
	float GetEndDelay(){ return m_flEndAnimDelay; }

	bool BeginPlaybackNoAudio( const char *pFilename );

protected:
	virtual void ReleaseVideo();
	virtual void OnVideoOver();

private:
	float m_flStartAnimDelay;
	float m_flEndAnimDelay;
};

#endif // TDC_VGUI_VIDEO_H
