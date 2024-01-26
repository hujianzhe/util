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
__declspec_dll void mathTriangleToPolygon(const float tri[3][3], GeometryPolygon_t* polygon);

__declspec_dll int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]);
__declspec_dll void mathRectVertices(const GeometryRect_t* rect, float p[4][3]);
__declspec_dll void mathRectToPolygon(const GeometryRect_t* rect, GeometryPolygon_t* polygon, float p[4][3]);

__declspec_dll unsigned int mathVerticesDistinctCount(const float(*src_v)[3], unsigned int src_v_cnt);
__declspec_dll unsigned int mathVerticesMerge(const float(*src_v)[3], unsigned int src_v_cnt, float(*dst_v)[3], unsigned int* indices, unsigned int indices_cnt);

__declspec_dll int mathPolygonIsConvex(const float(*v)[3], const unsigned int* v_indices, unsigned int v_indices_cnt);
__declspec_dll int mathPolygonHasPoint(const GeometryPolygon_t* polygon, const float p[3]);
__declspec_dll int mathPolygonCooking(const float(*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygon_t* polygon);
__declspec_dll void mathPolygonFreeCookingData(GeometryPolygon_t* polygon);

__declspec_dll int mathMeshCooking(const float (*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryMesh_t* mesh);
__declspec_dll void mathMeshFreeCookingData(GeometryMesh_t* mesh);
__declspec_dll int mathMeshIsClosed(const GeometryMesh_t* mesh);
__declspec_dll int mathMeshIsConvex(const GeometryMesh_t* mesh);

#ifdef	__cplusplus
}
#endif

#endif
