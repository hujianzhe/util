//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_TRIANGLE_H
#define	UTIL_C_CRT_GEOMETRY_TRIANGLE_H

#include "../../compiler_define.h"

typedef struct GeometryRect_t {
	float o[3];
	float w_axis[3];
	float h_axis[3];
	float normal[3];
	float half_w;
	float half_h;
} GeometryRect_t;

typedef struct GeometryPolygen_t {
	const float (*v)[3]; // vertices data must be ordered(clockwise or counterclockwise)
	unsigned int v_cnt;
	float normal[3];
	unsigned int* indices; // v_cnt > 3, need set field
	unsigned int indices_cnt; // v_cnt > 3, need set field
} GeometryPolygen_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathTriangleGetPoint(const float tri[3][3], float u, float v, float p[3]);
__declspec_dll int mathTrianglePointUV(const float tri[3][3], const float p[3], float* p_u, float* p_v);
__declspec_dll int mathTriangleHasPoint(const float tri[3][3], const float p[3]);

__declspec_dll int mathRectHasPoint(const GeometryRect_t* rect, const float p[3]);
__declspec_dll void mathRectVertices(const GeometryRect_t* rect, float p[4][3]);
__declspec_dll GeometryRect_t* mathRectFromVertices4(GeometryRect_t* rect, const float p[4][3]);

__declspec_dll int mathPolygenHasPoint(const GeometryPolygen_t* polygen, const float p[3]);

#ifdef	__cplusplus
}
#endif

#endif
