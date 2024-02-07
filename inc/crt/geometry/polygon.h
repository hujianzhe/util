//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_POLYGON_H
#define	UTIL_C_CRT_GEOMETRY_POLYGON_H

#include "geometry_def.h"

typedef struct GeometryRect_t {
	float o[3];
	float w_axis[3];
	float h_axis[3];
	float normal[3];
	float half_w;
	float half_h;
} GeometryRect_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathTriangleGetPoint(const float tri[3][3], float u, float v, float p[3]);
__declspec_dll int mathTrianglePointUV(const float tri[3][3], const float p[3], float* p_u, float* p_v);
__declspec_dll int mathTriangleHasPoint(const float tri[3][3], const float p[3]);
__declspec_dll void mathTriangleToPolygon(const float tri[3][3], GeometryPolygon_t* polygon);

__declspec_dll int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]);
__declspec_dll void mathRectVertices(const GeometryRect_t* rect, float p[4][3]);
__declspec_dll void mathRectToPolygon(const GeometryRect_t* rect, GeometryPolygon_t* polygon, float buf_points[4][3]);
__declspec_dll GeometryRect_t* mathAABBPlaneRect(const float o[3], const float half[3], unsigned int idx, GeometryRect_t* rect);
__declspec_dll GeometryRect_t* mathOBBPlaneRect(const GeometryOBB_t* obb, unsigned int idx, GeometryRect_t* rect);

__declspec_dll int mathPolygonIsConvex(const GeometryPolygon_t* polygon);
__declspec_dll int mathPolygonHasPoint(const GeometryPolygon_t* polygon, const float p[3]);
__declspec_dll int mathPolygonContainPolygon(const GeometryPolygon_t* polygon1, const GeometryPolygon_t* polygon2);
__declspec_dll GeometryPolygon_t* mathPolygonCooking(const float(*v)[3], unsigned int v_cnt, const unsigned int* tri_indices, unsigned int tri_indices_cnt, GeometryPolygon_t* polygon);
__declspec_dll void mathPolygonFreeCookingData(GeometryPolygon_t* polygon);

#ifdef	__cplusplus
}
#endif

#endif