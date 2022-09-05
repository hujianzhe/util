//
// Created by hujianzhe
//

#include "../../../inc/crt/math.h"
#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/obb.h"

#ifdef __cplusplus
extern "C" {
#endif

GeometryOBB_t* mathOBBFromAABB(GeometryOBB_t* obb, const float o[3], const float half[3]) {
	mathVec3Copy(obb->o, o);
	mathVec3Copy(obb->half, half);
	mathVec3Set(obb->axis[0], 1.0f, 0.0f, 0.0f);
	mathVec3Set(obb->axis[1], 0.0f, 1.0f, 0.0f);
	mathVec3Set(obb->axis[2], 0.0f, 0.0f, 1.0f);
	return obb;
}

/*
void mathOBBVertices(const GeometryOBB_t* obb, float v[8][3]) {
	mathVec3Copy(v[0], obb->o);
	mathVec3AddScalar(v[0], obb->axis[0], -obb->half[0]);
	mathVec3AddScalar(v[0], obb->axis[1], -obb->half[1]);
	mathVec3AddScalar(v[0], obb->axis[2], -obb->half[2]);
	mathVec3Copy(v[1], obb->o);
	mathVec3AddScalar(v[1], obb->axis[0], obb->half[0]);
	mathVec3AddScalar(v[1], obb->axis[1], -obb->half[1]);
	mathVec3AddScalar(v[1], obb->axis[2], -obb->half[2]);
	mathVec3Copy(v[2], obb->o);
	mathVec3AddScalar(v[2], obb->axis[0], obb->half[0]);
	mathVec3AddScalar(v[2], obb->axis[1], obb->half[1]);
	mathVec3AddScalar(v[2], obb->axis[2], -obb->half[2]);
	mathVec3Copy(v[3], obb->o);
	mathVec3AddScalar(v[3], obb->axis[0], -obb->half[0]);
	mathVec3AddScalar(v[3], obb->axis[1], obb->half[1]);
	mathVec3AddScalar(v[3], obb->axis[2], -obb->half[2]);
	mathVec3Copy(v[4], obb->o);
	mathVec3AddScalar(v[4], obb->axis[0], -obb->half[0]);
	mathVec3AddScalar(v[4], obb->axis[1], -obb->half[1]);
	mathVec3AddScalar(v[4], obb->axis[2], obb->half[2]);
	mathVec3Copy(v[5], obb->o);
	mathVec3AddScalar(v[5], obb->axis[0], obb->half[0]);
	mathVec3AddScalar(v[5], obb->axis[1], -obb->half[1]);
	mathVec3AddScalar(v[5], obb->axis[2], obb->half[2]);
	mathVec3Copy(v[6], obb->o);
	mathVec3AddScalar(v[6], obb->axis[0], obb->half[0]);
	mathVec3AddScalar(v[6], obb->axis[1], obb->half[1]);
	mathVec3AddScalar(v[6], obb->axis[2], obb->half[2]);
	mathVec3Copy(v[7], obb->o);
	mathVec3AddScalar(v[7], obb->axis[0], -obb->half[0]);
	mathVec3AddScalar(v[7], obb->axis[1], obb->half[1]);
	mathVec3AddScalar(v[7], obb->axis[2], obb->half[2]);
}

void mathOBBToAABB(const GeometryOBB_t* obb, float o[3], float half[3]) {
	int i;
	float v[8][3], min_v[3], max_v[3];
	mathOBBVertices(obb, v);
	for (i = 0; i < 8; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			if (!i || min_v[j] > v[i][j]) {
				min_v[j] = v[i][j];
			}
			if (!i || max_v[j] < v[i][j]) {
				max_v[j] = v[i][j];
			}
		}
	}
	mathVec3Copy(o, obb->o);
	for (i = 0; i < 3; ++i) {
		half[i] = 0.5f * (max_v[i] - min_v[i]);
	}
}

void mathOBBPlaneVertices(const GeometryOBB_t* obb, float v[6][3]) {
	mathVec3Copy(v[0], obb->o);
	mathVec3AddScalar(v[0], obb->axis[2], obb->half[2]);
	mathVec3Copy(v[1], obb->o);
	mathVec3AddScalar(v[1], obb->axis[2], -obb->half[2]);

	mathVec3Copy(v[2], obb->o);
	mathVec3AddScalar(v[2], obb->axis[0], obb->half[0]);
	mathVec3Copy(v[3], obb->o);
	mathVec3AddScalar(v[3], obb->axis[0], -obb->half[0]);

	mathVec3Copy(v[4], obb->o);
	mathVec3AddScalar(v[4], obb->axis[1], obb->half[1]);
	mathVec3Copy(v[5], obb->o);
	mathVec3AddScalar(v[5], obb->axis[1], -obb->half[1]);
}

int mathOBBHasPoint(const GeometryOBB_t* obb, const float p[3]) {
	float op[3], dot;
	mathVec3Sub(op, p, obb->o);
	dot = mathVec3Dot(op, obb->axis[0]);
	if (dot > obb->half[0] + CCT_EPSILON || dot < -obb->half[0] - CCT_EPSILON) {
		return 0;
	}
	dot = mathVec3Dot(op, obb->axis[1]);
	if (dot > obb->half[1] + CCT_EPSILON || dot < -obb->half[1] - CCT_EPSILON) {
		return 0;
	}
	dot = mathVec3Dot(op, obb->axis[2]);
	if (dot > obb->half[2] + CCT_EPSILON || dot < -obb->half[2] - CCT_EPSILON) {
		return 0;
	}
	return 1;
}
*/

int mathOBBIntersectOBB(const GeometryOBB_t* obb0, const GeometryOBB_t* obb1) {
	/* these code is copy from PhysX-3.4 */
	float v[3], T[3];
	float R[3][3], FR[3][3], ra, rb, t;
	const float* e0 = obb0->half, *e1 = obb1->half;
	int i;
	mathVec3Sub(v, obb1->o, obb0->o);
	mathVec3Set(T, mathVec3Dot(v, obb0->axis[0]), mathVec3Dot(v, obb0->axis[1]), mathVec3Dot(v, obb0->axis[2]));

	for (i = 0; i < 3; ++i) {
		int k;
		for (k = 0; k < 3; ++k) {
			R[i][k] = mathVec3Dot(obb0->axis[i], obb1->axis[k]);
			if (R[i][k] < 0.0f) {
				FR[i][k] = 1e-6f - R[i][k];
			}
			else {
				FR[i][k] = 1e-6f + R[i][k];
			}
		}
	}

	for (i = 0; i < 3; ++i) {
		ra = e0[i];
		rb = e1[0]*FR[i][0] + e1[1]*FR[i][1] + e1[2]*FR[i][2];
		if (T[i] < 0.0f) {
			t = -T[i];
		}
		else {
			t = T[i];
		}
		if (t > ra + rb) {
			return 0;
		}
	}

	for (i = 0; i < 3; ++i) {
		ra = e0[0]*FR[0][i] + e0[1]*FR[1][i] + e0[2]*FR[2][i];
		rb = e1[i];
		t = T[0]*R[0][i] + T[1]*R[1][i] + T[2]*R[2][i];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
	}

	if (1) {
		/* 9 cross products */

		//L = A0 x B0
		ra	= e0[1]*FR[2][0] + e0[2]*FR[1][0];
		rb	= e1[1]*FR[0][2] + e1[2]*FR[0][1];
		t	= T[2]*R[1][0] - T[1]*R[2][0];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A0 x B1
		ra	= e0[1]*FR[2][1] + e0[2]*FR[1][1];
		rb	= e1[0]*FR[0][2] + e1[2]*FR[0][0];
		t	= T[2]*R[1][1] - T[1]*R[2][1];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A0 x B2
		ra	= e0[1]*FR[2][2] + e0[2]*FR[1][2];
		rb	= e1[0]*FR[0][1] + e1[1]*FR[0][0];
		t	= T[2]*R[1][2] - T[1]*R[2][2];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A1 x B0
		ra	= e0[0]*FR[2][0] + e0[2]*FR[0][0];
		rb	= e1[1]*FR[1][2] + e1[2]*FR[1][1];
		t	= T[0]*R[2][0] - T[2]*R[0][0];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A1 x B1
		ra	= e0[0]*FR[2][1] + e0[2]*FR[0][1];
		rb	= e1[0]*FR[1][2] + e1[2]*FR[1][0];
		t	= T[0]*R[2][1] - T[2]*R[0][1];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A1 x B2
		ra	= e0[0]*FR[2][2] + e0[2]*FR[0][2];
		rb	= e1[0]*FR[1][1] + e1[1]*FR[1][0];
		t	= T[0]*R[2][2] - T[2]*R[0][2];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A2 x B0
		ra	= e0[0]*FR[1][0] + e0[1]*FR[0][0];
		rb	= e1[1]*FR[2][2] + e1[2]*FR[2][1];
		t	= T[1]*R[0][0] - T[0]*R[1][0];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A2 x B1
		ra	= e0[0]*FR[1][1] + e0[1]*FR[0][1];
		rb	= e1[0] *FR[2][2] + e1[2]*FR[2][0];
		t	= T[1]*R[0][1] - T[0]*R[1][1];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
		//L = A2 x B2
		ra	= e0[0]*FR[1][2] + e0[1]*FR[0][2];
		rb	= e1[0]*FR[2][1] + e1[1]*FR[2][0];
		t	= T[1]*R[0][2] - T[0]*R[1][2];
		if (t < 0.0f) {
			t = -t;
		}
		if (t > ra + rb) {
			return 0;
		}
	}
	return 1;
}

#ifdef __cplusplus
}
#endif
