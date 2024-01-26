//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "c_tdc_bloodmoney.h"

IMPLEMENT_CLIENTCLASS_DT( C_MoneyDeliveryZone, DT_MoneyDeliveryZone, CMoneyDeliveryZone )
	RecvPropVector( RECVINFO( m_vecIconOrigin ) ),
END_RECV_TABLE()

C_MoneyDeliveryZone::C_MoneyDeliveryZone()
{
	m_vecIconOrigin.Init();
	m_vecIconAbsOrigin.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MoneyDeliveryZone::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	EntityToWorldSpace( m_vecIconOrigin, &m_vecIconAbsOrigin );
}
