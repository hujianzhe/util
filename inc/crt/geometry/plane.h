//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_PLANE_H
#define	UTIL_C_CRT_GEOMETRY_PLANE_H

#include "geometry_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathPointProjectionPlane(const float p[3], const float plane_v[3], const float plane_n[3], float np[3], float* distance);
__declspec_dll float mathPlaneNormalByVertices3(const float v0[3], const float v1[3], const float v2[3], float normal[3]);
__declspec_dll int mathPlaneHasPoint(const float plane_v[3], const float plane_normal[3], const float p[3]);
__declspec_dll int mathPlaneIntersectPlane(const float v1[3], const float n1[3], const float v2[3], const float n2[3]);

#ifdef	__cplusplus
}
#endif

#endif
