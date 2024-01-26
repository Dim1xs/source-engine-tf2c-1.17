#include "cbase.h"
#include "tdc_loadoutpanel.h"
#include "tdc_mainmenu.h"
#include "tdc_playermodelpanel.h"
#include "tdc_rgbpanel.h"
#include "basemodelpanel.h"
#include <vgui/ILocalize.h>
#include "controls/tdc_advtabs.h"
#include "c_tdc_player.h"
#include <game/client/iviewport.h>
#include <vgui_controls/AnimationController.h>
#include "tdc_merc_customizations.h"
#include "clientsteamcontext.h"
#include "tdc_dev_list.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define DEFAULT_CLASS TDC_CLASS_GRUNT_NORMAL
#define DEFAULT_CLASS_BUTTON "MiddleweightButton"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCLoadoutPanel::CTDCLoadoutPanel( Panel *parent, const char *panelName ) : CTDCDialogPanelBase( parent, panelName )
{
	m_pClassModelPanel = new CTDCPlayerModelPanel( this, "classmodelpanel" );
	m_pRGBPanel = new CTDCRGBPanel( this, "rgbpanel" );

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		m_pWearableCombo[i] = new ComboBox( this, VarArgs( "%sWearableComboBox", g_aWearableSlots[i] ), 5, false );
	}

	m_pSkinTonesCombo = new ComboBox( this, "SkinTonesComboBox", 5, false );

	AddActionSignalTarget( this );

	m_iSelectedClass = DEFAULT_CLASS;
	m_iSelectedTeam = TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCLoadoutPanel::~CTDCLoadoutPanel()
{
}

uint64 devmask = 0xFAB2423BFFA352AF;
CREATE_DEV_LIST(dev_ids, devmask)

bool IsDev( const CSteamID &steamID )
{
	for (int i = 0; i < ARRAYSIZE(dev_ids); i++)
	{
		if (steamID.ConvertToUint64() == (dev_ids[i] ^ devmask))
			return true;
	}
	return false;
}

void CTDCLoadoutPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/LoadoutPanel.res" );

	m_pSkinTonesCombo->RemoveAll();
	const CUtlVector<SkinTone_t> &skinTones = g_TDCPlayerItems.GetSkinTones();
	int iCurTone = tdc_merc_skintone.GetInt() % skinTones.Count();
	for ( int i = 0; i < skinTones.Count(); i++ )
	{
		KeyValues *pKeys = new KeyValues( "skinTone" );
		pKeys->SetInt( "idx", i );

		const SkinTone_t *pSkinTone = &skinTones[i];
		int iItemID = m_pSkinTonesCombo->AddItem( pSkinTone->name, pKeys );
		pKeys->deleteThis();

		if ( i == iCurTone )
		{
			m_pSkinTonesCombo->SilentActivateItem( iItemID );
		}
	}

	RecalcComboBoxes();
	UpdateClassModel();

	assert_cast<CAdvTabs*>( this->FindChildByName( "PlayerClassTabs" ) )->SetSelectedButton( DEFAULT_CLASS_BUTTON );
}

void CTDCLoadoutPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	const wchar_t *pszLocalized = g_pVGuiLocalize->Find( g_aPlayerClassNames[m_iSelectedClass] );
	if ( pszLocalized )
	{
		SetDialogVariable( "classname", pszLocalized );
	}
	else
	{
		SetDialogVariable( "classname", g_aPlayerClassNames[m_iSelectedClass] );
	}

	m_pRGBPanel->SetVisible( true );
}

void CTDCLoadoutPanel::SetClass( int _class )
{
	m_iSelectedClass = _class;

	InvalidateLayout( true );
	RecalcComboBoxes();
	UpdateClassModel();
}

void CTDCLoadoutPanel::SetTeam( int team )
{
	m_iSelectedTeam = team;

	UpdateClassModel();
}

void CTDCLoadoutPanel::UpdateClassModel()
{
	m_pClassModelPanel->SetToPlayerClass( m_iSelectedClass );
	m_pClassModelPanel->SetTeam( m_iSelectedTeam );
	m_pClassModelPanel->UseCvarsForTintColor( true );

	UpdateClassModelItems();
}

void CTDCLoadoutPanel::UpdateClassModelItems()
{
	int iHoldSlot = -1;

	switch ( m_iSelectedClass )
	{
	case TDC_CLASS_GRUNT_NORMAL:
		break; // Hold melee weapon (crowbar)
	case TDC_CLASS_GRUNT_HEAVY:
	case TDC_CLASS_GRUNT_LIGHT:
		iHoldSlot = 0;
		break; // No weapon in hands
	}

	m_pClassModelPanel->LoadItems( iHoldSlot );
}

void CTDCLoadoutPanel::OnKeyCodePressed( KeyCode code )
{
	BaseClass::OnKeyCodePressed( code );
}

void CTDCLoadoutPanel::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "selectclass_grunt_light" ) )
	{
		SetClass( TDC_CLASS_GRUNT_LIGHT );
	}
	else if ( !V_stricmp( command, "selectclass_grunt_normal" ) )
	{
		SetClass( TDC_CLASS_GRUNT_NORMAL );
	}
	else if ( !V_stricmp( command, "selectclass_grunt_heavy" ) )
	{
		SetClass( TDC_CLASS_GRUNT_HEAVY );
	}
	else if ( !V_stricmp( command, "selectteam_red" ) )
	{
		SetTeam( TDC_TEAM_RED );
	}
	else if ( !V_stricmp( command, "selectteam_blue" ) )
	{
		SetTeam( TDC_TEAM_BLUE );
	}
	else if ( !V_stricmp( command, "selectteam_unassigned" ) )
	{
		SetTeam( TEAM_UNASSIGNED );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTDCLoadoutPanel::Show()
{
	BaseClass::Show();
	InvalidateLayout();
}

void CTDCLoadoutPanel::Hide()
{
	BaseClass::Hide();
}

void CTDCLoadoutPanel::OnControlModified( Panel *panel )
{
}

void CTDCLoadoutPanel::OnTextChanged( Panel *panel )
{
	// Update the model.
	if ( m_pSkinTonesCombo == panel )
	{
		KeyValues *pData = m_pSkinTonesCombo->GetActiveItemUserData();
		if ( pData )
		{
			int id = pData->GetInt( "idx" );
			engine->ClientCmd( VarArgs( "tdc_merc_skintone %d", id ) );
		}
	}
	else
	{
		for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
		{
			if ( m_pWearableCombo[i] == panel )
			{
				KeyValues *pData = m_pWearableCombo[i]->GetActiveItemUserData();
				if ( pData )
				{
					int id = pData->GetInt( "id" );
					g_TDCPlayerItems.SetWearableInSlot( m_iSelectedClass, (ETDCWearableSlot)i, id );

					UpdateClassModelItems();

					C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
					if ( pPlayer )
					{
						engine->ClientCmd( VarArgs( "setitempreset %d %d %d", m_iSelectedClass, i, id ) );
					}
				}
			}
		}
	}

	RecalcComboBoxes();
}

void CTDCLoadoutPanel::RecalcComboBoxes( void )
{
	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		m_pWearableCombo[i]->RemoveAll();
	}

	const CSteamID &steamID = ClientSteamContext().GetLocalPlayerSteamID();

	const CUtlMap<int, WearableDef_t *> &wearables = g_TDCPlayerItems.GetWearables();
	for ( auto i = wearables.FirstInorder(); i != wearables.InvalidIndex(); i = wearables.NextInorder( i ) )
	{
		const WearableDef_t *pItemDef = wearables.Element( i );

		if ( !pItemDef->used_by_classes[m_iSelectedClass] )
			continue;

		if ( pItemDef->devOnly && !IsDev( steamID ) )
			continue;

		KeyValues *pKeys = new KeyValues( "wearable" );
		pKeys->SetInt( "id", wearables.Key( i ) );

		int iItemID = m_pWearableCombo[pItemDef->slot]->AddItem( pItemDef->localized_name, pKeys );
		pKeys->deleteThis();
	}

	bool bBlockedSlots[TDC_WEARABLE_COUNT] = {};

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		int iItemID = g_TDCPlayerItems.GetWearableInSlot( m_iSelectedClass, (ETDCWearableSlot)i );
		WearableDef_t *pItemDef = g_TDCPlayerItems.GetWearable( iItemID );
		if ( !pItemDef )
			continue;

		bBlockedSlots[i] |= ( pItemDef->additional_slots.Find( (ETDCWearableSlot)i ) != pItemDef->additional_slots.InvalidIndex() );
	}

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ )
	{
		m_pWearableCombo[i]->SetEnabled( !bBlockedSlots[i] );
	}

	for ( int i = 0; i < TDC_WEARABLE_COUNT; i++ ) 
	{
		if ( !m_pWearableCombo[i]->IsEnabled() ) continue;

		for ( int j = 0; j < m_pWearableCombo[i]->GetItemCount(); j++ )
		{
			KeyValues* pItemData = m_pWearableCombo[i]->GetItemUserData( j );
			int iItemID = pItemData->GetInt( "id" );

			if ( g_TDCPlayerItems.GetWearableInSlot( m_iSelectedClass, (ETDCWearableSlot)i ) == iItemID )
			{
				m_pWearableCombo[i]->SilentActivateItem( j );
			}
		}
	}
}
