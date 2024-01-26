#ifndef C_TDC_FX_H
#define C_TDC_FX_H

#ifdef _WIN32
#pragma once
#endif

void TE_BoltImpact( const Vector &vecPos, const Vector &vecDir, ProjectileType_t iType, int nModelIndex, CBaseEntity *pEntity = NULL, int nFlags = 0, const Vector &vecPinPos = vec3_origin, int iBone = -1, const Vector &vecBonePos = vec3_origin, const QAngle &vecBoneAngles = vec3_angle );

#endif // C_TDC_FX_H