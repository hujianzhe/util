//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_COLLISION_H
#define	UTIL_C_CRT_COLLISION_H

#include "geometry_def.h"

typedef struct CCTResult_t {
	float distance;
	int hit_point_cnt;
	float hit_point[3];
	float hit_normal[3];
} CCTResult_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll GeometryAABB_t* mathCollisionBoundingBox(const GeometryBodyRef_t* b, GeometryAABB_t* aabb);
__declspec_dll int mathCollisionContain(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two);
__declspec_dll int mathCollisionIntersect(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two);
__declspec_dll CCTResult_t* mathCollisionSweep(const GeometryBodyRef_t* one, const float dir[3], const GeometryBodyRef_t* two, CCTResult_t* result);

#ifdef	__cplusplus
}
#endif

#endif
