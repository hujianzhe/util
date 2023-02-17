//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_SPHERE_H
#define	UTIL_C_CRT_GEOMETRY_SPHERE_H

#include "geometry_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int mathSphereHasPoint(const float o[3], float radius, const float p[3]);
__declspec_dll int mathSphereIntersectSphere(const float o1[3], float r1, const float o2[3], float r2, float p[3]);
__declspec_dll int mathSphereContainSphere(const float o1[3], float r1, const float o2[3], float r2);

#ifdef	__cplusplus
}
#endif

#endif
