//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"
#include "../../../inc/crt/geometry/aabb.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
const unsigned int Box_Edge_Indices[24] = {
	0, 1,	1, 2,	2, 3,	3, 0,
	7, 6,	6, 5,	5, 4,	4, 7,
	1, 5,	6, 2,
	3, 7,	4, 0
};
const unsigned int Box_Vertice_Adjacent_Indices[8][3] = {
	{ 1, 3, 4 },
	{ 0, 2, 5 },
	{ 1, 3, 6 },
	{ 0, 2, 7 },
	{ 0, 5, 7 },
	{ 1, 4, 6 },
	{ 2, 5, 7 },
	{ 3, 4, 6 }
};
const unsigned int Box_Triangle_Vertices_Indices[36] = {
	0, 1, 2,	2, 3, 0,
	7, 6, 5,	5, 4, 7,
	1, 5, 6,	6, 2, 1,
	3, 7, 4,	4, 0, 3,
	3, 7, 6,	6, 2, 3,
	0, 4, 5,	5, 1, 0
};
const float AABB_Plane_Normal[6][3] = {
	{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f },
	{ 1.0f, 0.0f, 0.0f },{ -1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f },{ 0.0f, -1.0f, 0.0f }
};

void mathAABBPlaneVertices(const float o[3], const float half[3], float v[6][3]) {
	mathVec3Copy(v[0], o);
	v[0][2] += half[2];
	mathVec3Copy(v[1], o);
	v[1][2] -= half[2];

	mathVec3Copy(v[2], o);
	v[2][0] += half[0];
	mathVec3Copy(v[3], o);
	v[3][0] -= half[0];

	mathVec3Copy(v[4], o);
	v[4][1] += half[1];
	mathVec3Copy(v[5], o);
	v[5][1] -= half[1];
}

void mathAABBVertices(const float o[3], const float half[3], float v[8][3]) {
	v[0][0] = o[0] - half[0], v[0][1] = o[1] - half[1], v[0][2] = o[2] - half[2];
	v[1][0] = o[0] + half[0], v[1][1] = o[1] - half[1], v[1][2] = o[2] - half[2];
	v[2][0] = o[0] + half[0], v[2][1] = o[1] + half[1], v[2][2] = o[2] - half[2];
	v[3][0] = o[0] - half[0], v[3][1] = o[1] + half[1], v[3][2] = o[2] - half[2];
	v[4][0] = o[0] - half[0], v[4][1] = o[1] - half[1], v[4][2] = o[2] + half[2];
	v[5][0] = o[0] + half[0], v[5][1] = o[1] - half[1], v[5][2] = o[2] + half[2];
	v[6][0] = o[0] + half[0], v[6][1] = o[1] + half[1], v[6][2] = o[2] + half[2];
	v[7][0] = o[0] - half[0], v[7][1] = o[1] + half[1], v[7][2] = o[2] + half[2];
}

void mathAABBMinVertice(const float o[3], const float half[3], float v[3]) {
	v[0] = o[0] - half[0];
	v[1] = o[1] - half[1];
	v[2] = o[2] - half[2];
}

void mathAABBMaxVertice(const float o[3], const float half[3], float v[3]) {
	v[0] = o[0] + half[0];
	v[1] = o[1] + half[1];
	v[2] = o[2] + half[2];
}

void mathAABBFromTwoVertice(const float a[3], const float b[3], float o[3], float half[3]) {
	unsigned int i;
	for (i = 0; i < 3; ++i) {
		o[i] = (a[i] + b[i]) * 0.5f;
		if (a[i] > b[i]) {
			half[i] = (a[i] - b[i]) * 0.5f;
		}
		else {
			half[i] = (b[i] - a[i]) * 0.5f;
		}
		if (half[i] < GEOMETRY_BODY_BOX_MIN_HALF) {
			half[i] = GEOMETRY_BODY_BOX_MIN_HALF;
		}
	}
}

int mathAABBHasPoint(const float o[3], const float half[3], const float p[3]) {
	return p[0] >= o[0] - half[0] - CCT_EPSILON && p[0] <= o[0] + half[0] + CCT_EPSILON &&
		   p[1] >= o[1] - half[1] - CCT_EPSILON && p[1] <= o[1] + half[1] + CCT_EPSILON &&
		   p[2] >= o[2] - half[2] - CCT_EPSILON && p[2] <= o[2] + half[2] + CCT_EPSILON;
}

void mathAABBClosestPointTo(const float o[3], const float half[3], const float p[3], float closest_p[3]) {
	int i;
	float min_v[3], max_v[3];
	mathAABBMinVertice(o, half, min_v);
	mathAABBMaxVertice(o, half, max_v);
	for (i = 0; i < 3; ++i) {
		if (p[i] < min_v[i]) {
			closest_p[i] = min_v[i];
		}
		else if (p[i] > max_v[i]) {
			closest_p[i] = max_v[i];
		}
		else {
			closest_p[i] = p[i];
		}
	}
}

void mathAABBStretch(float o[3], float half[3], const float delta[3]) {
	unsigned int i;
	for (i = 0; i < 3; ++i) {
		float d = delta[i] * 0.5f;
		if (d > 0.0f) {
			half[i] += d;
		}
		else {
			half[i] -= d;
		}
		o[i] += d;
	}
}

void mathAABBSplit(const float o[3], const float half[3], float new_o[8][3], float new_half[3]) {
	mathVec3MultiplyScalar(new_half, half, 0.5f);
	mathAABBVertices(o, new_half, new_o);
}

int mathAABBIntersectAABB(const float o1[3], const float half1[3], const float o2[3], const float half2[3]) {
	int i;
	for (i = 0; i < 3; ++i) {
		float half = half1[i] + half2[i] + CCT_EPSILON;
		if (o2[i] - o1[i] > half || o1[i] - o2[i] > half) {
			return 0;
		}
	}
	return 1;
}

int mathAABBContainAABB(const float o1[3], const float half1[3], const float o2[3], const float half2[3]) {
	float v[3];
	mathAABBMinVertice(o2, half2, v);
	if (!mathAABBHasPoint(o1, half1, v)) {
		return 0;
	}
	mathAABBMaxVertice(o2, half2, v);
	return mathAABBHasPoint(o1, half1, v);
}

#ifdef __cplusplus
}
#endif
