//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef FUNC_FLAG_ALERT_H
#define FUNC_FLAG_ALERT_H

#ifdef _WIN32
#pragma once
#endif

#include "modelentities.h"

class CFuncForceField : public CFuncBrush
{
public:
	DECLARE_CLASS( CFuncForceField, CFuncBrush );
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual int UpdateTransmitState( void );
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;
	virtual void TurnOn( void );
	virtual void TurnOff( void );

	void SetActive( bool bActive );
};

#endif // FUNC_FLAG_ALERT_H
