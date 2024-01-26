//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TDC_CREATESERVERDIALOG_H
#define TDC_CREATESERVERDIALOG_H

#ifdef _WIN32
#pragma once
#endif

class CDescription;
class CTDCCreateServerMapPanel;
class CTDCCreateServerGamePanel;
class mpcontrol_t;

#include "tdc_dialogpanelbase.h"

class CTDCCreateServerDialog : public CTDCDialogPanelBase
{
public:
	DECLARE_CLASS_SIMPLE( CTDCCreateServerDialog, CTDCDialogPanelBase );

	CTDCCreateServerDialog( vgui::Panel *pParent, const char *pszName );
	~CTDCCreateServerDialog();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Show( void );
	virtual void Hide( void );
	virtual void CreateControls( void );
	virtual void DestroyControls( void );
	virtual void OnApplyChanges( void );

	// returns currently entered information about the server
	void SetMap( const char *pszName );
	bool IsRandomMapSelected( void );
	const char *GetMapName( void );

	// returns currently entered information about the server
	int GetMaxPlayers( void );
	const char *GetPassword( void );
	const char *GetHostName( void );
	const char *GetValue( const char *cvarName, const char *defaultValue );

	void LoadGameOptionsList();
	void GatherCurrentValues();

private:
	void LoadMaps( const char *pszPathID );
	void LoadMapList( void );

	bool m_bBotsEnabled;

	// for loading/saving game config
	KeyValues *m_pSavedData;

	vgui::ComboBox *m_pMapList;

	CDescription *m_pDescription;
	mpcontrol_t *m_pList;
};

#endif // TDC_CREATESERVERDIALOG_H
