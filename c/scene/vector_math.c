//
// Created by hujianzhe on 18-4-13.
//

#include "vector_math.h"
#include <float.h>
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct vector2_t* vector2_assign(struct vector2_t* v, double x, double y) {
	v->x = x;
	v->y = y;
	return v;
}

struct vector3_t* vector3_assign(struct vector3_t* v, double x, double y, double z) {
	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}

int vector_equal(const double* v1, const double* v2, int dimension) {
	int i;
	for (i = 0; i < dimension; ++i) {
		double res = v1[i] - v2[i];
		if (res < -DBL_EPSILON || DBL_EPSILON < res)
			return 0;
	}
	return 1;
}
int vector2_equal(const struct vector2_t* v1, const struct vector2_t* v2) {
	double res;
	res = v1->x - v2->x;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	res = v1->y - v2->y;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	return 1;
}
int vector3_equal(const struct vector3_t* v1, const struct vector3_t* v2) {
	double res;
	res = v1->x - v2->x;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	res = v1->y - v2->y;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	res = v1->z - v2->z;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	return 1;
}

double vector_dot(const double* v1, const double* v2, int dimension) {
	double res = 0.0;
	int i;
	for (i = 0; i < dimension; ++i) {
		res += v1[i] * v2[i];
	}
	return res;
}
double vector2_dot(const struct vector2_t* v1, const struct vector2_t* v2) {
	return v1->x * v2->x + v1->y * v2->y;
}
double vector3_dot(const struct vector3_t* v1, const struct vector3_t* v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

double vector2_cross(const struct vector2_t* v1, const struct vector2_t* v2) {
	return v1->x * v2->y - v2->x * v1->y;
}
struct vector3_t* vector3_cross(struct vector3_t* res, const struct vector3_t* v1, const struct vector3_t* v2) {
	double x = v1->y * v2->z - v2->y * v1->z;
	double y = v2->x * v1->z - v1->x * v2->z;
	double z = v1->x * v2->y - v2->x * v1->y;
	res->x = x;
	res->y = y;
	res->z = z;
	return res;
}

double vector_lensq(const double* v, int dimension) {
	double res = 0.0;
	int i;
	for (i = 0; i < dimension; ++i) {
		res += v[i] * v[i];
	}
	return res;
}
double vector2_lensq(const struct vector2_t* v) {
	return v->x * v->x + v->y * v->y;
}
double vector3_lensq(const struct vector3_t* v) {
	return v->x * v->x + v->y * v->y + v->z * v->z;
}
double vector_len(const double* v, int dimension) {
	return sqrt(vector_lensq(v, dimension));
}
double vector2_len(const struct vector2_t* v) {
	return sqrt(vector2_lensq(v));
}
double vector3_len(const struct vector3_t* v) {
	return sqrt(vector3_lensq(v));
}

double vector2_radian(const struct vector2_t* v1, const struct vector2_t* v2) {
	return acos(vector2_dot(v1, v2) / vector2_len(v1) * vector2_len(v2));
}
double vector3_radian(const struct vector3_t* v1, const struct vector3_t* v2) {
	return acos(vector3_dot(v1, v2) / vector3_len(v1) * vector3_len(v2));
}

double* vector_normalized(const double* v, double* n, int dimension) {
	int i;
	double len = vector_len(v, dimension);
	if (len < DBL_EPSILON) {
		for (i = 0; i < dimension; ++i) {
			n[i] = 0.0;
		}
	}
	else {
		for (i = 0; i < dimension; ++i) {
			n[i] = v[i] / len;
		}
	}
	return n;
}
struct vector2_t* vector2_normalized(const struct vector2_t* v, struct vector2_t* n) {
	double len = vector2_len(v);
	if (len < DBL_EPSILON) {
		n->x = n->y = 0.0;
	}
	else {
		n->x = v->x / len;
		n->y = v->y / len;
	}
	return n;
}
struct vector3_t* vector3_normalized(const struct vector3_t* v, struct vector3_t* n) {
	double len = vector3_len(v);
	if (len < DBL_EPSILON) {
		n->x = n->y = n->z = 0.0;
	}
	else {
		n->x = v->x / len;
		n->y = v->y / len;
		n->z = v->z / len;
	}
	return n;
}

#ifdef	__cplusplus
}
#endif