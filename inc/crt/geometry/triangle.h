//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_TRIANGLE_H
#define	UTIL_C_CRT_GEOMETRY_TRIANGLE_H

#include "geometry_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathTriangleGetPoint(const float tri[3][3], float u, float v, float p[3]);
__declspec_dll int mathTrianglePointUV(const float tri[3][3], const float p[3], float* p_u, float* p_v);
__declspec_dll int mathTriangleHasPoint(const float tri[3][3], const float p[3]);
__declspec_dll void mathTriangleToPolygen(const float tri[3][3], GeometryPolygen_t* polygen);

__declspec_dll int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]);
__declspec_dll void mathRectVertices(const GeometryRect_t* rect, float p[4][3]);
__declspec_dll void mathRectToPolygen(const GeometryRect_t* rect, GeometryPolygen_t* polygen, float p[4][3]);

__declspec_dll int mathPolygenHasPoint(const GeometryPolygen_t* polygen, const float p[3]);
__declspec_dll int mathMeshVerticesToAABB(const float (*v)[3], const unsigned int* v_indices, unsigned int v_indices_cnt, float o[3], float half[3]);

__declspec_dll int mathTriangleMeshCooking(const float (*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryTriangleMesh_t* mesh);
__declspec_dll void mathTriangleMeshFreeData(GeometryTriangleMesh_t* mesh);

#ifdef	__cplusplus
}
#endif

#endif
