//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_AABB_H
#define	UTIL_C_CRT_GEOMETRY_AABB_H

#include "../../compiler_define.h"

typedef struct GeometryAABB_t {
	float o[3];
	float half[3];
} GeometryAABB_t;

#ifdef	__cplusplus
extern "C" {
#endif

extern int Box_Edge_Indices[24];
extern int Box_Triangle_Vertices_Indices[36];
extern float AABB_Plane_Normal[6][3];
extern float AABB_Rect_Axis[6][3];

__declspec_dll void AABBPlaneVertices(const float o[3], const float half[3], float v[6][3]);
__declspec_dll void AABBPlaneRectSizes(const float aabb_half[3], float half_w[6], float half_h[6]);
__declspec_dll void AABBVertices(const float o[3], const float half[3], float v[8][3]);
__declspec_dll float* AABBMinVertice(const float o[3], const float half[3], float v[3]);
__declspec_dll float* AABBMaxVertice(const float o[3], const float half[3], float v[3]);

__declspec_dll int mathAABBHasPoint(const float o[3], const float half[3], const float p[3]);
__declspec_dll void mathAABBClosestPointTo(const float o[3], const float half[3], const float p[3], float closest_p[3]);
__declspec_dll void mathAABBSplit(const float o[3], const float half[3], float new_o[8][3], float new_half[3]);
__declspec_dll int mathAABBIntersectAABB(const float o1[3], const float half1[3], const float o2[3], const float half2[3]);
__declspec_dll int mathAABBContainAABB(const float o1[3], const float half1[3], const float o2[3], const float half2[3]);

#ifdef	__cplusplus
}
#endif

#endif
