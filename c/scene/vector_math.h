//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_VECTOR_MATH_H
#define	UTIL_C_SCENE_VECTOR_MATH_H

typedef struct vector2_t {
	double x, y;
} vector2_t;
typedef struct vector3_t {
	double x, y, z;
} vector3_t;

#ifdef	__cplusplus
extern "C" {
#endif

double vector_dot(const double* v1, const double* v2, int dimension);
double vector2_dot(const struct vector2_t* v1, const struct vector2_t* v2);
double vector3_dot(const struct vector3_t* v1, const struct vector3_t* v2);

double vector2_cross(const vector2_t* v1, const vector2_t* v2);

double vector_lensq(const double* v, int dimension);
double vector2_lensq(const struct vector2_t* v);
double vector3_lensq(const struct vector3_t* v);
double vector_len(const double* v, int dimension);
double vector2_len(const struct vector2_t* v);
double vector3_len(const struct vector3_t* v);

double vector2_radian(const struct vector2_t* v1, const struct vector2_t* v2);
double vector3_radian(const struct vector3_t* v1, const struct vector3_t* v2);

double* vector_normalized(const double* v, double* n, int dimension);
struct vector2_t* vector2_normalized(const struct vector2_t* v, struct vector2_t* n);
struct vector3_t* vector3_normalized(const struct vector3_t* v, struct vector3_t* n);

#ifdef	__cplusplus
}
#endif

#endif