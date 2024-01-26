#include "cbase.h"
#include "tdc_tooltippanel.h"
#include "tdc_mainmenupanel.h"
#include "tdc_mainmenu.h"
#include "controls/tdc_advbuttonbase.h"
#include <vgui_controls/TextImage.h>
#include <vgui/ISurface.h>

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCToolTipPanel::CTDCToolTipPanel( vgui::Panel* parent, const char *panelName ) : CTDCMenuPanelBase( parent, panelName )
{
	m_pText = new CExLabel( this, "TextLabel", "" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCToolTipPanel::~CTDCToolTipPanel()
{

}

void CTDCToolTipPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFilename() );
}

void CTDCToolTipPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	AdjustToolTipSize();
}

void CTDCToolTipPanel::ShowToolTip( const char *pszText )
{
	Show();

	m_pText->SetText( pszText );
	InvalidateLayout( true );
}

void CTDCToolTipPanel::HideToolTip( void )
{
	Hide();
}

void CTDCToolTipPanel::Show( void )
{
	BaseClass::Show();
	MakePopup();

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
}

void CTDCToolTipPanel::Hide( void )
{
	BaseClass::Hide();
}

void CTDCToolTipPanel::OnThink( void )
{
	BaseClass::OnThink();
	int cursorX, cursorY;
	surface()->SurfaceGetCursorPos( cursorX, cursorY );
	//SetPos(cursorX + toProportionalWide(8), cursorY + toProportionalTall(10));

	int iTipW, iTipH;
	GetSize( iTipW, iTipH );

	int wide, tall;
	surface()->GetScreenSize( wide, tall );

	if ( wide - iTipW > cursorX )
	{
		cursorY += YRES( 10 );
		cursorX += XRES( 8 );

		// menu hanging right
		if ( tall - iTipH > cursorY )
		{
			// menu hanging down
			SetPos( cursorX, cursorY );
		}
		else
		{
			// menu hanging up
			SetPos( cursorX, cursorY - iTipH - YRES( 20 ) );
		}
	}
	else
	{
		cursorY += YRES( 10 );
		cursorX += XRES( 8 );
		// menu hanging left
		if ( tall - iTipH > cursorY )
		{
			// menu hanging down
			SetPos( cursorX - iTipW, cursorY );
		}
		else
		{
			// menu hanging up
			SetPos( cursorX - iTipW, cursorY - iTipH - YRES( 20 ) );
		}
	}
}

void CTDCToolTipPanel::AdjustToolTipSize( void )
{
	// Figure out text size and resize tooltip accordingly.
	if ( m_pText )
	{
		int x, y, wide, tall;
		m_pText->GetPos( x, y );
		m_pText->GetTextImage()->GetContentSize( wide, tall );

		m_pText->SetTall( tall );
		SetSize( x * 2 + wide, y * 2 + tall );
	}
}
