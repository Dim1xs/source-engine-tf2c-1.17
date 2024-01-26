//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TDC_VIEWRENDER_H
#define TDC_VIEWRENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "iviewrender.h"
#include "viewrender.h"

//-----------------------------------------------------------------------------
// Purpose: Implements the interview to view rendering for the client .dll
//-----------------------------------------------------------------------------
class CTDCViewRender : public CViewRender
{
public:
	DECLARE_CLASS_GAMEROOT( CTDCViewRender, CViewRender );

	CTDCViewRender();

	virtual void Render2DEffectsPreHUD( const CViewSetup &view );
	virtual void Render2DEffectsPostHUD( const CViewSetup &view );
};

#endif //TDC_VIEWRENDER_H