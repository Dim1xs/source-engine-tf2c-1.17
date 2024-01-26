//=============================================================================//
//
// Purpose: Respawn particles parser
//
//=============================================================================//
#ifndef TDC_RESPAWN_PARTICLES_H
#define TDC_RESPAWN_PARTICLES_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "tdc_shareddefs.h"

struct Visuals_t
{
	Visuals_t()
	{
		model = "";
		viewmodel = "";
	}

	const char *model;
	const char *viewmodel;
	CUtlDict<int> bodygroups;
	CUtlDict<int> viewmodelbodygroups;
};

struct WearableDef_t
{
	WearableDef_t()
	{
		localized_name = "";
		description = "";
		slot = TDC_WEARABLE_INVALID;
		iViewmodelIndex = -1;
		devOnly = false;
	}

	const char *localized_name;
	const char *description;
	ETDCWearableSlot slot;
	int iViewmodelIndex;
	CUtlVector<ETDCWearableSlot> additional_slots;
	Visuals_t visuals[TDC_CLASS_COUNT_ALL];
	bool used_by_classes[TDC_CLASS_COUNT_ALL];
	bool devOnly;

private:
	WearableDef_t( const WearableDef_t & );
};

struct WinAnim_t
{
	int id;
	const char *sequence;
	const char *localized_name;
};

struct ZombieBodygroup_t
{
	const char *name;
	int states;
};

struct SkinTone_t
{
	const char *name;
	Vector tone;
};

struct PlayerColor_t
{
	const char *name;
	Vector color;
};

class CTDCPlayerItems : public CAutoGameSystem
{
public:
	CTDCPlayerItems();
	~CTDCPlayerItems();

	virtual char const *Name() { return "CTDCPlayerItems"; }

	virtual bool Init( void );
	virtual void LevelInitPreEntity( void );

	const CUtlMap<int, WearableDef_t *> &GetWearables( void ) { return m_Wearables; }
	WearableDef_t *GetWearable( int id );
	int GetFirstWearableIDForSlot( ETDCWearableSlot iSlot, int _class );
	int GetRandomWearableIDForSlot( ETDCWearableSlot iSlot, int _class );

	const CUtlVector<SkinTone_t> &GetSkinTones( void ) { return m_SkinTones; }
	const CUtlVector<PlayerColor_t> &GetPlayerColors( void ) { return m_PlayerColors; }

#ifdef GAME_DLL
	const CUtlVector<ZombieBodygroup_t> &GetZombieBodygroups( void ) { return m_ZombieBodygroups; }
#else
	int NumAnimations( void ) { return m_WinAnimations.Count(); }
	WinAnim_t *GetAnimation( int index );
	WinAnim_t *GetAnimationById( int id );

	int GetWearableInSlot( int iClass, ETDCWearableSlot iSlot );
	void SetWearableInSlot( int iClass, ETDCWearableSlot iSlot, int iItemID );

	void LoadInventory();
	void ResetInventory();
	void SaveInventory();
#endif

private:
	KeyValues *m_pKeyValues;

	CUtlMap<int, WearableDef_t *> m_Wearables;
	CUtlVector<SkinTone_t> m_SkinTones;
	CUtlVector<PlayerColor_t> m_PlayerColors;

#ifdef GAME_DLL
	CUtlVector<ZombieBodygroup_t> m_ZombieBodygroups;
#else
	CUtlVector<WinAnim_t> m_WinAnimations;
	KeyValues *m_pInventory;
#endif
};

extern CTDCPlayerItems g_TDCPlayerItems;

#endif // TDC_RESPAWN_PARTICLES_H
