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

#ifdef __cplusplus
extern "C" {
#endif

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

int mathSegmentIntersectPolygen(const float ls[2][3], const GeometryPolygen_t* polygen, float p[3]) {
	int res;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathSegmentIntersectPlane(ls, polygen->v[polygen->v_indices[0]], polygen->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return mathPolygenHasPoint(polygen, p);
	}
	if (mathPolygenHasPoint(polygen, ls[0]) || mathPolygenHasPoint(polygen, ls[1])) {
		return 2;
	}
	else {
		int i;
		for (i = 0; i < polygen->v_indices_cnt; ) {
			float edge[2][3];
			mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
			mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
			if (mathSegmentIntersectSegment(ls, (const float(*)[3])edge, NULL, NULL)) {
				return 2;
			}
		}
		return 0;
	}
}

int mathPolygenIntersectPolygen(const GeometryPolygen_t* polygen1, const GeometryPolygen_t* polygen2) {
	int i;
	if (!mathPlaneIntersectPlane(polygen1->v[polygen1->v_indices[0]], polygen1->normal, polygen2->v[polygen2->v_indices[0]], polygen2->normal)) {
		return 0;
	}
	for (i = 0; i < polygen1->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygen1->v[polygen1->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen1->v[polygen1->v_indices[i >= polygen1->v_indices_cnt ? 0 : i]]);
		if (mathSegmentIntersectPolygen((const float(*)[3])edge, polygen2, NULL)) {
			return 1;
		}
	}
	return 0;
}

int mathPolygenIntersectPlane(const GeometryPolygen_t* polygen, const float plane_v[3], const float plane_n[3], float p[3]) {
	int i, has_gt0, has_le0, idx_0;
	if (!mathPlaneIntersectPlane(polygen->v[polygen->v_indices[0]], polygen->normal, plane_v, plane_n)) {
		return 0;
	}
	idx_0 = -1;
	has_gt0 = has_le0 = 0;
	for (i = 0; i < polygen->v_indices_cnt; ++i) {
		int cmp;
		float d;
		mathPointProjectionPlane(polygen->v[polygen->v_indices[i]], plane_v, plane_n, NULL, &d);
		cmp = fcmpf(d, 0.0f, CCT_EPSILON);
		if (cmp > 0) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (cmp < 0) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0 >= 0) {
			return 2;
		}
		else {
			idx_0 = i;
		}
	}
	if (idx_0 < 0) {
		return 0;
	}
	if (p) {
		mathVec3Copy(p, polygen->v[polygen->v_indices[idx_0]]);
	}
	return 1;
}

int mathSphereIntersectLine(const float o[3], float radius, const float ls_vertice[3], const float lsdir[3], float distance[2]) {
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

int mathSphereIntersectPolygen(const float o[3], float radius, const GeometryPolygen_t* polygen, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathSphereIntersectPlane(o, radius, polygen->v[polygen->v_indices[0]], polygen->normal, p, NULL);
	if (0 == res) {
		return 0;
	}
	if (mathPolygenHasPoint(polygen, p)) {
		return res;
	}
	for (i = 0; i < polygen->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
		res = mathSphereIntersectSegment(o, radius, (const float(*)[3])edge, p);
		if (res != 0) {
			return res;
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
	int i, has_gt0 = 0, has_le0 = 0, idx_0 = -1;
	for (i = 0; i < 8; ++i) {
		int cmp;
		float d;
		mathPointProjectionPlane(vertices[i], plane_v, plane_n, NULL, &d);
		cmp = fcmpf(d, 0.0f, CCT_EPSILON);
		if (cmp > 0) {
			if (has_le0) {
				return 2;
			}
			has_gt0 = 1;
		}
		else if (cmp < 0) {
			if (has_gt0) {
				return 2;
			}
			has_le0 = 1;
		}
		else if (idx_0 >= 0) {
			return 2;
		}
		else {
			idx_0 = i;
		}
	}
	if (idx_0 < 0) {
		return 0;
	}
	if (p) {
		mathVec3Copy(p, vertices[idx_0]);
	}
	return 1;
}

int mathOBBIntersectPlane(const GeometryOBB_t* obb, const float plane_v[3], const float plane_n[3], float p[3]) {
	float vertices[8][3];
	mathOBBVertices(obb, vertices);
	return mathBoxIntersectPlane((const float(*)[3])vertices, plane_v, plane_n, p);
}

int mathAABBIntersectPlane(const float o[3], const float half[3], const float plane_v[3], const float plane_n[3], float p[3]) {
	float vertices[8][3];
	mathAABBVertices(o, half, vertices);
	return mathBoxIntersectPlane((const float(*)[3])vertices, plane_v, plane_n, p);
}

int mathAABBIntersectSphere(const float aabb_o[3], const float aabb_half[3], const float sp_o[3], float sp_radius) {
	float closest_v[3];
	mathAABBClosestPointTo(aabb_o, aabb_half, sp_o, closest_v);
	mathVec3Sub(closest_v, closest_v, sp_o);
	return mathVec3LenSq(closest_v) <= sp_radius * sp_radius;
}

int mathAABBIntersectSegment(const float o[3], const float half[3], const float ls[2][3]) {
	int i;
	GeometryPolygen_t polygen;
	if (mathAABBHasPoint(o, half, ls[0]) || mathAABBHasPoint(o, half, ls[1])) {
		return 1;
	}
	for (i = 0; i < 6; ++i) {
		GeometryRect_t rect;
		float p[4][3];
		mathAABBPlaneRect(o, half, i, &rect);
		mathRectToPolygen(&rect, &polygen, p);
		if (mathSegmentIntersectPolygen(ls, &polygen, NULL)) {
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
		GeometryPolygen_t polygen;
		GeometryRect_t rect;
		float p[4][3];

		mathOBBPlaneRect(obb, i, &rect);
		mathRectToPolygen(&rect, &polygen, p);
		if (mathSegmentIntersectPolygen(ls, &polygen, NULL)) {
			return 1;
		}
	}
	return 0;
}

int mathAABBIntersectPolygen(const float o[3], const float half[3], const GeometryPolygen_t* polygen, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathAABBIntersectPlane(o, half, polygen->v[polygen->v_indices[0]], polygen->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return mathPolygenHasPoint(polygen, p);
	}
	mathPointProjectionPlane(o, polygen->v[polygen->v_indices[0]], polygen->normal, p, NULL);
	if (mathPolygenHasPoint(polygen, p)) {
		return 2;
	}
	for (i = 0; i < polygen->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
		if (mathAABBIntersectSegment(o, half, (const float(*)[3])edge)) {
			return 2;
		}
	}
	return 0;
}

int mathOBBIntersectPolygen(const GeometryOBB_t* obb, const GeometryPolygen_t* polygen, float p[3]) {
	int res, i;
	float point[3];
	if (!p) {
		p = point;
	}
	res = mathOBBIntersectPlane(obb, polygen->v[polygen->v_indices[0]], polygen->normal, p);
	if (0 == res) {
		return 0;
	}
	if (1 == res) {
		return mathPolygenHasPoint(polygen, p);
	}
	mathPointProjectionPlane(obb->o, polygen->v[polygen->v_indices[0]], polygen->normal, p, NULL);
	if (mathPolygenHasPoint(polygen, p)) {
		return 2;
	}
	for (i = 0; i < polygen->v_indices_cnt; ) {
		float edge[2][3];
		mathVec3Copy(edge[0], polygen->v[polygen->v_indices[i++]]);
		mathVec3Copy(edge[1], polygen->v[polygen->v_indices[i >= polygen->v_indices_cnt ? 0 : i]]);
		if (mathOBBIntersectSegment(obb, (const float(*)[3])edge)) {
			return 2;
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GeometryAABB_t* mathCollisionBodyBoundingBox(const GeometryBodyRef_t* b, const float delta_half_v[3], GeometryAABB_t* aabb) {
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
		case GEOMETRY_BODY_POLYGEN:
		{
			const GeometryPolygen_t* polygen = b->polygen;
			if (!mathMeshVerticesToAABB(polygen->v, polygen->v_indices, polygen->v_indices_cnt, aabb->o, aabb->half)) {
				return NULL;
			}
			break;
		}
		case GEOMETRY_BODY_TRIANGLE_MESH:
		{
			const GeometryTriangleMesh_t* mesh = b->mesh;
			if (!mathMeshVerticesToAABB(mesh->v, mesh->v_indices, mesh->v_indices_cnt, aabb->o, aabb->half)) {
				return NULL;
			}
			break;
		}
		default:
		{
			return NULL;
		}
	}
	if (delta_half_v) {
		int i;
		mathVec3Add(aabb->o, aabb->o, delta_half_v);
		for (i = 0; i < 3; ++i) {
			aabb->half[i] += (delta_half_v[i] > 0.0f ? delta_half_v[i] : -delta_half_v[i]);
		}
	}
	return aabb;
}

int mathCollisionBodyIntersect(const GeometryBodyRef_t* one, const GeometryBodyRef_t* two) {
	if (one == two) {
		return 1;
	}
	else if (GEOMETRY_BODY_POINT == one->type) {
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathPolygenHasPoint(two->polygen, one->point);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBHasPoint(two->obb, one->point);
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathSegmentIntersectPolygen(one->segment->v, two->polygen, NULL);
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathAABBIntersectPolygen(one->aabb->o, one->aabb->half, two->polygen, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				GeometryOBB_t one_obb;
				mathOBBFromAABB(&one_obb, one->aabb->o, one->aabb->half);
				return mathOBBIntersectOBB(&one_obb, two->obb);
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathSphereIntersectPolygen(one->sphere->o, one->sphere->radius, two->polygen, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathSphereIntersectOBB(one->sphere->o, one->sphere->radius, two->obb);
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathPolygenIntersectPlane(two->polygen, one->plane->v, one->plane->normal, NULL);
			}
		}
	}
	else if (GEOMETRY_BODY_POLYGEN == one->type) {
		switch (two->type) {
			case GEOMETRY_BODY_POINT:
			{
				return mathPolygenHasPoint(one->polygen, two->point);
			}
			case GEOMETRY_BODY_SEGMENT:
			{
				return mathSegmentIntersectPolygen(two->segment->v, one->polygen, NULL);
			}
			case GEOMETRY_BODY_PLANE:
			{
				return mathPolygenIntersectPlane(one->polygen, two->plane->v, two->plane->normal, NULL);
			}
			case GEOMETRY_BODY_SPHERE:
			{
				return mathSphereIntersectPolygen(two->sphere->o, two->sphere->radius, one->polygen, NULL);
			}
			case GEOMETRY_BODY_AABB:
			{
				return mathAABBIntersectPolygen(two->aabb->o, two->aabb->half, one->polygen, NULL);
			}
			case GEOMETRY_BODY_OBB:
			{
				return mathOBBIntersectPolygen(two->obb, one->polygen, NULL);
			}
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathPolygenIntersectPolygen(one->polygen, two->polygen);
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
			case GEOMETRY_BODY_POLYGEN:
			{
				return mathOBBIntersectPolygen(one->obb, two->polygen, NULL);
			}
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
