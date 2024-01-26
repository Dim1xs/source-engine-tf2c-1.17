//=============================================================================
//
// Purpose:
//
//=============================================================================
#ifndef TDC_HUD_BLOODMONEY_H
#define TDC_HUD_BLOODMONEY_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ScalableImagePanel.h>
#include "vgui_avatarimage.h"
#include "tdc_controls.h"

class CBloodMoneyProgressBar;
class CHudBloodMoneyRadar;
class CTDCProgressBar;

class CHudBloodMoney : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudBloodMoney, vgui::EditablePanel );

	CHudBloodMoney( vgui::Panel *pParent, const char *pszName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void LevelInit( void );
	virtual void Reset( void );
	virtual void OnTick( void );
	virtual void OnThink( void );

private:
	void SetPlayingToLabelVisible( bool bVisible );

	vgui::EditablePanel *m_pLocalPlayerPanel;
	vgui::EditablePanel *m_pBestPlayerPanel;
	CAvatarImagePanel *m_pPlayerAvatar;
	CAvatarImagePanel *m_pRivalAvatar;
	CBloodMoneyProgressBar *m_pLocalProgress;
	CBloodMoneyProgressBar *m_pRivalProgress;
	CBloodMoneyProgressBar *m_pLocalProgressCarried;
	CBloodMoneyProgressBar *m_pRivalProgressCarried;
	CTDCProgressBar *m_pZoneProgressBar;
	CHudBloodMoneyRadar *m_pRadar;

	CExLabel *m_pPlayingTo;
	vgui::ScalableImagePanel *m_pPlayingToBG;

	int m_iRivalPlayer;

	enum
	{
		DM_STATUS_NONE,
		DM_STATUS_LEADTAKEN,
		DM_STATUS_LEADLOST,
		DM_STATUS_LEADTIED,
	};

	int m_iLeadStatus;
};

class CHudTeamBloodMoney : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudTeamBloodMoney, vgui::EditablePanel );

	CHudTeamBloodMoney( vgui::Panel *pParent, const char *pszName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void LevelInit( void );
	virtual void OnTick( void );
	virtual void OnThink( void );

private:
	void SetPlayingToLabelVisible( bool bVisible );
	vgui::EditablePanel *GetTeamPanel( int iTeam );
	CBloodMoneyProgressBar *GetProgressBar( int iTeam, bool bCarried );

	vgui::EditablePanel *m_pRedPanel;
	vgui::EditablePanel *m_pBluePanel;
	CBloodMoneyProgressBar *m_pRedProgress;
	CBloodMoneyProgressBar *m_pBlueProgress;
	CBloodMoneyProgressBar *m_pRedProgressCarried;
	CBloodMoneyProgressBar *m_pBlueProgressCarried;
	CTDCProgressBar *m_pZoneProgressBar;
	CHudBloodMoneyRadar *m_pRadar;

	CExLabel *m_pPlayingTo;
	vgui::ScalableImagePanel *m_pPlayingToBG;

	enum
	{
		DM_STATUS_NONE,
		DM_STATUS_LEADTAKEN,
		DM_STATUS_LEADLOST,
		DM_STATUS_LEADTIED,
	};

	int m_iLeadStatus;
};

class CBloodMoneyProgressBar : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CBloodMoneyProgressBar, vgui::Panel );

	CBloodMoneyProgressBar( vgui::Panel *pParent, const char *pszName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void Paint( void );

	void SetProgress( float flProgress ) { m_flProgress = clamp( flProgress, 0.0f, 1.0f ); }
	void SetDrawColor( Color col ) { m_DrawColor = col; }

private:
	bool m_bLeftToRight;
	float m_flProgress;
	Color m_DrawColor;
	int m_iTextureId;
};

class CHudBloodMoneyRadar : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudBloodMoneyRadar, vgui::EditablePanel );

	CHudBloodMoneyRadar( vgui::Panel *pParent, const char *pszName );
	~CHudBloodMoneyRadar();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void PostChildPaint( void );

private:
	void GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );

	vgui::ImagePanel *m_pZoneIcon;
	CMaterialReference m_ArrowMaterial;
	int m_iRadarAlpha;

	float m_flRotation;
	bool m_bOnScreen;
};

#endif // TDC_HUD_BLOODMONEY_H
