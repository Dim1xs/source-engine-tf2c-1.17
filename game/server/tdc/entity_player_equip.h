//=============================================================================//
//
// Purpose: A modified game_player_equip that works with TF2C
//
//=============================================================================//
#ifndef ENTITY_PLAYER_EQUIP_H
#define ENTITY_PLAYER_EQUIP_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_shareddefs.h"

class CTDCPlayer;

class CTDCPlayerEquip : public CLogicalEntity, public TAutoList<CTDCPlayerEquip>
{
public:
	DECLARE_CLASS( CTDCPlayerEquip, CLogicalEntity );
	DECLARE_DATADESC();

	CTDCPlayerEquip();

	virtual void	Spawn( void );

	bool			CanEquip( CTDCPlayer *pPlayer, bool bAuto );
	void			EquipPlayer( CTDCPlayer *pPlayer );
	ETDCWeaponID		GetWeapon( int iSlot );

	void			InputEquipAllPlayers( inputdata_t &inputdata );
	void			InputEquipPlayer( inputdata_t &inputdata );
	void			InputEnable( inputdata_t &input );
	void			InputDisable( inputdata_t &input );

private:
	string_t		m_iszWeapons[TDC_PLAYER_WEAPON_COUNT];
	ETDCWeaponID		m_iWeapons[TDC_PLAYER_WEAPON_COUNT];
	bool			m_bDisabled;
};

#endif // ENTITY_PLAYER_EQUIP_H
