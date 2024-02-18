//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/vertex.h"
#include "../../../inc/crt/geometry/line_segment.h"
#include "../../../inc/crt/geometry/plane.h"
#include "../../../inc/crt/geometry/sphere.h"
#include "../../../inc/crt/geometry/aabb.h"
#include "../../../inc/crt/geometry/obb.h"
#include "../../../inc/crt/geometry/polygon.h"
#include "../../../inc/crt/geometry/mesh.h"
#include "../../../inc/crt/geometry/collision.h"
#include <stddef.h>

static int OBB_Contain_Sphere(const GeometryOBB_t* obb, const float o[3], float radius) {
	int i;
	float v[3];
	mathVec3Sub(v, o, obb->o);
	for (i = 0; i < 3; ++i) {
		float dot = mathVec3Dot(v, obb->axis[i]);
		if (dot < 0.0f) {
			dot = -dot;
		}
		if (dot > obb->half[i] - radius) {
			return 0;
		}
	}
	return 1;
}

static int AABB_Contain_Mesh(const float o[3], const float half[3], const GeometryMesh_t* mesh) {
	unsigned int i;
	if (mathAABBContainAABB(o, half, mesh->bound_box.o, mesh->bound_box.half)) {
		return 1;
	}
	for (i = 0; i < mesh->v_indices_cnt; ++i) {
		const float* p = mesh->v[mesh->v_indices[i]];
		if (!mathAABBHasPoint(o, half, p)) {
			return 0;
		}
	}
	return 1;
}

static int Sphere_Contain_Mesh(const float o[3], float radius, const GeometryMesh_t* mesh) {
	float v[3];
	unsigned int i;
	mathAABBMinVertice(mesh->bound_box.o, mesh->bound_box.half, v);
	if (mathSphereHasPoint(o, radius, v)) {
		mathAABBMaxVertice(mesh->bound_box.o, mesh->bound_box.half, v);
		if (mathSphereHasPoint(o, radius, v)) {
			return 1;
		}
	}
	for (i = 0; i < mesh->v_indices_cnt; ++i) {
		const float* p = mesh->v[mesh->v_indices[i]];
		if (!mathSphereHasPoint(o, radius, p)) {
			return 0;
		}
	}
	return 1;
}

static int OBB_Contain_Mesh(const GeometryOBB_t* obb, const GeometryMesh_t* mesh) {
	float p[8][3];
	unsigned int i;
	mathAABBVertices(mesh->bound_box.o, mesh->bound_box.half, p);
	for (i = 0; i < 8; ++i) {
		if (!mathOBBHasPoint(obb, p[i])) {
			break;
		}
	}
	if (i >= 8) {
		return 1;
	}
	for (i = 0; i < mesh->v_indices_cnt; ++i) {
		const float* p = mesh->v[mesh->v_indices[i]];
		if (!mathOBBHasPoint(obb, p)) {
			return 0;
		}
	}
	return 1;
}

static int ConvexMesh_Contain_AABB(const GeometryMesh_t* mesh, const float o[3], const float half[3]) {
	float p[8][3];
	unsigned int i;
	if (!mathAABBContainAABB(mesh->bound_box.o, mesh->bound_box.half, o, half)) {
		return 0;
	}
	mathAABBVertices(o, half, p);
	for (i = 0; i < 8; ++i) {
		if (!mathConvexMeshHasPoint(mesh, p[i])) {
			return 0;
		}
	}
	return 1;
}

static int ConvexMesh_Contain_OBB(const GeometryMesh_t* mesh, const GeometryOBB_t* obb) {
	float p[8][3];
	unsigned int i;
	mathOBBVertices(obb, p);
	for (i = 0; i < 8; ++i) {
		if (!mathConvexMeshHasPoint(mesh, p[i])) {
			return 0;
		}
	}
	return 1;
}

static int ConvexMesh_Contain_Sphere(const GeometryMesh_t* mesh, const float o[3], float radius) {
	unsigned int i;
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		float d;
		const GeometryPolygon_t* polygon = mesh->polygons + i;
		mathPointProjectionPlane(o, polygon->v[polygon->v_indices[0]], polygon->normal, NULL, &d);
		if (d < radius) {
			return 0;
		}
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

GeometryAABB_t* mathCollisionBoundingBox(const GeometryBodyRef_t* b, GeometryAABB_t* aabb) {
	switch (b->type) {
		case GEOMETRY_BODY_AABB:
		{
			*aabb = *(b->aabb);
			break;
		}
		case GEOMETRY_BODY_SPHERE:
		{
			mathVec3Copy(aabb->o, b->sphere->o);
			mathVec3Set(aabb->half, b->sphere->radius, b->sphere->radius, b->sphere->radius);
			break;
		}
		case GEOMETRY_BODY_OBB:
		{
			mathOBBToAABB(b->obb, aabb->o, aabb->half);
			break;
		}
		case GEOMETRY_BODY_POLYGON:
		{
			float min_v[3], max_v[3];
			const GeometryPolygon_t* polygon = b->polygon;
			if (!mathVertexIndicesFindMaxMinXYZ((const float(*)[3])polygon->v, polygon->v_indices, polygon->v_indices_cnt, min_v, max_v)) {
				return NULL;
			}
			mathAABBFromTwoVertice(min_v, max_v, aabb->o, aabb->half);
			break;
		}
		case GEOMETRY_BODY_CONVEX_MESH:
		{
			*aabb = b->mesh->bound_box;
			break;
		}
		default:
		{
			return NULL;
		}
	}
	return aabb;
}

int mathCollisionContain(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two) {
	if (one->data == two->data) {
		return 1;
	}
	if (GEOMETRY_BODY_AABB == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathAABBHasPoint(one->aabb->o, one->aabb->half, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return	mathAABBHasPoint(one->aabb->o, one->aabb->half, two->segment->v[0]) &&
				mathAABBHasPoint(one->aabb->o, one->aabb->half, two->segment->v[1]);
		}
		case GEOMETRY_BODY_AABB:
		{
			return mathAABBContainAABB(one->aabb->o, one->aabb->half, two->aabb->o, two->aabb->half);
		}
		case GEOMETRY_BODY_OBB:
		{
			GeometryOBB_t one_obb;
			mathOBBFromAABB(&one_obb, one->aabb->o, one->aabb->half);
			return mathOBBContainOBB(&one_obb, two->obb);
		}
		case GEOMETRY_BODY_SPHERE:
		{
			GeometryOBB_t one_obb;
			mathOBBFromAABB(&one_obb, one->aabb->o, one->aabb->half);
			return OBB_Contain_Sphere(&one_obb, two->sphere->o, two->sphere->radius);
		}
		case GEOMETRY_BODY_CONVEX_MESH:
		{
			return AABB_Contain_Mesh(one->aabb->o, one->aabb->half, two->mesh);
		}
		}
	}
	else if (GEOMETRY_BODY_OBB == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathOBBHasPoint(one->obb, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return	mathOBBHasPoint(one->obb, two->segment->v[0]) &&
				mathOBBHasPoint(one->obb, two->segment->v[1]);
		}
		case GEOMETRY_BODY_AABB:
		{
			GeometryOBB_t two_obb;
			mathOBBFromAABB(&two_obb, two->aabb->o, two->aabb->half);
			return mathOBBContainOBB(one->obb, &two_obb);
		}
		case GEOMETRY_BODY_OBB:
		{
			return mathOBBContainOBB(one->obb, two->obb);
		}
		case GEOMETRY_BODY_SPHERE:
		{
			return OBB_Contain_Sphere(one->obb, two->sphere->o, two->sphere->radius);
		}
		case GEOMETRY_BODY_CONVEX_MESH:
		{
			return OBB_Contain_Mesh(one->obb, two->mesh);
		}
		}
	}
	else if (GEOMETRY_BODY_SPHERE == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathSphereHasPoint(one->sphere->o, one->sphere->radius, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return	mathSphereHasPoint(one->sphere->o, one->sphere->radius, two->segment->v[0]) &&
				mathSphereHasPoint(one->sphere->o, one->sphere->radius, two->segment->v[1]);
		}
		case GEOMETRY_BODY_AABB:
		{
			float v[3];
			mathAABBMinVertice(two->aabb->o, two->aabb->half, v);
			if (!mathSphereHasPoint(one->sphere->o, one->sphere->radius, v)) {
				return 0;
			}
			mathAABBMaxVertice(two->aabb->o, two->aabb->half, v);
			return mathSphereHasPoint(one->sphere->o, one->sphere->radius, v);
		}
		case GEOMETRY_BODY_OBB:
		{
			float v[3];
			mathOBBMinVertice(two->obb, v);
			if (!mathSphereHasPoint(one->sphere->o, one->sphere->radius, v)) {
				return 0;
			}
			mathOBBMaxVertice(two->obb, v);
			return mathSphereHasPoint(one->sphere->o, one->sphere->radius, v);
		}
		case GEOMETRY_BODY_SPHERE:
		{
			return mathSphereContainSphere(one->sphere->o, one->sphere->radius, two->sphere->o, two->sphere->radius);
		}
		case GEOMETRY_BODY_CONVEX_MESH:
		{
			return Sphere_Contain_Mesh(one->sphere->o, one->sphere->radius, two->mesh);
		}
		}
	}
	else if (GEOMETRY_BODY_CONVEX_MESH == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathConvexMeshHasPoint(one->mesh, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return	mathConvexMeshHasPoint(one->mesh, two->segment->v[0]) &&
				mathConvexMeshHasPoint(one->mesh, two->segment->v[1]);
		}
		case GEOMETRY_BODY_AABB:
		{
			return ConvexMesh_Contain_AABB(one->mesh, two->aabb->o, two->aabb->half);
		}
		case GEOMETRY_BODY_OBB:
		{
			return ConvexMesh_Contain_OBB(one->mesh, two->obb);
		}
		case GEOMETRY_BODY_SPHERE:
		{
			return ConvexMesh_Contain_Sphere(one->mesh, two->sphere->o, two->sphere->radius);
		}
		case GEOMETRY_BODY_CONVEX_MESH:
		{
			return mathConvexMeshContainConvexMesh(one->mesh, two->mesh);
		}
		}
	}
	else if (GEOMETRY_BODY_POLYGON == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathPolygonHasPoint(one->polygon, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return	mathPolygonHasPoint(one->polygon, two->segment->v[0]) &&
				mathPolygonHasPoint(one->polygon, two->segment->v[1]);
		}
		case GEOMETRY_BODY_POLYGON:
		{
			return mathPolygonContainPolygon(one->polygon, two->polygon);
		}
		}
	}
	else if (GEOMETRY_BODY_PLANE == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathPlaneHasPoint(one->plane->v, one->plane->normal, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return	mathPlaneHasPoint(one->plane->v, one->plane->normal, two->segment->v[0]) &&
				mathPlaneHasPoint(one->plane->v, one->plane->normal, two->segment->v[1]);
		}
		case GEOMETRY_BODY_PLANE:
		{
			return mathPlaneIntersectPlane(one->plane->v, one->plane->normal, two->plane->v, two->plane->normal) == 2;
		}
		case GEOMETRY_BODY_POLYGON:
		{
			const GeometryPolygon_t* polygon = two->polygon;
			return mathPlaneIntersectPlane(one->plane->v, one->plane->normal, polygon->v[polygon->v_indices[0]], polygon->normal) == 2;
		}
		}
	}
	else if (GEOMETRY_BODY_SEGMENT == one->type) {
		switch (two->type) {
		case GEOMETRY_BODY_POINT:
		{
			return mathSegmentHasPoint((const float(*)[3])one->segment->v, two->point);
		}
		case GEOMETRY_BODY_SEGMENT:
		{
			return mathSegmentContainSegment((const float(*)[3])one->segment->v, (const float(*)[3])two->segment->v);
		}
		}
	}
	else if (GEOMETRY_BODY_POINT == one->type) {
		if (GEOMETRY_BODY_POINT == two->type) {
			return mathVec3Equal(one->point, two->point);
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
