//
// Created by hujianzhe
//

#include "../../inc/crt/dynarr.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void* dynarrReserve_raw(DynArrRaw_t* dynarr, size_t capacity, size_t ele_size) {
	if (dynarr->capacity < capacity) {
		void* p = realloc(dynarr->buf, ele_size * capacity);
		if (!p) {
			return NULL;
		}
		dynarr->buf = p;
		dynarr->capacity = capacity;
	}
	return dynarr->buf;
}

void* dynarrResize_raw(DynArrRaw_t* dynarr, size_t len, size_t ele_size) {
	if (!dynarrReserve_raw(dynarr, len, ele_size)) {
		return NULL;
	}
	dynarr->len = len;
	return dynarr;
}

void dynarrSwap_raw(DynArrRaw_t* a1, DynArrRaw_t* a2) {
	if (a1 != a2) {
		DynArrRaw_t temp = *a1;
		*a1 = *a2;
		*a2 = temp;
	}
}

void dynarrInitZero_raw(DynArrRaw_t* dynarr) {
	dynarr->buf = NULL;
	dynarr->len = 0;
	dynarr->capacity = 0;
}

void dynarrFreeMemory_raw(DynArrRaw_t* dynarr) {
	free(dynarr->buf);
	dynarrInitZero_raw(dynarr);
}

#ifdef __cplusplus
}
#endif
