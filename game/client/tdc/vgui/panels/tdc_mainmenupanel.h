#ifndef TFMAINMENUPANEL_H
#define TFMAINMENUPANEL_H

#include "tdc_menupanelbase.h"
#include "steam/steam_api.h"

class CAvatarImagePanel;
class CTDCButton;
class CTDCSlider;

enum MusicStatus
{
	MUSIC_STOP,
	MUSIC_FIND,
	MUSIC_PLAY,
	MUSIC_STOP_FIND,
	MUSIC_STOP_PLAY,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTDCMainMenuPanel : public CTDCMenuPanelBase
{
	DECLARE_CLASS_SIMPLE( CTDCMainMenuPanel, CTDCMenuPanelBase );

public:
	CTDCMainMenuPanel( vgui::Panel* parent, const char *panelName );
	virtual ~CTDCMainMenuPanel();

	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick();
	virtual void OnThink();
	virtual void Show();
	virtual void Hide();
	virtual void OnCommand( const char* command );

	void HideFakeIntro( void );

private:
	CAvatarImagePanel	*m_pProfileAvatar;
	vgui::ImagePanel	*m_pFakeBGImage;

	int					m_iShowFakeIntro;

	int					m_nSongGuid;
	MusicStatus			m_psMusicStatus;

	CSteamID			m_SteamID;
};

#endif // TFMAINMENUPANEL_H
