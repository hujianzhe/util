//
// Created by hujianzhe
//

#include "../../../inc/crt/math_vec3.h"

#ifdef	__cplusplus
extern "C" {
#endif

unsigned int mathVerticesDistinctCount(const float(*v)[3], unsigned int v_cnt) {
	unsigned int i, len = v_cnt;
	for (i = 0; i < v_cnt; ++i) {
		unsigned int j;
		for (j = i + 1; j < v_cnt; ++j) {
			if (mathVec3Equal(v[i], v[j])) {
				--len;
				break;
			}
		}
	}
	return len;
}

unsigned int mathVerticesMerge(const float(*src_v)[3], unsigned int v_cnt, const unsigned int* src_indices, unsigned int indices_cnt, float(*dst_v)[3], unsigned int* dst_indices) {
	/* allow src_v == dst_v */
	/* allow src_indices == dst_indices */
	unsigned int i, dst_v_cnt = 0;
	for (i = 0; i < v_cnt; ++i) {
		unsigned int j;
		for (j = 0; j < dst_v_cnt; ++j) {
			if (mathVec3Equal(src_v[i], dst_v[j])) {
				break;
			}
		}
		if (j < dst_v_cnt) {
			continue;
		}
		for (j = 0; j < indices_cnt; ++j) {
			if (src_indices[j] == i || mathVec3Equal(src_v[src_indices[j]], src_v[i])) {
				dst_indices[j] = dst_v_cnt;
			}
		}
		mathVec3Copy(dst_v[dst_v_cnt++], src_v[i]);
	}
	return dst_v_cnt;
}

int mathVertexIndicesFindMaxMinXYZ(const float(*v)[3], const unsigned int* v_indices, unsigned int v_indices_cnt, float v_minXYZ[3], float v_maxXYZ[3]) {
	unsigned int i;
	if (v_indices_cnt <= 0) {
		return 0;
	}
	mathVec3Copy(v_minXYZ, v[v_indices[0]]);
	mathVec3Copy(v_maxXYZ, v[v_indices[0]]);
	for (i = 1; i < v_indices_cnt; ++i) {
		unsigned int j;
		const float* cur_v = v[v_indices[i]];
		for (j = 0; j < 3; ++j) {
			if (cur_v[j] < v_minXYZ[j]) {
				v_minXYZ[j] = cur_v[j];
			}
			else if (cur_v[j] > v_maxXYZ[j]) {
				v_maxXYZ[j] = cur_v[j];
			}
		}
	}
	return 1;
}

int mathVerticesFindMaxMinXYZ(const float(*v)[3], unsigned int v_cnt, float v_minXYZ[3], float v_maxXYZ[3]) {
	unsigned int i;
	if (v_cnt <= 0) {
		return 0;
	}
	mathVec3Copy(v_minXYZ, v[0]);
	mathVec3Copy(v_maxXYZ, v[0]);
	for (i = 1; i < v_cnt; ++i) {
		unsigned int j;
		const float* cur_v = v[i];
		for (j = 0; j < 3; ++j) {
			if (cur_v[j] < v_minXYZ[j]) {
				v_minXYZ[j] = cur_v[j];
			}
			else if (cur_v[j] > v_maxXYZ[j]) {
				v_maxXYZ[j] = cur_v[j];
			}
		}
	}
	return 1;
}

#ifdef	__cplusplus
}
#endif
