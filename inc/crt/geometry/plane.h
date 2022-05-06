//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_PLANE_H
#define	UTIL_C_CRT_GEOMETRY_PLANE_H

#include "../../compiler_define.h"

typedef struct GeometryPlane_t {
	float v[3];
	float normal[3];
} GeometryPlane_t;

typedef struct GeometryRect_t {
	float o[3];
	float h_axis[3];
	float normal[3];
	float half_w;
	float half_h;
} GeometryRect_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathPointProjectionPlane(const float p[3], const float plane_v[3], const float plane_n[3], float np[3], float* distance);
__declspec_dll void mathPlaneNormalByVertices3(const float vertices[3][3], float normal[3]);
__declspec_dll int mathPlaneHasPoint(const float plane_v[3], const float plane_normal[3], const float p[3]);
__declspec_dll int mathPlaneIntersectPlane(const float v1[3], const float n1[3], const float v2[3], const float n2[3]);

__declspec_dll int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]);
__declspec_dll void mathRectVertices(const GeometryRect_t* rect, float p[4][3]);

#ifdef	__cplusplus
}
#endif

#endif
