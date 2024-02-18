//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/line_segment.h"
#include "../../../inc/crt/geometry/plane.h"
#include "../../../inc/crt/geometry/sphere.h"
#include "../../../inc/crt/geometry/aabb.h"
#include "../../../inc/crt/geometry/obb.h"
#include "../../../inc/crt/geometry/polygon.h"
#include "../../../inc/crt/geometry/mesh.h"
#include "../../../inc/crt/geometry/collision.h"
#include <math.h>
#include <stddef.h>

int Segment_Intersect_Plane(const float ls[2][3], const float plane_v[3], const float plane_normal[3], float p[3]) {
	float d[2], lsdir[3], dot;
	mathVec3Sub(lsdir, ls[1], ls[0]);
	dot = mathVec3Dot(lsdir, plane_normal);
	if (dot <= CCT_EPSILON && dot >= CCT_EPSILON_NEGATE) {
		return mathPlaneHasPoint(plane_v, plane_normal, ls[0]) ? 2 : 0;
	}
	mathPointProjectionPlane(ls[0], plane_v, plane_normal, NULL, &d[0]);
	mathPointProjectionPlane(ls[1], plane_v, plane_normal, NULL, &d[1]);
	if (d[0] > CCT_EPSILON && d[1] > CCT_EPSILON) {
		return 0;
	}
	if (d[0] < CCT_EPSILON_NEGATE && d[1] < CCT_EPSILON_NEGATE) {
		return 0;
	}
	if (d[0] <= CCT_EPSILON && d[0] >= CCT_EPSILON_NEGATE) {
		if (p) {
			mathVec3Copy(p, ls[0]);
		}
		return 1;
	}
	if (d[1] <= CCT_EPSILON && d[1] >= CCT_EPSILON_NEGATE) {
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

static int Segment_Intersect_Polygon(const float ls[2][3], const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = Segment_Intersect_Plane(ls, polygon->v[polygon->v_indices[0]], polygon->normal, p);
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

int Segment_Intersect_ConvexMesh(const float ls[2][3], const GeometryMesh_t* mesh) {
	if (mathConvexMeshHasPoint(mesh, ls[0])) {
		return 1;
	}
	if (mathConvexMeshHasPoint(mesh, ls[1])) {
		return 1;
	}
	return 0;
}

static int Polygon_Intersect_Polygon(const GeometryPolygon_t* polygon1, const GeometryPolygon_t* polygon2) {
	int i;
	if (!mathPlaneIntersectPlane(polygon1->v[polygon1->v_indices[0]], polygon1->normal, polygon2->v[polygon2->v_indices[0]], polygon2->normal)) {
		return 0;
	}
	for (i = 0; i < polygon1->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygon1->v[polygon1->v_indices[i++]]);
		mathVec3Copy(edge[1], polygon1->v[polygon1->v_indices[i >= polygon1->v_indices_cnt ? 0 : i]]);
		if (Segment_Intersect_Polygon((const float(*)[3])edge, polygon2, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int Polygon_Intersect_ConvexMesh(const GeometryPolygon_t* polygon, const GeometryMesh_t* mesh) {
	unsigned int i;
	for (i = 0; i < polygon->v_indices_cnt; ++i) {
		const float* p = polygon->v[polygon->v_indices[i]];
		if (mathConvexMeshHasPoint(mesh, p)) {
			return 1;
		}
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		if (Polygon_Intersect_Polygon(polygon, mesh->polygons + i)) {
			return 1;
		}
	}
	return 0;
}

static int Polygon_Intersect_Plane(const GeometryPolygon_t* polygon, const float plane_v[3], const float plane_n[3], float p[3]) {
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

static int Mesh_Intersect_Plane(const GeometryMesh_t* mesh, const float plane_v[3], const float plane_n[3], float p[3]) {
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

int Sphere_Intersect_Segment(const float o[3], float radius, const float ls[2][3], float p[3]) {
	float closest_v_lensq, radius_sq;
	float closest_p[3], closest_v[3];
	mathSegmentClosestPointTo(ls, o, closest_p);
	mathVec3Sub(closest_v, closest_p, o);
	closest_v_lensq = mathVec3LenSq(closest_v);
	radius_sq = radius * radius;
	if (closest_v_lensq < radius_sq - CCT_EPSILON) {
		return 2;
	}
	if (closest_v_lensq <= radius_sq + CCT_EPSILON) {
		if (p) {
			mathVec3Copy(p, closest_p);
		}
		return 1;
	}
	return 0;
}

int Sphere_Intersect_Plane(const float o[3], float radius, const float plane_v[3], const float plane_normal[3], float new_o[3], float* new_r) {
	float pp[3], ppd, ppo[3], ppolensq, radius_sq;
	mathPointProjectionPlane(o, plane_v, plane_normal, pp, &ppd);
	mathVec3Sub(ppo, o, pp);
	ppolensq = mathVec3LenSq(ppo);
	radius_sq = radius * radius;
	if (ppolensq > radius_sq + CCT_EPSILON) {
		return 0;
	}
	if (new_o) {
		mathVec3Copy(new_o, pp);
	}
	if (ppolensq >= radius_sq - CCT_EPSILON) {
		if (new_r) {
			*new_r = 0.0f;
		}
		return 1;
	}
	if (new_r) {
		*new_r = sqrtf(radius_sq - ppolensq);
	}
	return 2;
}

static int Sphere_Intersect_Polygon(const float o[3], float radius, const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = Sphere_Intersect_Plane(o, radius, polygon->v[polygon->v_indices[0]], polygon->normal, p, NULL);
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
		res = Sphere_Intersect_Segment(o, radius, (const float(*)[3])edge, p);
		if (res != 0) {
			return res;
		}
	}
	return 0;
}

int Sphere_Intersect_ConvexMesh(const float o[3], float radius, const GeometryMesh_t* mesh, float p[3]) {
	unsigned int i;
	if (mathConvexMeshHasPoint(mesh, o)) {
		return 1;
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		int ret = Sphere_Intersect_Polygon(o, radius, mesh->polygons + i, p);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

int Sphere_Intersect_OBB(const float o[3], float radius, const GeometryOBB_t* obb) {
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

static int Box_Intersect_Plane(const float vertices[8][3], const float plane_v[3], const float plane_n[3], float p[3]) {
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

static int OBB_Intersect_Plane(const GeometryOBB_t* obb, const float plane_v[3], const float plane_n[3], float p[3]) {
	float vertices[8][3];
	mathOBBVertices(obb, vertices);
	return Box_Intersect_Plane((const float(*)[3])vertices, plane_v, plane_n, p);
}

static int AABB_Intersect_Plane(const float o[3], const float half[3], const float plane_v[3], const float plane_n[3], float p[3]) {
	float vertices[8][3];
	mathAABBVertices(o, half, vertices);
	return Box_Intersect_Plane((const float(*)[3])vertices, plane_v, plane_n, p);
}

static int AABB_Intersect_Sphere(const float aabb_o[3], const float aabb_half[3], const float sp_o[3], float sp_radius) {
	float closest_v[3];
	mathAABBClosestPointTo(aabb_o, aabb_half, sp_o, closest_v);
	mathVec3Sub(closest_v, closest_v, sp_o);
	return mathVec3LenSq(closest_v) <= sp_radius * sp_radius;
}

static int AABB_Intersect_Segment(const float o[3], const float half[3], const float ls[2][3]) {
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
		if (Segment_Intersect_Polygon(ls, &polygon, NULL)) {
			return 1;
		}
	}
	return 0;
}

int OBB_Intersect_Segment(const GeometryOBB_t* obb, const float ls[2][3]) {
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
		if (Segment_Intersect_Polygon(ls, &polygon, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int AABB_Intersect_Polygon(const float o[3], const float half[3], const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = AABB_Intersect_Plane(o, half, polygon->v[polygon->v_indices[0]], polygon->normal, p);
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
		if (AABB_Intersect_Segment(o, half, (const float(*)[3])edge)) {
			return 2;
		}
	}
	return 0;
}

static int OBB_Intersect_Polygon(const GeometryOBB_t* obb, const GeometryPolygon_t* polygon, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = OBB_Intersect_Plane(obb, polygon->v[polygon->v_indices[0]], polygon->normal, p);
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
		if (OBB_Intersect_Segment(obb, (const float(*)[3])edge)) {
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

static int OBB_Intersect_ConvexMesh(const GeometryOBB_t* obb, const GeometryMesh_t* mesh) {
	unsigned int i;
	if (ConvexMesh_HasAny_OBBVertices(mesh, obb)) {
		return 1;
	}
	for (i = 0; i < mesh->polygons_cnt; ++i) {
		if (OBB_Intersect_Polygon(obb, mesh->polygons + i, NULL)) {
			return 1;
		}
	}
	return 0;
}

static int ConvexMesh_Intersect_ConvexMesh(const GeometryMesh_t* mesh1, const GeometryMesh_t* mesh2) {
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
			if (Polygon_Intersect_Polygon(mesh1->polygons + i, mesh2->polygons + j)) {
				return 1;
			}
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

int mathCollisionIntersect(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two) {
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
				return mathSegmentHasPoint((const float(*)[3])two->segment->v, one->point);
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
		const float(*one_segment_v)[3] = (const float(*)[3])one->segment->v;
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathSegmentHasPoint(one_segment_v, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectSegment(one_segment_v, (const float(*)[3])two->segment->v, NULL, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return Segment_Intersect_Plane(one_segment_v, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return AABB_Intersect_Segment(two->aabb->o, two->aabb->half, one_segment_v);
			}
			case GEOMETRY_BODY_OBB:
			{
				return OBB_Intersect_Segment(two->obb, one_segment_v);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return Sphere_Intersect_Segment(two->sphere->o, two->sphere->radius, one_segment_v, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return Segment_Intersect_Polygon(one_segment_v, two->polygon, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return Segment_Intersect_ConvexMesh(one_segment_v, two->mesh);
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
				return AABB_Intersect_Sphere(one->aabb->o, one->aabb->half, two->sphere->o, two->sphere->radius);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return AABB_Intersect_Plane(one->aabb->o, one->aabb->half, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return AABB_Intersect_Segment(one->aabb->o, one->aabb->half, (const float(*)[3])two->segment->v);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return AABB_Intersect_Polygon(one->aabb->o, one->aabb->half, two->polygon, NULL);
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
				return OBB_Intersect_ConvexMesh(&one_obb, two->mesh);
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
				return AABB_Intersect_Sphere(two->aabb->o, two->aabb->half, one->sphere->o, one->sphere->radius);
			}
			case GEOMETRY_BODY_OBB:
			{
				return Sphere_Intersect_OBB(one->sphere->o, one->sphere->radius, two->obb);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectSphere(one->sphere->o, one->sphere->radius, two->sphere->o, two->sphere->radius, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return Sphere_Intersect_Plane(one->sphere->o, one->sphere->radius, two->plane->v, two->plane->normal, NULL, NULL);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return Sphere_Intersect_Segment(one->sphere->o, one->sphere->radius, (const float(*)[3])two->segment->v, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return Sphere_Intersect_Polygon(one->sphere->o, one->sphere->radius, two->polygon, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return Sphere_Intersect_ConvexMesh(one->sphere->o, one->sphere->radius, two->mesh, NULL);
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
				return AABB_Intersect_Plane(two->aabb->o, two->aabb->half, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return OBB_Intersect_Plane(two->obb, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return Sphere_Intersect_Plane(two->sphere->o, two->sphere->radius, one->plane->v, one->plane->normal, NULL, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPlaneIntersectPlane(one->plane->v, one->plane->normal, two->plane->v, two->plane->normal);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return Segment_Intersect_Plane((const float(*)[3])two->segment->v, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return Polygon_Intersect_Plane(two->polygon, one->plane->v, one->plane->normal, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return Mesh_Intersect_Plane(two->mesh, one->plane->v, one->plane->normal, NULL);
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
				return Segment_Intersect_Polygon((const float(*)[3])two->segment->v, one->polygon, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return Polygon_Intersect_Plane(one->polygon, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return Sphere_Intersect_Polygon(two->sphere->o, two->sphere->radius, one->polygon, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return AABB_Intersect_Polygon(two->aabb->o, two->aabb->half, one->polygon, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return OBB_Intersect_Polygon(two->obb, one->polygon, NULL);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return Polygon_Intersect_Polygon(one->polygon, two->polygon);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return Polygon_Intersect_ConvexMesh(one->polygon, two->mesh);
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
				return OBB_Intersect_Segment(one->obb, (const float(*)[3])two->segment->v);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return OBB_Intersect_Plane(one->obb, two->plane->v, two->plane->normal, NULL);
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
				return Sphere_Intersect_OBB(two->sphere->o, two->sphere->radius, one->obb);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return OBB_Intersect_Polygon(one->obb, two->polygon, NULL);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return OBB_Intersect_ConvexMesh(one->obb, two->mesh);
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
				return Segment_Intersect_ConvexMesh((const float(*)[3])two->segment->v, one->mesh);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return Mesh_Intersect_Plane(one->mesh, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return Sphere_Intersect_ConvexMesh(two->sphere->o, two->sphere->radius, one->mesh, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				GeometryOBB_t two_obb;
				mathOBBFromAABB(&two_obb, two->aabb->o, two->aabb->half);
				return OBB_Intersect_ConvexMesh(&two_obb, one->mesh);
			}
			case GEOMETRY_BODY_OBB:
			{
				return OBB_Intersect_ConvexMesh(two->obb, one->mesh);
			}
			case GEOMETRY_BODY_POLYGON:
			{
				return Polygon_Intersect_ConvexMesh(two->polygon, one->mesh);
			}
			case GEOMETRY_BODY_CONVEX_MESH:
			{
				return ConvexMesh_Intersect_ConvexMesh(one->mesh, two->mesh);
			}
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
