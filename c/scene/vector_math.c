//
// Created by hujianzhe on 18-4-13.
//

#include "vector_math.h"
#include <float.h>
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

double* vector_assign(double* v, const double val[], int dimension) {
	int i;
	for (i = 0; i < dimension; ++i) {
		v[i] = val[i];
	}
	return v;
}

vector2_t* vector2_assign(vector2_t* v, double x, double y) {
	v->x = x;
	v->y = y;
	return v;
}

vector3_t* vector3_assign(vector3_t* v, double x, double y, double z) {
	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}

int vector_is_zero(const double* v, int dimension) {
	int i;
	for (i = 0; i < dimension; ++i) {
		if (v[i] <= -DBL_EPSILON && v[i] >= DBL_EPSILON)
			return 0;
	}
	return 1;
}

int vector3_is_zero(const vector3_t* v) {
	return v->x > -DBL_EPSILON && v->x < DBL_EPSILON
		&& v->y > -DBL_EPSILON && v->y < DBL_EPSILON
		&& v->z > -DBL_EPSILON && v->z < DBL_EPSILON;
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
int vector2_equal(const vector2_t* v1, const vector2_t* v2) {
	double res;
	res = v1->x - v2->x;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	res = v1->y - v2->y;
	if (res < -DBL_EPSILON || DBL_EPSILON < res)
		return 0;
	return 1;
}
int vector3_equal(const vector3_t* v1, const vector3_t* v2) {
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
double vector2_dot(const vector2_t* v1, const vector2_t* v2) {
	return v1->x * v2->x + v1->y * v2->y;
}
double vector3_dot(const vector3_t* v1, const vector3_t* v2) {
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

double vector2_cross(const vector2_t* v1, const vector2_t* v2) {
	return v1->x * v2->y - v2->x * v1->y;
}
vector3_t* vector3_cross(vector3_t* res, const vector3_t* v1, const vector3_t* v2) {
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
double vector2_lensq(const vector2_t* v) {
	return v->x * v->x + v->y * v->y;
}
double vector3_lensq(const vector3_t* v) {
	return v->x * v->x + v->y * v->y + v->z * v->z;
}
double vector_len(const double* v, int dimension) {
	return sqrt(vector_lensq(v, dimension));
}
double vector2_len(const vector2_t* v) {
	return sqrt(vector2_lensq(v));
}
double vector3_len(const vector3_t* v) {
	return sqrt(vector3_lensq(v));
}

double vector2_radian(const vector2_t* v1, const vector2_t* v2) {
	return acos(vector2_dot(v1, v2) / vector2_len(v1) * vector2_len(v2));
}
double vector3_radian(const vector3_t* v1, const vector3_t* v2) {
	return acos(vector3_dot(v1, v2) / vector3_len(v1) * vector3_len(v2));
}

double* vector_normalized(double* n, const double* v, int dimension) {
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
vector2_t* vector2_normalized(vector2_t* n, const vector2_t* v) {
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
vector3_t* vector3_normalized(vector3_t* n, const vector3_t* v) {
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