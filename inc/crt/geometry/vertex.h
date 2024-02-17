//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_GEOMETRY_VERTEX_H
#define	UTIL_C_CRT_GEOMETRY_VERTEX_H

#include "geometry_def.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll unsigned int mathVerticesDistinctCount(const float(*src_v)[3], unsigned int src_v_cnt);
__declspec_dll unsigned int mathVerticesMerge(const float(*src_v)[3], unsigned int v_cnt, const unsigned int* src_indices, unsigned int indices_cnt, float(*dst_v)[3], unsigned int* dst_indices);

__declspec_dll int mathVertexIndicesFindMaxMinXYZ(const float(*v)[3], const unsigned int* v_indices, unsigned int v_indices_cnt, float v_minXYZ[3], float v_maxXYZ[3]);
__declspec_dll int mathVerticesFindMaxMinXYZ(const float(*v)[3], unsigned int v_cnt, float v_minXYZ[3], float v_maxXYZ[3]);

#ifdef	__cplusplus
}
#endif

#endif
