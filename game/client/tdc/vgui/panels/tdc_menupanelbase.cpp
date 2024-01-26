#include "cbase.h"
#include "tdc_menupanelbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCMenuPanelBase::CTDCMenuPanelBase( vgui::Panel *parent, const char *panelName ) : EditablePanel( parent, panelName, scheme()->GetScheme( "ClientScheme_tf2c" ) )
{
	SetProportional( true );
	SetVisible( true );

	int x, y, width, height;
	//surface()->GetScreenSize(width, height);
	GetParent()->GetBounds( x, y, width, height );
	SetBounds( 0, 0, width, height );
	m_bShowSingle = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCMenuPanelBase::~CTDCMenuPanelBase()
{
}

void CTDCMenuPanelBase::SetShowSingle( bool ShowSingle )
{
	m_bShowSingle = ShowSingle;
}

void CTDCMenuPanelBase::Show()
{
	SetVisible( true );
}

void CTDCMenuPanelBase::Hide()
{
	SetVisible( false );
}
