#include "cbase.h"
#include "tdc_shadebackgroundpanel.h"
#include "tdc_mainmenu.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTDCShadeBackgroundPanel::CTDCShadeBackgroundPanel( Panel *parent, const char *panelName ) : CTDCMenuPanelBase( parent, panelName )
{
	m_pShadedBG = new ImagePanel( this, "ShadedBG" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTDCShadeBackgroundPanel::~CTDCShadeBackgroundPanel()
{
}

void CTDCShadeBackgroundPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/main_menu/ShadeBackgroundPanel.res" );
}

void CTDCShadeBackgroundPanel::Show()
{
	BaseClass::Show();
	GetAnimationController()->RunAnimationCommand( m_pShadedBG, "Alpha", 220, 0.0f, 0.3f, AnimationController::INTERPOLATOR_LINEAR );
}

void CTDCShadeBackgroundPanel::Hide()
{
	BaseClass::Hide();
	GetAnimationController()->RunAnimationCommand( m_pShadedBG, "Alpha", 0, 0.0f, 0.1f, AnimationController::INTERPOLATOR_LINEAR );
}
