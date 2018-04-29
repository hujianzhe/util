//
// Created by hujianzhe on 18-4-13.
//

#ifndef	UTIL_C_SCENE_VECTOR_MATH_H
#define	UTIL_C_SCENE_VECTOR_MATH_H

typedef union vector2_t {
	struct {
		double x, y;
	};
	double val[2];
} vector2_t;
typedef union vector3_t {
	struct {
		double x, y, z;
	};
	double val[3];
} vector3_t;

#ifdef	__cplusplus
extern "C" {
#endif

double* vector_assign(double* v, const double val[], int dimension);
vector2_t* vector2_assign(vector2_t* v, double x, double y);
vector3_t* vector3_assign(vector3_t* v, double x, double y, double z);

int vector_equal(const double* v1, const double* v2, int dimension);
int vector2_equal(const vector2_t* v1, const vector2_t* v2);
int vector3_equal(const vector3_t* v1, const vector3_t* v2);

double vector_dot(const double* v1, const double* v2, int dimension);
double vector2_dot(const vector2_t* v1, const vector2_t* v2);
double vector3_dot(const vector3_t* v1, const vector3_t* v2);

double vector2_cross(const vector2_t* v1, const vector2_t* v2);
vector3_t* vector3_cross(vector3_t* res, const vector3_t* v1, const vector3_t* v2);

double vector_lensq(const double* v, int dimension);
double vector2_lensq(const vector2_t* v);
double vector3_lensq(const vector3_t* v);
double vector_len(const double* v, int dimension);
double vector2_len(const vector2_t* v);
double vector3_len(const vector3_t* v);

double vector2_radian(const vector2_t* v1, const vector2_t* v2);
double vector3_radian(const vector3_t* v1, const vector3_t* v2);

double* vector_normalized(const double* v, double* n, int dimension);
vector2_t* vector2_normalized(const vector2_t* v, vector2_t* n);
vector3_t* vector3_normalized(const vector3_t* v, vector3_t* n);

#ifdef	__cplusplus
}
#endif

#endif