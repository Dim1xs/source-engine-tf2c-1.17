//========= Copyright © 1996-2007, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef C_FUNC_CAPTURE_ZONE_H
#define C_FUNC_CAPTURE_ZONE_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_CaptureZone : public C_BaseEntity, public TAutoList<C_CaptureZone>
{
public:
	DECLARE_CLASS( C_CaptureZone, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	bool IsDisabled( void ) { return m_bDisabled; }

private:
	bool m_bDisabled;
};

#endif // C_FUNC_CAPTURE_ZONE_H
