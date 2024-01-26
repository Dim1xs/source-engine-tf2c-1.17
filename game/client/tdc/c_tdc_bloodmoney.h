//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef C_TDC_PD_ENTITIES_H
#define C_TDC_PD_ENTITIES_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_MoneyDeliveryZone : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_MoneyDeliveryZone, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_MoneyDeliveryZone();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	const Vector &GetIconPosition( void ) { return m_vecIconAbsOrigin; }

private:
	Vector m_vecIconOrigin;
	Vector m_vecIconAbsOrigin;
};

#endif // C_TDC_PD_ENTITIES_H
