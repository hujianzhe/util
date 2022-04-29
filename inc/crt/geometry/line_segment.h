//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_LINE_SEGMENT_H
#define	UTIL_C_CRT_GEOMETRY_LINE_SEGMENT_H

#include "../../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void mathPointProjectionLine(const float p[3], const float ls_v[3], const float lsdir[3], float np_to_p[3], float np[3]);
__declspec_dll int mathLineClosestLine(const float lsv1[3], const float lsdir1[3], const float lsv2[3], const float lsdir2[3], float* min_d, float dir_d[2]);
__declspec_dll int mathLineIntersectLine(const float ls1v[3], const float ls1dir[3], const float ls2v[3], const float ls2dir[3], float distance[2]);

__declspec_dll int mathSegmentHasPoint(const float ls[2][3], const float p[3]);
__declspec_dll int mathSegmentContainSegment(const float ls1[2][3], const float ls2[2][3]);
__declspec_dll int mathSegmentIntersectSegment(const float ls1[2][3], const float ls2[2][3], float p[3]);

#ifdef	__cplusplus
}
#endif

#endif
