//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/math_quat.h"
#include "../../../inc/crt/geometry/vertex.h"
#include "../../../inc/crt/geometry/aabb.h"
#include "../../../inc/crt/geometry/obb.h"
#include "../../../inc/crt/geometry/polygon.h"
#include "../../../inc/crt/geometry/collision.h"
#include <stddef.h>

static void point_rotate(float p[3], const float mark_pos[3], const float q[4]) {
	float v[3];
	mathVec3Sub(v, p, mark_pos);
	mathQuatMulVec3(v, q, v);
	mathVec3Add(p, mark_pos, v);
}

static void indices_rotate(float(*p)[3], const unsigned int* indices, unsigned int indices_cnt, const float mark_pos[3], const float q[4]) {
	unsigned int i;
	for (i = 0; i < indices_cnt; ++i) {
		point_rotate(p[indices[i]], mark_pos, q);
	}
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
			float min_v[3], max_v[3];
			const GeometryPolygon_t* polygon = b->polygon;
			if (!mathVertexIndicesFindMinMaxXYZ((const float(*)[3])polygon->v, polygon->v_indices, polygon->v_indices_cnt, min_v, max_v)) {
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

int mathCollisionBodyRotate(GeometryBodyRef_t* b, const float mark_pos[3], const float q[4]) {
	switch (b->type) {
		case GEOMETRY_BODY_POINT:
		{
			float* point = b->point;
			if (point == mark_pos || mathVec3Equal(point, mark_pos)) {
				break;
			}
			point_rotate(point, mark_pos, q);
			break;
		}
		case GEOMETRY_BODY_PLANE:
		{
			GeometryPlane_t* plane = b->plane;
			mathQuatMulVec3(plane->normal, q, plane->normal);
			if (plane->v == mark_pos || mathVec3Equal(plane->v, mark_pos)) {
				break;
			}
			point_rotate(plane->v, mark_pos, q);
			break;
		}
		case GEOMETRY_BODY_SPHERE:
		{
			GeometrySphere_t* sphere = b->sphere;
			if (sphere->o == mark_pos || mathVec3Equal(sphere->o, mark_pos)) {
				break;
			}
			point_rotate(sphere->o, mark_pos, q);
			break;
		}
		case GEOMETRY_BODY_AABB:
		{
			GeometryAABB_t* aabb = b->aabb;
			if (aabb->o == mark_pos || mathVec3Equal(aabb->o, mark_pos)) { /* AABB rotate center position */
				float axis[3], new_axis[3];
				/* check rotate by X axis ??? */
				mathVec3Set(axis, 1.0f, 0.0f, 0.0f);
				mathQuatMulVec3(new_axis, q, axis);
				if (mathVec3Equal(new_axis, axis)) {
					float dot;
					mathVec3Set(axis, 0.0f, 1.0f, 0.0f);
					mathQuatMulVec3(new_axis, q, axis);
					dot = mathVec3Dot(new_axis, axis);
					if (dot <= CCT_EPSILON && dot >= CCT_EPSILON_NEGATE) { /* rotate PI/2 */
						dot = aabb->half[1];
						aabb->half[1] = aabb->half[2];
						aabb->half[2] = dot;
						break;
					}
					else if (dot <= 1.0 + CCT_EPSILON && dot >= 1.0f - CCT_EPSILON) { /* rotate PI */
						break;
					}
					return 0;
				}
				/* check rotate by Y axis ??? */
				mathVec3Set(axis, 0.0f, 1.0f, 0.0f);
				mathQuatMulVec3(new_axis, q, axis);
				if (mathVec3Equal(new_axis, axis)) {
					float dot;
					mathVec3Set(axis, 1.0f, 0.0f, 0.0f);
					mathQuatMulVec3(new_axis, q, axis);
					dot = mathVec3Dot(new_axis, axis);
					if (dot <= CCT_EPSILON && dot >= CCT_EPSILON_NEGATE) { /* rotate PI/2 */
						dot = aabb->half[0];
						aabb->half[0] = aabb->half[2];
						aabb->half[2] = dot;
						break;
					}
					else if (dot <= 1.0 + CCT_EPSILON && dot >= 1.0f - CCT_EPSILON) { /* rotate PI */
						break;
					}
					return 0;
				}
				/* check rotate by Z axis ??? */
				mathVec3Set(axis, 0.0f, 0.0f, 1.0f);
				mathQuatMulVec3(new_axis, q, axis);
				if (mathVec3Equal(new_axis, axis)) {
					float dot;
					mathVec3Set(axis, 1.0f, 0.0f, 0.0f);
					mathQuatMulVec3(new_axis, q, axis);
					dot = mathVec3Dot(new_axis, axis);
					if (dot <= CCT_EPSILON && dot >= CCT_EPSILON_NEGATE) { /* rotate PI/2 */
						dot = aabb->half[1];
						aabb->half[1] = aabb->half[0];
						aabb->half[0] = dot;
						break;
					}
					else if (dot <= 1.0 + CCT_EPSILON && dot >= 1.0f - CCT_EPSILON) { /* rotate PI */
						break;
					}
					return 0;
				}
				/* not allow rotate by other axies */
				return 0;
			}
			else { /* AABB rotate by other position */
				point_rotate(aabb->o, mark_pos, q);
			}
			break;
		}
		case GEOMETRY_BODY_OBB:
		{
			GeometryOBB_t* obb = b->obb;
			mathQuatMulVec3(obb->axis[0], q, obb->axis[0]);
			mathQuatMulVec3(obb->axis[1], q, obb->axis[1]);
			mathQuatMulVec3(obb->axis[2], q, obb->axis[2]);
			if (obb->o == mark_pos || mathVec3Equal(obb->o, mark_pos)) {
				break;
			}
			point_rotate(obb->o, mark_pos, q);
			break;
		}
		case GEOMETRY_BODY_POLYGON:
		{
			GeometryPolygon_t* polygon = b->polygon;
			indices_rotate(polygon->v, polygon->v_indices, polygon->v_indices_cnt, mark_pos, q);
			mathQuatMulVec3(polygon->normal, q, polygon->normal);
			point_rotate(polygon->o, mark_pos, q);
			break;
		}
		case GEOMETRY_BODY_CONVEX_MESH:
		{
			GeometryMesh_t* mesh = b->mesh;
			unsigned int i;
			float min_xyz[3], max_xyz[3];

			indices_rotate(mesh->v, mesh->v_indices, mesh->v_indices_cnt, mark_pos, q);
			point_rotate(mesh->o, mark_pos, q);
			for (i = 0; i < mesh->polygons_cnt; ++i) {
				GeometryPolygon_t* polygon = mesh->polygons + i;
				if (polygon->v != mesh->v) {
					indices_rotate(polygon->v, polygon->v_indices, polygon->v_indices_cnt, mark_pos, q);
				}
				mathQuatMulVec3(polygon->normal, q, polygon->normal);
				mathVec3Copy(polygon->o, mesh->o);
			}
			mathVertexIndicesFindMinMaxXYZ(mesh->v, mesh->v_indices, mesh->v_indices_cnt, min_xyz, max_xyz);
			mathAABBFromTwoVertice(min_xyz, max_xyz, mesh->bound_box.o, mesh->bound_box.half);
			break;
		}
		default:
			return 0;
	}
	return 1;
}

int mathCollisionBodyRotateAxisRadian(GeometryBodyRef_t* b, const float mark_pos[3], const float axis[3], float radian) {
	float q[4];
	mathQuatFromAxisRadian(q, axis, radian);
	return mathCollisionBodyRotate(b, mark_pos, q);
}

#ifdef __cplusplus
}
#endif
