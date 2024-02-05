//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/line_segment.h"
#include "../../../inc/crt/geometry/plane.h"
#include "../../../inc/crt/geometry/sphere.h"
#include "../../../inc/crt/geometry/aabb.h"
#include "../../../inc/crt/geometry/obb.h"
#include "../../../inc/crt/geometry/triangle.h"
#include "../../../inc/crt/geometry/collision_intersect.h"
#include <stddef.h>

int mathSegmentIntersectPlane(const float ls[2][3], const float plane_v[3], const float plane_normal[3], float p[3]) {
	int cmp[2];
	float d[2], lsdir[3], dot;
	mathVec3Sub(lsdir, ls[1], ls[0]);
	dot = mathVec3Dot(lsdir, plane_normal);
	if (fcmpf(dot, 0.0f, CCT_EPSILON) == 0) {
		return mathPlaneHasPoint(plane_v, plane_normal, ls[0]) ? 2 : 0;
	}
	mathPointProjectionPlane(ls[0], plane_v, plane_normal, NULL, &d[0]);
	mathPointProjectionPlane(ls[1], plane_v, plane_normal, NULL, &d[1]);
	cmp[0] = fcmpf(d[0], 0.0f, CCT_EPSILON);
	cmp[1] = fcmpf(d[1], 0.0f, CCT_EPSILON);
	if (cmp[0] * cmp[1] > 0) {
		return 0;
	}
	if (0 == cmp[0]) {
		if (p) {
			mathVec3Copy(p, ls[0]);
		}
		return 1;
	}
	if (0 == cmp[1]) {
		if (p) {
			mathVec3Copy(p, ls[1]);
		}
		return 1;
	}
	if (p) {
		mathVec3Normalized(lsdir, lsdir);
		dot = mathVec3Dot(lsdir, plane_normal);
		mathVec3AddScalar(mathVec3Copy(p, ls[0]), lsdir, d[0] / dot);
	}
	return 1;
}

static int mathSegmentIntersectPolygon(const float ls[2][3], const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathSegmentIntersectPlane(ls, polygon->v[polygon->v_indices[0]], polygon->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return mathPolygonHasPoint(polygon, p);
	}
	if (mathPolygonHasPoint(polygon, ls[0]) || mathPolygonHasPoint(polygon, ls[1])) {
		return 2;
	}
	for (i = 0; i < polygon->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygon->v[polygon->v_indices[i++]]);
		mathVec3Copy(edge[1], polygon->v[polygon->v_indices[i >= polygon->v_indices_cnt ? 0 : i]]);
		if (mathSegmentIntersectSegment(ls, (const float(*)[3])edge, NULL, NULL)) {
			return 2;
		}
	}
	return 0;
}

int mathSegmentIntersectConvexMesh(const float ls[2][3], const GeometryMesh_t* mesh) {
	if (mathConvexMeshHasPoint(mesh, ls[0])) {
		return 1;
	}
	if (mathConvexMeshHasPoint(mesh, ls[1])) {
		return 1;
	}
	return 0;
}

static int mathPolygonIntersectPolygon(const GeometryPolygon_t* polygon1, const GeometryPolygon_t* polygon2) {
	int i;
	if (!mathPlaneIntersectPlane(polygon1->v[polygon1->v_indices[0]], polygon1->normal, polygon2->v[polygon2->v_indices[0]], polygon2->normal)) {
		return 0;
	}
	for (i = 0; i < polygon1->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygon1->v[polygon1->v_indices[i++]]);
		mathVec3Copy(edge[1], polygon1->v[polygon1->v_indices[i >= polygon1->v_indices_cnt ? 0 : i]]);
		if (mathSegmentIntersectPolygon((const float(*)[3])edge, polygon2, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int mathPolygonIntersectConvexMesh(const GeometryPolygon_t* polygon, const GeometryMesh_t* mesh) {
	unsigned int i;
	for (i = 0; i < polygon->v_indices_cnt; ++i) {
		const float* p = polygon->v[polygon->v_indices[i]];
		if (mathConvexMeshHasPoint(mesh, p)) {
			return 1;
		}
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		if (mathPolygonIntersectPolygon(polygon, mesh->polygons + i)) {
			return 1;
		}
	}
	return 0;
}

static int mathPolygonIntersectPlane(const GeometryPolygon_t* polygon, const float plane_v[3], const float plane_n[3], float p[3]) {
	unsigned int i, has_gt0, has_le0, idx_0;
	if (!mathPlaneIntersectPlane(polygon->v[polygon->v_indices[0]], polygon->normal, plane_v, plane_n)) {
		return 0;
	}
	idx_0 = has_gt0 = has_le0 = 0;
	for (i = 0; i < polygon->v_indices_cnt; ++i) {
		float d;
		mathPointProjectionPlane(polygon->v[polygon->v_indices[i]], plane_v, plane_n, NULL, &d);
		if (d > CCT_EPSILON) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (d < CCT_EPSILON_NEGATE) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0) {
			return 2;
		}
		else {
			if (p) {
				mathVec3Copy(p, polygon->v[polygon->v_indices[i]]);
			}
			idx_0 = 1;
		}
	}
	return idx_0;
}

static int mathMeshIntersectPlane(const GeometryMesh_t* mesh, const float plane_v[3], const float plane_n[3], float p[3]) {
	unsigned int i, has_gt0 = 0, has_le0 = 0, idx_0 = 0;
	for (i = 0; i < mesh->v_indices_cnt; ++i) {
		float d;
		mathPointProjectionPlane(mesh->v[mesh->v_indices[i]], plane_v, plane_n, NULL, &d);
		if (d > CCT_EPSILON) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (d < CCT_EPSILON_NEGATE) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0) {
			return 2;
		}
		else {
			if (p) {
				mathVec3Copy(p, mesh->v[mesh->v_indices[i]]);
			}
			idx_0 = 1;
		}
	}
	return idx_0;
}

/*
static int mathSphereIntersectLine(const float o[3], float radius, const float ls_vertice[3], const float lsdir[3], float distance[2]) {
	int cmp;
	float vo[3], lp[3], lpo[3], lpolensq, radiussq, dot;
	mathVec3Sub(vo, o, ls_vertice);
	dot = mathVec3Dot(vo, lsdir);
	mathVec3AddScalar(mathVec3Copy(lp, ls_vertice), lsdir, dot);
	mathVec3Sub(lpo, o, lp);
	lpolensq = mathVec3LenSq(lpo);
	radiussq = radius * radius;
	cmp = fcmpf(lpolensq, radiussq, CCT_EPSILON);
	if (cmp > 0) {
		return 0;
	}
	else if (0 == cmp) {
		distance[0] = distance[1] = dot;
		return 1;
	}
	else {
		float d = sqrtf(radiussq - lpolensq);
		distance[0] = dot + d;
		distance[1] = dot - d;
		return 2;
	}
}
*/

int mathSphereIntersectSegment(const float o[3], float radius, const float ls[2][3], float p[3]) {
	int res;
	float closest_p[3], closest_v[3];
	mathSegmentClosestPointTo(ls, o, closest_p);
	mathVec3Sub(closest_v, closest_p, o);
	res = fcmpf(mathVec3LenSq(closest_v), radius * radius, CCT_EPSILON);
	if (res == 0) {
		if (p) {
			mathVec3Copy(p, closest_p);
		}
		return 1;
	}
	return res < 0 ? 2 : 0;
}

int mathSphereIntersectPlane(const float o[3], float radius, const float plane_v[3], const float plane_normal[3], float new_o[3], float* new_r) {
	int cmp;
	float pp[3], ppd, ppo[3], ppolensq;
	mathPointProjectionPlane(o, plane_v, plane_normal, pp, &ppd);
	mathVec3Sub(ppo, o, pp);
	ppolensq = mathVec3LenSq(ppo);
	cmp = fcmpf(ppolensq, radius * radius, CCT_EPSILON);
	if (cmp > 0) {
		return 0;
	}
	if (new_o) {
		mathVec3Copy(new_o, pp);
	}
	if (0 == cmp) {
		if (new_r) {
			*new_r = 0.0f;
		}
		return 1;
	}
	else {
		if (new_r) {
			*new_r = sqrtf(radius * radius - ppolensq);
		}
		return 2;
	}
}

static int mathSphereIntersectPolygon(const float o[3], float radius, const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathSphereIntersectPlane(o, radius, polygon->v[polygon->v_indices[0]], polygon->normal, p, NULL);
	if (0 == res) {
		return 0;
	}
	if (mathPolygonHasPoint(polygon, p)) {
		return res;
	}
	for (i = 0; i < polygon->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygon->v[polygon->v_indices[i++]]);
		mathVec3Copy(edge[1], polygon->v[polygon->v_indices[i >= polygon->v_indices_cnt ? 0 : i]]);
		res = mathSphereIntersectSegment(o, radius, (const float(*)[3])edge, p);
		if (res != 0) {
			return res;
		}
	}
	return 0;
}

static int mathSphereIntersectConvexMesh(const float o[3], float radius, const GeometryMesh_t* mesh, float p[3]) {
	unsigned int i;
	if (mathConvexMeshHasPoint(mesh, o)) {
		return 1;
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		int ret = mathSphereIntersectPolygon(o, radius, mesh->polygons + i, p);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

int mathSphereIntersectOBB(const float o[3], float radius, const GeometryOBB_t* obb) {
	int i;
	float v[3];
	mathVec3Sub(v, o, obb->o);
	for (i = 0; i < 3; ++i) {
		float dot = mathVec3Dot(v, obb->axis[i]);
		if (dot < 0.0f) {
			dot = -dot;
		}
		if (dot > obb->half[i] + radius) {
			return 0;
		}
	}
	return 1;
}

static int mathBoxIntersectPlane(const float vertices[8][3], const float plane_v[3], const float plane_n[3], float p[3]) {
	int i, has_gt0 = 0, has_le0 = 0, idx_0 = 0;
	for (i = 0; i < 8; ++i) {
		float d;
		mathPointProjectionPlane(vertices[i], plane_v, plane_n, NULL, &d);
		if (d > CCT_EPSILON) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (d < CCT_EPSILON_NEGATE) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0) {
			return 2;
		}
		else {
			if (p) {
				mathVec3Copy(p, vertices[i]);
			}
			idx_0 = 1;
		}
	}
	return idx_0;
}

static int mathOBBIntersectPlane(const GeometryOBB_t* obb, const float plane_v[3], const float plane_n[3], float p[3]) {
	float vertices[8][3];
	mathOBBVertices(obb, vertices);
	return mathBoxIntersectPlane((const float(*)[3])vertices, plane_v, plane_n, p);
}

static int mathAABBIntersectPlane(const float o[3], const float half[3], const float plane_v[3], const float plane_n[3], float p[3]) {
	float vertices[8][3];
	mathAABBVertices(o, half, vertices);
	return mathBoxIntersectPlane((const float(*)[3])vertices, plane_v, plane_n, p);
}

static int mathAABBIntersectSphere(const float aabb_o[3], const float aabb_half[3], const float sp_o[3], float sp_radius) {
	float closest_v[3];
	mathAABBClosestPointTo(aabb_o, aabb_half, sp_o, closest_v);
	mathVec3Sub(closest_v, closest_v, sp_o);
	return mathVec3LenSq(closest_v) <= sp_radius * sp_radius;
}

static int mathAABBIntersectSegment(const float o[3], const float half[3], const float ls[2][3]) {
	int i;
	GeometryPolygon_t polygon;
	if (mathAABBHasPoint(o, half, ls[0]) || mathAABBHasPoint(o, half, ls[1])) {
		return 1;
	}
	for (i = 0; i < 6; ++i) {
		GeometryRect_t rect;
		float p[4][3];
		mathAABBPlaneRect(o, half, i, &rect);
		mathRectToPolygon(&rect, &polygon, p);
		if (mathSegmentIntersectPolygon(ls, &polygon, NULL)) {
			return 1;
		}
	}
	return 0;
}

int mathOBBIntersectSegment(const GeometryOBB_t* obb, const float ls[2][3]) {
	int i;
	if (mathOBBHasPoint(obb, ls[0]) || mathOBBHasPoint(obb, ls[1])) {
		return 1;
	}
	for (i = 0; i < 6; ++i) {
		GeometryPolygon_t polygon;
		GeometryRect_t rect;
		float p[4][3];

		mathOBBPlaneRect(obb, i, &rect);
		mathRectToPolygon(&rect, &polygon, p);
		if (mathSegmentIntersectPolygon(ls, &polygon, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int mathAABBIntersectPolygon(const float o[3], const float half[3], const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathAABBIntersectPlane(o, half, polygon->v[polygon->v_indices[0]], polygon->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return mathPolygonHasPoint(polygon, p);
	}
	mathPointProjectionPlane(o, polygon->v[polygon->v_indices[0]], polygon->normal, p, NULL);
	if (mathPolygonHasPoint(polygon, p)) {
		return 2;
	}
	for (i = 0; i < polygon->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygon->v[polygon->v_indices[i++]]);
		mathVec3Copy(edge[1], polygon->v[polygon->v_indices[i >= polygon->v_indices_cnt ? 0 : i]]);
		if (mathAABBIntersectSegment(o, half, (const float(*)[3])edge)) {
			return 2;
		}
	}
	return 0;
}

static int mathOBBIntersectPolygon(const GeometryOBB_t* obb, const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathOBBIntersectPlane(obb, polygon->v[polygon->v_indices[0]], polygon->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return mathPolygonHasPoint(polygon, p);
	}
	mathPointProjectionPlane(obb->o, polygon->v[polygon->v_indices[0]], polygon->normal, p, NULL);
	if (mathPolygonHasPoint(polygon, p)) {
		return 2;
	}
	for (i = 0; i < polygon->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygon->v[polygon->v_indices[i++]]);
		mathVec3Copy(edge[1], polygon->v[polygon->v_indices[i >= polygon->v_indices_cnt ? 0 : i]]);
		if (mathOBBIntersectSegment(obb, (const float(*)[3])edge)) {
			return 2;
		}
	}
	return 0;
}

static int ConvexMesh_HasAny_OBBVertices(const GeometryMesh_t* mesh, const GeometryOBB_t* obb) {
	float v[8][3];
	unsigned int i;
	mathOBBVertices(obb, v);
	for (i = 0; i < 8; ++i) {
		if (mathConvexMeshHasPoint(mesh, v[i])) {
			return 1;
		}
	}
	return 0;
}

static int mathOBBIntersectConvexMesh(const GeometryOBB_t* obb, const GeometryMesh_t* mesh) {
	unsigned int i;
	if (ConvexMesh_HasAny_OBBVertices(mesh, obb)) {
		return 1;
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		if (mathOBBIntersectPolygon(obb, mesh->polygons + i, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int mathConvexMeshIntersectConvexMesh(const GeometryMesh_t* mesh1, const GeometryMesh_t* mesh2) {
	unsigned int i;
	for (i = 0; i < mesh1->v_indices_cnt; ++i) {
		const float* p = mesh1->v[mesh1->v_indices[i]];
		if (mathConvexMeshHasPoint(mesh2, p)) {
			return 1;
		}
	}
	for (i = 0; i < mesh2->v_indices_cnt; ++i) {
		const float* p = mesh2->v[mesh2->v_indices[i]];
		if (mathConvexMeshHasPoint(mesh1, p)) {
			return 1;
		}
	}
	for (i = 0; i < mesh1->polygons_cnt; ++i) {
		unsigned int j;
		for (j = 0; j < mesh2->polygons_cnt; ++j) {
			if (mathPolygonIntersectPolygon(mesh1->polygons + i, mesh2->polygons + j)) {
				return 1;
			}
		}
	}
	return 0;
}

/*
static int mathLineIntersectCylinderInfinite(const float ls_v[3], const float lsdir[3], const float cp[3], const float axis[3], float radius, float distance[2]) {
	float new_o[3], new_dir[3], radius_sq = radius * radius;
	float new_axies[3][3];
	float A, B, C;
	int rcnt;
	mathVec3Copy(new_axies[2], axis);
	new_axies[1][0] = 0.0f;
	new_axies[1][1] = -new_axies[2][2];
	new_axies[1][2] = new_axies[2][1];
	if (mathVec3IsZero(new_axies[1])) {
		new_axies[1][0] = -new_axies[2][2];
		new_axies[1][1] = 0.0f;
		new_axies[1][2] = new_axies[2][0];
	}
	mathVec3Normalized(new_axies[1], new_axies[1]);
	mathVec3Cross(new_axies[0], new_axies[1], new_axies[2]);
	mathCoordinateSystemTransformPoint(ls_v, cp, new_axies, new_o);
	mathCoordinateSystemTransformNormalVec3(lsdir, new_axies, new_dir);
	A = new_dir[0] * new_dir[0] + new_dir[1] * new_dir[1];
	B = 2.0f * (new_o[0] * new_dir[0] + new_o[1] * new_dir[1]);
	C = new_o[0] * new_o[0] + new_o[1] * new_o[1] - radius_sq;
	rcnt = mathQuadraticEquation(A, B, C, distance);
	if (0 == rcnt) {
		float v[3], dot;
		mathVec3Sub(v, ls_v, cp);
		dot = mathVec3Dot(v, axis);
		return fcmpf(mathVec3LenSq(v) - dot * dot, radius_sq, CCT_EPSILON) > 0 ? 0 : -1;
	}
	return rcnt;
}
*/

static int mathOBBContainSphere(const GeometryOBB_t* obb, const float o[3], float radius) {
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

static int mathAABBContainMesh(const float o[3], const float half[3], const GeometryMesh_t* mesh) {
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

static int mathSphereContainMesh(const float o[3], float radius, const GeometryMesh_t* mesh) {
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

static int mathOBBContainMesh(const GeometryOBB_t* obb, const GeometryMesh_t* mesh) {
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

static int mathConvexMeshContainAABB(const GeometryMesh_t* mesh, const float o[3], const float half[3]) {
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

static int mathConvexMeshContainOBB(const GeometryMesh_t* mesh, const GeometryOBB_t* obb) {
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

static int mathConvexMeshContainSphere(const GeometryMesh_t* mesh, const float o[3], float radius) {
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

GeometryAABB_t* mathCollisionBodyBoundingBox(const GeometryBodyRef_t* b, GeometryAABB_t* aabb) {
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
			const GeometryPolygon_t* polygon = b->polygon;
			if (!mathAABBFromVertexIndices((const float(*)[3])polygon->v, polygon->v_indices, polygon->v_indices_cnt, aabb->o, aabb->half)) {
				return NULL;
			}
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

int mathCollisionBodyIntersect(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two) {
	if (one->data == two->data) {
		return 1;
	}
	if (GEOMETRY_BODY_POINT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathVec3Equal(one->point, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentHasPoint(two->segment->v, one->point);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPlaneHasPoint(two->plane->v, two->plane->normal, one->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBHasPoint(two->aabb->o, two->aabb->half, one->point);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereHasPoint(two->sphere->o, two->sphere->radius, one->point);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBHasPoint(two->obb, one->point);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathPolygonHasPoint(two->polygon, one->point);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathConvexMeshHasPoint(two->mesh, one->point);
			}
		}
	}
	else if (GEOMETRY_BODY_SEGMENT == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathSegmentHasPoint(one->segment->v, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectSegment(one->segment->v, two->segment->v, NULL, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathSegmentIntersectPlane(one->segment->v, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectSegment(two->aabb->o, two->aabb->half, one->segment->v);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBIntersectSegment(two->obb, one->segment->v);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectSegment(two->sphere->o, two->sphere->radius, one->segment->v, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathSegmentIntersectPolygon(one->segment->v, two->polygon, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathSegmentIntersectConvexMesh(one->segment->v, two->mesh);
			}
		}
	}
	else if (GEOMETRY_BODY_AABB == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathAABBHasPoint(one->aabb->o, one->aabb->half, two->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectAABB(one->aabb->o, one->aabb->half, two->aabb->o, two->aabb->half);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathAABBIntersectSphere(one->aabb->o, one->aabb->half, two->sphere->o, two->sphere->radius);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathAABBIntersectPlane(one->aabb->o, one->aabb->half, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathAABBIntersectSegment(one->aabb->o, one->aabb->half, two->segment->v);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathAABBIntersectPolygon(one->aabb->o, one->aabb->half, two->polygon, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				GeometryOBB_t one_obb;
				mathOBBFromAABB(&one_obb, one->aabb->o, one->aabb->half);
				return mathOBBIntersectOBB(&one_obb, two->obb);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				GeometryOBB_t one_obb;
				mathOBBFromAABB(&one_obb, one->aabb->o, one->aabb->half);
				return mathOBBIntersectConvexMesh(&one_obb, two->mesh);
			}
		}
	}
	else if (GEOMETRY_BODY_SPHERE == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathSphereHasPoint(one->sphere->o, one->sphere->radius, two->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectSphere(two->aabb->o, two->aabb->half, one->sphere->o, one->sphere->radius);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathSphereIntersectOBB(one->sphere->o, one->sphere->radius, two->obb);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectSphere(one->sphere->o, one->sphere->radius, two->sphere->o, two->sphere->radius, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathSphereIntersectPlane(one->sphere->o, one->sphere->radius, two->plane->v, two->plane->normal, NULL, NULL);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSphereIntersectSegment(one->sphere->o, one->sphere->radius, two->segment->v, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathSphereIntersectPolygon(one->sphere->o, one->sphere->radius, two->polygon, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathSphereIntersectConvexMesh(one->sphere->o, one->sphere->radius, two->mesh, NULL);
			}
		}
	}
	else if (GEOMETRY_BODY_PLANE == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathPlaneHasPoint(one->plane->v, one->plane->normal, two->point);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectPlane(two->aabb->o, two->aabb->half, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBIntersectPlane(two->obb, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectPlane(two->sphere->o, two->sphere->radius, one->plane->v, one->plane->normal, NULL, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPlaneIntersectPlane(one->plane->v, one->plane->normal, two->plane->v, two->plane->normal);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectPlane(two->segment->v, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathPolygonIntersectPlane(two->polygon, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathMeshIntersectPlane(two->mesh, one->plane->v, one->plane->normal, NULL);
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
				return mathSegmentIntersectPolygon(two->segment->v, one->polygon, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPolygonIntersectPlane(one->polygon, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectPolygon(two->sphere->o, two->sphere->radius, one->polygon, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectPolygon(two->aabb->o, two->aabb->half, one->polygon, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBIntersectPolygon(two->obb, one->polygon, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathPolygonIntersectPolygon(one->polygon, two->polygon);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathPolygonIntersectConvexMesh(one->polygon, two->mesh);
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
				return mathOBBIntersectSegment(one->obb, two->segment->v);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathOBBIntersectPlane(one->obb, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBIntersectOBB(one->obb, two->obb);
			}
			case GEOMETRY_BODY_AABB:
			{
				GeometryOBB_t two_obb;
				mathOBBFromAABB(&two_obb, two->aabb->o, two->aabb->half);
				return mathOBBIntersectOBB(one->obb, &two_obb);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectOBB(two->sphere->o, two->sphere->radius, one->obb);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathOBBIntersectPolygon(one->obb, two->polygon, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathOBBIntersectConvexMesh(one->obb, two->mesh);
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
				return mathSegmentIntersectConvexMesh(two->segment->v, one->mesh);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathMeshIntersectPlane(one->mesh, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectConvexMesh(two->sphere->o, two->sphere->radius, one->mesh, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				GeometryOBB_t two_obb;
				mathOBBFromAABB(&two_obb, two->aabb->o, two->aabb->half);
				return mathOBBIntersectConvexMesh(&two_obb, one->mesh);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBIntersectConvexMesh(two->obb, one->mesh);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return mathPolygonIntersectConvexMesh(two->polygon, one->mesh);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathConvexMeshIntersectConvexMesh(one->mesh, two->mesh);
			}
		}
	}
	return 0;
}

int mathCollisionBodyContain(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two) {
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
				return mathOBBContainSphere(&one_obb, two->sphere->o, two->sphere->radius);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathAABBContainMesh(one->aabb->o, one->aabb->half, two->mesh);
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
				return mathOBBContainSphere(one->obb, two->sphere->o, two->sphere->radius);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return mathOBBContainMesh(one->obb, two->mesh);
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
				return mathSphereContainMesh(one->sphere->o, one->sphere->radius, two->mesh);
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
				return mathConvexMeshContainAABB(one->mesh, two->aabb->o, two->aabb->half);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathConvexMeshContainOBB(one->mesh, two->obb);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathConvexMeshContainSphere(one->mesh, two->sphere->o, two->sphere->radius);
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
				return mathSegmentHasPoint(one->segment->v, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentContainSegment(one->segment->v, two->segment->v);
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
