#include "cbase.h"
#include "tdc_rgbpanel.h"
#include <vgui_controls/ComboBox.h>
#include "controls/tdc_cvarslider.h"
#include <vgui/ILocalize.h>
#include "c_tdc_player.h"
#include "tdc_merc_customizations.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCRGBPanel::CTDCRGBPanel( Panel *parent, const char *panelName ) : BaseClass( parent, panelName )
{
	m_pRedScrollBar = new CTDCCvarSlider( this, "RedScrollBar" );
	m_pGrnScrollBar = new CTDCCvarSlider( this, "GrnScrollBar" );
	m_pBluScrollBar = new CTDCCvarSlider( this, "BluScrollBar" );
	m_pColorCombo = new ComboBox( this, "ColorComboBox", 5, false );
	m_pColorBG = new ImagePanel( this, "ColorBG" );
	m_pAnimCombo = new ComboBox( this, "WinAnimComboBox", 5, false );
}

void CTDCRGBPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/RGBPanel.res" );

	m_pAnimCombo->RemoveAll();

	for ( int i = 0; i < g_TDCPlayerItems.NumAnimations(); i++ )
	{
		WinAnim_t *pData = g_TDCPlayerItems.GetAnimation( i );

		KeyValues *pKeys = new KeyValues( "animation" );
		pKeys->SetInt( "id", pData->id );
		pKeys->SetString( "sequence", pData->sequence );

		int iItemID = m_pAnimCombo->AddItem( pData->localized_name, pKeys );
		pKeys->deleteThis();

		if ( pData->id == tdc_merc_winanim.GetInt() )
		{
			m_pAnimCombo->SilentActivateItem( iItemID );
		}
	}

	if (tdc_merc_color.GetInt() == -1)
	{
		// -1 is default value which disables the proxy. So that means this is the first game launch.
		// Let's pick a random color...
		tdc_merc_color.SetValue( RandomInt(0, 1024) );
	}

	m_pColorCombo->RemoveAll();
	const CUtlVector<PlayerColor_t> &playerColors = g_TDCPlayerItems.GetPlayerColors();
	if ( !playerColors.IsEmpty() )
	{
		int iCurTone = tdc_merc_color.GetInt() % playerColors.Count();
		for ( int i = 0; i < playerColors.Count(); i++)
		{
			KeyValues *pKeys = new KeyValues( "playerColors" );
			pKeys->SetInt("idx", i);

			const PlayerColor_t *pPlayerColor = &playerColors[i];
			int iItemID = m_pColorCombo->AddItem( pPlayerColor->name, pKeys );
			pKeys->deleteThis();

			if ( i == iCurTone )
			{
				m_pColorCombo->SilentActivateItem( iItemID );
				OnControlModified( m_pColorCombo );
			}
		}
	}
}

void CTDCRGBPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	OnControlModified( nullptr );
}

void CTDCRGBPanel::OnControlModified( Panel *panel )
{
	PostActionSignal( new KeyValues( "ControlModified" ) );

	// Prevent players from using presets to go out of range
	if ( panel == m_pRedScrollBar )
	{
		m_pGrnScrollBar->ClampValue();
		m_pGrnScrollBar->ApplyChanges();
		m_pBluScrollBar->ClampValue();
		m_pBluScrollBar->ApplyChanges();
	}
	else if ( panel == m_pGrnScrollBar )
	{
		m_pRedScrollBar->ClampValue();
		m_pRedScrollBar->ApplyChanges();
		m_pBluScrollBar->ClampValue();
		m_pBluScrollBar->ApplyChanges();
	}
	else if ( panel == m_pBluScrollBar )
	{
		m_pRedScrollBar->ClampValue();
		m_pRedScrollBar->ApplyChanges();
		m_pGrnScrollBar->ClampValue();
		m_pGrnScrollBar->ApplyChanges();
	}
	else if ( panel == m_pColorCombo )
	{
		m_pRedScrollBar->SetValue( tdc_merc_color_r.GetInt() );
		m_pGrnScrollBar->SetValue( tdc_merc_color_g.GetInt() );
		m_pBluScrollBar->SetValue( tdc_merc_color_b.GetInt() );

		m_pRedScrollBar->ApplyChanges();
		m_pGrnScrollBar->ApplyChanges();
		m_pBluScrollBar->ApplyChanges();
	}

	// Set the color on the panel.
	Color clr( m_pRedScrollBar->GetValue(), m_pGrnScrollBar->GetValue(), m_pBluScrollBar->GetValue(), 255 );
	m_pColorBG->SetFillColor( clr );
}

void CTDCRGBPanel::OnTextChanged( Panel *panel )
{
	if ( panel == m_pAnimCombo )
	{
		KeyValues *pKeys = m_pAnimCombo->GetActiveItemUserData();

		if ( pKeys )
		{
			tdc_merc_winanim.SetValue( pKeys->GetInt( "id" ) );
		}
	}
	else if ( panel == m_pColorCombo )
	{
		KeyValues *pData = m_pColorCombo->GetActiveItemUserData();
		if ( pData )
		{
			int id = pData->GetInt( "idx" );
			tdc_merc_color.SetValue( VarArgs( "%d", id ) );

			OnControlModified( panel );
		}
	}
}
