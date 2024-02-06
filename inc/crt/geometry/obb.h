//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_OBB_H
#define	UTIL_C_CRT_GEOMETRY_OBB_H

#include "geometry_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll GeometryOBB_t* mathOBBFromAABB(GeometryOBB_t* obb, const float o[3], const float half[3]);
__declspec_dll void mathOBBToAABB(const GeometryOBB_t* obb, float o[3], float half[3]);

__declspec_dll void mathOBBVertices(const GeometryOBB_t* obb, float v[8][3]);
__declspec_dll void mathOBBMinVertice(const GeometryOBB_t* obb, float v[3]);
__declspec_dll void mathOBBMaxVertice(const GeometryOBB_t* obb, float v[3]);

__declspec_dll void mathOBBPlaneVertices(const GeometryOBB_t* obb, float v[6][3]);

__declspec_dll int mathOBBHasPoint(const GeometryOBB_t* obb, const float p[3]);
__declspec_dll int mathOBBIntersectOBB(const GeometryOBB_t* obb0, const GeometryOBB_t* obb1);
__declspec_dll int mathOBBContainOBB(const GeometryOBB_t* obb0, const GeometryOBB_t* obb1);

#ifdef	__cplusplus
}
#endif

#endif
