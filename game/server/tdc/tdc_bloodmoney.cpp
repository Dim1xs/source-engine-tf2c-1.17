//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tdc_bloodmoney.h"
#include "tdc_player.h"
#include "tdc_gamerules.h"
#include "tdc_team.h"
#include "tdc_gamestats.h"
#include "SpriteTrail.h"
#include "tdc_objective_resource.h"
#include "tdc_announcer.h"

LINK_ENTITY_TO_CLASS( item_moneypack, CMoneyPack );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CMoneyPack *CMoneyPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CMoneyPack *pEntity = static_cast<CMoneyPack *>( CBaseEntity::CreateNoSpawn( "item_moneypack", vecOrigin, vecAngles, pOwner ) );

	if ( pEntity && pOwner )
	{
		pEntity->ChangeTeam( pOwner->GetTeamNumber() );

		Vector vecVelocity;
		QAngle angDir;
		angDir[PITCH] = RandomFloat( -60.0f, -90.0f );
		angDir[YAW] = RandomFloat( -180.0f, 180.0f );
		AngleVectors( angDir, &vecVelocity );
		vecVelocity *= 250.0f;

		pEntity->DropSingleInstance( vecVelocity, pOwner->MyCombatCharacterPointer(), 0.0f, 0.1f, 15.0f );
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyPack::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/items/pd_moneypack.mdl" );
	PrecacheScriptSound( "MoneyPack.Touch" );

	ForEachTeamName( [=]( const char *pszTeam )
	{
		PrecacheModel( UTIL_VarArgs( "effects/arrowtrail_%s.vmt", pszTeam ) );
	}, true, g_aTeamNamesShort );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyPack::Spawn( void )
{
	Precache();
	SetModel( "models/items/pd_moneypack.mdl" );

	BaseClass::Spawn();

	SetCollisionBounds( Vector( -10, -10, -10 ), Vector( 10, 10, 10 ) );

	m_nSkin = GetTeamSkin( GetTeamNumber() );

	const char *pszEffect = ConstructTeamParticle( "effects/arrowtrail_%s.vmt", GetTeamNumber(), true, g_aTeamNamesShort );
	CSpriteTrail *pTrail = CSpriteTrail::SpriteTrailCreate( pszEffect, GetLocalOrigin(), false );

	pTrail->FollowEntity( this );

	CTDCPlayer *pPlayer = ToTDCPlayer( GetOwnerEntity() );

	if ( TDCGameRules()->IsTeamplay() )
	{
		pTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 255, kRenderFxNone );
	}
	else
	{
		Vector vecColor = pPlayer ? pPlayer->m_vecPlayerColor * 255.0f : vec3_origin;
		pTrail->SetTransparency( kRenderTransColor, vecColor.x, vecColor.y, vecColor.z, 255, kRenderFxNone );
	}

	pTrail->SetStartWidth( 10.0f );
	pTrail->SetTextureResolution( 0.01f );
	pTrail->SetLifeTime( 0.3f );
	pTrail->TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMoneyPack::MyTouch( CBasePlayer *pPlayer )
{
	CTDCPlayer *pTFPlayer = ToTDCPlayer( pPlayer );

	// If picked up by a teammate, just remove myself (pickup deny).
	if ( pTFPlayer->IsEnemy( this ) )
	{
		pTFPlayer->AddMoneyPack();

		CSingleUserRecipientFilter filter( pTFPlayer );
		EmitSound( filter, entindex(), "MoneyPack.Touch" );
	}

	return true;
}

IMPLEMENT_SERVERCLASS_ST( CMoneyDeliveryZone, DT_MoneyDeliveryZone )
	SendPropVector( SENDINFO( m_vecIconOrigin ), 32, SPROP_COORD_MP ),
END_SEND_TABLE()

BEGIN_DATADESC( CMoneyDeliveryZone )
	DEFINE_KEYFIELD( m_iZoneIndex, FIELD_INTEGER, "ZoneIndex" ),
	DEFINE_KEYFIELD( m_vecIconInitialOrigin, FIELD_VECTOR, "IconOrigin" ),

	DEFINE_OUTPUT( m_OnActive, "OnActive" ),
	DEFINE_OUTPUT( m_OnInactive, "OnInactive" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_moneydeliveryzone, CMoneyDeliveryZone );

CMoneyDeliveryZone::CMoneyDeliveryZone()
{
	m_iZoneIndex = 0;
	m_vecIconOrigin.Init();
	m_flNextControlSpeak = 0.0f;
	memset( m_iNumPlayers, 0, sizeof( m_iNumPlayers ) );
	m_iTeamInZone = TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "MoneyPack.Deliver" );
	PrecacheScriptSound( "BloodMoney.ZoneStart" );
	PrecacheScriptSound( "BloodMoney.ZoneActive" );
	PrecacheScriptSound( "BloodMoney.ZoneStop" );
	PrecacheScriptSound( "BloodMoney.ZoneBlocked" );

	UTIL_PrecacheOther( "item_moneypack" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
	InitTrigger();

	// Calc the local origin of the UI icon.
	WorldToEntitySpace( m_vecIconInitialOrigin, &m_vecIconOrigin.GetForModify() );
	
	DeactivateDeliveryZone( true );
}

//-----------------------------------------------------------------------------
// Purpose: The timer is always transmitted to clients
//-----------------------------------------------------------------------------
int CMoneyDeliveryZone::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CMoneyDeliveryZone::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CMoneyDeliveryZone::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( IsTouching( pOther ) )
	{
		int iOldTeam = m_iTeamInZone;
		RecalculateTeamInZone();

		if ( m_iTeamInZone != iOldTeam )
		{
			// Zone went from Idle to Active or from Active to Blocked.
			if ( m_iTeamInZone >= FIRST_GAME_TEAM )
			{
				// First player entered the zone, play the announcement.
				PlayControlVoice( m_iTeamInZone );

				EmitSound( "BloodMoney.ZoneStart" );
				EmitSound( "BloodMoney.ZoneActive" );
			}
			else if ( m_iTeamInZone == TEAM_SPECTATOR )
			{
				// TEAM_SPECTATOR means zone is blocked.
				StopSound( "BloodMoney.ZoneActive" );
				EmitSound( "BloodMoney.ZoneBlocked" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::EndTouch( CBaseEntity *pOther )
{
	bool bWasTouching = IsTouching( pOther );

	BaseClass::EndTouch( pOther );

	if ( bWasTouching )
	{
		int iOldTeam = m_iTeamInZone;
		RecalculateTeamInZone();

		if ( m_iTeamInZone != iOldTeam )
		{
			// Zone went from Blocked to Active or from Active to Idle.
			if ( m_iTeamInZone >= FIRST_GAME_TEAM )
			{
				// All teams but one left the zone.
				PlayControlVoice( m_iTeamInZone );

				StopSound( "BloodMoney.ZoneBlocked" );
				EmitSound( "BloodMoney.ZoneActive" );
			}
			else if ( m_iTeamInZone == TEAM_UNASSIGNED )
			{
				// All teams left the zone.
				StopSound( "BloodMoney.ZoneActive" );
				EmitSound( "BloodMoney.ZoneStop" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::RecalculateTeamInZone( void )
{
	memset( m_iNumPlayers, 0, sizeof( m_iNumPlayers ) );

	for ( CBaseEntity *pEntity : m_hTouchingEntities )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( pEntity );
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			m_iNumPlayers[pPlayer->GetTeamNumber()]++;
		}
	}

	m_iTeamInZone = TEAM_UNASSIGNED;

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		if ( m_iNumPlayers[i] != 0 )
		{
			if ( !m_iTeamInZone )
			{
				m_iTeamInZone = i;
			}
			else
			{
				// Not the only team in zone, abort.
				m_iTeamInZone = TEAM_SPECTATOR;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::PlayControlVoice( int iTeam )
{
	if ( m_bDisabled || !TDCGameRules()->IsTeamplay() || gpGlobals->curtime < m_flNextControlSpeak )
		return;

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		if ( i != iTeam )
		{
			g_TFAnnouncer.Speak( i, TDC_ANNOUNCER_PD_ENEMY_INZONE );
		}
		else
		{
			g_TFAnnouncer.Speak( i, TDC_ANNOUNCER_PD_TEAM_INZONE );
		}
	}

	m_flNextControlSpeak = gpGlobals->curtime + 10.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::CaptureThink( void )
{
	// Players can only score if there are no enemies in the zone.
	if ( TDCGameRules()->PointsMayBeCaptured() && m_iTeamInZone >= FIRST_GAME_TEAM )
	{
		for ( CBaseEntity *pOther : m_hTouchingEntities )
		{
			// Drain points from each player.
			CTDCPlayer *pPlayer = ToTDCPlayer( pOther );
			if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->GetNumMoneyPacks() == 0 )
				continue;

			if ( TDCGameRules()->IsTeamplay() )
			{
				TFTeamMgr()->AddRoundScore( pPlayer->GetTeamNumber(), 1 );
			}

			CTDC_GameStats.Event_PlayerCapturedPoint( pPlayer );

			pPlayer->DrainMoneyPack();

			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), "MoneyPack.Deliver" );
		}
	}

	SetContextThink( &CMoneyDeliveryZone::CaptureThink, gpGlobals->curtime + 1.0f, "CaptureThink" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::ActivateDeliveryZone( bool bSilent )
{
	if ( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->EnableCollisions( true );
	}

	if ( !IsSolidFlagSet( FSOLID_TRIGGER ) )
	{
		AddSolidFlags( FSOLID_TRIGGER );
		PhysicsTouchTriggers();
	}

	SetContextThink( &CMoneyDeliveryZone::CaptureThink, gpGlobals->curtime, "CaptureThink" );

	if ( !bSilent )
	{
		g_TFAnnouncer.Speak( TDC_ANNOUNCER_PD_CAPTUREZONE_ACTIVE );
		m_OnActive.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::DeactivateDeliveryZone( bool bSilent )
{
	if ( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->EnableCollisions( false );
	}

	if ( IsSolidFlagSet( FSOLID_TRIGGER ) )
	{
		RemoveSolidFlags( FSOLID_TRIGGER );
		PhysicsTouchTriggers();
	}

	SetContextThink( NULL, 0, "CaptureThink" );

	if ( !bSilent )
	{
		g_TFAnnouncer.Speak( TDC_ANNOUNCER_PD_CAPTUREZONE_INACTIVE );
		m_OnInactive.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMoneyDeliveryZone::StopLoopingSounds( void )
{
	StopSound( "BloodMoney.ZoneActive" );
	StopSound( "BloodMoney.ZoneBlocked" );
}

BEGIN_DATADESC( CTDCBloodMoneyZoneManager )
	DEFINE_KEYFIELD( m_flEnableInterval, FIELD_FLOAT, "EnableInterval" ),
	DEFINE_KEYFIELD( m_flEnableDuration, FIELD_FLOAT, "EnableDuration" ),
	DEFINE_KEYFIELD( m_bRandom, FIELD_BOOLEAN, "RotationMode" ),

	DEFINE_OUTPUT( m_OnActive, "OnActive" ),
	DEFINE_OUTPUT( m_OnInactive, "OnInactive" ),
	DEFINE_OUTPUT( m_On15SecBeforeActive, "On15SecBeforeActive" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_bloodmoney_zonemanager, CTDCBloodMoneyZoneManager );

CTDCBloodMoneyZoneManager::CTDCBloodMoneyZoneManager()
{
	SetDefLessFunc( m_Zones );
	m_flEnableDuration = 0.0f;
	m_flEnableInterval = 0.0f;
	m_flStateChangeTime = 0.0f;
	m_bFire15SecRemain = false;
	m_bActive = false;
	m_iLastZoneIndex = m_Zones.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBloodMoneyZoneManager::Activate( void )
{
	BaseClass::Activate();

	if ( TDCGameRules()->IsBloodMoney() )
	{
		// Find all delivery zones.
		for ( CMoneyDeliveryZone *pZone : CMoneyDeliveryZone::AutoList() )
		{
			int idx = pZone->GetZoneIndex();

			if ( m_Zones.Find( idx ) != m_Zones.InvalidIndex() )
			{
				Warning( "%s found a duplicate delivery zone with index %d!\n", GetClassname(), idx );
				continue;
			}

			m_Zones.Insert( idx, pZone );
		}

		if ( m_Zones.Count() == 0 )
		{
			Warning( "%s failed to find delivery zones to use!\n", GetClassname() );
			return;
		}

		// Start the cycle.
		m_flStateChangeTime = gpGlobals->curtime + m_flEnableInterval;
		ObjectiveResource()->SetMoneyZoneSwitchTime( m_flStateChangeTime, m_flEnableInterval );
		m_bFire15SecRemain = true;
		SetContextThink( &CTDCBloodMoneyZoneManager::StateThink, gpGlobals->curtime, "StateThink" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBloodMoneyZoneManager::StateThink( void )
{
	if ( !TDCGameRules()->PointsMayBeCaptured() )
	{
		SetContextThink( &CTDCBloodMoneyZoneManager::StateThink, gpGlobals->curtime, "StateThink" );
		return;
	}

	if ( !m_bActive && m_bFire15SecRemain && m_flStateChangeTime - gpGlobals->curtime <= 15.0f )
	{
		m_bFire15SecRemain = false;
		m_On15SecBeforeActive.FireOutput( this, this );
	}

	if ( gpGlobals->curtime >= m_flStateChangeTime )
	{
		// Flip the state.
		if ( !m_bActive )
		{
			m_bActive = true;

			// Find the zone to enable
			if ( m_bRandom )
			{
				CUtlVector<CMoneyDeliveryZone *> eligibleZones;

				for ( int i = m_Zones.FirstInorder(); i != m_Zones.InvalidIndex(); i = m_Zones.NextInorder( i ) )
				{
					CMoneyDeliveryZone *pZone = m_Zones[i];

					if ( pZone && !pZone->m_bDisabled )
					{
						eligibleZones.AddToTail( pZone );
					}
				}

				if ( eligibleZones.Count() != 0 )
				{
					m_hCurrentZone = eligibleZones.Random();
				}
			}
			else
			{
				int iStartIndex;

				if ( m_iLastZoneIndex != m_Zones.InvalidIndex() )
				{
					iStartIndex = m_Zones.NextInorder( m_iLastZoneIndex );
				}
				else
				{
					iStartIndex = m_Zones.FirstInorder();
				}

				// Keep iterating through zones until we loop around.
				for ( int i = iStartIndex; i != m_iLastZoneIndex; i = m_Zones.NextInorder( i ) )
				{
					if ( i == m_Zones.InvalidIndex() )
					{
						i = m_Zones.FirstInorder();
					}

					CMoneyDeliveryZone *pZone = m_Zones[i];

					if ( pZone && !pZone->m_bDisabled )
					{
						m_hCurrentZone = pZone;
						m_iLastZoneIndex = i;
						break;
					}
				}
			}

			if ( m_hCurrentZone )
			{
				m_hCurrentZone->ActivateDeliveryZone( false );

				if ( ObjectiveResource() )
				{
					ObjectiveResource()->SetActiveMoneyZone( m_hCurrentZone->entindex() );
					ObjectiveResource()->SetMoneyZoneSwitchTime( gpGlobals->curtime + m_flEnableDuration, m_flEnableDuration );
				}
			}

			m_OnActive.FireOutput( this, this );
			m_flStateChangeTime = gpGlobals->curtime + m_flEnableDuration;
		}
		else
		{
			m_bActive = false;

			if ( m_hCurrentZone )
			{
				m_hCurrentZone->DeactivateDeliveryZone( false );
			}

			if ( ObjectiveResource() )
			{
				ObjectiveResource()->SetActiveMoneyZone( 0 );
				ObjectiveResource()->SetMoneyZoneSwitchTime( gpGlobals->curtime + m_flEnableInterval, m_flEnableInterval );
			}

			m_hCurrentZone = NULL;
			m_OnInactive.FireOutput( this, this );
			m_flStateChangeTime = gpGlobals->curtime + m_flEnableInterval;
			m_bFire15SecRemain = true;
		}
	}

	SetContextThink( &CTDCBloodMoneyZoneManager::StateThink, gpGlobals->curtime, "StateThink" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCBloodMoneyZoneManager::UpdateOnRemove( void )
{
	if ( ObjectiveResource() )
	{
		ObjectiveResource()->SetActiveMoneyZone( 0 );
		ObjectiveResource()->SetMoneyZoneSwitchTime( -1.0f, 0.0f );
	}

	BaseClass::UpdateOnRemove();
}
