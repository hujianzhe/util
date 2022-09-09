//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_COLLISION_INTERSECT_H
#define	UTIL_C_CRT_COLLISION_INTERSECT_H

#include "geometry_def.h"

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll GeometryAABB_t* mathCollisionBodyBoundingBox(const GeometryBodyRef_t* b, const float delta_half_v[3], GeometryAABB_t* aabb);

__declspec_dll int mathCollisionBodyIntersect(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two);

#ifdef	__cplusplus
}
#endif

#endif
