#ifndef TFMAINMENUBACKGROUNDPANEL_H
#define TFMAINMENUBACKGROUNDPANEL_H

#include "vgui_controls/Panel.h"
#include "tdc_menupanelbase.h"
#include "tdc_vgui_video.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCBackgroundPanel : public CTDCMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCBackgroundPanel, CTDCMenuPanelBase );

public:
	CTDCBackgroundPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTDCBackgroundPanel();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void SetVisible( bool bVisible );

	MESSAGE_FUNC( VideoReplay, "IntroFinished" );

private:
	CTDCVideoPanel		*m_pVideo;
	char				m_szVideoFile[MAX_PATH];
	void				GetRandomVideo( char *pszBuf, int iBufLength, bool bWidescreen );
};

#endif // TFMAINMENUBACKGROUNDPANEL_H
