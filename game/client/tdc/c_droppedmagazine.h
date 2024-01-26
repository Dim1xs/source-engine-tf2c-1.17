//=============================================================================
//
//	Ejected magazine.
//
//=============================================================================
#ifndef C_DROPPEDMAGAZINE_H
#define C_DROPPEDMAGAZINE_H

#ifdef _WIN32
#pragma once
#endif

#include "physpropclientside.h"

extern ConVar tdc_ejectmag_max_count;

class C_DroppedMagazine : public C_PhysPropClientside, public TAutoList<C_DroppedMagazine>
{
public:
	static C_DroppedMagazine *Create( const char *pszModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, C_BaseCombatWeapon *pWeapon );
};

#endif // C_DROPPEDMAGAZINE_H
