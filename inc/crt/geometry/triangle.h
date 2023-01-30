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
	const float (*v)[3]; /* vertices vec3 */
	float normal[3];
	unsigned int edge_indices_cnt; /* number of edge vertices index */
	unsigned int tri_indices_cnt;  /* number of triangle vertices index */
	const unsigned int* edge_indices; /* edge vertices index, must be ordered(clockwise or counterclockwise) */
	const unsigned int* tri_indices; /* triangle vertices index */
} GeometryPolygen_t;

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

__declspec_dll int mathPolygenEdgeCooking(const float (*v)[3], const unsigned int* tri_indices, unsigned int tri_indices_cnt, unsigned int** edge_indices, unsigned int* edge_indices_cnt);

#ifdef	__cplusplus
}
#endif

#endif
