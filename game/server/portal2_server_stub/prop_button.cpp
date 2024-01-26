// stub for shit
// nigler gay

#include "cbase.h"
#include "props.h"

// realman
#include "tier0/memdbgon.h"

class CPropButton : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropButton, CBaseAnimating );

	void Spawn()
	{
		Precache();
		SetModel( "models/props/switch001.mdl" );
		SetSolid( SOLID_VPHYSICS );
	};

	void Precache()
	{
		PrecacheModel( "models/props/switch001.mdl" );
	};
};
LINK_ENTITY_TO_CLASS( prop_button, CPropButton );

class CPropFloorButton : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropFloorButton, CBaseAnimating );
	DECLARE_DATADESC();

	void Spawn()
	{
		Precache();
//		SetModel( !FStrEq( m_iModel, "" ) ? m_iModel : "models/props/portal_button.mdl" );
		SetModel( "models/props/portal_button.mdl" );
		SetSolid( SOLID_VPHYSICS );
	};

	void Precache()
	{
//		PrecacheModel( !FStrEq( m_iModel, "" ) ? m_iModel : "models/props/portal_button.mdl" );
		PrecacheModel( "models/props/portal_button.mdl" );
	};
private:
	const char	*m_iModel;
};
BEGIN_DATADESC( CPropFloorButton )
	DEFINE_KEYFIELD( m_iModel, FIELD_STRING, "model" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_floor_button, CPropFloorButton );

class CPropFloorCubeButton : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropFloorCubeButton, CBaseAnimating );
	void Spawn()
	{
		Precache();
		SetModel( "models/props/box_socket.mdl" );
		SetSolid( SOLID_VPHYSICS );
	};
	void Precache()
	{
		PrecacheModel( "models/props/box_socket.mdl" );
	};
};
LINK_ENTITY_TO_CLASS( prop_floor_cube_button, CPropFloorCubeButton );

class CPropFloorBallButton : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropFloorBallButton, CBaseAnimating );
	void Spawn()
	{
		Precache();
		SetModel( "models/props/ball_button.mdl" );
		SetSolid( SOLID_VPHYSICS );
	};
	void Precache()
	{
		PrecacheModel( "models/props/ball_button.mdl" );
	};
};
LINK_ENTITY_TO_CLASS( prop_floor_ball_button, CPropFloorBallButton );


// UNDER
class CPropUnderButton : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropUnderButton, CBaseAnimating );

	void Spawn()
	{
		Precache();
		SetModel( "models/props_underground/underground_testchamber_button.mdl" );
		SetSolid( SOLID_VPHYSICS );
	};

	void Precache()
	{
		PrecacheModel( "models/props_underground/underground_testchamber_button.mdl" );
	};
};
LINK_ENTITY_TO_CLASS( prop_under_button, CPropUnderButton );

class CPropUnderFloorButton : public CBaseAnimating
{
public:
	DECLARE_CLASS( CPropUnderFloorButton, CBaseAnimating );

	void Spawn()
	{
		Precache();
		SetModel( "models/props_underground/underground_floor_button.mdl" );
		SetSolid( SOLID_VPHYSICS );
	};

	void Precache()
	{
		PrecacheModel( "models/props_underground/underground_floor_button.mdl" );
	};
};
LINK_ENTITY_TO_CLASS( prop_under_floor_button, CPropUnderFloorButton );

