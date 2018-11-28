//
// Created by hujianzhe
//

#include "math.h"
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
a == b return 0;
a > b return >0;
a < b return <0;
*/
int fcmpf(float a, float b, float epsilon) {
	float v = a - b;
	/* a == b */
	if (v > -epsilon && v < epsilon)
		return 0;
	return v >= epsilon ? 1 : -1;
}
/*
a == b return 0;
a > b return >0;
a < b return <0;
*/
int fcmp(double a, double b, double epsilon) {
	double v = a - b;
	/* a == b */
	if (v > -epsilon && v < epsilon)
		return 0;
	return v >= epsilon ? 1 : -1;
}

float fsqrtf(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i >> 1);
	x = *(float*)&i;
	x = x * (1.5f - xhalf * x * x);
	return x;
}

double fsqrt(double x) {
	double xhalf = 0.5 * x;
	long long i = *(long long*)&x;
	i = 0x5fe6ec85e7de30da - (i >> 1);
	x = *(double*)&i;
	x = x * (1.5 - xhalf * x * x);
	return x;
}

int mathVec3IsZero(float v[3]) {
	return	fcmpf(v[0], 0.0f, CCT_EPSILON) == 0 &&
			fcmpf(v[1], 0.0f, CCT_EPSILON) == 0 &&
			fcmpf(v[2], 0.0f, CCT_EPSILON) == 0;
}

int mathVec3Equal(float v1[3], float v2[3]) {
	return	fcmpf(v1[0], v2[0], CCT_EPSILON) == 0 &&
			fcmpf(v1[1], v2[1], CCT_EPSILON) == 0 &&
			fcmpf(v1[2], v2[2], CCT_EPSILON) == 0;
}

/* r = v */
float* mathVec3Copy(float r[3], float v[3]) {
	r[0] = v[0];
	r[1] = v[1];
	r[2] = v[2];
	return r;
}

float mathVec3LenSq(float v[3]) {
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

float mathVec3Len(float v[3]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float* mathVec3Normalized(float r[3], float v[3]) {
	float len = mathVec3Len(v);
	if (fcmpf(len, 0.0f, CCT_EPSILON) > 0) {
		float inv_len = 1.0f / len;
		r[0] = v[0] * inv_len;
		r[1] = v[1] * inv_len;
		r[2] = v[2] * inv_len;
	}
	return r;
}

/* r = -v */
float* mathVec3Negate(float r[3], float v[3]) {
	r[0] = -v[0];
	r[1] = -v[1];
	r[2] = -v[2];
	return r;
}

/* r = v1 + v2 */
float* mathVec3Add(float r[3], float v1[3], float v2[3]) {
	r[0] = v1[0] + v2[0];
	r[1] = v1[1] + v2[1];
	r[2] = v1[2] + v2[2];
	return r;
}

/* r += v * n */
float* mathVec3AddScalar(float r[3], float v[3], float n) {
	r[0] += v[0] * n;
	r[1] += v[1] * n;
	r[2] += v[2] * n;
	return r;
}

/* r = v1 - v2 */
float* mathVec3Sub(float r[3], float v1[3], float v2[3]) {
	r[0] = v1[0] - v2[0];
	r[1] = v1[1] - v2[1];
	r[2] = v1[2] - v2[2];
	return r;
}

/* r = v * n */
float* mathVec3MultiplyScalar(float r[3], float v[3], float n) {
	r[0] = v[0] * n;
	r[1] = v[1] * n;
	r[2] = v[2] * n;
	return r;
}

float mathVec3Dot(float v1[3], float v2[3]) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float mathVec3Radian(float v1[3], float v2[3]) {
	return acosf(mathVec3Dot(v1, v2) / mathVec3Len(v1) * mathVec3Len(v2));
}

/* r = v1 X v2 */
float* mathVec3Cross(float r[3], float v1[3], float v2[3]) {
	float x = v1[1] * v2[2] - v1[2] * v2[1];
	float y = v1[2] * v2[0] - v1[0] * v2[2];
	float z = v1[0] * v2[1] - v1[1] * v2[0];
	r[0] = x;
	r[1] = y;
	r[2] = z;
	return r;
}


float* mathQuatNormalized(float r[4], float q[4]) {
	float m = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if (fcmpf(m, 0.0f, CCT_EPSILON) > 0) {
		m = 1.0f / sqrtf(m);
		r[0] = q[0] * m;
		r[1] = q[1] * m;
		r[2] = q[2] * m;
		r[3] = q[3] * m;
	}
	return r;
}

float* mathQuatFromEuler(float q[4], float e[3], const char order[3]) {
	float pitch_x = e[0];
	float yaw_y = e[1];
	float roll_z = e[2];

	float c1 = cosf(pitch_x * 0.5f);
	float c2 = cosf(yaw_y * 0.5f);
	float c3 = cosf(roll_z * 0.5f);
	float s1 = sinf(pitch_x * 0.5f);
	float s2 = sinf(yaw_y * 0.5f);
	float s3 = sinf(roll_z * 0.5f);

	if (order[0] == 'X' && order[1] == 'Y' && order[2] == 'Z') {
		q[0] = s1 * c2 * c3 + c1 * s2 * s3;
		q[1] = c1 * s2 * c3 - s1 * c2 * s3;
		q[2] = c1 * c2 * s3 + s1 * s2 * c3;
		q[3] = c1 * c2 * c3 - s1 * s2 * s3;
	}
	else if (order[0] == 'Y' && order[1] == 'X' && order[2] == 'Z') {
		q[0] = s1 * c2 * c3 + c1 * s2 * s3;
		q[1] = c1 * s2 * c3 - s1 * c2 * s3;
		q[2] = c1 * c2 * s3 - s1 * s2 * c3;
		q[3] = c1 * c2 * c3 + s1 * s2 * s3;
	}
	else if (order[0] == 'Z' && order[1] == 'X' && order[2] == 'Y') {
		q[0] = s1 * c2 * c3 - c1 * s2 * s3;
		q[1] = c1 * s2 * c3 + s1 * c2 * s3;
		q[2] = c1 * c2 * s3 + s1 * s2 * c3;
		q[3] = c1 * c2 * c3 - s1 * s2 * s3;
	}
	else if (order[0] == 'Z' && order[1] == 'Y' && order[2] == 'X') {
		q[0] = s1 * c2 * c3 - c1 * s2 * s3;
		q[1] = c1 * s2 * c3 + s1 * c2 * s3;
		q[2] = c1 * c2 * s3 - s1 * s2 * c3;
		q[3] = c1 * c2 * c3 + s1 * s2 * s3;
	}
	else if (order[0] == 'Y' && order[1] == 'Z' && order[2] == 'X') {
		q[0] = s1 * c2 * c3 + c1 * s2 * s3;
		q[1] = c1 * s2 * c3 + s1 * c2 * s3;
		q[2] = c1 * c2 * s3 - s1 * s2 * c3;
		q[3] = c1 * c2 * c3 - s1 * s2 * s3;
	}
	else if (order[0] == 'X' && order[1] == 'Z' && order[2] == 'Y') {
		q[0] = s1 * c2 * c3 - c1 * s2 * s3;
		q[1] = c1 * s2 * c3 - s1 * c2 * s3;
		q[2] = c1 * c2 * s3 + s1 * s2 * c3;
		q[3] = c1 * c2 * c3 + s1 * s2 * s3;
	}
	else {
		q[0] = q[1] = q[2] = 0.0f;
		q[3] = 1.0f;
	}
	return q;
}

float* mathQuatFromUnitVec3(float q[4], float from[3], float to[3]) {
	float v[3];
	float w = mathVec3Dot(from, to) + 1.0f;
	if (w < 1E-7f) {
		float from_abs_x = from[0] > 0.0f ? from[0] : -from[0];
		float from_abs_z = from[2] > 0.0f ? from[2] : -from[2];
		if (from_abs_x > from_abs_z) {
			v[0] = -from[1];
			v[1] = from[0];
			v[2] = 0.0f;
		}
		else {
			v[0] = 0.0f;
			v[1] = -from[2];
			v[2] = from[1];
		}
		w = 0.0f;
	}
	else {
		mathVec3Cross(v, from, to);
	}

	q[0] = v[0];
	q[1] = v[1];
	q[2] = v[2];
	q[3] = w;
	return mathQuatNormalized(q, q);
}

float* mathQuatFromAxisRadian(float q[4], float axis[3], float radian) {
	const float half_rad = radian * 0.5f;
	const float s = sinf(half_rad);
	q[0] = axis[0] * s;
	q[1] = axis[1] * s;
	q[2] = axis[2] * s;
	q[3] = cosf(half_rad);
	return q;
}

void mathQuatToAxisRadian(float q[4], float axis[3], float* radian) {
	const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float s2 = qx*qx + qy*qy + qz*qz;
	const float s = 1.0f / sqrtf(s2);
	axis[0] = qx * s;
	axis[1] = qy * s;
	axis[2] = qz * s;
	*radian = atan2f(s2*s, qw) * 2.0f;
}

/* r = -q */
float* mathQuatConjugate(float r[4], float q[4]) {
	r[0] = -q[0];
	r[1] = -q[1];
	r[2] = -q[2];
	r[3] = q[3];
	return r;
}

/* r = q1 * q2 */
float* mathQuatMulQuat(float r[4], float q1[4], float q2[4]) {
	const float q1x = q1[0], q1y = q1[1], q1z = q1[2], q1w = q1[3];
	const float q2x = q2[0], q2y = q2[1], q2z = q2[2], q2w = q2[3];
	r[0] = q1w*q2x + q2w*q1x + q1y*q2z - q2y*q1z;
	r[1] = q1w*q2y + q2w*q1y + q1z*q2x - q2z*q1x;
	r[2] = q1w*q2z + q2w*q1z + q1x*q2y - q2x*q1y;
	r[3] = q1w*q2w - q2x*q1x - q1y*q2y - q2z*q1z;
	return r;
}

/* r = q * v */
float* mathQuatMulVec3(float r[3], float q[4], float v[3]) {
	const float vx = 2.0f * v[0];
	const float vy = 2.0f * v[1];
	const float vz = 2.0f * v[2];
	const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
	const float qw2 = qw*qw - 0.5f;
	const float dot2 = qx*vx + qy*vy + qz*vz;
	r[0] = vx*qw2 + (qy * vz - qz * vy)*qw + qx*dot2;
	r[1] = vy*qw2 + (qz * vx - qx * vz)*qw + qy*dot2;
	r[2] = vz*qw2 + (qx * vy - qy * vx)*qw + qz*dot2;
	return r;
}

/*
	continue collision detection
*/

static CCTResult_t* select_min_result(CCTResult_t** results, int cnt) {
	CCTResult_t* result = NULL;
	int i;
	for (i = 0; i < cnt; ++i) {
		if (!results[i])
			continue;
		if (!result || result->distance > results[i]->distance) {
			result = results[i];
		}
	}
	return result;
}

static void copy_result(CCTResult_t* dst, CCTResult_t* src) {
	if (dst == src)
		return;
	dst->distance = src->distance;
	dst->hit_line = src->hit_line;
	if (0 == src->hit_line) {
		mathVec3Copy(dst->hit_point, src->hit_point);
	}
}

float* mathPlaneNormalByVertices3(float vertices[3][3], float normal[3]) {
	float v0v1[3], v0v2[3];
	mathVec3Sub(v0v1, vertices[1], vertices[0]);
	mathVec3Sub(v0v2, vertices[2], vertices[0]);
	mathVec3Cross(normal, v0v1, v0v2);
	return mathVec3Normalized(normal, normal);
}

float* mathPlaneNormalByVertices2(float vertices[2][3], float v[3], float normal[3]) {
	float v0v1[3];
	mathVec3Sub(v0v1, vertices[1], vertices[0]);
	mathVec3Cross(normal, v, v0v1);
	return mathVec3Normalized(normal, normal);
}

float* mathPlaneNormalByLine2(float ls1[2][3], float ls2[2][3], float normal[3]) {
	float ls1v[3];
	return mathPlaneNormalByVertices2(ls2, mathVec3Sub(ls1v, ls1[1], ls1[0]), normal);
}

void mathPointProjectionLine(float p[3], float ls[2][3], float np[3], float* distance) {
	float v0v1[3], v0p[3], pp[3];
	mathVec3Sub(v0v1, ls[1], ls[0]);
	mathVec3Sub(v0p, p, ls[0]);
	mathVec3Normalized(v0v1, v0v1);
	mathVec3Copy(pp, ls[0]);
	mathVec3AddScalar(pp, v0v1, mathVec3Dot(v0v1, v0p));
	if (np)
		mathVec3Copy(np, pp);
	if (distance) {
		float v[3];
		*distance = mathVec3Len(mathVec3Sub(v, pp, p));
	}
}

void mathPointProjectionPlane(float p[3], float plane_v[3], float plane_normal[3], float np[3], float* distance) {
	float pv[3], d;
	mathVec3Sub(pv, plane_v, p);
	d = mathVec3Dot(pv, plane_normal);
	if (distance)
		*distance = d;
	if (np) {
		mathVec3Copy(np, p);
		mathVec3AddScalar(np, plane_normal, d);
	}
}

float mathPointPointDistanceSq(float p1[3], float p2[3]) {
	float v[3];
	return mathVec3LenSq(mathVec3Sub(v, p1, p2));
}

int mathLineParallelLine(float ls1[2][3], float ls2[2][3]) {
	float v1[3], v2[3], n[3];
	mathVec3Sub(v1, ls1[1], ls1[0]);
	mathVec3Sub(v2, ls2[1], ls2[0]);
	mathVec3Cross(n, v1, v2);
	return mathVec3IsZero(n);
}

float* mathPointLineSegmentNearestVertice(float p[3], float ls[2][3], float* np) {
	float pls0[3], pls1[3];
	mathVec3Sub(pls0, ls[0], p);
	mathVec3Sub(pls1, ls[1], p);
	if (mathVec3LenSq(pls0) > mathVec3LenSq(pls1))
		return mathVec3Copy(np, ls[1]);
	else
		return mathVec3Copy(np, ls[0]);
}

int mathLineSegmentHasPoint(float ls[2][3], float p[3]) {
	float *v1 = ls[0], *v2 = ls[1];
	float pv1[3], pv2[3], N[3];
	mathVec3Sub(pv1, v1, p);
	mathVec3Sub(pv2, v2, p);
	if (!mathVec3IsZero(mathVec3Cross(N, pv1, pv2)))
		return 0;
	else {
		float min_x, max_x, min_y, max_y, min_z, max_z;
		v1[0] < v2[0] ? (min_x = v1[0], max_x = v2[0]) : (min_x = v2[0], max_x = v1[0]);
		v1[1] < v2[1] ? (min_y = v1[1], max_y = v2[1]) : (min_y = v2[1], max_y = v1[1]);
		v1[2] < v2[2] ? (min_z = v1[2], max_z = v2[2]) : (min_z = v2[2], max_z = v1[2]);
		return	fcmpf(p[0], min_x, CCT_EPSILON) >= 0 && fcmpf(p[0], max_x, CCT_EPSILON) <= 0 &&
				fcmpf(p[1], min_y, CCT_EPSILON) >= 0 && fcmpf(p[1], max_y, CCT_EPSILON) <= 0 &&
				fcmpf(p[2], min_z, CCT_EPSILON) >= 0 && fcmpf(p[2], max_z, CCT_EPSILON) <= 0;
	}
}

int mathTriangleHasPoint(float tri[3][3], float p[3]) {
	float ap[3], ab[3], ac[3], N[3];
	mathVec3Sub(ap, p, tri[0]);
	mathVec3Sub(ab, tri[1], tri[0]);
	mathVec3Sub(ac, tri[2], tri[0]);
	mathVec3Cross(N, ab, ac);
	if (fcmpf(mathVec3Dot(N, ap), 0.0f, CCT_EPSILON))
		return 0;
	else {
		float u, v;
		float dot_ac_ac = mathVec3Dot(ac, ac);
		float dot_ac_ab = mathVec3Dot(ac, ab);
		float dot_ac_ap = mathVec3Dot(ac, ap);
		float dot_ab_ab = mathVec3Dot(ab, ab);
		float dot_ab_ap = mathVec3Dot(ab, ap);
		float tmp = 1.0f / (dot_ac_ac * dot_ab_ab - dot_ac_ab * dot_ac_ab);
		u = (dot_ab_ab * dot_ac_ap - dot_ac_ab * dot_ab_ap) * tmp;
		if (fcmpf(u, 0.0f, CCT_EPSILON) < 0 || fcmpf(u, 1.0f, CCT_EPSILON) > 0)
			return 0;
		v = (dot_ac_ac * dot_ab_ap - dot_ac_ab * dot_ac_ap) * tmp;
		if (fcmpf(v, 0.0f, CCT_EPSILON) < 0 || fcmpf(v + u, 1.0f, CCT_EPSILON) > 0)
			return 0;
		return 1;
	}
}

CCTResult_t* mathRaycastLine(float o[3], float dir[3], float ls[2][3], CCTResult_t* result) {
	float N[3], p[3], d;
	mathPlaneNormalByVertices2(ls, dir, N);
	mathPointProjectionPlane(o, ls[0], N, NULL, &d);
	if (fcmpf(d, 0.0f, CCT_EPSILON))
		return NULL;
	mathPointProjectionLine(o, ls, p, &d);
	if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
		result->distance = 0.0f;
		result->hit_line = 0;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	else {
		float op[3], cos_theta;
		mathVec3Normalized(op, mathVec3Sub(op, p, o));
		cos_theta = mathVec3Dot(dir, op);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) <= 0)
			return NULL;
		result->distance = d / cos_theta;
		result->hit_line = 0;
		mathVec3Copy(result->hit_point, o);
		mathVec3AddScalar(result->hit_point, dir, result->distance);
		return result;
	}
}

CCTResult_t* mathRaycastLineSegment(float o[3], float dir[3], float ls[2][3], CCTResult_t* result) {
	if (mathRaycastLine(o, dir, ls, result)) {
		if (mathLineSegmentHasPoint(ls, result->hit_point))
			return result;
		else if (fcmpf(result->distance, 0.0f, CCT_EPSILON) <= 0) {
			float v[3], ov[3], d;
			mathPointLineSegmentNearestVertice(o, ls, v);
			mathVec3Sub(ov, v, o);
			d = mathVec3Dot(ov, dir);
			if (fcmpf(d, 0.0f, CCT_EPSILON) < 0 || fcmpf(d * d, mathVec3LenSq(ov), CCT_EPSILON))
				return NULL;
			result->distance = d;
			result->hit_line = 0;
			mathVec3Copy(result->hit_point, v);
			return result;
		}
	}
	return NULL;
}

CCTResult_t* mathRaycastPlane(float o[3], float dir[3], float vertices[3][3], CCTResult_t* result) {
	float N[3], d, cos_theta;
	mathPlaneNormalByVertices3(vertices, N);
	mathPointProjectionPlane(o, vertices[0], N, NULL, &d);
	if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
		result->distance = 0.0f;
		result->hit_line = 0;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	cos_theta = mathVec3Dot(dir, N);
	if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
		return NULL;
	d /= cos_theta;
	if (fcmpf(d, 0.0f, CCT_EPSILON) < 0)
		return NULL;
	result->distance = d;
	result->hit_line = 0;
	mathVec3Copy(result->hit_point, o);
	mathVec3AddScalar(result->hit_point, dir, d);
	return result;
}

CCTResult_t* mathRaycastTriangle(float o[3], float dir[3], float tri[3][3], CCTResult_t* result) {
	if (mathRaycastPlane(o, dir, tri, result)) {
		if (mathTriangleHasPoint(tri, result->hit_point))
			return result;
		else if (fcmpf(result->distance, 0.0f, CCT_EPSILON) == 0) {
			CCTResult_t results[3], *p_results = NULL;
			int i;
			for (i = 0; i < 3; ++i) {
				float edge[2][3];
				mathVec3Copy(edge[0], tri[i % 3]);
				mathVec3Copy(edge[1], tri[(i + 1) % 3]);
				if (!mathRaycastLineSegment(o, dir, edge, results + i))
					continue;
				if (!p_results || p_results->distance > results[i].distance) {
					p_results = results + i;
				}
			}
			if (p_results) {
				copy_result(result, p_results);
				return result;
			}
		}
	}
	return NULL;
}

CCTResult_t* mathRaycastSphere(float o[3], float dir[3], float center[3], float radius, CCTResult_t* result) {
	float radius2 = radius * radius;
	float d, dr2, oc2, dir_d;
	float oc[3];
	mathVec3Sub(oc, center, o);
	oc2 = mathVec3LenSq(oc);
	dir_d = mathVec3Dot(dir, oc);
	if (fcmpf(oc2, radius2, CCT_EPSILON) < 0) {
		result->distance = 0.0f;
		result->hit_line = 0;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	else if (fcmpf(dir_d, 0.0f, CCT_EPSILON) <= 0)
		return NULL;

	dr2 = oc2 - dir_d * dir_d;
	if (fcmpf(dr2, radius2, CCT_EPSILON) >= 0)
		return NULL;

	d = sqrtf(radius2 - dr2);
	result->distance = dir_d - d;
	result->hit_line = 0;
	mathVec3Copy(result->hit_point, o);
	mathVec3AddScalar(result->hit_point, dir, result->distance);
	return result;
}

CCTResult_t* mathLineSegmentcastPlane(float ls[2][3], float dir[3], float vertices[3][3], CCTResult_t* result) {
	int cmp[2];
	float N[3], d[2];
	mathPlaneNormalByVertices3(vertices, N);
	mathPointProjectionPlane(ls[0], vertices[0], N, NULL, &d[0]);
	mathPointProjectionPlane(ls[1], vertices[0], N, NULL, &d[1]);
	cmp[0] = fcmpf(d[0], 0.0f, CCT_EPSILON);
	cmp[1] = fcmpf(d[1], 0.0f, CCT_EPSILON);
	if (0 == cmp[0] || 0 == cmp[1]) {
		result->distance = 0.0f;
		if (cmp[0] + cmp[1]) {
			result->hit_line = 0;
			mathVec3Copy(result->hit_point, cmp[0] ? ls[1] : ls[0]);
		}
		else {
			result->hit_line = 1;
		}
		return result;
	}
	else if (cmp[0] * cmp[1] < 0) {
		float ldir[3], cos_theta, ddir;
		mathVec3Sub(ldir, ls[1], ls[0]);
		mathVec3Normalized(ldir, ldir);
		cos_theta = mathVec3Dot(N, ldir);
		ddir = d[0] / cos_theta;
		result->distance = 0.0f;
		result->hit_line = 0;
		mathVec3Copy(result->hit_point, ls[0]);
		mathVec3AddScalar(result->hit_point, ldir, ddir);
		return result;
	}
	else {
		float min_d, *p;
		float cos_theta = mathVec3Dot(N, dir);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
			return NULL;
		if (cmp[0] > 0) {
			if (d[0] < d[1]) {
				min_d = d[0];
				p = ls[0];
			}
			else {
				min_d = d[1];
				p = ls[1];
			}
		}
		else {
			if (d[0] < d[1]) {
				min_d = d[1];
				p = ls[1];
			}
			else {
				min_d = d[0];
				p = ls[0];
			}
		}
		min_d /= cos_theta;
		if (fcmpf(min_d, 0.0f, CCT_EPSILON) < 0)
			return NULL;
		result->distance = min_d;
		result->hit_line = 0;
		mathVec3Copy(result->hit_point, p);
		mathVec3AddScalar(result->hit_point, dir, result->distance);
		return result;
	}
}

CCTResult_t* mathLineSegmentcastLineSegment(float ls1[2][3], float dir[3], float ls2[2][3], CCTResult_t* result) {
	float N[3];
	mathPlaneNormalByVertices2(ls1, dir, N);
	if (mathVec3IsZero(N)) {
		CCTResult_t results[2], *p_result;
		if (!mathRaycastLineSegment(ls1[0], dir, ls2, &results[0]))
			return NULL;
		if (!mathRaycastLineSegment(ls1[1], dir, ls2, &results[1]))
			return NULL;
		p_result = results[0].distance < results[1].distance ? &results[0] : &results[1];
		copy_result(result, p_result);
		return result;
	}
	else {
		float ls_plane[3][3], neg_dir[3];
		mathVec3Copy(ls_plane[0], ls1[0]);
		mathVec3Copy(ls_plane[1], ls1[1]);
		mathVec3Add(ls_plane[2], ls1[0], dir);
		if (!mathLineSegmentcastPlane(ls2, dir, ls_plane, result))
			return NULL;
		mathVec3Negate(neg_dir, dir);
		if (result->hit_line) {
			int is_parallel, dot;
			float lsdir1[3], lsdir2[3], test_n[3];
			CCTResult_t results[4], *p_result = NULL;

			mathVec3Sub(lsdir1, ls1[1], ls1[0]);
			mathVec3Sub(lsdir2, ls2[1], ls2[0]);
			mathVec3Cross(test_n, lsdir1, lsdir2);
			is_parallel = mathVec3IsZero(test_n);
			dot = mathVec3Dot(lsdir1, lsdir2);
			do {
				int c0 = 0, c1 = 0;
				if (mathRaycastLineSegment(ls1[0], dir, ls2, &results[0])) {
					c0 = 1;
					if (!p_result)
						p_result = &results[0];
					if (is_parallel) {
						if (!mathVec3Equal(results[0].hit_point, ls2[0]) && !mathVec3Equal(results[0].hit_point, ls2[1]))
							p_result->hit_line = 1;
						else if (mathVec3Equal(results[0].hit_point, ls2[0]) && dot > 0)
							p_result->hit_line = 1;
						else if (mathVec3Equal(results[0].hit_point, ls2[1]) && dot < 0)
							p_result->hit_line = 1;
						break;
					}
				}
				if (mathRaycastLineSegment(ls1[1], dir, ls2, &results[1])) {
					c1 = 1;
					if (!p_result || p_result->distance > results[1].distance)
						p_result = &results[1];
					if (is_parallel) {
						if (!(mathVec3Equal(results[1].hit_point, ls2[0]) && dot > 0) &&
							!(mathVec3Equal(results[1].hit_point, ls2[1]) && dot < 0))
						{
							p_result->hit_line = 1;
						}
						break;
					}
				}
				if (c0 && c1)
					break;
				if (mathRaycastLineSegment(ls2[0], neg_dir, ls1, &results[2])) {
					if (!p_result || p_result->distance > results[2].distance) {
						p_result = &results[2];
						mathVec3Copy(p_result->hit_point, ls2[0]);
					}
					if (is_parallel) {
						p_result->hit_line = 1;
						break;
					}
				}
				if (mathRaycastLineSegment(ls2[1], neg_dir, ls1, &results[3])) {
					if (!p_result || p_result->distance > results[3].distance) {
						p_result = &results[3];
						mathVec3Copy(p_result->hit_point, ls2[1]);
					}
					if (is_parallel) {
						p_result->hit_line = 1;
						break;
					}
				}
			} while (0);
			if (p_result) {
				copy_result(result, p_result);
				return result;
			}
			return NULL;
		}
		else {
			float hit_point[3];
			mathVec3Copy(hit_point, result->hit_point);
			if (!mathRaycastLineSegment(hit_point, neg_dir, ls1, result))
				return NULL;
			mathVec3Copy(result->hit_point, hit_point);
			return result;
		}
	}
}

CCTResult_t* mathSpherecastPlane(float o[3], float radius, float dir[3], float vertices[3][3], CCTResult_t* result) {
	float N[3], dn, d, cos_theta;
	mathPlaneNormalByVertices3(vertices, N);
	mathPointProjectionPlane(o, vertices[0], N, NULL, &dn);
	if (fcmpf(dn * dn, radius * radius, CCT_EPSILON) < 0) {
		result->distance = 0.0f;
		result->hit_line = 1;
		return NULL;
	}
	cos_theta = mathVec3Dot(N, dir);
	if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
		return NULL;
	d = dn / cos_theta;
	if (fcmpf(d, 0.0f, CCT_EPSILON) < 0)
		return NULL;
	d -= radius / dn * d;
	result->distance = d;
	result->hit_line = 0;
	mathVec3Copy(result->hit_point, o);
	mathVec3AddScalar(result->hit_point, dir, d);
	if (fcmpf(dn, 0.0f, CCT_EPSILON) < 0)
		mathVec3AddScalar(result->hit_point, N, -radius);
	else
		mathVec3AddScalar(result->hit_point, N, radius);
	return result;
}

CCTResult_t* mathSpherecastSphere(float o1[3], float r1, float dir[3], float o2[3], float r2, CCTResult_t* result) {
	float r12 = r1 + r2;
	float o1o2[3];
	mathVec3Sub(o1o2, o2, o1);
	if (fcmpf(mathVec3LenSq(o1o2), r12 * r12, CCT_EPSILON) < 0) {
		result->distance = 0.0f;
		result->hit_line = 1;
		return result;
	}
	else {
		float dot = mathVec3Dot(o1o2, dir);
		if (fcmpf(dot, 0.0f, CCT_EPSILON) <= 0)
			return NULL;
		else {
			float dn = mathVec3LenSq(o1o2) - dot * dot;
			float delta_len = r12 * r12 - dn;
			if (fcmpf(delta_len, 0.0f, CCT_EPSILON) < 0)
				return NULL;
			result->distance = dot - sqrtf(delta_len);
			result->hit_line = 0;
			mathVec3Copy(result->hit_point, o1);
			mathVec3AddScalar(result->hit_point, dir, result->distance);
			return result;
		}
	}
}

CCTResult_t* mathTrianglecastPlane(float tri[3][3], float dir, float vertices[3][3], CCTResult_t* result) {
	return NULL;
}
/*
int mathLinecastSphere(float ls[2][3], float dir[3], float center[3], float radius, float* distance, float normal[3], float point[3]) {
	const float epsilon = 0.000001f;
	float cl[3] = {
		ls[0][0] - center[0],
		ls[0][1] - center[1],
		ls[0][2] - center[2]
	};
	float l[3] = {
		ls[1][0] - ls[0][0],
		ls[1][1] - ls[0][1],
		ls[1][2] - ls[0][2]
	};
	float N[3], dn;
	mathVec3Normalized(N, mathVec3Cross(N, l, dir));
	dn = mathVec3Dot(N, cl);
	if (fcmpf(dn, 0.0f, epsilon) < 0) {
		dn = -dn;
		mathVec3Negate(N, N);
	}
	if (fcmpf(dn, radius, epsilon) >= 0)
		return 0;
	else {
		float P[3] = {
			center[0] + N[0] * dn,
			center[1] + N[1] * dn,
			center[2] + N[2] * dn
		};
		float lp[3] = {
			P[0] - ls[0][0],
			P[1] - ls[0][1],
			P[2] - ls[0][2]
		};
		float ldir[3], o[3], op[3], cos_theta, delta_len, op_len, len;
		delta_len = sqrt(radius * radius - dn * dn);
		mathVec3Normalized(ldir, l);
		dn = mathVec3Dot(lp, ldir);
		o[0] = ls[0][0] + ldir[0] * dn;
		o[1] = ls[0][1] + ldir[1] * dn;
		o[2] = ls[0][2] + ldir[2] * dn;
		op[0] = P[0] - o[0];
		op[1] = P[1] - o[1];
		op[2] = P[2] - o[2];
		op_len = mathVec3Len(op);
		if (fcmpf(op_len, delta_len, epsilon) < 0) {
			*distance = 0.0f;
			normal[0] = normal[1] = normal[2] = 0.0f;
			return 1;
		}
		mathVec3Normalized(normal, op);
		cos_theta = mathVec3Dot(normal, dir);
		len = op_len - delta_len;
		*distance = len / cos_theta;
		point[0] = o[0] + normal[0] * len;
		point[1] = o[1] + normal[1] * len;
		point[2] = o[2] + normal[2] * len;
		mathVec3Negate(normal, normal);
		return 1;
	}
}

int mathLineSegmentcastSphere(float ls[2][3], float dir[3], float center[3], float radius, float* distance, float normal[3], float point[3]) {
	float t[2], min_t;
	float n[2][3], p[2][3], *p_n, *p_p;
	int c[2];
	if (mathLinecastSphere(ls, dir, center, radius, distance, normal, point)) {
		if (!mathVec3IsZero(normal)) {
			float p[3] = {
				point[0] - *distance * dir[0],
				point[1] - *distance * dir[1],
				point[2] - *distance * dir[2]
			};
			return mathLineSegmentHasPoint(ls[0], ls[1], p);
		}
	}
	c[0] = mathRaycastSphere(ls[0], dir, center, radius, &t[0], n[0], p[0]);
	c[1] = mathRaycastSphere(ls[1], dir, center, radius, &t[1], n[1], p[1]);
	if (select_min_result(c, t, n, p, 2, &min_t, &p_n, &p_p) < 0)
		return 0;
	*distance = min_t;
	normal[0] = p_n[0];
	normal[1] = p_n[1];
	normal[2] = p_n[2];
	point[0] = p_p[0];
	point[1] = p_p[1];
	point[2] = p_p[2];
	return 1;
}

int mathTrianglecastPlane(float tri[3][3], float dir[3], float vertices[3][3], float* distance, float normal[3], float point[3]) {
	int i;
	const float epsilon = 0.000001f;
	float v0v1[3] = {
		vertices[1][0] - vertices[0][0],
		vertices[1][1] - vertices[0][1],
		vertices[1][2] - vertices[0][2]
	};
	float v0v2[3] = {
		vertices[2][0] - vertices[0][0],
		vertices[2][1] - vertices[0][1],
		vertices[2][2] - vertices[0][2]
	};
	float v0tri[3][3] = {
		{
			tri[0][0] - vertices[0][0],
			tri[0][1] - vertices[0][1],
			tri[0][2] - vertices[0][2]
		},
		{
			tri[1][0] - vertices[0][0],
			tri[1][1] - vertices[0][1],
			tri[1][2] - vertices[0][2]
		},
		{
			tri[2][0] - vertices[0][0],
			tri[2][1] - vertices[0][1],
			tri[2][2] - vertices[0][2]
		}
	};
	float N[3], dot[3];
	mathVec3Normalized(N, mathVec3Cross(N, v0v1, v0v2));
	for (i = 0; i < 3; ++i) {
		dot[i] = mathVec3Dot(N, v0tri[i]);
	}
	if ((dot[0] <= epsilon && dot[1] <= epsilon && dot[2] <= epsilon) ||
		dot[0] >= epsilon && dot[1] >= epsilon && dot[2] >= epsilon)
	{
		float t[3], min_t;
		float n[3][3], p[3][3], *p_n, *p_p;
		int c[3], i;
		for (i = 0; i < 3; ++i) {
			c[i] = mathRaycastPlane(tri[i], dir, vertices, &t[i], n[i], p[i]);
		}
		if (select_min_result(c, t, n, p, 3, &min_t, &p_n, &p_p) < 0)
			return 0;
		*distance = min_t;
		normal[0] = p_n[0];
		normal[1] = p_n[1];
		normal[2] = p_n[2];
		point[0] = p_p[0];
		point[1] = p_p[1];
		point[2] = p_p[2];
	}
	else {
		*distance = 0.0f;
		normal[0] = normal[1] = normal[2] = 0.0f;
	}
	return 1;
}

int mathTrianglecastTriangle(float tri1[3][3], float dir[3], float tri2[3][3], float* distance, float normal[3], float point[3]) {
	float tri1_ls[3][2][3] = {
		{
			{ tri1[0][0], tri1[0][1], tri1[0][2] },
			{ tri1[1][0], tri1[1][1], tri1[1][2] }
		},
		{
			{ tri1[0][0], tri1[0][1], tri1[0][2] },
			{ tri1[2][0], tri1[2][1], tri1[2][2] }
		},
		{
			{ tri1[1][0], tri1[1][1], tri1[1][2] },
			{ tri1[2][0], tri1[2][1], tri1[2][2] }
		}
	};
	float tri2_ls[3][2][3] = {
		{
			{ tri2[0][0], tri2[0][1], tri2[0][2] },
			{ tri2[1][0], tri2[1][1], tri2[1][2] }
		},
		{
			{ tri2[0][0], tri2[0][1], tri2[0][2] },
			{ tri2[2][0], tri2[2][1], tri2[2][2] },
		},
		{
			{ tri2[1][0], tri2[1][1], tri2[1][2] },
			{ tri2[2][0], tri2[2][1], tri2[2][2] }
		}
	};
	float t[9], min_t;
	float n[9][3], p[9][3], *p_n, *p_p;
	int c[9], i, j;
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			int k = i * 3 + j;
			c[k] = mathLineSegmentcastLineSegment(tri1_ls[i], dir, tri2_ls[j], &t[k], n[k], p[k]);
		}
	}
	if (select_min_result(c, t, n, p, 9, &min_t, &p_n, &p_p) < 0) {
		for (i = 0; i < 3; ++i) {
			c[i] = mathRaycastTriangle(tri1[i], dir, tri2, &t[i], n[i], p[i]);
		}
		if (select_min_result(c, t, n, p, 3, &min_t, &p_n, &p_p) < 0) {
			float neg_dir[3];
			mathVec3Negate(neg_dir, dir);
			for (i = 0; i < 3; ++i) {
				c[i] = mathRaycastTriangle(tri2[i], neg_dir, tri1, &t[i], n[i], p[i]);
				if (!c[i]) {
					*distance = 0.0f;
					normal[0] = normal[1] = normal[2] = 0.0f;
					return 1;
				}
			}
			i = select_min_result(c, t, n, p, 3, &min_t, &p_n, &p_p);
			if (i < 0)
				return 0;
			mathVec3Negate(p_n, p_n);
			p_p = tri2[i];
		}
	}
	*distance = min_t;
	normal[0] = p_n[0];
	normal[1] = p_n[1];
	normal[2] = p_n[2];
	point[0] = p_p[0];
	point[1] = p_p[1];
	point[2] = p_p[2];
	return 1;
}

int mathSpherecastTriangle(float o[3], float radius, float dir[3], float tri[3][3], float* distance, float normal[3], float point[3]) {
	if (mathSpherecastPlane(o, radius, dir, tri, distance, normal, point) &&
		mathTriangleHasPoint(tri, point))
	{
		return 1;
	}
	else {
		float tri_ls[3][2][3] = {
			{
				{ tri[0][0], tri[0][1], tri[0][2] },
				{ tri[1][0], tri[1][1], tri[1][2] }
			},
			{
				{ tri[0][0], tri[0][1], tri[0][2] },
				{ tri[2][0], tri[2][1], tri[2][2] },
			},
			{
				{ tri[1][0], tri[1][1], tri[1][2] },
				{ tri[2][0], tri[2][1], tri[2][2] }
			}
		};
		float t[3], min_t;
		float n[3][3], p[3][3], *p_n, *p_p;
		int c[3], i;
		float neg_dir[3];
		mathVec3Negate(neg_dir, dir);
		for (i = 0; i < 3; ++i) {
			c[i] = mathLineSegmentcastSphere(tri_ls[i], neg_dir, o, radius, &t[i], n[i], p[i]);
		}
		if (select_min_result(c, t, n, p, 3, &min_t, &p_n, &p_p) < 0)
			return 0;
		*distance = min_t;
		normal[0] = -p_n[0];
		normal[1] = -p_n[1];
		normal[2] = -p_n[2];
		point[0] = p_p[0] - min_t * neg_dir[0];
		point[1] = p_p[1] - min_t * neg_dir[1];
		point[2] = p_p[2] - min_t * neg_dir[2];
		return 1;
	}
}
*/

#ifdef	__cplusplus
}
#endif
