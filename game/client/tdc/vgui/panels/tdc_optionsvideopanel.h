//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONS_SUB_VIDEO_H
#define OPTIONS_SUB_VIDEO_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_dialogpanelbase.h"

class CTDCCvarSlider;
class CPanelListPanel;

struct AAMode_t
{
	int m_nNumSamples;
	int m_nQualityLevel;
};

//-----------------------------------------------------------------------------
// Purpose: Video Details, Part of OptionsDialog
//-----------------------------------------------------------------------------
class CTDCOptionsVideoPanel : public CTDCDialogPanelBase
{
	DECLARE_CLASS_SIMPLE(CTDCOptionsVideoPanel, CTDCDialogPanelBase);

public:
	CTDCOptionsVideoPanel(vgui::Panel* parent, const char *panelName);
	~CTDCOptionsVideoPanel();
	virtual void OnResetData();
	virtual void OnApplyChanges();
	virtual void OnCommand(const char *command);
	virtual bool RequiresRestart();
protected:
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void CreateControls();
	void DestroyControls();

private:
    void  SetCurrentResolutionComboItem();
    MESSAGE_FUNC( OnControlModified, "ControlModified" );
	MESSAGE_FUNC_PTR_CHARPTR( OnTextChanged, "TextChanged", panel, text );
   
	void		PrepareResolutionList();

	int m_nSelectedMode; // -1 if we are running in a nonstandard mode

	vgui::ComboBox		*m_pMode;
	vgui::ComboBox		*m_pWindowed;
	vgui::ComboBox		*m_pAspectRatio;
	CTDCCvarSlider		*m_pGammaSlider;
	vgui::Label			*m_pGammaLabel;

	bool m_bRequireRestart;

   //Adv video:
   bool m_bUseChanges;
   vgui::ComboBox *m_pModelDetail, *m_pTextureDetail, *m_pAntialiasingMode, *m_pFilteringMode;
   vgui::ComboBox *m_pShadowDetail, *m_pHDR, *m_pWaterDetail, *m_pVSync, *m_pShaderDetail;
   vgui::ComboBox *m_pColorCorrection;
   vgui::ComboBox *m_pMotionBlur;
   vgui::ComboBox *m_pDXLevel;
   CTDCCvarSlider	*m_pFOVSlider;
   vgui::ComboBox *m_pQueuedMode;

   void MarkDefaultSettingsAsRecommended();
   void SetComboItemAsRecommended(vgui::ComboBox *combo, int iItem);
   int FindMSAAMode(int nAASamples, int nAAQuality);
   void ResetDXLevelCombo();
   void ApplyChangesToConVar(const char *pConVarName, int value);

   int m_nNumAAModes;
   AAMode_t m_nAAModes[16];
};



#endif // OPTIONS_SUB_VIDEO_H