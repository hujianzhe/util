//
// Created by hujianzhe
//

#include "math.h"

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
	return fcmpf(v[0], 0.0f, 1E-7f) == 0 &&
		fcmpf(v[1], 0.0f, 1E-7f) == 0 &&
		fcmpf(v[2], 0.0f, 1E-7f) == 0;
}

int mathVec3EqualVec3(float v1[3], float v2[3]) {
	return fcmpf(v1[0], v2[0], 1E-7f) == 0 &&
		fcmpf(v1[1], v2[1], 1E-7f) == 0 &&
		fcmpf(v1[2], v2[2], 1E-7f) == 0;
}

float mathVec3LenSq(float v[3]) {
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

float mathVec3Len(float v[3]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float* mathVec3Normalized(float r[3], float v[3]) {
	float len = mathVec3Len(v);
	if (fcmpf(len, 0.0f, 1E-7f) > 0) {
		float inv_len = 1.0f / len;
		r[0] = v[0] * inv_len;
		r[1] = v[1] * inv_len;
		r[2] = v[2] * inv_len;
	}
	return r;
}

float* mathVec3Negate(float r[3], float v[3]) {
	r[0] = -v[0];
	r[1] = -v[1];
	r[2] = -v[2];
	return r;
}

float* mathVec3Mul(float r[3], float v[3], float n) {
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

/*
float* mathQuatToEuler(float q[4], float e[3]) {
	e[2] = atan2f(2.0f * (q[2]*q[3] + q[0]*q[1]), q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3]);
	e[1] = atan2f(2.0f * (q[1]*q[2] + q[0]*q[3]), q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3]);
	e[0] = asinf(2.0f * (q[0]*q[2] - q[1]*q[3]));
	return e;
}
*/

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
	const qx = q[0], qy = q[1], qz = q[2], qw = q[3];
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

int mathRaycastTriangle(float origin[3], float dir[3], float vertices[3][3], float* t, float* u, float* v, float n[3], int* is_lay_on_plane) {
	const float epsilon = 1E-7f;
	float det, inv_det;
	float *v0 = vertices[0], *v1 = vertices[1], *v2 = vertices[2];
	float E1[3] = {
		v1[0] - v0[0],
		v1[1] - v0[1],
		v1[2] - v0[2]
	};
	float E2[3] = {
		v2[0] - v0[0],
		v2[1] - v0[1],
		v2[2] - v0[2]
	};
	float P[3], T[3], Q[3];
	mathVec3Cross(P, dir, E2);
	det = mathVec3Dot(E1, P);
	/* if raycast_dir parallel to triangle plane, return false */
	*is_lay_on_plane = 0;
	if (fcmpf(det, 0.0f, epsilon) == 0) {
		float VO[3] = {
			origin[0] - v0[0],
			origin[1] - v0[1],
			origin[2] - v0[2]
		};
		float N[3];
		mathVec3Normalized(N, mathVec3Cross(N, E1, E2));
		if (fcmpf(mathVec3Dot(N, VO), 0.0f, epsilon) == 0)
			*is_lay_on_plane = 1;
		return 0;
	}
	inv_det = 1.0f / det;
	T[0] = origin[0] - v0[0];
	T[1] = origin[1] - v0[1];
	T[2] = origin[2] - v0[2];
	*u = mathVec3Dot(T, P) * inv_det;
	if (fcmpf(*u, 0.0f, epsilon) < 0 || fcmpf(*u, 1.0f, epsilon) > 0)
		return 0;
	mathVec3Cross(Q, T, E1);
	*v = mathVec3Dot(dir, Q) * inv_det;
	if (fcmpf(*v, 0.0f, epsilon) < 0 || fcmpf(*v + *u, 1.0f, epsilon) > 0)
		return 0;
	*t = mathVec3Dot(E2, Q) * inv_det;
	/* return 1 */
	if (fcmpf(*t, 0.0f, epsilon) >= 0) {
		mathVec3Normalized(n, mathVec3Cross(n, E1, E2));
		if (fcmpf(mathVec3Dot(n, dir), 0.0f, epsilon) > 0)
			mathVec3Negate(n, n);
		return 1;
	}
	return 0;
}

int mathRaycastPlane(float origin[3], float dir[3], float vertices[3][3], float* t, float n[3], int* is_lay_on_plane) {
	const float epsilon = 1E-7f;
	float *v0 = vertices[0], *v1 = vertices[1], *v2 = vertices[2];
	float E1[3] = {
		v1[0] - v0[0],
		v1[1] - v0[1],
		v1[2] - v0[2]
	};
	float E2[3] = {
		v2[0] - v0[0],
		v2[1] - v0[1],
		v2[2] - v0[2]
	};
	float OV[3] = {
		v0[0] - origin[0],
		v0[1] - origin[1],
		v0[2] - origin[2]
	};
	float dn;
	mathVec3Normalized(n, mathVec3Cross(n, E1, E2));
	dn = mathVec3Dot(n, dir);
	if (fcmpf(dn, 0.0f, epsilon) == 0) {
		*is_lay_on_plane = (fcmpf(mathVec3Dot(OV, n), 0.0f, epsilon) == 0);
		return 0;
	}
	else {
		*is_lay_on_plane = 0;
		*t = mathVec3Dot(n, OV) / dn;
		if (fcmpf(*t, 0.0f, epsilon) < 0)
			return 0;
		if (fcmpf(dn, 0.0f, epsilon) > 0)
			mathVec3Negate(n, n);
		return 1;
	}
}

int mathRaycastPlaneByNormalDistance(float origin[3], float dir[3], float normal[3], float d, float* t) {
	const float epsilon = 1E-7f;
	float dn = mathVec3Dot(dir, normal);
	if (fcmpf(dn, 0.0f, epsilon) == 0)
		return 0;
	*t = (mathVec3Dot(origin, normal) + d) / dn;
	return fcmpf(*t, 0.0f, epsilon) >= 0;
}

int mathRaycastSphere(float origin[3], float dir[3], float center[3], float radius, float* nearest, float* farest, float n[3]) {
	const float epsilon = 1E-7f;
	float radius2 = radius * radius;
	float d, dr2;
	float v[3] = {
		center[0] - origin[0],
		center[1] - origin[1],
		center[2] - origin[2]
	};
	float oc2 = mathVec3LenSq(v);
	float dir_d = mathVec3Dot(dir, v);
	if (fcmpf(oc2, radius2, epsilon) < 0)
		return 0;
	else if (fcmpf(dir_d, 0.0f, epsilon) <= 0)
		return 0;

	dr2 = oc2 - dir_d * dir_d;
	if (fcmpf(dr2, radius2, epsilon) >= 0)
		return 0;

	d = sqrtf(radius2 - dr2);
	*nearest = dir_d - d;
	*farest = dir_d + d;
	n[0] = origin[0] + *nearest * dir[0] - center[0];
	n[1] = origin[1] + *nearest * dir[1] - center[1];
	n[2] = origin[2] + *nearest * dir[2] - center[2];
	mathVec3Normalized(n, n);
	return 1;
}

int mathRaycastBox(float origin[3], float dir[3], float center[3], float half[3], float quat[4], float* nearest, float* farest, float n[3]) {
	unsigned int i, has_t1 = 0, has_t2 = 0;
	const float epsilon = 1E-7f;
	float t1, t2, n1[3], n2[3];
	float min_x = center[0] - half[0];
	float max_x = center[0] + half[0];
	float min_y = center[1] - half[1];
	float max_y = center[1] + half[1];
	float min_z = center[2] - half[2];
	float max_z = center[2] + half[2];
	float vertices[8][3] = {
		{ center[0] + half[0], center[1] + half[1], center[2] + half[2] },
		{ center[0] + half[0], center[1] + half[1], center[2] - half[2] },
		{ center[0] + half[0], center[1] - half[1], center[2] + half[2] },
		{ center[0] + half[0], center[1] - half[1], center[2] - half[2] },
		{ center[0] - half[0], center[1] + half[1], center[2] + half[2] },
		{ center[0] - half[0], center[1] + half[1], center[2] - half[2] },
		{ center[0] - half[0], center[1] - half[1], center[2] + half[2] },
		{ center[0] - half[0], center[1] - half[1], center[2] - half[2] }
	};
	int indices[] = {
		0, 1, 2, 3,
		0, 4, 6, 2,
		0, 4, 5, 1,
		7, 6, 5, 4,
		7, 3, 2, 6,
		7, 5, 1, 3
	};
	for (i = 0; i < 8; ++i) {
		mathQuatMulVec3(vertices[i], quat, vertices[i]);
	}
	for (i = 0; i < sizeof(indices) / sizeof(indices[0]); i += 4) {
		float t, N[3];
		float v[3][3] = {
			{ vertices[indices[i]][0], vertices[indices[i]][1], vertices[indices[i]][2] },
			{ vertices[indices[i+1]][0], vertices[indices[i+1]][1], vertices[indices[i+1]][2] },
			{ vertices[indices[i+2]][0], vertices[indices[i+2]][1], vertices[indices[i+2]][2] }
		};
		int is_lay_on_plane;
		if (mathRaycastPlane(origin, dir, v, &t, N, &is_lay_on_plane)) {
			float p[3] = {
				origin[0] + dir[0] * t,
				origin[1] + dir[1] * t,
				origin[2] + dir[2] * t
			};
			mathQuatMulVec3(p, quat, p);
			if (fcmpf(p[0], min_x, epsilon) < 0 ||
				fcmpf(p[0], max_x, epsilon) > 0 ||
				fcmpf(p[1], min_y, epsilon) < 0 ||
				fcmpf(p[1], max_y, epsilon) > 0 ||
				fcmpf(p[2], min_z, epsilon) < 0 ||
				fcmpf(p[2], max_z, epsilon) > 0)
			{
				continue;
			}
			if (!has_t1) {
				t1 = t;
				n1[0] = N[0];
				n1[1] = N[1];
				n1[2] = N[2];
				has_t1 = 1;
			}
			else if (!has_t2 && fcmpf(t1, t, epsilon)) {
				t2 = t;
				n2[0] = N[0];
				n2[1] = N[1];
				n2[2] = N[2];
				has_t2 = 1;
			}
		}
		if (is_lay_on_plane) {
			return 0;
		}
	}
	if (!has_t1 || !has_t2)
		return 0;
	else if (fcmpf(t1, t2, epsilon) <= 0) {
		*nearest = t1;
		*farest = t2;
		n[0] = n1[0];
		n[1] = n1[1];
		n[2] = n1[2];
	}
	else {
		*nearest = t2;
		*farest = t1;
		n[0] = n2[0];
		n[1] = n2[1];
		n[2] = n2[2];
	}
	return 1;
}

int mathRaycastConvex(float origin[3], float dir[3], float(*vertices)[3], int indices[], unsigned int indices_len, float* nearest, float* farest, float n[3]) {
	const float epsilon = 1E-7f;
	int has_t1 = 0, has_t2 = 0;
	float t1, t2, n1[3], n2[3];
	unsigned int i;
	for (i = 0; i < indices_len; i += 3) {
		int is_lay_on_plane;
		float t, u, v, N[3];
		float points[3][3] = {
			{ vertices[indices[i]][0], vertices[indices[i]][1], vertices[indices[i]][2] },
			{ vertices[indices[i+1]][0], vertices[indices[i+1]][1], vertices[indices[i+1]][2] },
			{ vertices[indices[i+2]][0], vertices[indices[i+2]][1], vertices[indices[i+2]][2] }
		};
		if (mathRaycastTriangle(origin, dir, points, &t, &u, &v, N, &is_lay_on_plane)) {
			if (!has_t1) {
				t1 = t;
				n1[0] = N[0];
				n1[1] = N[1];
				n1[2] = N[2];
				has_t1 = 1;
			}
			else if (!has_t2 && fcmpf(t1, t, epsilon)) {
				t2 = t;
				n2[0] = N[0];
				n2[1] = N[1];
				n2[2] = N[2];
				has_t2 = 1;
			}
		}
		if (is_lay_on_plane) {
			return 0;
		}
	}
	if (!has_t1 || !has_t2)
		return 0;
	else if (fcmpf(t1, t2, epsilon) <= 0) {
		*nearest = t1;
		*farest = t2;
		n[0] = n1[0];
		n[1] = n1[1];
		n[2] = n1[2];
	}
	else {
		*nearest = t2;
		*farest = t1;
		n[0] = n2[0];
		n[1] = n2[1];
		n[2] = n2[2];
	}
	return 1;
}

#ifdef	__cplusplus
}
#endif