//=============================================================================//
//
// Purpose: A modified game_player_equip that works with TF2C
//
//=============================================================================//
#include "cbase.h"
#include "entity_player_equip.h"
#include "tdc_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( game_player_equip_tdc, CTDCPlayerEquip );

BEGIN_DATADESC( CTDCPlayerEquip )

	DEFINE_KEYFIELD( m_iszWeapons[0], FIELD_STRING, "weapon_primary" ),
	DEFINE_KEYFIELD( m_iszWeapons[1], FIELD_STRING, "weapon_secondary" ),
	DEFINE_KEYFIELD( m_iszWeapons[2], FIELD_STRING, "weapon_melee" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EquipAllPlayers", InputEquipAllPlayers ),
	DEFINE_INPUTFUNC( FIELD_STRING, "EquipPlayer", InputEquipPlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()

CTDCPlayerEquip::CTDCPlayerEquip()
{
	memset( m_iWeapons, 0, sizeof( m_iWeapons ) );
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerEquip::Spawn( void )
{
	BaseClass::Spawn();

	for ( int i = 0; i < TDC_PLAYER_WEAPON_COUNT; i++ )
	{
		m_iWeapons[i] = GetWeaponId( STRING( m_iszWeapons[i] ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCPlayerEquip::CanEquip( CTDCPlayer *pPlayer, bool bAuto )
{
	if ( bAuto && m_bDisabled )
		return false;

	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	if ( GetTeamNumber() && pPlayer->GetTeamNumber() != GetTeamNumber() )
		return false;

	if ( !pPlayer->IsNormalClass() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerEquip::EquipPlayer( CTDCPlayer *pPlayer )
{
	pPlayer->SetEquipEntity( this );
	pPlayer->SetRegenerating( true );

	pPlayer->GiveDefaultItems();

	pPlayer->SetEquipEntity( NULL );
	pPlayer->SetRegenerating( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ETDCWeaponID CTDCPlayerEquip::GetWeapon( int iSlot )
{
	return m_iWeapons[iSlot];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerEquip::InputEquipAllPlayers( inputdata_t &inputdata )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTDCPlayer *pPlayer = ToTDCPlayer( UTIL_PlayerByIndex( i ) );

		if ( CanEquip( pPlayer, false ) )
		{
			EquipPlayer( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerEquip::InputEquipPlayer( inputdata_t &inputdata )
{
	CTDCPlayer *pPlayer = ToTDCPlayer( gEntList.FindEntityByName( NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller ) );

	if ( CanEquip( pPlayer, false ) )
	{
		EquipPlayer( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerEquip::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCPlayerEquip::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}
