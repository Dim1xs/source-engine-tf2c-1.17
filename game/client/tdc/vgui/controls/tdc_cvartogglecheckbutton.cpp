//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tdc_cvartogglecheckbutton.h"
//#include "EngineInterface.h"
#include <vgui/IVGui.h>
#include "tier1/KeyValues.h"
#include "tier1/convar.h"
#include "IGameUIFuncs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

vgui::Panel *CTDCCvarToggleCheckButton_Factory()
{
	return new CTDCCvarToggleCheckButton( NULL, NULL );
}
DECLARE_BUILD_FACTORY_CUSTOM( CTDCCvarToggleCheckButton, CTDCCvarToggleCheckButton_Factory );

CTDCCvarToggleCheckButton::CTDCCvarToggleCheckButton( Panel *parent, const char *panelName, const char *text,
	char const *cvarname, bool ignoreMissingCvar )
	: CTDCCheckButton( parent, panelName, text ), m_cvar( ( cvarname ) ? cvarname : "", ( cvarname ) ? ignoreMissingCvar : true )
{
	m_bAutoChange = false;

	Reset();
	AddActionSignalTarget( this );

	ivgui()->AddTickSignal( GetVPanel() );
}

CTDCCvarToggleCheckButton::~CTDCCvarToggleCheckButton()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCCvarToggleCheckButton::OnTick()
{
	// Check if cvar value was changed from under this.
	if ( !m_cvar.IsValid() )
		return;

	bool value = m_cvar.GetBool();

	if ( value != m_bStartValue )
	{
		SetChecked( value );
		m_bStartValue = value;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCCvarToggleCheckButton::OnApplyChanges()
{
	ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCCvarToggleCheckButton::ApplyChanges()
{
	if ( !m_cvar.IsValid() )
		return;

	m_bStartValue = IsChecked();
	m_cvar.SetValue( m_bStartValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCCvarToggleCheckButton::Reset()
{
	if ( !m_cvar.IsValid() )
		return;

	m_bStartValue = m_cvar.GetBool();
	SetChecked( m_bStartValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTDCCvarToggleCheckButton::HasBeenModified()
{
	return IsChecked() != m_bStartValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCCvarToggleCheckButton::OnButtonChecked()
{
	if ( HasBeenModified() )
	{
		PostActionSignal( new KeyValues( "ControlModified" ) );

		// If set to auto-change cvar, apply changes now.
		if ( m_bAutoChange )
		{
			ApplyChanges();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTDCCvarToggleCheckButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_bAutoChange = inResourceData->GetBool( "autochange" );

	const char *cvarName = inResourceData->GetString( "cvar_name", "" );
	const char *cvarValue = inResourceData->GetString( "cvar_value", "" );

	if ( cvarName[0] == '\0' )
		return;// Doesn't have cvar set up in res file, must have been constructed with it.

	if ( V_stricmp( cvarValue, "1" ) == 0 )
		m_bStartValue = true;
	else
		m_bStartValue = false;

	m_cvar.Init( cvarName, m_bIgnoreMissingCvar );

	if ( m_cvar.IsValid() )
	{
		SetChecked( m_cvar.GetBool() );
	}
}
