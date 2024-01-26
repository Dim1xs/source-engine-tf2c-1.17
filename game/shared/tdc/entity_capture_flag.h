//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTDC Flag.
//
//=============================================================================//
#ifndef ENTITY_CAPTURE_FLAG_H
#define ENTITY_CAPTURE_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tdc_item.h"
#include "SpriteTrail.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
#define CCaptureFlag C_CaptureFlag
#endif

//=============================================================================
//
// CTDC Flag defines.
//

#define TDC_CTF_FLAGSPAWN			"CaptureFlag.FlagSpawn"

#define TDC_CTF_CAPTURED_TEAM_FRAGS	1

#define TDC_CTF_RESET_TIME			60.0f

//=============================================================================
//
// Attack/Defend Flag defines.
//

#define TDC_AD_CAPTURED_SOUND		"AttackDefend.Captured"

#define TDC_AD_CAPTURED_FRAGS		30
#define TDC_AD_RESET_TIME			60.0f

//=============================================================================
//
// Invade Flag defines.
//

#define TDC_INVADE_CAPTURED_FRAGS		10
#define TDC_INVADE_CAPTURED_TEAM_FRAGS	1

#define TDC_INVADE_RESET_TIME			60.0f
#define TDC_INVADE_NEUTRAL_TIME			30.0f

//=============================================================================
//
// Special Delivery defines.
//

#define TDC_SD_FLAGSPAWN				"Resource.FlagSpawn"

#ifdef CLIENT_DLL
	#define CCaptureFlagReturnIcon C_CaptureFlagReturnIcon
	#define CBaseAnimating C_BaseAnimating
#endif

#ifdef CLIENT_DLL

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} progress_segment_t;

extern progress_segment_t Segments[8];

#endif

class CCaptureFlagReturnIcon: public CBaseAnimating
{
public:
	DECLARE_CLASS( CCaptureFlagReturnIcon, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CCaptureFlagReturnIcon();

#ifdef CLIENT_DLL

	virtual int		DrawModel( int flags );
	void			DrawReturnProgressBar( void );

	virtual RenderGroup_t GetRenderGroup( void );
	virtual bool	ShouldDraw( void ) { return true; }

	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );

private:

	IMaterial	*m_pReturnProgressMaterial_Empty;		// For labels above players' heads.
	IMaterial	*m_pReturnProgressMaterial_Full;

#else
public:
	virtual void Spawn( void );
	virtual int UpdateTransmitState( void );

#endif

};

//=============================================================================
//
// CTDC Flag class.
//
class CCaptureFlag : public CTDCItem, public TAutoList<CCaptureFlag>, public CGameEventListener
{
public:

	DECLARE_CLASS( CCaptureFlag, CTDCItem );
	DECLARE_NETWORKCLASS();

	CCaptureFlag();
	~CCaptureFlag();

	enum
	{
		TDC_FLAGEFFECTS_NONE = 0,
		TDC_FLAGEFFECTS_ALL,
		TDC_FLAGEFFECTS_PAPERTRAIL_ONLY,
		TDC_FLAGEFFECTS_COLORTRAIL_ONLY,
	};

	unsigned int	GetItemID( void ) { return TDC_ITEM_CAPTURE_FLAG; }

	CBaseEntity		*GetPrevOwner( void ) { return m_hPrevOwner.Get(); }

	bool			IsDropped( void ) { return ( m_nFlagStatus == TDC_FLAGINFO_DROPPED ); }
	bool			IsHome( void ) { return ( m_nFlagStatus == TDC_FLAGINFO_NONE ); }
	bool			IsStolen( void ) { return ( m_nFlagStatus == TDC_FLAGINFO_STOLEN ); }
	bool			IsDisabled( void ) { return m_bDisabled; }

	bool			IsVisibleWhenDisabled( void ) { return m_bVisibleWhenDisabled; }
	const char		*GetViewModel( void ) { return m_szViewModel.Get(); }

	virtual void	FireGameEvent( IGameEvent *event );

// Game DLL Functions
#ifdef GAME_DLL

	virtual bool	KeyValue( const char *szKeyName, const char *szValue );
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );

	void			Reset( void );
	void			ResetMessage( void );

	bool			CanTouchThisFlagType( const CBaseEntity *pOther );
	void			FlagTouch( CBaseEntity *pOther );

	void			Capture( CTDCPlayer *pPlayer, int nCapturePoint );
	virtual void	PickUp( CTDCPlayer *pPlayer, bool bInvisible );
	virtual void	Drop( CTDCPlayer *pPlayer, bool bVisible, bool bThrown = false, bool bMessage = true );

	void			SetDisabled( bool bDisabled );

	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputRoundActivate( inputdata_t &inputdata );
	void			InputForceDrop( inputdata_t &inputdata );
	void			InputForceReset( inputdata_t &inputdata );
	void			InputForceResetSilent( inputdata_t &inputdata );
	void			InputForceResetAndDisableSilent( inputdata_t &inputdata );
	void			InputSetReturnTime( inputdata_t &inputdata );
	void			InputShowTimer( inputdata_t &inputdata );
	void			InputForceGlowDisabled( inputdata_t &inputdata );
	void			InputSetLocked( inputdata_t &inputdata );
	void			InputSetUnlockTime( inputdata_t &inputdata );

	void			Think( void );
	
	void			SetFlagStatus( int iStatus );
	int				GetFlagStatus( void ) { return m_nFlagStatus; };
	void			ResetFlagReturnTime( void ) { m_flResetTime = 0; }
	void			SetFlagReturnIn( float flTime )
	{
		m_flResetTime = gpGlobals->curtime + flTime;
	}

	void			ResetFlagNeutralTime( void ) { m_flNeutralTime = 0; }
	void			SetFlagNeutralIn( float flTime )
	{ 
		m_flNeutralTime = gpGlobals->curtime + flTime;
	}
	bool			IsCaptured( void ) { return m_bCaptured; }

	int				UpdateTransmitState( void );

	void			GetTrailEffect( int iTeamNum, char *pszBuf, int iBufSize );
	void			ManageSpriteTrail( void );

	void			UpdateReturnIcon( void );

	void			SetLocked( bool bLocked );

#else // CLIENT DLL Functions

	virtual const char	*GetIDString(void) { return "entity_capture_flag"; };

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual int		GetSkin( void );
	virtual void	Simulate( void );
	void			ManageTrailEffects( void );
	void			UpdateFlagVisibility( void );

	float			GetResetDelay() { return m_flResetDelay; }
	float			GetReturnProgress( void );

	void			UpdateGlowEffect( void );

	void			GetHudIcon( int iTeamNum, char *pszBuf, int buflen );

#endif

private:

	CNetworkVar( bool,			m_bDisabled );	// Enabled/Disabled?
	CNetworkVar( int, m_nFlagStatus );

	CNetworkVar( float, m_flResetDelay );
	CNetworkVar( float,	m_flResetTime );		// Time until the flag is placed back at spawn.
	CNetworkVar( float, m_flNeutralTime );	// Time until the flag becomes neutral (used for the invade gametype)
	CNetworkHandle( CBaseEntity, m_hPrevOwner );

	CNetworkVar( int, m_nUseTrailEffect );
	CNetworkString( m_szHudIcon, MAX_PATH );
	CNetworkString( m_szPaperEffect, MAX_PATH );
	CNetworkVar( bool, m_bVisibleWhenDisabled );
	CNetworkVar( bool, m_bGlowEnabled );

	CNetworkVar( bool, m_bLocked );
	CNetworkVar( float, m_flUnlockTime );
	CNetworkVar( float, m_flUnlockDelay );
	CNetworkString( m_szViewModel, MAX_PATH );

	int				m_iOriginalTeam;
	float			m_flOwnerPickupTime;

	EHANDLE		m_hReturnIcon;

#ifdef GAME_DLL
	DECLARE_DATADESC();

	Vector			m_vecResetPos;		// The position the flag should respawn (reset) at.
	QAngle			m_vecResetAng;		// The angle the flag should respawn (reset) at.

	COutputEvent	m_outputOnReturn;	// Fired when the flag is returned via timer.
	COutputEvent	m_outputOnPickUp;	// Fired when the flag is picked up.
	COutputEvent	m_outputOnPickUpTeam1;	
	COutputEvent	m_outputOnPickUpTeam2;
	COutputEvent	m_outputOnDrop;		// Fired when the flag is dropped.
	COutputEvent	m_outputOnCapture;	// Fired when the flag is captured.
	COutputEvent	m_outputOnCapTeam1;
	COutputEvent	m_outputOnCapTeam2;
	COutputEvent	m_outputOnTouchSameTeam;

	bool			m_bAllowOwnerPickup;
	bool			m_bCaptured;
	string_t		m_szModel;
	string_t		m_szTrailEffect;
	CHandle<CSpriteTrail>	m_hGlowTrail;
#else
	IMaterial	*m_pReturnProgressMaterial_Empty;		// For labels above players' heads.
	IMaterial	*m_pReturnProgressMaterial_Full;		

	int			m_iOldTeam;
	int			m_nOldFlagStatus;
	bool		m_bWasGlowEnabled;

	CNewParticleEffect	*m_pPaperTrailEffect;
	CGlowObject	*m_pGlowEffect;
#endif
};

#endif // ENTITY_CAPTURE_FLAG_H