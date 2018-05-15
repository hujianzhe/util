//
// Created by hujianzhe
//

#include "math_vector.h"

#ifdef	__cplusplus
extern "C" {
#endif

real_t* vector_assign(real_t* v, const real_t val[], int dimension) {
	int i;
	for (i = 0; i < dimension; ++i) {
		v[i] = val[i];
	}
	return v;
}

vector2_t* vector2_assign(vector2_t* v, real_t x, real_t y) {
	v->x = x;
	v->y = y;
	return v;
}

vector3_t* vector3_assign(vector3_t* v, real_t x, real_t y, real_t z) {
	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}

int vector_is_zero(const real_t* v, int dimension) {
	int i;
	for (i = 0; i < dimension; ++i) {
		if (!REAL_MATH_FUNC(fequ)(v[i], 0.0f))
			return 0;
	}
	return 1;
}

int vector3_is_zero(const vector3_t* v) {
	return REAL_MATH_FUNC(fequ)(v->x, 0.0f)
		&& REAL_MATH_FUNC(fequ)(v->y, 0.0f)
		&& REAL_MATH_FUNC(fequ)(v->z, 0.0f);
}

int vector_equal(const real_t* v1, const real_t* v2, int dimension) {
	int i;
	for (i = 0; i < dimension; ++i) {
		real_t res = v1[i] - v2[i];
		if (!REAL_MATH_FUNC(fequ)(res, 0.0f))
			return 0;
	}
	return 1;
}
int vector2_equal(const vector2_t* v1, const vector2_t* v2) {
	real_t res;
	res = v1->x - v2->x;
	if (!REAL_MATH_FUNC(fequ)(res, 0.0f))
		return 0;
	res = v1->y - v2->y;
	if (!REAL_MATH_FUNC(fequ)(res, 0.0f))
		return 0;
	return 1;
}
int vector3_equal(const vector3_t* v1, const vector3_t* v2) {
	real_t res;
	res = v1->x - v2->x;
	if (!REAL_MATH_FUNC(fequ)(res, 0.0f))
		return 0;
	res = v1->y - v2->y;
	if (!REAL_MATH_FUNC(fequ)(res, 0.0f))
		return 0;
	res = v1->z - v2->z;
	if (!REAL_MATH_FUNC(fequ)(res, 0.0f))
		return 0;
	return 1;
}

real_t vector_dot(const real_t* v1, const real_t* v2, int dimension) {
	real_t res = 0.0f;
	int i;
	for (i = 0; i < dimension; ++i) {
		res += v1[i] * v2[i];
	}
	return res;
}
real_t vector2_dot(const vector2_t* v1, const vector2_t* v2) {
	return v1->x * v2->x + v1->y * v2->y;
}
real_t vector3_dot(const vector3_t* v1, const vector3_t* v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

real_t vector2_cross(const vector2_t* v1, const vector2_t* v2) {
	return v1->x * v2->y - v2->x * v1->y;
}
vector3_t* vector3_cross(vector3_t* res, const vector3_t* v1, const vector3_t* v2) {
	real_t x = v1->y * v2->z - v2->y * v1->z;
	real_t y = v2->x * v1->z - v1->x * v2->z;
	real_t z = v1->x * v2->y - v2->x * v1->y;
	res->x = x;
	res->y = y;
	res->z = z;
	return res;
}

real_t vector_lensq(const real_t* v, int dimension) {
	real_t res = 0.0f;
	int i;
	for (i = 0; i < dimension; ++i) {
		res += v[i] * v[i];
	}
	return res;
}
real_t vector2_lensq(const vector2_t* v) {
	return v->x * v->x + v->y * v->y;
}
real_t vector3_lensq(const vector3_t* v) {
	return v->x * v->x + v->y * v->y + v->z * v->z;
}
real_t vector_len(const real_t* v, int dimension) {
	return REAL_MATH_FUNC(fsqrt)(vector_lensq(v, dimension));
}
real_t vector2_len(const vector2_t* v) {
	return REAL_MATH_FUNC(fsqrt)(vector2_lensq(v));
}
real_t vector3_len(const vector3_t* v) {
	return REAL_MATH_FUNC(fsqrt)(vector3_lensq(v));
}

real_t vector2_radian(const vector2_t* v1, const vector2_t* v2) {
	return REAL_MATH_FUNC(acos)(vector2_dot(v1, v2) / vector2_len(v1) * vector2_len(v2));
}
real_t vector3_radian(const vector3_t* v1, const vector3_t* v2) {
	return REAL_MATH_FUNC(acos)(vector3_dot(v1, v2) / vector3_len(v1) * vector3_len(v2));
}

real_t* vector_normalized(real_t* n, const real_t* v, int dimension) {
	int i;
	real_t len = vector_len(v, dimension);
	if (len < REAL_EPSILON) {
		for (i = 0; i < dimension; ++i) {
			n[i] = 0.0f;
		}
	}
	else {
		real_t m_1_len = 1.0f / len;
		for (i = 0; i < dimension; ++i) {
			n[i] = v[i] * m_1_len;
		}
	}
	return n;
}
vector2_t* vector2_normalized(vector2_t* n, const vector2_t* v) {
	real_t len = vector2_len(v);
	if (len < REAL_EPSILON) {
		n->x = n->y = 0.0f;
	}
	else {
		real_t m_1_len = 1.0f / len;
		n->x = v->x * m_1_len;
		n->y = v->y * m_1_len;
	}
	return n;
}
vector3_t* vector3_normalized(vector3_t* n, const vector3_t* v) {
	real_t len = vector3_len(v);
	if (len < REAL_EPSILON) {
		n->x = n->y = n->z = 0.0f;
	}
	else {
		real_t m_1_len = 1.0f / len;
		n->x = v->x * m_1_len;
		n->y = v->y * m_1_len;
		n->z = v->z * m_1_len;
	}
	return n;
}

#ifdef	__cplusplus
}
#endif