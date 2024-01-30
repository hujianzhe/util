//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_LINE_SEGMENT_H
#define	UTIL_C_CRT_GEOMETRY_LINE_SEGMENT_H

#include "geometry_def.h"

enum {
	GEOMETRY_LINE_SKEW = 0,
	GEOMETRY_LINE_PARALLEL = 1,
	GEOMETRY_LINE_CROSS = 2,
	GEOMETRY_LINE_OVERLAP = 3,

	GEOMETRY_SEGMENT_CONTACT = 1,
	GEOMETRY_SEGMENT_OVERLAP = 2,
};

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll int mathProjectionRay(const float o[3], const float projection_p[3], const float dir[3], float* dir_d, float projection_v[3]);
__declspec_dll float mathPointProjectionLine(const float p[3], const float ls_v[3], const float lsdir[3], float np[3]);
__declspec_dll int mathLineClosestLine(const float lsv1[3], const float lsdir1[3], const float lsv2[3], const float lsdir2[3], float* min_d, float dir_d[2]);
__declspec_dll int mathLineIntersectLine(const float ls1v[3], const float ls1dir[3], const float ls2v[3], const float ls2dir[3], float distance[2]);

__declspec_dll int mathSegmentHasPoint(const float ls[2][3], const float p[3]);
__declspec_dll int mathSegmentIsSame(const float ls1[2][3], const float ls2[2][3]);
__declspec_dll void mathSegmentClosestPointTo(const float ls[2][3], const float p[3], float closest_p[3]);
__declspec_dll int mathSegmentContainSegment(const float ls1[2][3], const float ls2[2][3]);
__declspec_dll int mathSegmentIntersectSegment(const float ls1[2][3], const float ls2[2][3], float p[3], int* line_mask);
__declspec_dll void mathSegmentClosestSegmentVertice(const float ls1[2][3], const float ls2[2][3], float closest_p[2][3]);
__declspec_dll int mathSegmentClosestSegment(const float ls1[2][3], const float ls2[2][3], float closest_p[2][3]);

#ifdef	__cplusplus
}
#endif

#endif
