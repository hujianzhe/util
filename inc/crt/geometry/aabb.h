//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_AABB_H
#define	UTIL_C_CRT_GEOMETRY_AABB_H

#include "geometry_def.h"

#ifndef GEOMETRY_BODY_BOX_MIN_HALF
#define	GEOMETRY_BODY_BOX_MIN_HALF	(1e-5f + 1e-5f)
#endif

#ifdef	__cplusplus
extern "C" {
#endif

extern const unsigned int Box_Edge_Indices[24];
extern const unsigned int Box_Vertice_Adjacent_Indices[8][3];
extern const unsigned int Box_Triangle_Vertices_Indices[36];
extern const float AABB_Plane_Normal[6][3];

__declspec_dll void mathAABBPlaneVertices(const float o[3], const float half[3], float v[6][3]);

__declspec_dll void mathAABBVertices(const float o[3], const float half[3], float v[8][3]);
__declspec_dll void mathAABBMinVertice(const float o[3], const float half[3], float v[3]);
__declspec_dll void mathAABBMaxVertice(const float o[3], const float half[3], float v[3]);
__declspec_dll void mathAABBFromTwoVertice(const float a[3], const float b[3], float o[3], float half[3]);

__declspec_dll int mathAABBHasPoint(const float o[3], const float half[3], const float p[3]);
__declspec_dll void mathAABBClosestPointTo(const float o[3], const float half[3], const float p[3], float closest_p[3]);
__declspec_dll void mathAABBStretch(float o[3], float half[3], const float delta[3]);
__declspec_dll void mathAABBSplit(const float o[3], const float half[3], float new_o[8][3], float new_half[3]);
__declspec_dll int mathAABBIntersectAABB(const float o1[3], const float half1[3], const float o2[3], const float half2[3]);
__declspec_dll int mathAABBContainAABB(const float o1[3], const float half1[3], const float o2[3], const float half2[3]);

#ifdef	__cplusplus
}
#endif

#endif
