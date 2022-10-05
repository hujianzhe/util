//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_DYNARR_H
#define	UTIL_C_CRT_DYNARR_H

#include "../compiler_define.h"
#include <stddef.h>

typedef struct DynArrRaw_t {
	void* buf;
	size_t len;
	size_t capacity;
} DynArrRaw_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll void* dynarrReserve_raw(DynArrRaw_t* dynarr, size_t capacity, size_t ele_size);
__declspec_dll void* dynarrResize_raw(DynArrRaw_t* dynarr, size_t len, size_t ele_size);
__declspec_dll void dynarrSwap_raw(DynArrRaw_t* a1, DynArrRaw_t* a2);
__declspec_dll void dynarrInitZero_raw(DynArrRaw_t* dynarr);
__declspec_dll void dynarrFreeMemory_raw(DynArrRaw_t* dynarr);

#ifdef __cplusplus
}
#endif

#define	DynArr_t(type)\
union {\
	struct {\
		type* buf;\
		size_t len;\
		size_t capacity;\
	};\
	DynArrRaw_t raw;\
}

#define	dynarrInitZero(dynarr)	dynarrInitZero_raw(&(dynarr)->raw)

#define	dynarrIsEmpty(dynarr)	(0 == (dynarr)->len)

#define	dynarrReserve(dynarr, capacity)	dynarrReserve_raw(&(dynarr)->raw, capacity, sizeof((dynarr)->buf[0]))

#define	dynarrResize(dynarr, len)	dynarrResize_raw(&(dynarr)->raw, len, sizeof((dynarr)->buf[0]))

#define	dynarrClearData(dynarr) ((dynarr)->len = 0)

#define	dynarrFreeMemory(dynarr)	dynarrFreeMemory_raw(&(dynarr)->raw)

#define	dynarrSwap(a1, a2)	dynarrSwap_raw(&(a1)->raw, &(a2)->raw)

#define	dynarrInsert(dynarr, idx, val, ret_ok)\
do {\
	size_t __i, __idx;\
	if (!dynarrReserve(dynarr, (dynarr)->len + 1)) {\
		ret_ok = 0;\
		break;\
	}\
	ret_ok = 1;\
	__idx = idx;\
	for (__i = (dynarr)->len; __i > __idx; --__i) {\
		(dynarr)->buf[__i] = (dynarr)->buf[__i - 1];\
	}\
	(dynarr)->buf[__i] = val;\
	++((dynarr)->len);\
} while (0)

#define	dynarrRemoveIdx(dynarr, idx)\
do {\
	size_t __i = idx;\
	if (__i >= (dynarr)->len) {\
		break;\
	}\
	for (; __i + 1 < (dynarr)->len; ++__i) {\
		(dynarr)->buf[__i] = (dynarr)->buf[__i + 1];\
	}\
	--((dynarr)->len);\
} while (0)

#define	dynarrCopyAppend(dst, _buf, _len, ret_ok)\
do {\
	size_t __i;\
	if (!dynarrReserve(dst, (dst)->len + _len)) {\
		ret_ok = 0;\
		break;\
	}\
	ret_ok = 1;\
	for (__i = 0; __i < _len; ++__i) {\
		(dst)->buf[((dst)->len)++] = _buf[__i];\
	}\
} while (0)

#define	dynarrFindValue(dynarr, val, ret_idx)\
do {\
	size_t __i;\
	ret_idx = -1;\
	for (__i = 0; __i < (dynarr)->len; ++__i) {\
		if ((val) == (dynarr)->buf[__i]) {\
			ret_idx = __i;\
			break;\
		}\
	}\
} while (0)

#endif
