//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_GEOMETRY_DEF_H
#define	UTIL_C_CRT_GEOMETRY_GEOMETRY_DEF_H

#include "../../compiler_define.h"

/*********************************************************************/

typedef struct GeometrySegment_t {
	float v[2][3];
} GeometrySegment_t;

typedef struct GeometryPlane_t {
	float v[3];
	float normal[3];
} GeometryPlane_t;

typedef struct GeometrySphere_t {
	float o[3];
	float radius;
} GeometrySphere_t;

typedef struct GeometryAABB_t {
	float o[3];
	float half[3];
} GeometryAABB_t;

typedef struct GeometryOBB_t {
	float o[3];
	float half[3];
	float axis[3][3];
} GeometryOBB_t;

typedef struct GeometryRect_t {
	float o[3];
	float w_axis[3];
	float h_axis[3];
	float normal[3];
	float half_w;
	float half_h;
} GeometryRect_t;

typedef struct GeometryPolygen_t {
	float (*v)[3]; /* vertices vec3 */
	float normal[3];
	unsigned int v_indices_cnt; /* number of edge vertices index */
	unsigned int tri_indices_cnt;  /* number of triangle vertices index */
	const unsigned int* v_indices; /* edge vertices index, must be ordered(clockwise or counterclockwise) */
	const unsigned int* tri_indices; /* triangle vertices index */
} GeometryPolygen_t;

typedef struct GeometryTriangleMesh_t {
	float (*v)[3]; /* vertices vec3 */
	unsigned int polygens_cnt; /* number of polygen plane */
	unsigned int edge_indices_cnt; /* number of edge vertices index */
	unsigned int v_indices_cnt; /* number of triangle vertices index */
	GeometryPolygen_t* polygens; /* array of polygens */
	const unsigned int* edge_indices; /* edge vertices index */
	const unsigned int* v_indices; /* triangle vertices index */
} GeometryTriangleMesh_t;

/*********************************************************************/

enum GeometryBodyType {
	GEOMETRY_BODY_POINT = 1,
	GEOMETRY_BODY_SEGMENT = 2,
	GEOMETRY_BODY_PLANE = 3,
	GEOMETRY_BODY_SPHERE = 4,
	GEOMETRY_BODY_AABB = 5,
	GEOMETRY_BODY_POLYGEN = 7,
	GEOMETRY_BODY_OBB = 8,
	GEOMETRY_BODY_TRIANGLE_MESH = 9,
};

typedef struct GeometryBody_t {
	union {
		float point[3];
		GeometrySegment_t segment;
		GeometryPlane_t plane;
		GeometrySphere_t sphere;
		GeometryAABB_t aabb;
		GeometryPolygen_t polygen;
		GeometryOBB_t obb;
		GeometryTriangleMesh_t mesh;
	};
	int type;
} GeometryBody_t;

typedef struct GeometryBodyRef_t {
	union {
		void* data;
		const float* point; /* float[3] */
		const GeometrySegment_t* segment;
		const GeometryPlane_t* plane;
		const GeometrySphere_t* sphere;
		const GeometryAABB_t* aabb;
		const GeometryPolygen_t* polygen;
		const GeometryOBB_t* obb;
		const GeometryTriangleMesh_t* mesh;
	};
	int type;
} GeometryBodyRef_t;

#endif
