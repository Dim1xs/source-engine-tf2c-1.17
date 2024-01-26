//=============================================================================//
//
// Purpose: A powerup entity which lets you select the powerup, duration, model
//			and pickup sound.
//
//			Initially we didn't want people to change any of these things on 
//			the regular powerup entities, as we felt like it'd cause people 
//			to make unbalanced maps. However, later we realized that by doing so
//			we were massively hindering the creativity of map makers, which is
//			why we decided to make this separate entity
//		
//=============================================================================//

#ifndef POWERUP_CUSTOM_H
#define POWERUP_CUSTOM_H

#ifdef _WIN32
#pragma once
#endif

#include "tdc_powerupbase.h"

//=============================================================================

class CTDCPowerupCustom : public CTDCPowerupBase
{
public:
	DECLARE_CLASS( CTDCPowerupCustom, CTDCPowerupBase );
	DECLARE_DATADESC();

	CTDCPowerupCustom();

	virtual ETDCCond	GetCondition( void ) { return m_iPowerupCondition; }

private:
	ETDCCond	m_iPowerupCondition;
};

#endif // POWERUP_CUSTOM_H
