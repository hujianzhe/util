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
	return fcmpf(v1[0], v2[0], 0.000001f) == 0 &&
		fcmpf(v1[1], v2[1], 0.000001f) == 0 &&
		fcmpf(v1[2], v2[2], 0.000001f) == 0;
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


float* mathQuatNormalized(float r[4], float q[4]) {
	float m = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if (fcmpf(m, 0.0f, 1E-7f) > 0) {
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

float mathPointLineDistanceSq(float l1[3], float l2[3], float p[3]) {
	float l[3] = {
		l2[0] - l1[0],
		l2[1] - l1[1],
		l2[2] - l1[2]
	};
	float l1p[3] = {
		p[0] - l1[0],
		p[1] - l1[1],
		p[2] - l1[2]
	};
	float dot = mathVec3Dot(mathVec3Normalized(l, l), l1p);
	return mathVec3LenSq(l1p) - dot * dot;
}

int mathLineSegmentHasPoint(float l1[3], float l2[3], float p[3]) {
	float pl1[3] = {
		l1[0] - p[0],
		l1[1] - p[1],
		l1[2] - p[2]
	};
	if (mathVec3IsZero(pl1))
		return 1;
	else {
		float pl2[3] = {
			l2[0] - p[0],
			l2[1] - p[1],
			l2[2] - p[2]
		};
		if (mathVec3IsZero(pl2))
			return 1;
		mathVec3Normalized(pl1, pl1);
		mathVec3Normalized(pl2, pl2);
		return mathVec3EqualVec3(mathVec3Negate(pl1, pl1), pl2);
	}
}

float mathLineLineDistance(float a1[3], float a2[3], float b1[3], float b2[3]) {
	float ab[3] = {
		b1[0] - a1[0],
		b1[1] - a1[1],
		b1[2] - a1[2]
	};
	float l1[3] = {
		a2[0] - a1[0],
		a2[1] - a1[1],
		a2[2] - a1[2]
	};
	float l2[3] = {
		b2[0] - b1[0],
		b2[1] - b1[1],
		b2[2] - b1[1]
	};
	float N[3];
	mathVec3Cross(N, l1, l2);
	if (mathVec3IsZero(N))
		return 0.0f;
	else {
		float d = mathVec3Dot(ab, mathVec3Normalized(N, N));
		if (d < 0.0f)
			d = -d;
		return d;
	}
}

int mathLineSegmentcastLineSegment(float ls1[2][3], float dir[3], float ls2[2][3], float* t, float p[3]) {
	float n[3], t0, t1, t2, t3, neg_dir[3];
	int c0, c1, c2, c3;
	c0 = mathRaycastLineSegment(ls1[0], dir, ls2[0], ls2[1], &t0, n);
	c1 = mathRaycastLineSegment(ls1[1], dir, ls2[0], ls2[1], &t1, n);
	mathVec3Negate(neg_dir, dir);
	c2 = mathRaycastLineSegment(ls2[0], neg_dir, ls1[0], ls1[1], &t2, n);
	c3 = mathRaycastLineSegment(ls2[1], neg_dir, ls1[0], ls1[1], &t3, n);
	if (c0 || c1 || c2 || c3) {
		float* p_dir, *o;
		if (c0) {
			if (c1 && t0 > t1) {
				*t = t1;
				o = ls1[1];
				p_dir = dir;
			}
			else {
				*t = t0;
				o = ls1[0];
				p_dir = dir;
			}
			if (c2 && *t > t2) {
				*t = t2;
				o = ls2[0];
				p_dir = neg_dir;
			}
			if (c3 && *t > t3) {
				*t = t3;
				o = ls2[1];
				p_dir = neg_dir;
			}
		}
		else if (c1) {
			if (c2 && t1 > t2) {
				*t = t2;
				o = ls2[0];
				p_dir = neg_dir;
			}
			else {
				*t = t1;
				o = ls1[1];
				p_dir = dir;
			}
			if (c3 && *t > t3) {
				*t = t3;
				o = ls2[1];
				p_dir = neg_dir;
			}
		}
		else if (c2) {
			if (c3 && t2 > t3) {
				*t = t3;
				o = ls2[1];
				p_dir = neg_dir;
			}
			else {
				*t = t2;
				o = ls2[0];
				p_dir = neg_dir;
			}
		}
		else {
			*t = t3;
			o = ls2[1];
			p_dir = neg_dir;
		}
		p[0] = o[0];
		p[1] = o[1];
		p[2] = o[2];
		if (fcmpf(*t, 0.0f, 0.000001f) > 0) {
			p[0] += *t * p_dir[0];
			p[1] += *t * p_dir[1];
			p[2] += *t * p_dir[2];
		}
		return 1;
	}
	return 0;
}

int mathRaycastLine(float origin[3], float dir[3], float l1[3], float l2[3], float* t, float n[3]) {
	const float epsilon = 0.000001f;
	float dot, op[3], dn;
	float l1O[3] = {
		origin[0] - l1[0],
		origin[1] - l1[1],
		origin[2] - l1[2]
	};
	float l[3] = {
		l2[0] - l1[0],
		l2[1] - l1[1],
		l2[2] - l1[2]
	};
	mathVec3Cross(n, l1O, l);
	if (mathVec3IsZero(n)) {
		*t = 0.0f;
		n[0] = n[1] = n[2] = 0.0f;
		return 1;
	}
	dot = mathVec3Dot(n, dir);
	if (fcmpf(dot, 0.0f, epsilon))
		return 0;
	mathVec3Normalized(l, l);
	dot = mathVec3Dot(l1O, l);
	op[0] = l1[0] + l[0] * dot - origin[0];
	op[1] = l1[1] + l[1] * dot - origin[1];
	op[2] = l1[2] + l[2] * dot - origin[2];
	dn = mathVec3LenSq(op);
	mathVec3Normalized(n, op);
	dot = mathVec3Dot(n, dir);/* cos theta */
	if (fcmpf(dot, 0.0f, epsilon) <= 0)
		return 0;
	else if (fcmpf(dot, 1.0f, epsilon) == 0)
		*t = sqrtf(dn);
	else
		*t = sqrtf(dn) / dot;
	mathVec3Negate(n, n);
	return 1;
}

int mathRaycastLineSegment(float origin[3], float dir[3], float l1[3], float l2[3], float *t, float n[3]) {
	if (mathRaycastLine(origin, dir, l1, l2, t, n)) {
		const float epsilon = 0.000001f;
		float p[3] = {
			origin[0] + *t * dir[0],
			origin[1] + *t * dir[1],
			origin[2] + *t * dir[2]
		};
		if (mathLineSegmentHasPoint(l1, l2, p))
			return 1;
		else if (mathVec3IsZero(n)) {
			float pl1[3] = {
				l1[0] - p[0],
				l1[1] - p[1],
				l1[2] - p[2]
			};
			float pn[3], dot;
			mathVec3Cross(pn, pl1, dir);
			if (!mathVec3IsZero(pn))
				return 0;
			dot = mathVec3Dot(pl1, dir);
			if (fcmpf(dot, 0.0f, epsilon) < 0)
				return 0;
			else {
				float pl2[3] = {
					l2[0] - p[0],
					l2[1] - p[1],
					l2[2] - p[2]
				};
				float dn1 = mathVec3LenSq(pl1);
				float dn2 = mathVec3LenSq(pl2);
				*t = fcmpf(dn1, dn2, epsilon) < 0 ? dn1 : dn2;
				*t = sqrtf(*t);
				mathVec3Negate(n, dir);
				return 1;
			}
		}
		else
			return 0;
	}
	return 0;
}

static int mathTrianglePlaneHasPoint(float vertices[3][3], float p[3]) {
	const float epsilon = 0.000001f;
	float *a = vertices[0], *b = vertices[1], *c = vertices[2];
	float ap[3] = {
		p[0] - a[0],
		p[1] - a[1],
		p[2] - a[2]
	};
	float ab[3] = {
		b[0] - a[0],
		b[1] - a[1],
		b[2] - a[2]
	};
	float ac[3] = {
		c[0] - a[0],
		c[1] - a[1],
		c[2] - a[2]
	};
	float u, v;
	float dot_ac_ac = mathVec3Dot(ac, ac);
	float dot_ac_ab = mathVec3Dot(ac, ab);
	float dot_ac_ap = mathVec3Dot(ac, ap);
	float dot_ab_ab = mathVec3Dot(ab, ab);
	float dot_ab_ap = mathVec3Dot(ab, ap);
	float tmp = 1.0f / (dot_ac_ac * dot_ab_ab - dot_ac_ab * dot_ac_ab);
	u = (dot_ab_ab * dot_ac_ap - dot_ac_ab * dot_ab_ap) * tmp;
	if (fcmpf(u, 0.0f, epsilon) < 0 || fcmpf(u, 1.0f, epsilon) > 0)
		return 0;
	v = (dot_ac_ac * dot_ab_ap - dot_ac_ab * dot_ac_ap) * tmp;
	if (fcmpf(v, 0.0f, epsilon) < 0 || fcmpf(v + u, 1.0f, epsilon) > 0)
		return 0;
	return 1;
}

int mathRaycastTriangle(float origin[3], float dir[3], float vertices[3][3], float* t, float n[3]) {
	const float epsilon = 0.000001f;
	float det, inv_det, u, v;
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
	if (fcmpf(det, 0.0f, epsilon) == 0) {
		float VO[3] = {
			origin[0] - v0[0],
			origin[1] - v0[1],
			origin[2] - v0[2]
		};
		float N[3];
		mathVec3Normalized(N, mathVec3Cross(N, E1, E2));
		if (fcmpf(mathVec3Dot(N, VO), 0.0f, epsilon) == 0) {
			if (mathTrianglePlaneHasPoint(vertices, origin)) {
				*t = 0.0f;
				n[0] = n[1] = n[2] = 0.0f;
				return 1;
			}
			else {
				float t01, t02, t12, n01[3], n02[3], n12[3], *p_n;
				int c01 = mathRaycastLineSegment(origin, dir, vertices[0], vertices[1], &t01, n01);
				int c02 = mathRaycastLineSegment(origin, dir, vertices[0], vertices[2], &t02, n02);
				int c12 = mathRaycastLineSegment(origin, dir, vertices[1], vertices[2], &t12, n12);
				if (c01 || c02 || c12) {
					if (c01) {
						if (c02 && t02 < t01) {
							*t = t02;
							p_n = n02;
						}
						else {
							*t = t01;
							p_n = n01;
						}
						if (c12 && t12 < *t) {
							*t = t12;
							p_n = n12;
						}
					}
					else if (c02) {
						if (c12 && t12 < t02) {
							*t = t12;
							p_n = n12;
						}
						else {
							*t = t02;
							p_n = n02;
						}
					}
					else {
						*t = t12;
						p_n = n12;
					}
					n[0] = p_n[0];
					n[1] = p_n[1];
					n[2] = p_n[2];
					return 1;
				}
				else
					return 0;
			}
		}
		return 0;
	}
	inv_det = 1.0f / det;
	T[0] = origin[0] - v0[0];
	T[1] = origin[1] - v0[1];
	T[2] = origin[2] - v0[2];
	u = mathVec3Dot(T, P) * inv_det;
	if (fcmpf(u, 0.0f, epsilon) < 0 || fcmpf(u, 1.0f, epsilon) > 0)
		return 0;
	mathVec3Cross(Q, T, E1);
	v = mathVec3Dot(dir, Q) * inv_det;
	if (fcmpf(v, 0.0f, epsilon) < 0 || fcmpf(v + u, 1.0f, epsilon) > 0)
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

int mathRaycastPlane(float origin[3], float dir[3], float vertices[3][3], float* t, float n[3]) {
	const float epsilon = 0.000001f;
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
		if (fcmpf(mathVec3Dot(OV, n), 0.0f, epsilon) == 0) {
			n[0] = n[1] = n[2] = 0.0f;
			*t = 0.0f;
			return 1;
		}
		return 0;
	}
	else {
		*t = mathVec3Dot(n, OV) / dn;
		if (fcmpf(*t, 0.0f, epsilon) < 0)
			return 0;
		if (fcmpf(dn, 0.0f, epsilon) > 0)
			mathVec3Negate(n, n);
		return 1;
	}
}

int mathRaycastSphere(float origin[3], float dir[3], float center[3], float radius, float* nearest, float* farest, float n[3]) {
	const float epsilon = 0.000001f;
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

int mathRaycastConvex(float origin[3], float dir[3], float(*vertices)[3], int indices[], unsigned int indices_len, float* nearest, float* farest, float n[3]) {
	const float epsilon = 0.000001f;
	int has_t1 = 0, has_t2 = 0;
	float t1, t2, n1[3], n2[3];
	unsigned int i;
	for (i = 0; (!has_t1 || !has_t2) && i < indices_len; i += 3) {
		float t, N[3];
		float points[3][3] = {
			{ vertices[indices[i]][0], vertices[indices[i]][1], vertices[indices[i]][2] },
			{ vertices[indices[i+1]][0], vertices[indices[i+1]][1], vertices[indices[i+1]][2] },
			{ vertices[indices[i+2]][0], vertices[indices[i+2]][1], vertices[indices[i+2]][2] }
		};
		if (mathRaycastTriangle(origin, dir, points, &t, N)) {
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
	}
	if (!has_t1)
		return 0;
	else if (!has_t2) {
		*nearest = *farest = t1;
		n[0] = n1[0];
		n[1] = n1[1];
		n[2] = n1[2];
		return 1;
	}
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

int mathSpherecastPlane(float origin[3], float dir[3], float radius, float vertices[3][3], float* t, float n[3]) {
	if (mathRaycastPlane(origin, dir, vertices, t, n)) {
		const float epsilon = 0.000001f;
		float VO[3] = {
			origin[0] - vertices[0][0],
			origin[1] - vertices[0][1],
			origin[2] - vertices[0][2]
		};
		float dn = mathVec3Dot(VO, n);
		if (fcmpf(dn, 0.0f, epsilon) < 0) {
			dn = -dn;
		}
		if (fcmpf(radius, dn, epsilon) > 0) {
			*t = 0.0f;
			n[0] = n[1] = n[2] = 0.0f;
			return 1;
		}
		else if (fcmpf(radius, dn, epsilon) == 0) {
			*t = 0.0f;
		}
		else {
			*t *= 1.0f - (radius / dn);
		}
		return 1;
	}
	return 0;
}

#ifdef	__cplusplus
}
#endif
