//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Responsible for drawing the scene
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "tdc_viewrender.h"
#include "viewpostprocess.h"
#include <game/client/iviewport.h>
#include "tdc_hud_screenoverlays.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CTDCViewRender g_ViewRender;

CTDCViewRender::CTDCViewRender()
{
	view = ( IViewRender * )this;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewRender::Render2DEffectsPreHUD( const CViewSetup &view )
{
	BaseClass::Render2DEffectsPreHUD( view );
	g_ScreenOverlayManager.DrawOverlays( view );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTDCViewRender::Render2DEffectsPostHUD( const CViewSetup &view )
{
	BaseClass::Render2DEffectsPostHUD( view );

#if 0
	// if we're in the intro menus
	if ( gViewPortInterface->GetActivePanel() != NULL )
	{
		DoEnginePostProcessing( view.x, view.y, view.width, view.height, false, true );
	}
#endif
}
