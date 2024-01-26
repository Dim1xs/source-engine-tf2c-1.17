//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef TDC_PD_ENTITIES_H
#define TDC_PD_ENTITIES_H

#include "tdc_pickupitem.h"
#include "triggers.h"

class CSpriteTrail;

class CMoneyPack : public CTDCPickupItem
{
public:
	DECLARE_CLASS( CMoneyPack, CTDCPickupItem );

	static CMoneyPack *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner );

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
};

class CMoneyDeliveryZone : public CBaseTrigger, public TAutoList<CMoneyDeliveryZone>
{
public:
	DECLARE_CLASS( CMoneyDeliveryZone, CBaseTrigger );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CMoneyDeliveryZone();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual int UpdateTransmitState( void );
	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );
	virtual void StopLoopingSounds( void );

	virtual void InputEnable( inputdata_t &inputdata );
	virtual void InputDisable( inputdata_t &inputdata );

	int GetZoneIndex( void ) { return m_iZoneIndex; }
	void CaptureThink( void );
	void ActivateDeliveryZone( bool bSilent );
	void DeactivateDeliveryZone( bool bSilent );
	void RecalculateTeamInZone( void );
	void PlayControlVoice( int iTeam );

private:
	int m_iZoneIndex;
	Vector m_vecIconInitialOrigin;
	CNetworkVector( m_vecIconOrigin );

	float m_flNextControlSpeak;
	int m_iNumPlayers[TDC_TEAM_COUNT];
	int m_iTeamInZone;

	COutputEvent m_OnActive;
	COutputEvent m_OnInactive;
};

class CTDCBloodMoneyZoneManager : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTDCBloodMoneyZoneManager, CLogicalEntity );
	DECLARE_DATADESC();

	CTDCBloodMoneyZoneManager();

	virtual void Activate( void );
	virtual void UpdateOnRemove( void );

	void StateThink( void );

private:
	float m_flEnableInterval;
	float m_flEnableDuration;
	bool m_bRandom;

	CUtlMap<int, CHandle<CMoneyDeliveryZone>> m_Zones;
	CHandle<CMoneyDeliveryZone> m_hCurrentZone;
	int m_iLastZoneIndex;
	bool m_bActive;
	float m_flStateChangeTime;
	bool m_bFire15SecRemain;

	COutputEvent m_OnActive;
	COutputEvent m_OnInactive;
	COutputEvent m_On15SecBeforeActive;
};

#endif // TDC_PD_ENTITIES_H
