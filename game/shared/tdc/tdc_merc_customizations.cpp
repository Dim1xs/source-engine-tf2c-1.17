//=============================================================================//
//
// Purpose: Respawn particles parser
//
//=============================================================================//
#include "cbase.h"
#include "tdc_merc_customizations.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TDC_MERC_CUSTOMIZATIONS_FILE "scripts/player_items.txt"
#define TDC_INVENTORY_FILE "tdc_inventory.txt"

CTDCPlayerItems g_TDCPlayerItems;

CTDCPlayerItems::CTDCPlayerItems()
{
	m_pKeyValues = NULL;
	SetDefLessFunc( m_Wearables );

#ifdef CLIENT_DLL
	m_pInventory = NULL;
#endif
}

CTDCPlayerItems::~CTDCPlayerItems()
{
	m_Wearables.PurgeAndDeleteElements();
	m_pKeyValues->deleteThis();

#ifdef CLIENT_DLL
	m_pInventory->deleteThis();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTDCPlayerItems::Init( void )
{
	m_pKeyValues = new KeyValues( "PlayerItems" );
	if ( !m_pKeyValues->LoadFromFile( filesystem, TDC_MERC_CUSTOMIZATIONS_FILE, "MOD" ) )
	{
		m_pKeyValues->deleteThis();
		Warning( "Failed to load player items data from %s!\n", TDC_MERC_CUSTOMIZATIONS_FILE );
		return false;
	}

	KeyValues *pWearableKeys = m_pKeyValues->FindKey( "Wearables" );
	if ( pWearableKeys )
	{
		for ( KeyValues *pData = pWearableKeys->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
		{
			int id = atoi( pData->GetName() );
			if ( id == 0 )
				continue;

			WearableDef_t *pItemDef = new WearableDef_t();
			pItemDef->slot = (ETDCWearableSlot)UTIL_StringFieldToInt( pData->GetString( "slot" ), g_aWearableSlots, TDC_WEARABLE_COUNT );
			if ( pItemDef->slot == TDC_WEARABLE_INVALID )
			{
				delete pItemDef;
				continue;
			}

			pItemDef->localized_name = pData->GetString( "name" );
			pItemDef->description = pData->GetString( "description" );

			KeyValues *pAdditionalSlots = pData->FindKey( "additional_slots" );
			if ( pAdditionalSlots )
			{
				for ( KeyValues *pSlotData = pAdditionalSlots->GetFirstSubKey(); pSlotData != NULL; pSlotData = pSlotData->GetNextKey() )
				{
					ETDCWearableSlot iSlot = (ETDCWearableSlot)UTIL_StringFieldToInt( pSlotData->GetName(), g_aWearableSlots, TDC_WEARABLE_COUNT );
					if ( iSlot == TDC_WEARABLE_INVALID || iSlot == pItemDef->slot )
						continue;

					pItemDef->additional_slots.AddToTail( iSlot );
				}
			}

			KeyValues *pVisuals = pData->FindKey( "visuals" );
			if ( pVisuals )
			{
				for ( KeyValues *pVisualData = pVisuals->GetFirstSubKey(); pVisualData != NULL; pVisualData = pVisualData->GetNextKey() )
				{
					// Visuals like model and bodygroup must be explicitly supported to be used by a class.
					int iClass = UTIL_StringFieldToInt( pVisualData->GetName(), g_aPlayerClassNames_NonLocalized, TDC_CLASS_COUNT_ALL );
					if ( iClass != -1 )
					{
						pItemDef->visuals[iClass].model = pVisualData->GetString( "model" );

						KeyValues *pBodygroups = pVisualData->FindKey( "bodygroups" );
						if ( pBodygroups )
						{
							for ( KeyValues *pBodyData = pBodygroups->GetFirstSubKey(); pBodyData != NULL; pBodyData = pBodyData->GetNextKey() )
							{
								pItemDef->visuals[iClass].bodygroups.Insert( pBodyData->GetName(), pBodyData->GetInt() );
							}
						}

						KeyValues *pViewmodel = pVisualData->FindKey( "viewmodel" );
						if ( pViewmodel )
						{
							KeyValues *pData = pViewmodel->FindKey( "model" );
							if ( pData )
								pItemDef->visuals[iClass].viewmodel = pData->GetString();

							KeyValues *pVMBodyGroups = pViewmodel->FindKey( "bodygroups" );
							if ( pVMBodyGroups )
							{
								for ( KeyValues *pBodyData = pVMBodyGroups->GetFirstSubKey(); pBodyData != NULL; pBodyData = pBodyData->GetNextKey() )
								{
									pItemDef->visuals[iClass].viewmodelbodygroups.Insert( pBodyData->GetName(), pBodyData->GetInt() );
								}
							}
						}
					}
				}
			}

			pItemDef->devOnly = pData->GetBool( "dev_only" );

			KeyValues *pUsedByClasses = pData->FindKey( "used_by_classes" );

			for ( int i = 0; i < TDC_CLASS_COUNT_ALL; i++ )
				pItemDef->used_by_classes[i] = !pUsedByClasses; // if key not found, assume all classes supported
			
			if ( pUsedByClasses )
			{
				for ( KeyValues *pClassData = pUsedByClasses->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
				{
					int iClass = UTIL_StringFieldToInt( pClassData->GetName(), g_aPlayerClassNames_NonLocalized, TDC_CLASS_COUNT_ALL );
					if ( iClass != -1 )
					{
						pItemDef->used_by_classes[iClass] = pClassData->GetBool();
					}
				}
			}

			m_Wearables.Insert( id, pItemDef );
		}
	}

	KeyValues *pSkinToneKeys = m_pKeyValues->FindKey( "SkinTones" );
	if ( pSkinToneKeys )
	{
		for ( KeyValues *pData = pSkinToneKeys->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
		{
			SkinTone_t &newTone = m_SkinTones[m_SkinTones.AddToTail()];
			newTone.name = pData->GetString( "name" );
			UTIL_StringToVector( newTone.tone.Base(), pData->GetString( "tone" ) );
		}
	}

	KeyValues *pPlayerColorKeys = m_pKeyValues->FindKey("PlayerColors");
	if ( pPlayerColorKeys )
	{
		for ( KeyValues *pData = pPlayerColorKeys->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
		{
			PlayerColor_t &newTone = m_PlayerColors[m_PlayerColors.AddToTail()];
			newTone.name = pData->GetString( "name" );
			UTIL_StringToVector( newTone.color.Base(), pData->GetString( "color" ) );
		}
	}

#ifdef GAME_DLL
	KeyValues *pZombieKeys = m_pKeyValues->FindKey( "ZombieStates" );
	if ( pZombieKeys )
	{
		for ( KeyValues *pData = pZombieKeys->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
		{
			ZombieBodygroup_t &newBody = m_ZombieBodygroups[m_ZombieBodygroups.AddToTail()];
			newBody.name = pData->GetName();
			newBody.states = pData->GetInt( "states" );
		}
	}
#else
	KeyValues *pAnimKeys = m_pKeyValues->FindKey( "WinAnimations" );
	if ( pAnimKeys )
	{
		for ( KeyValues *pData = pAnimKeys->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
		{
			int index = atoi( pData->GetName() );
			if ( index == 0 )
				continue;

			WinAnim_t &newAnim = m_WinAnimations[m_WinAnimations.AddToTail()];
			newAnim.id = index;
			newAnim.sequence = pData->GetString( "sequence" );
			newAnim.localized_name = pData->GetString( "name" );
		}
	}
#endif

#if defined( CLIENT_DLL )
	LoadInventory();
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerItems::LevelInitPreEntity( void )
{
#ifdef GAME_DLL
	// Precache all wearables.
	for ( auto i = m_Wearables.FirstInorder(); i != m_Wearables.InvalidIndex(); i = m_Wearables.NextInorder( i ) )
	{
		for ( int j = TDC_CLASS_UNDEFINED; j < TDC_CLASS_COUNT_ALL; j++ )
		{
			if ( m_Wearables[i]->visuals[j].model[0] )
			{
				CBaseEntity::PrecacheModel( m_Wearables[i]->visuals[j].model );
			}

			if ( m_Wearables[i]->visuals[j].viewmodel[0] )
			{
				m_Wearables[i]->iViewmodelIndex = CBaseEntity::PrecacheModel( m_Wearables[i]->visuals[j].viewmodel );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
WearableDef_t *CTDCPlayerItems::GetWearable( int id )
{
	auto idx = m_Wearables.Find( id );
	if ( idx != m_Wearables.InvalidIndex() )
	{
		return m_Wearables[idx];
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCPlayerItems::GetFirstWearableIDForSlot( ETDCWearableSlot iSlot, int _class )
{
	for ( auto i = m_Wearables.FirstInorder(); i != m_Wearables.InvalidIndex(); i = m_Wearables.NextInorder( i ) )
	{
		if ( m_Wearables.Element( i )->slot == iSlot && m_Wearables.Element( i )->used_by_classes[_class] )
		{
			return m_Wearables.Key( i );
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a random wearable item from the specified slot
//-----------------------------------------------------------------------------
int CTDCPlayerItems::GetRandomWearableIDForSlot( ETDCWearableSlot iSlot, int _class )
{
	CUtlVector<int> itemIDs;

	for ( auto i = m_Wearables.FirstInorder(); i != m_Wearables.InvalidIndex(); i = m_Wearables.NextInorder( i ) )
	{
		if ( m_Wearables.Element( i )->slot == iSlot && m_Wearables.Element( i )->used_by_classes[_class] )
			itemIDs.AddToTail( m_Wearables.Key( i ) );
	}

	return ( itemIDs.Count() > 0 ) ? itemIDs.Element( RandomInt( 0, itemIDs.Count() - 1 ) ) : TDC_WEARABLE_INVALID;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
WinAnim_t *CTDCPlayerItems::GetAnimation( int index )
{
	if ( m_WinAnimations.IsValidIndex( index ) )
	{
		return &m_WinAnimations[index];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
WinAnim_t *CTDCPlayerItems::GetAnimationById( int id )
{
	// Find particle with this ID.
	for ( int i = 0; i < m_WinAnimations.Count(); i++ )
	{
		if ( m_WinAnimations[i].id == id )
		{
			return &m_WinAnimations[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerItems::LoadInventory()
{
	bool bExist = filesystem->FileExists( TDC_INVENTORY_FILE, "MOD" );
	if ( bExist )
	{
		if ( !m_pInventory )
		{
			m_pInventory = new KeyValues( "Inventory" );
		}

		m_pInventory->LoadFromFile( filesystem, TDC_INVENTORY_FILE, "MOD" );
	}
	else
	{
		ResetInventory();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerItems::SaveInventory()
{
	m_pInventory->SaveToFile( filesystem, TDC_INVENTORY_FILE, "MOD" );
}

//-----------------------------------------------------------------------------
// Purpose: Create a default inventory file.
//-----------------------------------------------------------------------------
void CTDCPlayerItems::ResetInventory()
{
	if ( m_pInventory )
	{
		m_pInventory->deleteThis();
	}

	m_pInventory = new KeyValues( "Inventory" );

	for ( int i = TDC_CLASS_UNDEFINED; i < TDC_CLASS_COUNT_ALL; i++ )
	{
		KeyValues *pClassInv = new KeyValues( g_aPlayerClassNames_NonLocalized[i] );
		for ( int iSlot = 0; iSlot < TDC_WEARABLE_COUNT; iSlot++ )
		{
			// Find the first cosmetic for this slot.
			pClassInv->SetInt( g_aWearableSlots[iSlot], GetFirstWearableIDForSlot( (ETDCWearableSlot)iSlot, i ) );
		}

		m_pInventory->AddSubKey( pClassInv );
	}

	SaveInventory();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTDCPlayerItems::GetWearableInSlot( int iClass, ETDCWearableSlot iSlot )
{
	KeyValues *pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
	if ( !pClass )
	{
		// Cannot find class node.
		ResetInventory();
		return 0;
	}

	int id = pClass->GetInt( g_aWearableSlots[iSlot], -1 );
	if ( id == -1 )	
	{
		// Cannot find slot node.
		ResetInventory();
		return 0;
	}

	// Verify that it's valid item.
	WearableDef_t *pItemDef = GetWearable( id );
	if ( !pItemDef || pItemDef->slot != iSlot )
	{
		ResetInventory();
		return 0;
	}

	return id;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCPlayerItems::SetWearableInSlot( int iClass, ETDCWearableSlot iSlot, int iPreset )
{
	KeyValues *pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
	if ( !pClass )
	{
		// Cannot find class node.
		ResetInventory();
		pClass = m_pInventory->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
	}

	pClass->SetInt( g_aWearableSlots[iSlot], iPreset );
	SaveInventory();
}
#endif
