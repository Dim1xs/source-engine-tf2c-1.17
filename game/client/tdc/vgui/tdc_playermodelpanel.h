//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TDC_PLAYERMODELPANEL_H
#define TDC_PLAYERMODELPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "basemodel_panel.h"
#include "ichoreoeventcallback.h"
#include "choreoscene.h"
#include "tdc_shareddefs.h"
#include "tdc_merc_customizations.h"

class CTDCPlayerModelPanel : public CBaseModelPanel, public CDefaultClientRenderable, public IChoreoEventCallback, public IHasLocalToGlobalFlexSettings
{
public:
	DECLARE_CLASS_SIMPLE( CTDCPlayerModelPanel, CBaseModelPanel );

	CTDCPlayerModelPanel( vgui::Panel *pParent, const char *pName );
	~CTDCPlayerModelPanel();

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void RequestFocus( int direction = 0 ) {}

	void LockStudioHdr();
	void UnlockStudioHdr();
	CStudioHdr *GetModelPtr( void ) { return m_pStudioHdr; }
	int LookupSequence( const char *pszName );
	float GetSequenceFrameRate( int nSequence );

	virtual void PrePaint3D( IMatRenderContext *pRenderContext ) OVERRIDE;
	virtual void PostPaint3D( IMatRenderContext *pRenderContext ) OVERRIDE;
	virtual void SetupFlexWeights( void );
	virtual void FireEvent( const char *pszEventName, const char *pszEventOptions );

	void SetToPlayerClass( int iClass );
	void SetTeam( int iTeam );
	void LoadItems( int iSlot = -1 );
	void LoadWearablesFromPlayer( int iPlayerIndex );
	ETDCWeaponID GetItemInSlot( int iSlot );
	void HoldFirstValidItem( void );
	bool HoldItemInSlot( int iSlot );
	void AddCarriedItem( ETDCWeaponID iWeaponID );
	void AddWearable( int iItemID );
	void ClearCarriedItems( void );
	void UpdateBodygroups( void );

	void InitPhonemeMappings( void );
	void SetFlexWeight( LocalFlexController_t index, float value );
	float GetFlexWeight( LocalFlexController_t index );
	void ResetFlexWeights( void );
	int FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key );
	LocalFlexController_t FindFlexController( const char *szName );

	void ProcessVisemes( Emphasized_Phoneme *classes );
	void AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted );
	void AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression );
	bool SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme );
	void ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity );
	
	// IHasLocalToGlobalFlexSettings
	virtual void EnsureTranslations( const flexsettinghdr_t *pSettinghdr );

	CChoreoScene *LoadScene( const char *filename );
	void PlayVCD( const char *pszFile );
	void StopVCD();
	void ProcessLoop( CChoreoScene *scene, CChoreoEvent *event );
	void ProcessSequence( CChoreoScene *scene, CChoreoEvent *event );
	void ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event );
	void ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event );
	void AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr );

	// IChoreoEventCallback
	virtual void StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	// IClientRenderable
	virtual const Vector&			GetRenderOrigin( void ) { return vec3_origin; }
	virtual const QAngle&			GetRenderAngles( void ) { return vec3_angle; }
	virtual void					GetRenderBounds( Vector& mins, Vector& maxs )
	{
		GetBoundingBox( mins, maxs );
	}
	virtual const matrix3x4_t &		RenderableToWorldTransform()
	{
		static matrix3x4_t mat;
		SetIdentityMatrix( mat );
		return mat;
	}
	virtual bool					ShouldDraw( void ) { return false; }
	virtual bool					IsTransparent( void ) { return false; }
	virtual bool					ShouldReceiveProjectedTextures( int flags ) { return false; }

	const Vector &GetModelTintColor( void );
	void SetModelTintColor( const Vector &vecColor );
	void UseCvarsForTintColor( bool bUseCvars ) { m_bUseMercCvars = bUseCvars; }
	void EmitSpawnEffect( const char *pszName );
	const Vector &GetModelSkinToneColor ( void );
	void SetModelSkinToneColor( const Vector &vecColor );
private:
	CStudioHdr *m_pStudioHdr;
	CThreadFastMutex m_StudioHdrInitLock;

	int m_iTeamNum;
	int m_iClass;
	bool m_bCustomClassData[TDC_CLASS_COUNT_ALL];
	BMPResData_t m_ClassResData[TDC_CLASS_COUNT_ALL];
	CUtlVector<ETDCWeaponID> m_Weapons;
	WearableDef_t *m_Wearables[TDC_WEARABLE_COUNT];

	int m_aMergeMDLMap[TDC_PLAYER_WEAPON_COUNT];
	int m_iActiveWpnMDLIndex;
	int m_iTauntMDLIndex;

	Vector m_vecModelTintColor;
	bool m_bUseMercCvars;
	particle_data_t *m_pSpawnEffectData;
	Vector m_vecModelSkinToneColor;

	CChoreoScene *m_pScene;
	float m_flCurrentTime;
	bool m_bFlexEvents;

	CUtlRBTree<FS_LocalToGlobal_t, unsigned short> m_LocalToGlobal;
	Emphasized_Phoneme m_PhonemeClasses[NUM_PHONEME_CLASSES];
	float m_flexWeight[MAXSTUDIOFLEXCTRL];
};

#endif // TDC_PLAYERMODELPANEL_H
