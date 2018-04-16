//
// Created by hujianzhe on 18-4-13.
//

#include "vector_math.h"
#include <float.h>
#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif

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