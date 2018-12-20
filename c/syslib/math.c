//
// Created by hujianzhe
//

#include "math.h"
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
a == b return =0, a > b return >0, a < b return <0;
*/
int fcmpf(float a, float b, float epsilon) {
	float v = a - b;
	/* a == b */
	if (v > -epsilon && v < epsilon)
		return 0;
	return v >= epsilon ? 1 : -1;
}
/*
a == b return 0, a > b return >0, a < b return <0;
*/
int fcmp(double a, double b, double epsilon) {
	double v = a - b;
	/* a == b */
	if (v > -epsilon && v < epsilon)
		return 0;
	return v >= epsilon ? 1 : -1;
}

float finvsqrtf(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i >> 1);
	x = *(float*)&i;
	x = x * (1.5f - xhalf * x * x);
	x = x * (1.5f - xhalf * x * x);
	x = x * (1.5f - xhalf * x * x);
	return x;
}
float fsqrtf(float x) { return 1.0f / finvsqrtf(x); }

double finvsqrt(double x) {
	double xhalf = 0.5 * x;
	long long i = *(long long*)&x;
	i = 0x5fe6ec85e7de30da - (i >> 1);
	x = *(double*)&i;
	x = x * (1.5 - xhalf * x * x);
	x = x * (1.5 - xhalf * x * x);
	x = x * (1.5 - xhalf * x * x);
	return x;
}
double fsqrt(double x) { return 1.0 / finvsqrt(x); }

int mathQuadraticEquation(float a, float b, float c, float r[2]) {
	int cmp;
	float delta;
	if (fcmpf(a, 0.0f, CCT_EPSILON) == 0)
		return 0;
	delta = b * b - 4.0f * a * c;
	cmp = fcmpf(delta, 0.0f, CCT_EPSILON);
	if (cmp < 0)
		return 0;
	else if (0 == cmp) {
		r[0] = r[1] = -b / a * 0.5f;
		return 1;
	}
	else {
		float sqrt_delta = sqrtf(delta);
		r[0] = (-b + sqrt_delta) / a * 0.5f;
		r[1] = (-b - sqrt_delta) / a * 0.5f;
		return 2;
	}
}

/*
	vec3 and quat
*/

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

float* mathCoordinateSystemTransform(float v[3], float new_origin[3], float new_axies[3][3], float new_v[3]) {
	float t[3];
	if (new_origin)/* if v is normal vector, this field must be NULL */
		mathVec3Sub(t, v, new_origin);
	else
		mathVec3Copy(t, v);
	new_v[0] = mathVec3Dot(t, new_axies[0]);
	new_v[1] = mathVec3Dot(t, new_axies[1]);
	new_v[2] = mathVec3Dot(t, new_axies[2]);
	return new_v;
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

float* mathQuatIdentity(float q[4]) {
	q[0] = q[1] = q[2] = 0.0f;
	q[3] = 1.0f;
	return q;
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

static void copy_result(CCTResult_t* dst, CCTResult_t* src) {
	if (dst == src)
		return;
	dst->distance = src->distance;
	dst->hit_point_cnt = src->hit_point_cnt;
	if (1 == src->hit_point_cnt) {
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

int mathLineIntersectLine(float ls1[2][3], float ls2[2][3], float np[3]) {
	float E1[3], E2[3], N[3];
	mathVec3Sub(E1, ls1[1], ls1[0]);
	mathVec3Sub(E2, ls2[0], ls1[0]);
	mathVec3Cross(N, E1, E2);
	if (mathVec3IsZero(N)) {
		mathVec3Sub(E2, ls2[1], ls1[0]);
		mathVec3Cross(N, E1, E2);
		if (mathVec3IsZero(N))
			return 0;
	}
	mathVec3Sub(E2, ls2[1], ls2[0]);
	if (fcmpf(mathVec3Dot(N, E2), 0.0f, CCT_EPSILON))
		return 0;
	else {
		int cmp;
		float p[3], op[3], d, cos_theta;
		mathPointProjectionLine(ls1[0], ls2, p, &d);
		if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
			mathVec3Copy(np, p);
		}
		mathVec3Normalized(op, mathVec3Sub(op, p, ls1[0]));
		mathVec3Normalized(E1, E1);
		cos_theta = mathVec3Dot(E1, op);
		cmp = fcmpf(cos_theta, 0.0f, CCT_EPSILON);
		if (0 == cmp)
			return 0;
		else if (cmp < 0)
			cos_theta = mathVec3Dot(mathVec3Negate(E1, E1), op);
		d /= cos_theta;
		mathVec3Copy(np, ls1[0]);
		mathVec3AddScalar(np, E1, d);
		return 1;
	}
}

float* mathPointLineSegmentNearestVertice(float p[3], float ls[2][3]) {
	float pls0[3], pls1[3];
	mathVec3Sub(pls0, ls[0], p);
	mathVec3Sub(pls1, ls[1], p);
	return mathVec3LenSq(pls0) > mathVec3LenSq(pls1) ? ls[1] : ls[0];
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

int mathTriangleHasPoint(float tri[3][3], float p[3], float* p_u, float* p_v) {
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
		if (p_u)
			*p_u = u;
		if (p_v)
			*p_v = v;
		return 1;
	}
}

int mathCircleHasPoint(float o[3], float radius, float normal[3], float p[3]) {
	int cmp;
	float op[3];
	mathVec3Sub(op, p, o);
	if (fcmpf(mathVec3Dot(op, normal), 0.0f, CCT_EPSILON))
		return 0;
	cmp = fcmpf(mathVec3LenSq(op), radius * radius, CCT_EPSILON);
	if (cmp > 0)
		return 0;
	if (0 == cmp)
		return 1;
	else
		return 2;
}

void mathCircleProjectPlane(float center[3], float radius, float c_normal[3], float p_normal[3], float p[2][3]) {
	float N[3], q[4], v[3];
	mathVec3Cross(N, c_normal, p_normal);
	mathQuatFromAxisRadian(q, N, (float)M_PI * 0.5f);
	mathVec3Copy(v, c_normal);
	mathQuatMulVec3(v, q, v);
	mathVec3AddScalar(mathVec3Copy(p[0], center), v, radius);
	mathVec3AddScalar(mathVec3Copy(p[1], center), mathVec3Negate(v, v), radius);
}

int mathCircleIntersectPlane(float o[3], float r, float circle_normal[3], float circle_project_point[2][3], float plane_vertice[3], float plane_normal[3], float p[2][3]) {
	float d[2];
	int i, cmp[2];
	for (i = 0; i < 2; ++i) {
		mathPointProjectionPlane(circle_project_point[i], plane_vertice, plane_normal, NULL, &d[i]);
		cmp[i] = fcmpf(d[i], 0.0f, CCT_EPSILON);
	}
	if (cmp[0] * cmp[1] > 0)
		return 0;
	else if (0 == cmp[0] + cmp[1])
		return -1;
	else if (0 == cmp[0]) {
		mathVec3Copy(p[0], circle_project_point[0]);
		return 1;
	}
	else if (0 == cmp[1]) {
		mathVec3Copy(p[1], circle_project_point[1]);
		return 1;
	}
	else {
		float lsdir[3], q[4], lp[3], cos_theta, distance = d[0];
		mathVec3Sub(lsdir, circle_project_point[1], circle_project_point[0]);
		mathVec3Normalized(lsdir, lsdir);
		distance /= mathVec3Dot(plane_normal, lsdir);
		mathVec3AddScalar(mathVec3Copy(lp, circle_project_point[0]), lsdir, distance);
		mathQuatFromAxisRadian(q, circle_normal, (float)M_PI * 0.5f);
		mathQuatMulVec3(lsdir, q, lsdir);
		distance = sqrtf((radius * radius - (distance - radius) * (distance - radius)));
		mathVec3AddScalar(mathVec3Copy(p[0], lp), lsdir, distance);
		mathVec3AddScalar(mathVec3Copy(p[1], lp), mathVec3Negate(lsdir, lsdir), distance);
		return 2;
	}
}

int mathSphereHasPoint(float o[3], float radius, float p[3]) {
	float op[3];
	int cmp = fcmpf(mathVec3LenSq(mathVec3Sub(op, p, o)), radius * radius, CCT_EPSILON);
	if (cmp > 0)
		return 0;
	if (0 == cmp)
		return 1;
	else
		return 2;
}

int mathSphereHasLineSegment(float o[3], float radius, float ls[2][3], float pointcut[3]) {
	int c[2];
	c[0] = mathSphereHasPoint(o, radius, ls[0]);
	c[1] = mathSphereHasPoint(o, radius, ls[1]);
	if (0 == c[0] + c[1]) {
		float pl[3], plo[3];
		mathPointProjectionLine(o, ls, pl, NULL);
		if (!mathLineSegmentHasPoint(ls, pl))
			return 0;
		mathVec3Sub(plo, o, pl);
		c[0] = fcmpf(mathVec3LenSq(plo), radius * radius, CCT_EPSILON);
		if (c[0] < 0)
			return 2;
		if (0 == c[0]) {
			if (pointcut)
				mathVec3Copy(pointcut, pl);
			return 1;
		}
		return 0;
	}
	else if (c[0] + c[1] >= 2)
		return 2;
	if (pointcut) {
		if (c[0])
			mathVec3Copy(pointcut, ls[0]);
		else
			mathVec3Copy(pointcut, ls[1]);
	}
	return 1;
}

float* mathTriangleGetPoint(float tri[3][3], float u, float v, float p[3]) {
	float v0[3], v1[3], v2[3];
	mathVec3MultiplyScalar(v0, tri[0], 1.0f - u - v);
	mathVec3MultiplyScalar(v1, tri[1], u);
	mathVec3MultiplyScalar(v2, tri[2], v);
	return mathVec3Add(p, mathVec3Add(p, v0, v1), v2);
}

CCTResult_t* mathRaycastLine(float o[3], float dir[3], float ls[2][3], CCTResult_t* result) {
	float N[3], p[3], dl, dp, lsdir[3];
	mathPointProjectionLine(o, ls, p, &dl);
	if (fcmpf(dl, 0.0f, CCT_EPSILON) == 0) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	mathVec3Sub(lsdir, ls[1], ls[0]);
	mathVec3Cross(N, lsdir, dir);
	if (mathVec3IsZero(N))
		return NULL;
	mathPointProjectionPlane(o, ls[0], N, NULL, &dp);
	if (fcmpf(dp, 0.0f, CCT_EPSILON))
		return NULL;
	else {
		float op[3], cos_theta;
		mathVec3Normalized(op, mathVec3Sub(op, p, o));
		cos_theta = mathVec3Dot(dir, op);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) <= 0)
			return NULL;
		result->distance = dl / cos_theta;
		result->hit_point_cnt = 1;
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
			float ov[3], d;
			float* v = mathPointLineSegmentNearestVertice(o, ls);
			mathVec3Sub(ov, v, o);
			d = mathVec3Dot(ov, dir);
			if (fcmpf(d, 0.0f, CCT_EPSILON) < 0 || fcmpf(d * d, mathVec3LenSq(ov), CCT_EPSILON))
				return NULL;
			result->distance = d;
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, v);
			return result;
		}
	}
	return NULL;
}

CCTResult_t* mathRaycastPlane(float o[3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result) {
	float d, cos_theta;
	mathPointProjectionPlane(o, vertice, normal, NULL, &d);
	if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	cos_theta = mathVec3Dot(dir, normal);
	if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
		return NULL;
	d /= cos_theta;
	if (fcmpf(d, 0.0f, CCT_EPSILON) < 0)
		return NULL;
	result->distance = d;
	result->hit_point_cnt = 1;
	mathVec3Copy(result->hit_point, o);
	mathVec3AddScalar(result->hit_point, dir, d);
	return result;
}

CCTResult_t* mathRaycastTriangle(float o[3], float dir[3], float tri[3][3], CCTResult_t* result) {
	float N[3];
	if (mathRaycastPlane(o, dir, tri[0], mathPlaneNormalByVertices3(tri, N), result)) {
		if (mathTriangleHasPoint(tri, result->hit_point, NULL, NULL))
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
	if (fcmpf(oc2, radius2, CCT_EPSILON) <= 0) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	else if (fcmpf(dir_d, 0.0f, CCT_EPSILON) <= 0)
		return NULL;

	dr2 = oc2 - dir_d * dir_d;
	if (fcmpf(dr2, radius2, CCT_EPSILON) > 0)
		return NULL;

	d = sqrtf(radius2 - dr2);
	result->distance = dir_d - d;
	result->hit_point_cnt = 1;
	mathVec3Copy(result->hit_point, o);
	mathVec3AddScalar(result->hit_point, dir, result->distance);
	return result;
}

CCTResult_t* mathRaycastCircle(float o[3], float dir[3], float center[3], float radius, float normal[3], CCTResult_t* result) {
	if (mathRaycastPlane(o, dir, center, normal, result)) {
		if (mathCircleHasPoint(center, radius, normal, result->hit_point))
			return result;
		else if (fcmpf(result->distance, 0.0f, CCT_EPSILON) == 0) {
			float dn_sq, oc[3], dot, radius_sq;
			if (fcmpf(mathVec3Dot(normal, dir), 0.0f, CCT_EPSILON))
				return NULL;
			mathVec3Sub(oc, center, o);
			dot = mathVec3Dot(oc, dir);
			if (fcmpf(dot, 0.0f, CCT_EPSILON) <= 0)
				return NULL;
			dn_sq = mathVec3LenSq(oc) - dot * dot;
			radius_sq = radius * radius;
			if (fcmpf(dn_sq, radius_sq, CCT_EPSILON) > 0)
				return NULL;
			result->distance = dot - sqrt(radius_sq - dn_sq);
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, o);
			mathVec3AddScalar(result->hit_point, dir, result->distance);
			return result;
		}
	}
	return NULL;
}

CCTResult_t* mathRaycastCylinder(float o[3], float dir[3], float p0[3], float p1[3], float radius, CCTResult_t* result) {
	float new_axies[3][3], p0p1len, dot;
	float new_o[3], z_axies_normal[3] = { 0.0f, 0.0f, 1.0f };
	mathVec3Sub(new_axies[2], p1, p0);
	p0p1len = mathVec3Len(new_axies[2]);
	mathVec3Normalized(new_axies[2], new_axies[2]);
	new_axies[1][0] = 0.0f;
	new_axies[1][1] = new_axies[2][2];
	new_axies[1][2] = new_axies[2][1];
	mathVec3Cross(new_axies[0], new_axies[1], new_axies[2]);
	mathVec3Normalized(new_axies[0], new_axies[0]);
	mathCoordinateSystemTransform(o, p0, new_axies, new_o);
	dot = mathVec3Dot(new_o, z_axies_normal);
	if (new_o[2] > -CCT_EPSILON && new_o[2] < p0p1len + CCT_EPSILON &&
		fcmpf(mathVec3LenSq(new_o) - dot * dot, radius * radius, CCT_EPSILON) <= 0)
	{
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, o);
		return result;
	}
	else {
		int rcnt;
		CCTResult_t results[2], *p_result;
		float new_dir[3], new_p[3];
		float A, B, C, r[2];
		mathCoordinateSystemTransform(dir, NULL, new_axies, new_dir);
		A = new_dir[0] * new_dir[0] + new_dir[1] * new_dir[1];
		B = 2.0f * (new_o[0] * new_dir[0] + new_o[1] * new_dir[1]);
		C = new_o[0] * new_o[0] + new_o[1] * new_o[1] - radius * radius;
		rcnt = mathQuadraticEquation(A, B, C, r);
		if (2 == rcnt) {
			int has_zero = 0;
			float z;
			if (fcmpf(r[0], 0.0f, CCT_EPSILON) < 0)
				--rcnt;
			else {
				z = new_o[2] + new_dir[2] * r[0];
				if (z <= -CCT_EPSILON || z >= p0p1len + CCT_EPSILON)
					--rcnt;
			}

			if (fcmpf(r[1], 0.0f, CCT_EPSILON) < 0)
				--rcnt;
			else {
				z = new_o[2] + new_dir[2] * r[1];
				if (z <= -CCT_EPSILON || z >= p0p1len + CCT_EPSILON)
					--rcnt;
			}

			if (0 == rcnt)
				return NULL;
			if (2 == rcnt) {
				float t = r[0] < r[1] ? r[0] : r[1];
				result->distance = t;
				result->hit_point_cnt = 1;
				mathVec3Copy(result->hit_point, o);
				mathVec3AddScalar(result->hit_point, dir, t);
				return result;
			}
		}
		else if (1 == rcnt) {
			float z = new_o[2] + new_dir[2] * r[0];
			if (z > -CCT_EPSILON && z < p0p1len + CCT_EPSILON) {
				result->distance = r[0];
				result->hit_point_cnt = 1;
				mathVec3Copy(result->hit_point, o);
				mathVec3AddScalar(result->hit_point, dir, r[0]);
				return result;
			}
		}
		new_p[0] = 0.0f;
		new_p[1] = 0.0f;
		new_p[2] = p0p1len;
		p_result = NULL;
		if (mathRaycastCircle(new_o, new_dir, new_p, radius, z_axies_normal, &results[0]) &&
			(!p_result || p_result->distance > results[0].distance))
		{
			p_result = &results[0];
		}
		new_p[2] = 0.0f;
		if (mathRaycastCircle(new_o, new_dir, new_p, radius, z_axies_normal, &results[1]) &&
			(!p_result || p_result->distance > results[1].distance))
		{
			p_result = &results[1];
		}
		if (p_result) {
			copy_result(result, p_result);
			mathVec3Copy(result->hit_point, o);
			mathVec3AddScalar(result->hit_point, dir, result->distance);
			return result;
		}
		return NULL;
	}
}

CCTResult_t* mathLineSegmentcastPlane(float ls[2][3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result) {
	int cmp[2];
	float d[2];
	mathPointProjectionPlane(ls[0], vertice, normal, NULL, &d[0]);
	mathPointProjectionPlane(ls[1], vertice, normal, NULL, &d[1]);
	cmp[0] = fcmpf(d[0], 0.0f, CCT_EPSILON);
	cmp[1] = fcmpf(d[1], 0.0f, CCT_EPSILON);
	if (0 == cmp[0] || 0 == cmp[1]) {
		result->distance = 0.0f;
		if (cmp[0] + cmp[1]) {
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, cmp[0] ? ls[1] : ls[0]);
		}
		else {
			result->hit_point_cnt = -1;
		}
		return result;
	}
	else if (cmp[0] * cmp[1] < 0) {
		float ldir[3], cos_theta, ddir;
		mathVec3Sub(ldir, ls[1], ls[0]);
		mathVec3Normalized(ldir, ldir);
		cos_theta = mathVec3Dot(normal, ldir);
		ddir = d[0] / cos_theta;
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, ls[0]);
		mathVec3AddScalar(result->hit_point, ldir, ddir);
		return result;
	}
	else {
		float min_d, *p;
		float cos_theta = mathVec3Dot(normal, dir);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
			return NULL;
		else if (fcmpf(d[0], d[1], CCT_EPSILON) == 0) {
			min_d = d[0];
			result->hit_point_cnt = -1;
		}
		else {
			result->hit_point_cnt = 1;
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
		}
		min_d /= cos_theta;
		if (fcmpf(min_d, 0.0f, CCT_EPSILON) < 0)
			return NULL;
		result->distance = min_d;
		if (1 == result->hit_point_cnt) {
			mathVec3Copy(result->hit_point, p);
			mathVec3AddScalar(result->hit_point, dir, result->distance);
		}
		return result;
	}
}

CCTResult_t* mathLineSegmentcastLineSegment(float ls1[2][3], float dir[3], float ls2[2][3], CCTResult_t* result) {
	float N[3], lsdir1[3];
	mathVec3Sub(lsdir1, ls1[1], ls1[0]);
	mathVec3Cross(N, lsdir1, dir);
	if (mathVec3IsZero(N)) {
		int c0, c1;
		CCTResult_t results[2], *p_result;
		c0 = (mathRaycastLineSegment(ls1[0], dir, ls2, &results[0]) != NULL);
		c1 = (mathRaycastLineSegment(ls1[1], dir, ls2, &results[1]) != NULL);
		if (!c0 && !c1)
			return NULL;
		else if (c0 && c1) {
			p_result = results[0].distance < results[1].distance ? &results[0] : &results[1];
			copy_result(result, p_result);
		}
		else {
			result->distance = 0.0f;
			result->hit_point_cnt = -1;
		}
		return result;
	}
	else {
		float neg_dir[3];
		if (!mathLineSegmentcastPlane(ls2, dir, ls1[0], N, result))
			return NULL;
		mathVec3Negate(neg_dir, dir);
		if (result->hit_point_cnt < 0) {
			int is_parallel, dot;
			float lsdir2[3], test_n[3];
			CCTResult_t results[4], *p_result = NULL;

			mathVec3Sub(lsdir2, ls2[1], ls2[0]);
			mathVec3Cross(test_n, lsdir1, lsdir2);
			is_parallel = mathVec3IsZero(test_n);

			if (is_parallel) {
				dot = mathVec3Dot(lsdir1, lsdir2);
			}
			else {
				float p[3], op[3], d, cos_theta;
				mathPointProjectionLine(ls1[0], ls2, p, &d);
				if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
					result->distance = 0.0f;
					result->hit_point_cnt = 1;
					mathVec3Copy(result->hit_point, ls1[0]);
					return result;
				}
				mathVec3Normalized(op, mathVec3Sub(op, p, ls1[0]));
				mathVec3Normalized(lsdir1, lsdir1);
				cos_theta = mathVec3Dot(lsdir1, op);
				if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) < 0) {
					cos_theta = mathVec3Dot(mathVec3Negate(lsdir1, lsdir1), op);
				}
				d /= cos_theta;
				mathVec3Copy(p, ls1[0]);
				mathVec3AddScalar(p, lsdir1, d);
				if (mathLineSegmentHasPoint(ls1, p) && mathLineSegmentHasPoint(ls2, p)) {
					result->distance = 0.0f;
					result->hit_point_cnt = 1;
					mathVec3Copy(result->hit_point, p);
					return result;
				}
			}

			do {
				int c0 = 0, c1 = 0;
				if (mathRaycastLineSegment(ls1[0], dir, ls2, &results[0])) {
					c0 = 1;
					if (!p_result)
						p_result = &results[0];
					if (is_parallel) {
						if (!mathVec3Equal(results[0].hit_point, ls2[0]) && !mathVec3Equal(results[0].hit_point, ls2[1]))
							p_result->hit_point_cnt = -1;
						else if (mathVec3Equal(results[0].hit_point, ls2[0]) && dot > 0)
							p_result->hit_point_cnt = -1;
						else if (mathVec3Equal(results[0].hit_point, ls2[1]) && dot < 0)
							p_result->hit_point_cnt = -1;
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
							p_result->hit_point_cnt = -1;
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
						p_result->hit_point_cnt = -1;
						break;
					}
				}
				if (mathRaycastLineSegment(ls2[1], neg_dir, ls1, &results[3])) {
					if (!p_result || p_result->distance > results[3].distance) {
						p_result = &results[3];
						mathVec3Copy(p_result->hit_point, ls2[1]);
					}
					if (is_parallel) {
						p_result->hit_point_cnt = -1;
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

CCTResult_t* mathLineSegmentcastTriangle(float ls[2][3], float dir[3], float tri[3][3], CCTResult_t* result) {
	int i;
	CCTResult_t results[3], *p_result = NULL;
	float N[3];
	mathPlaneNormalByVertices3(tri, N);
	if (!mathLineSegmentcastPlane(ls, dir, tri[0], N, result))
		return NULL;
	else if (result->hit_point_cnt < 0) {
		int c[2];
		for (i = 0; i < 2; ++i) {
			float test_p[3];
			mathVec3Copy(test_p, ls[i]);
			mathVec3AddScalar(test_p, dir, result->distance);
			c[i] = mathTriangleHasPoint(tri, test_p, NULL, NULL);
		}
		if (c[0] && c[1])
			return result;
	}
	else if (mathTriangleHasPoint(tri, result->hit_point, NULL, NULL))
		return result;

	for (i = 0; i < 3; ++i) {
		float edge[2][3];
		mathVec3Copy(edge[0], tri[i % 3]);
		mathVec3Copy(edge[1], tri[(i + 1) % 3]);
		if (!mathLineSegmentcastLineSegment(ls, dir, edge, &results[i]))
			continue;
		if (!p_result)
			p_result = &results[i];
		else {
			int cmp = fcmpf(p_result->distance, results[i].distance, CCT_EPSILON);
			if (0 == cmp) {
				if (results[i].hit_point_cnt < 0 ||
					p_result->hit_point_cnt < 0 ||
					!mathVec3Equal(p_result->hit_point, results[i].hit_point))
				{
					p_result->hit_point_cnt = -1;
				}
				break;
			}
			else if (cmp > 0)
				p_result = &results[i];
		}
	}
	if (p_result) {
		copy_result(result, p_result);
		return result;
	}
	return NULL;
}

CCTResult_t* mathLineSegmentcastSphere(float ls[2][3], float dir[3], float center[3], float radius, CCTResult_t* result) {
	int c = mathSphereHasLineSegment(center, radius, ls, result->hit_point);
	if (1 == c) {
		result->distance = 0.0f;
		result->hit_point_cnt = 1;
		return result;
	}
	else if (2 == c) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		float lsdir[3], N[3];
		mathVec3Sub(lsdir, ls[1], ls[0]);
		mathVec3Cross(N, lsdir, dir);
		if (mathVec3IsZero(N)) {
			CCTResult_t results[2], *p_result;
			if (!mathRaycastSphere(ls[0], dir, center, radius, &results[0]))
				return NULL;
			if (!mathRaycastSphere(ls[1], dir, center, radius, &results[1]))
				return NULL;
			p_result = results[0].distance < results[1].distance ? &results[0] : &results[1];
			copy_result(result, p_result);
			return result;
		}
		else {
			CCTResult_t results[2], *p_result;
			int i;
			float np[3], lp[3], npd, cos_theta;
			mathVec3Normalized(N, N);
			mathPointProjectionPlane(center, ls[0], N, np, &npd);
			if (fcmpf(npd * npd, radius * radius, CCT_EPSILON) > 0)
				return NULL;
			mathPointProjectionLine(np, ls, lp, NULL);
			if (mathLineSegmentHasPoint(ls, lp)) {
				float delta_len, lpnplen, lpnp[3], p[3], d, new_ls[2][3];
				mathVec3Sub(lpnp, np, lp);
				lpnplen = mathVec3Len(lpnp);
				mathVec3Normalized(lpnp, lpnp);
				delta_len = sqrtf(radius * radius - npd * npd);
				d = lpnplen - delta_len;
				mathVec3AddScalar(mathVec3Copy(p, lp), lpnp, d);
				cos_theta = mathVec3Dot(lpnp, dir);
				if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) <= 0)
					return NULL;
				d /= cos_theta;
				mathVec3AddScalar(mathVec3Copy(new_ls[0], ls[0]), dir, d);
				mathVec3AddScalar(mathVec3Copy(new_ls[1], ls[1]), dir, d);
				if (mathLineSegmentHasPoint(new_ls, p)) {
					result->distance = d;
					result->hit_point_cnt = 1;
					mathVec3Copy(result->hit_point, p);
					return result;
				}
			}
			p_result = NULL;
			for (i = 0; i < 2; ++i) {
				if (mathRaycastSphere(ls[i], dir, center, radius, &results[i]) &&
					(!p_result || p_result->distance > results[i].distance))
				{
					p_result = &results[i];
				}
			}
			if (p_result) {
				copy_result(result, p_result);
				return result;
			}
			return NULL;
		}
	}
}

CCTResult_t* mathLineSegmentcastCircle(float ls[2][3], float dir[3], float center[3], float radius, float normal[3], CCTResult_t* result) {
	if (!mathLineSegmentcastPlane(ls, dir, center, normal, result))
		return NULL;
	else if (result->hit_point_cnt < 0) {
		int c[2], i;
		float new_ls[2][3];
		for (i = 0; i < 2; ++i) {
			mathVec3AddScalar(mathVec3Copy(new_ls[i], ls[i]), dir, result->distance);
			c[i] = mathCircleHasPoint(center, radius, normal, new_ls[i]);
		}
		if (c[0] + c[1] >= 2) {
			result->hit_point_cnt = -1;
			return result;
		}
		else if (c[0]) {
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, new_ls[0]);
			return result;
		}
		else if (c[1]) {
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, new_ls[1]);
			return result;
		}
		else if (fcmpf(result->distance, 0.0f, CCT_EPSILON))
			return NULL;
		else {
			CCTResult_t results[2], *p_result;
			float lp[3];
			mathPointProjectionLine(center, ls, lp, NULL);
			if (mathLineSegmentHasPoint(ls, lp)) {
				float lpc[3], lpclen, cos_theta, d, p[3];
				mathVec3Sub(lpc, center, lp);
				lpclen = mathVec3LenSq(lpc);
				c[0] = fcmpf(lpclen, radius * radius, CCT_EPSILON);
				if (0 == c[0]) {
					result->distance = 0.0f;
					result->hit_point_cnt = 1;
					mathVec3Copy(result->hit_point, lp);
					return result;
				}
				else if (c[0] < 0) {
					result->distance = 0.0f;
					result->hit_point_cnt = -1;
					return result;
				}
				cos_theta = mathVec3Dot(lpc, normal);
				if (fcmpf(cos_theta, 0.0f, CCT_EPSILON))
					return NULL;
				cos_theta = mathVec3Dot(dir, lpc);
				if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) <= 0)
					return NULL;
				mathVec3Normalized(lpc, lpc);
				cos_theta = mathVec3Dot(dir, lpc);
				d = sqrtf(lpclen) - radius;
				mathVec3AddScalar(mathVec3Copy(p, lp), lpc, d);
				d /= cos_theta;
				mathVec3AddScalar(mathVec3Copy(new_ls[0], ls[0]), dir, d);
				mathVec3AddScalar(mathVec3Copy(new_ls[1], ls[1]), dir, d);
				if (mathLineSegmentHasPoint(new_ls, p)) {
					result->distance = d;
					result->hit_point_cnt = 1;
					mathVec3Copy(result->hit_point, p);
					return result;
				}
			}
			p_result = NULL;
			for (i = 0; i < 2; ++i) {
				if (mathRaycastCircle(ls[i], dir, center, radius, normal, &results[i]) &&
					(!p_result || p_result->distance > results[i].distance))
				{
					p_result = &results[i];
				}
			}
			if (p_result) {
				copy_result(result, p_result);
				return result;
			}
			return NULL;
		}
	}
	else if (mathCircleHasPoint(center, radius, normal, result->hit_point))
		return result;
	else {
		int i;
		CCTResult_t results[2], *p_result;
		float N[3], lsdir[3], p[2][3], d, lp[3], lpc[3], q[4];
		mathVec3Sub(lsdir, ls[1], ls[0]);
		mathVec3Cross(N, dir, lsdir);
		if (mathVec3IsZero(N))
			return NULL;
		mathVec3Normalized(N, N);
		mathCircleProjectPlane(center, radius, normal, N, p);
		mathVec3Sub(lsdir, p[1], p[0]);
		mathVec3Normalized(lsdir, lsdir);
		mathPointProjectionPlane(p[0], ls[0], N, NULL, &d);
		d /= mathVec3Dot(N, lsdir);
		mathVec3AddScalar(mathVec3Copy(lp, p[0]), lsdir, d);
		mathQuatFromAxisRadian(q, normal, (float)M_PI * 0.5f);
		mathQuatMulVec3(lsdir, q, lsdir);
		mathVec3Sub(lpc, center, lp);
		d = sqrtf(radius * radius - mathVec3LenSq(lpc));
		mathVec3AddScalar(mathVec3Copy(p[0], lp), lsdir, d);
		mathVec3AddScalar(mathVec3Copy(p[1], lp), mathVec3Negate(lsdir, lsdir), d);

		p_result = NULL;
		mathVec3Negate(lsdir, dir);
		for (i = 0; i < 2; ++i) {
			if (mathRaycastLineSegment(p[i], lsdir, ls, &results[i]) &&
				(!p_result || p_result->distance > results[i].distance))
			{
				p_result = &results[i];
				mathVec3Copy(p_result->hit_point, p[i]);
			}
		}
		if (p_result) {
			copy_result(result, p_result);
			return result;
		}
		return NULL;
	}
}

CCTResult_t* mathCirclecastPlane(float center[3], float radius, float c_normal[3], float dir[3], float vertice[3], float p_normal[3], CCTResult_t* result) {
	float N[3], cos_theta;
	mathVec3Cross(N, c_normal, p_normal);
	if (mathVec3IsZero(N)) {
		float d;
		mathPointProjectionPlane(center, vertice, p_normal, NULL, &d);
		if (fcmpf(d, 0.0f, CCT_EPSILON) == 0) {
			result->distance = 0.0f;
			result->hit_point_cnt = -1;
			return result;
		}
		cos_theta = mathVec3Dot(p_normal, dir);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
			return NULL;
		d /= cos_theta;
		if (fcmpf(d, 0.0f, CCT_EPSILON) < 0)
			return NULL;
		result->distance = d;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		int cmp[2];
		float p[2][3], d[2], min_d, *min_p, cos_theta;
		mathCircleProjectPlane(center, radius, c_normal, p_normal, p);
		mathPointProjectionPlane(p[0], vertice, p_normal, NULL, &d[0]);
		cmp[0] = fcmpf(d[0], 0.0f, CCT_EPSILON);
		if (0 == cmp[0]) {
			result->distance = 0.0f;
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, p[0]);
			return result;
		}
		mathPointProjectionPlane(p[1], vertice, p_normal, NULL, &d[1]);
		cmp[1] = fcmpf(d[1], 0.0f, CCT_EPSILON);
		if (0 == cmp[1]) {
			result->distance = 0.0f;
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, p[1]);
			return result;
		}
		if (cmp[0] * cmp[1] < 0) {
			result->distance = 0.0f;
			result->hit_point_cnt = -1;
			return result;
		}
		else if (cmp[0] < 0) {
			if (d[0] < d[1]) {
				min_d = d[1];
				min_p = p[1];
			}
			else {
				min_d = d[0];
				min_p = p[0];
			}
		}
		else {
			if (d[0] < d[1]) {
				min_d = d[0];
				min_p = p[0];
			}
			else {
				min_d = d[1];
				min_p = p[1];
			}
		}
		cos_theta = mathVec3Dot(dir, p_normal);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
			return NULL;
		min_d /= cos_theta;
		if (fcmpf(min_d, 0.0f, CCT_EPSILON) < 0)
			return NULL;
		result->distance = min_d;
		result->hit_point_cnt = 1;
		mathVec3Copy(result->hit_point, min_p);
		mathVec3AddScalar(result->hit_point, dir, min_d);
		return result;
	}
}

CCTResult_t* mathTrianglecastPlane(float tri[3][3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result) {
	CCTResult_t results[3], *p_result = NULL;
	int i;
	for (i = 0; i < 3; ++i) {
		float ls[2][3];
		mathVec3Copy(ls[0], tri[i % 3]);
		mathVec3Copy(ls[1], tri[(i + 1) % 3]);
		if (mathLineSegmentcastPlane(ls, dir, vertice, normal, &results[i])) {
			if (!p_result)
				p_result = &results[i];
			else {
				int cmp = fcmpf(p_result->distance, results[i].distance, CCT_EPSILON);
				if (0 == cmp) {
					if (results[i].hit_point_cnt < 0 ||
						p_result->hit_point_cnt < 0 ||
						!mathVec3Equal(p_result->hit_point, results[i].hit_point))
					{
						p_result->hit_point_cnt = -1;
					}
				}
				else if (cmp > 0)
					p_result = &results[i];
			}
		}
	}
	if (p_result) {
		copy_result(result, p_result);
		return result;
	}
	return NULL;
}

CCTResult_t* mathTrianglecastTriangle(float tri1[3][3], float dir[3], float tri2[3][3], CCTResult_t* result) {
	float neg_dir[3];
	CCTResult_t results[6], *p_result = NULL;
	int i;
	for (i = 0; i < 3; ++i) {
		float ls[2][3];
		mathVec3Copy(ls[0], tri1[i % 3]);
		mathVec3Copy(ls[1], tri1[(i + 1) % 3]);
		if (mathLineSegmentcastTriangle(ls, dir, tri2, &results[i]) &&
			(!p_result || p_result->distance > results[i].distance))
		{
			p_result = &results[i];
		}
	}
	mathVec3Negate(neg_dir, dir);
	for (; i < 6; ++i) {
		float ls[2][3];
		mathVec3Copy(ls[0], tri2[i % 3]);
		mathVec3Copy(ls[1], tri2[(i + 1) % 3]);
		if (mathLineSegmentcastTriangle(ls, neg_dir, tri1, &results[i]) &&
			(!p_result || p_result->distance > results[i].distance))
		{
			p_result = &results[i];
			if (results[i].hit_point_cnt > 0) {
				mathVec3AddScalar(results[i].hit_point, dir, results[i].distance);
			}
		}
	}
	if (p_result) {
		copy_result(result, p_result);
		return result;
	}
	return NULL;
}

/*
	     7+------+6			0 = ---
	     /|     /|			1 = +--
	    / |    / |			2 = ++-
	   / 4+---/--+5			3 = -+-
	 3+------+2 /    y   z	4 = --+
	  | /    | /     |  /	5 = +-+
	  |/     |/      |/		6 = +++
	 0+------+1      *---x	7 = -++
*/
static int Box_Edge_Indices[] = {
	0, 1,	1, 2,	2, 3,	3, 0,
	7, 6,	6, 5,	5, 4,	4, 7,
	1, 5,	6, 2,
	3, 7,	4, 0
};
static int Box_Triangle_Vertices_Indices[] = {
	0, 1, 2,	2, 3, 0,
	7, 6, 5,	5, 4, 7,
	1, 5, 6,	6, 2, 1,
	3, 7, 4,	4, 0, 3,
	3, 7, 6,	6, 2, 3,
	0, 4, 5,	5, 1, 0
};
static void AABBVertices(float o[3], float half[3], float v[8][3]) {
	v[0][0] = o[0] - half[0], v[0][1] = o[1] - half[1], v[0][2] = o[2] - half[2];
	v[1][0] = o[0] + half[0], v[1][1] = o[1] - half[1], v[1][2] = o[2] - half[2];
	v[2][0] = o[0] + half[0], v[2][1] = o[1] + half[1], v[2][2] = o[2] - half[2];
	v[3][0] = o[0] - half[0], v[3][1] = o[1] + half[1], v[3][2] = o[2] - half[2];
	v[4][0] = o[0] - half[0], v[4][1] = o[1] - half[1], v[4][2] = o[2] + half[2];
	v[5][0] = o[0] + half[0], v[5][1] = o[1] - half[1], v[5][2] = o[2] + half[2];
	v[6][0] = o[0] + half[0], v[6][1] = o[1] + half[1], v[6][2] = o[2] + half[2];
	v[7][0] = o[0] - half[0], v[7][1] = o[1] + half[1], v[7][2] = o[2] + half[2];
}

int mathAABBIntersectAABB(float o1[3], float half1[3], float o2[3], float half2[3]) {
	/*
	!(o2[0] - half2[0] > o1[0] + half1[0] || o1[0] - half1[0] > o2[0] + half2[0] ||
	o2[1] - half2[1] > o1[1] + half1[1] || o1[1] - half1[1] > o2[1] + half2[1] ||
	o2[2] - half2[2] > o1[2] + half1[2] || o1[2] - half1[2] > o2[2] + half2[2]);
	*/
	
	return !(o2[0] - o1[0] > half1[0] + half2[0] || o1[0] - o2[0] > half1[0] + half2[0] ||
			o2[1] - o1[1] > half1[1] + half2[1] || o1[1] - o2[1] > half1[1] + half2[1] ||
			o2[2] - o1[2] > half1[2] + half2[2] || o1[2] - o2[2] > half1[2] + half2[2]);
}

CCTResult_t* mathAABBcastPlane(float o[3], float half[3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result) {
	CCTResult_t* p_result = NULL;
	float v[8][3];
	int i;
	AABBVertices(o, half, v);
	for (i = 0; i < sizeof(Box_Triangle_Vertices_Indices) / sizeof(Box_Triangle_Vertices_Indices[0]); i += 3) {
		CCTResult_t result_temp;
		float tri[3][3];
		mathVec3Copy(tri[0], v[Box_Triangle_Vertices_Indices[i]]);
		mathVec3Copy(tri[1], v[Box_Triangle_Vertices_Indices[i + 1]]);
		mathVec3Copy(tri[2], v[Box_Triangle_Vertices_Indices[i + 2]]);
		if (mathTrianglecastPlane(tri, dir, vertice, normal, &result_temp) &&
			(!p_result || p_result->distance > result_temp.distance))
		{
			copy_result(result, &result_temp);
			p_result = result;
		}
	}
	return p_result;
}

CCTResult_t* mathAABBcastAABB(float o1[3], float half1[3], float dir[3], float o2[3], float half2[3], CCTResult_t* result) {
	if (mathAABBIntersectAABB(o1, half1, o2, half2)) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		CCTResult_t *p_result = NULL;
		int i;
		float v1[8][3], v2[8][3];
		AABBVertices(o1, half1, v1);
		AABBVertices(o2, half2, v2);
		for (i = 0; i < sizeof(Box_Triangle_Vertices_Indices) / sizeof(Box_Triangle_Vertices_Indices[0]); i += 3) {
			int j;
			float tri1[3][3];
			mathVec3Copy(tri1[0], v1[Box_Triangle_Vertices_Indices[i]]);
			mathVec3Copy(tri1[1], v1[Box_Triangle_Vertices_Indices[i + 1]]);
			mathVec3Copy(tri1[2], v1[Box_Triangle_Vertices_Indices[i + 2]]);
			for (j = 0; j < sizeof(Box_Triangle_Vertices_Indices) / sizeof(Box_Triangle_Vertices_Indices[0]); j += 3) {
				CCTResult_t result_temp;
				float tri2[3][3];
				mathVec3Copy(tri2[0], v2[Box_Triangle_Vertices_Indices[j]]);
				mathVec3Copy(tri2[1], v2[Box_Triangle_Vertices_Indices[j + 1]]);
				mathVec3Copy(tri2[2], v2[Box_Triangle_Vertices_Indices[j + 2]]);

				if (!mathTrianglecastTriangle(tri1, dir, tri2, &result_temp))
					continue;

				if (!p_result || p_result->distance > result_temp.distance) {
					p_result = result;
					copy_result(p_result, &result_temp);
				}
			}
		}
		return p_result;
	}
}

CCTResult_t* mathSpherecastPlane(float o[3], float radius, float dir[3], float vertices[3][3], CCTResult_t* result) {
	float N[3], dn, dn_abs, d, cos_theta;
	mathPlaneNormalByVertices3(vertices, N);
	mathPointProjectionPlane(o, vertices[0], N, NULL, &dn);
	if (fcmpf(dn * dn, radius * radius, CCT_EPSILON) < 0) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	cos_theta = mathVec3Dot(N, dir);
	if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
		return NULL;
	d = dn / cos_theta;
	if (fcmpf(d, 0.0f, CCT_EPSILON) < 0)
		return NULL;
	dn_abs = fcmpf(dn, 0.0f, CCT_EPSILON) > 0 ? dn : -dn;
	d -= radius / dn_abs * d;
	result->distance = d;
	result->hit_point_cnt = 1;
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
		result->hit_point_cnt = -1;
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
			result->hit_point_cnt = 1;
			mathVec3Copy(result->hit_point, o1);
			mathVec3AddScalar(result->hit_point, dir, result->distance);
			mathVec3AddScalar(result->hit_point, mathVec3Normalized(o1o2, o1o2), r1);
			return result;
		}
	}
}

CCTResult_t* mathSpherecastTriangle(float o[3], float radius, float dir[3], float tri[3][3], CCTResult_t* result) {
	if (!mathSpherecastPlane(o, radius, dir, tri, result))
		return NULL;
	if (result->hit_point_cnt > 0 && mathTriangleHasPoint(tri, result->hit_point, NULL, NULL))
		return result;
	else {
		CCTResult_t* p_result = NULL;
		int i;
		float neg_dir[3];
		mathVec3Negate(neg_dir, dir);
		for (i = 0; i < 3; ++i) {
			CCTResult_t result_temp;
			float ls[2][3];
			mathVec3Copy(ls[0], tri[i % 3]);
			mathVec3Copy(ls[1], tri[(i + 1) % 3]);
			if (mathLineSegmentcastSphere(ls, neg_dir, o, radius, &result_temp) &&
				(!p_result || p_result->distance > result_temp.distance))
			{
				copy_result(result, &result_temp);
				p_result = result;
			}
		}
		if (p_result)
			mathVec3AddScalar(p_result->hit_point, dir, p_result->distance);
		return p_result;
	}
}

CCTResult_t* mathSpherecastTrianglesPlane(float o[3], float radius, float dir[3], float vertices[][3], int indices[], int indicescnt, CCTResult_t* result) {
	float tri[3][3];
	mathVec3Copy(tri[0], vertices[indices[0]]);
	mathVec3Copy(tri[1], vertices[indices[1]]);
	mathVec3Copy(tri[2], vertices[indices[2]]);
	if (!mathSpherecastPlane(o, radius, dir, tri, result))
		return NULL;
	else if (result->hit_point_cnt > 0 && mathTriangleHasPoint(tri, result->hit_point, NULL, NULL))
		return result;
	else {
		float neg_dir[3];
		CCTResult_t *p_result;
		int i = 3;
		while (i < indicescnt) {
			mathVec3Copy(tri[0], vertices[indices[i++]]);
			mathVec3Copy(tri[1], vertices[indices[i++]]);
			mathVec3Copy(tri[2], vertices[indices[i++]]);
			if (result->hit_point_cnt > 0 && mathTriangleHasPoint(tri, result->hit_point, NULL, NULL))
				return result;
		}
		mathVec3Negate(neg_dir, dir);
		p_result = NULL;
		for (i = 0; i < indicescnt; i += 3) {
			int j;
			for (j = 0; j < 3; ++j) {
				CCTResult_t result_temp;
				float ls[2][3];
				mathVec3Copy(ls[0], vertices[indices[j % 3 + i]]);
				mathVec3Copy(ls[1], vertices[indices[(j + 1) % 3 + i]]);
				if (mathLineSegmentcastSphere(ls, neg_dir, o, radius, &result_temp) &&
					(!p_result || p_result->distance > result_temp.distance))
				{
					copy_result(result, &result_temp);
					p_result = result;
				}
			}
		}
		if (p_result)
			mathVec3AddScalar(p_result->hit_point, dir, p_result->distance);
		return p_result;
	}
}

CCTResult_t* mathSpherecastAABB(float o[3], float radius, float dir[3], float center[3], float half[3], CCTResult_t* result) {
	CCTResult_t *p_result = NULL;
	int i;
	float v[8][3];
	AABBVertices(center, half, v);
	for (i = 0; i < sizeof(Box_Triangle_Vertices_Indices) / sizeof(Box_Triangle_Vertices_Indices[0]); i += 6) {
		CCTResult_t result_temp;
		if (mathSpherecastTrianglesPlane(o, radius, dir, v, Box_Triangle_Vertices_Indices + i, 6, &result_temp) &&
			(!p_result || p_result->distance > result_temp.distance))
		{
			copy_result(result, &result_temp);
			p_result = result;
		}
	}
	return p_result;
}

CCTResult_t* mathCylindercastPlane(float p0[3], float p1[3], float radius, float dir[3], float vertices[3][3], CCTResult_t* result) {
	float dn0, dn1, N[3], pn0[3], pn1[3];
	mathPlaneNormalByVertices3(vertices, N);
	mathPointProjectionPlane(p0, vertices[0], N, pn0, &dn0);
	mathPointProjectionPlane(p1, vertices[0], N, pn1, &dn1);
	if (fcmpf(dn0 * dn1, 0.0f, CCT_EPSILON) <= 0) {
		result->distance = 0.0f;
		result->hit_point_cnt = -1;
		return result;
	}
	else {
		float p0p1[3], p0p1_lensq, d, cos_theta, pn[3], *p;
		float delta_d = dn1 - dn0;
		mathVec3Sub(p0p1, p1, p0);
		p0p1_lensq = mathVec3LenSq(p0p1);
		cos_theta = sqrtf((p0p1_lensq - delta_d * delta_d) / p0p1_lensq);
		result->hit_point_cnt = fcmpf(cos_theta, 1.0f, CCT_EPSILON) ? 1 : -1;
		if (fcmpf(dn0, 0.0f, CCT_EPSILON) < 0) {
			if (dn0 < dn1) {
				d = dn1;
				p = p1;
				mathVec3Sub(pn, pn1, p1);
			}
			else {
				d = dn0;
				p = p0;
				mathVec3Sub(pn, pn0, p0);
			}
			d += radius * cos_theta;
			if (fcmpf(d, 0.0f, CCT_EPSILON) > 0) {
				result->distance = 0.0f;
				result->hit_point_cnt = -1;
				return result;
			}
		}
		else {
			if (dn0 < dn1) {
				d = dn0;
				p = p0;
				mathVec3Sub(pn, pn0, p0);
			}
			else {
				d = dn1;
				p = p1;
				mathVec3Sub(pn, pn1, p1);
			}
			d -= radius * cos_theta;
			if (fcmpf(d, 0.0f, CCT_EPSILON) < 0) {
				result->distance = 0.0f;
				result->hit_point_cnt = -1;
				return result;
			}
		}
		cos_theta = mathVec3Dot(N, dir);
		if (fcmpf(cos_theta, 0.0f, CCT_EPSILON) == 0)
			return NULL;
		d /= cos_theta;
		if (fcmpf(d, 0.0f, CCT_EPSILON) < 0)
			return NULL;
		result->distance = d;
		if (result->hit_point_cnt > 0) {
			float q[4], v[3];
			mathVec3Cross(N, p0p1, N);
			mathQuatFromAxisRadian(q, N, (float)M_PI * 0.5f);
			mathQuatMulVec3(v, q, p0p1);
			if (fcmpf(mathVec3Dot(v, pn), 0.0f, CCT_EPSILON) < 0)
				mathVec3Negate(v, v);
			mathVec3Copy(result->hit_point, p);
			mathVec3AddScalar(result->hit_point, v, radius);
			mathVec3AddScalar(result->hit_point, dir, d);
		}
		return result;
	}
}

#ifdef	__cplusplus
}
#endif
