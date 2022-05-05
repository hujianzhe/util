//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_GEOMETRY_DEF_H
#define	UTIL_C_CRT_GEOMETRY_GEOMETRY_DEF_H

#include "../../compiler_define.h"
#include "line_segment.h"
#include "plane.h"
#include "sphere.h"
#include "aabb.h"
#include "triangle.h"

enum GeometryBodyType {
	GEOMETRY_BODY_POINT = 1,
	GEOMETRY_BODY_SEGMENT = 2,
	GEOMETRY_BODY_PLANE = 3,
	GEOMETRY_BODY_SPHERE = 4,
	GEOMETRY_BODY_AABB = 5,
};

typedef struct GeometryBody_t {
	union {
		float point[3];
		GeometrySegment_t segment;
		GeometryPlane_t plane;
		GeometrySphere_t sphere;
		GeometryAABB_t aabb;
	};
	int type;
} GeometryBody_t;

typedef struct GeometryBodyRef_t {
	union {
		const float* point;
		const GeometrySegment_t* segment;
		const GeometryPlane_t* plane;
		const GeometrySphere_t* sphere;
		const GeometryAABB_t* aabb;
	};
	int type;
} GeometryBodyRef_t;

#endif
