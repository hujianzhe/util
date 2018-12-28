//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_MATH_H
#define	UTIL_C_SYSLIB_MATH_H

#include "../compiler_define.h"

#ifndef	_USE_MATH_DEFINES
	#define	_USE_MATH_DEFINES
#endif
#include <float.h>
#include <math.h>
typedef	float		float32_t;
typedef double		float64_t;
#if REAL_FLOAT_BIT == 64
	typedef	float64_t					real_t;
	#define	REAL_EPSILON				DBL_EPSILON
	#define	REAL_MIN					DBL_MIN
	#define	REAL_MAX					DBL_MAX
	#define	REAL_MATH_FUNC(fn)			fn
#else
	#define	REAL_FLOAT_BIT				32
	typedef	float32_t					real_t;
	#define	REAL_EPSILON				FLT_EPSILON
	#define	REAL_MIN					FLT_MIN
	#define	REAL_MAX					FLT_MAX
	#define	REAL_MATH_FUNC(fn)			fn##f
#endif
#ifndef CCT_EPSILON
	#define	CCT_EPSILON					1E-5f
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int fcmpf(float a, float b, float epsilon);
__declspec_dll int fcmp(double a, double b, double epsilon);
__declspec_dll float finvsqrtf(float x);
__declspec_dll float fsqrtf(float x);
__declspec_dll double finvsqrt(double x);
__declspec_dll double fsqrt(double x);

#define	mathDegToRad(deg)	(M_PI / 180.0 * (deg))
#define	mathRadToDeg(rad)	((rad) * M_1_PI * 180.0)
#define	mathDegToRad_f(deg)	(((float)M_PI) / 180.0f * ((float)(deg)))
#define	mathRadToDeg_f(rad)	(((float)(rad)) * ((float)M_1_PI) * 180.0f)

__declspec_dll int mathQuadraticEquation(float a, float b, float c, float r[2]);

__declspec_dll int mathVec3IsZero(float v[3]);
__declspec_dll int mathVec3Equal(float v1[3], float v2[3]);
__declspec_dll float* mathVec3Copy(float r[3], float v[3]);
__declspec_dll float mathVec3LenSq(float v[3]);
__declspec_dll float mathVec3Len(float v[3]);
__declspec_dll float* mathVec3Normalized(float r[3], float v[3]);
__declspec_dll float* mathVec3Negate(float r[3], float v[3]);
__declspec_dll float* mathVec3Add(float r[3], float v1[3], float v2[3]);
__declspec_dll float* mathVec3AddScalar(float r[3], float v[3], float n);
__declspec_dll float* mathVec3Sub(float r[3], float v1[3], float v2[3]);
__declspec_dll float* mathVec3MultiplyScalar(float r[3], float v[3], float n);
__declspec_dll float mathVec3Dot(float v1[3], float v2[3]);
__declspec_dll float mathVec3Radian(float v1[3], float v2[3]);
__declspec_dll float* mathVec3Cross(float r[3], float v1[3], float v2[3]);

__declspec_dll float* mathCoordinateSystemTransform(float v[3], float new_origin[3], float new_axies[3][3], float new_v[3]);

__declspec_dll float* mathQuatNormalized(float r[4], float q[4]);
__declspec_dll float* mathQuatFromEuler(float q[4], float e[3], const char order[3]);
__declspec_dll float* mathQuatFromUnitVec3(float q[4], float from[3], float to[3]);
__declspec_dll float* mathQuatFromAxisRadian(float q[4], float axis[3], float radian);
__declspec_dll void mathQuatToAxisRadian(float q[4], float axis[3], float* radian);
__declspec_dll float* mathQuatIdentity(float q[4]);
__declspec_dll float* mathQuatConjugate(float r[4], float q[4]);
__declspec_dll float* mathQuatMulQuat(float r[4], float q1[4], float q2[4]);
__declspec_dll float* mathQuatMulVec3(float r[3], float q[4], float v[3]);

__declspec_dll float* mathPlaneNormalByVertices3(float vertices[3][3], float normal[3]);
__declspec_dll void mathPointProjectionLine(float p[3], float ls[2][3], float np[3], float* distance);
__declspec_dll void mathPointProjectionPlane(float p[3], float plane_v[3], float plane_normal[3], float np[3], float* distance);
__declspec_dll int mathLineSegmentHasPoint(float ls[2][3], float p[3]);
__declspec_dll int mathTriangleHasPoint(float tri[3][3], float p[3], float* p_u, float* p_v);
__declspec_dll float* mathTriangleGetPoint(float tri[3][3], float u, float v, float p[3]);
__declspec_dll int mathCircleHasPoint(float o[3], float radius, float normal[3], float p[3]);
__declspec_dll void mathCircleProjectPlane(float center[3], float radius, float c_normal[3], float p_normal[3], float p[2][3]);
__declspec_dll int mathCircleIntersectPlane(float circle_project_point[2][3], float r, float circle_normal[3], float plane_vertice[3], float plane_normal[3], float p[2][3]);
__declspec_dll int mathSphereHasPoint(float o[3], float radius, float p[3]);
__declspec_dll int mathSphereHasLineSegment(float o[3], float radius, float ls[2][3], float pointcut[3]);
__declspec_dll int mathSphereIntersectLine(float o[3], float radius, float ls_vertice[3], float lsdir[3], float p[2][3]);
__declspec_dll int mathCylinderHasPoint(float cp[2][3], float radius, float p[3]);
__declspec_dll int mathCylinderInfiniteIntersectLine(float cp[2][3], float radius, float ls_vertice[3], float lsdir[3], float p[2][3]);
__declspec_dll int mathCylinderInfiniteIntersectPlane(float cp[2][3], float radius, float plane_vertice[3], float plane_normal[3], float p[4][3]);
__declspec_dll int mathCylinderIntersectLine(float cp[2][3], float radius, float ls_vertice[3], float lsdir[3], float p[2][3]);
typedef struct CCTResult_t {
	float distance;
	int hit_point_cnt;
	float hit_point[3];
} CCTResult_t;
__declspec_dll CCTResult_t* mathRaycastLineSegment(float o[3], float dir[3], float ls[2][3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathRaycastPlane(float o[3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathRaycastTriangle(float o[3], float dir[3], float tri[3][3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathRaycastSphere(float o[3], float dir[3], float center[3], float radius, CCTResult_t* result);
__declspec_dll CCTResult_t* mathRaycastCircle(float o[3], float dir[3], float center[3], float radius, float normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathRaycastCylinder(float o[3], float dir[3], float cp[2][3], float radius, CCTResult_t* result);
__declspec_dll CCTResult_t* mathLineSegmentcastPlane(float ls[2][3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathLineSegmentcastLineSegment(float ls1[2][3], float dir[3], float ls2[2][3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathLineSegmentcastTriangle(float ls[2][3], float dir[3], float tri[3][3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathLineSegmentcastSphere(float ls[2][3], float dir[3], float center[3], float radius, CCTResult_t* result);
__declspec_dll CCTResult_t* mathLineSegmentcastCircle(float ls[2][3], float dir[3], float center[3], float radius, float normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathCirclecastPlane(float center[3], float radius, float c_normal[3], float dir[3], float vertice[3], float p_normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathCirclecastCircle(float o1[3], float r1, float n1[3], float dir[3], float o2[3], float r2, float n2[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathTrianglecastPlane(float tri[3][3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathTrianglecastTriangle(float tri1[3][3], float dir[3], float tri2[3][3], CCTResult_t* result);
__declspec_dll int mathAABBIntersectAABB(float o1[3], float half1[3], float o2[3], float half2[3]);
__declspec_dll CCTResult_t* mathAABBcastPlane(float o[3], float half[3], float dir[3], float vertice[3], float normal[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathAABBcastAABB(float o1[3], float half1[3], float dir[3], float o2[3], float half2[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathSpherecastPlane(float o[3], float radius, float dir[3], float vertices[3][3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathSpherecastSphere(float o1[3], float r1, float dir[3], float o2[3], float r2, CCTResult_t* result);
__declspec_dll CCTResult_t* mathSpherecastTriangle(float o[3], float radius, float dir[3], float tri[3][3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathSpherecastTrianglesPlane(float o[3], float radius, float dir[3], float vertices[][3], int indices[], int indicescnt, CCTResult_t* result);
__declspec_dll CCTResult_t* mathSpherecastAABB(float o[3], float radius, float dir[3], float center[3], float half[3], CCTResult_t* result);
__declspec_dll CCTResult_t* mathCylindercastPlane(float p[2][3], float radius, float dir[3], float vertices[3][3], CCTResult_t* result);

#ifdef	__cplusplus
}
#endif

#endif
