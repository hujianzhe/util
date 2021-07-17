//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_DYNARR_H
#define	UTIL_C_CRT_DYNARR_H

#include <stddef.h>
#include <stdlib.h>

#define	DynArr_t(type)\
struct {\
	type* buf;\
	size_t len;\
	size_t capacity;\
}

#define	dynarrInitZero(dynarr)\
do {\
	(dynarr)->buf = NULL;\
	(dynarr)->len = 0;\
	(dynarr)->capacity = 0;\
} while (0)

#define	dynarrReserve(dynarr, _capacity, ret_ok)\
do {\
	size_t __cap = _capacity;\
	if ((dynarr)->capacity < __cap) {\
		void* __p = realloc((dynarr)->buf, sizeof((dynarr)->buf[0]) * __cap);\
		if (!__p) {\
			ret_ok = 0;\
			break;\
		}\
		*(size_t*)&((dynarr)->buf) = (size_t)__p;\
		(dynarr)->capacity = __cap;\
	}\
	ret_ok = 1;\
} while (0)

#define	dynarrResize(dynarr, _len, def_val, ret_ok)\
do {\
	size_t __i;\
	size_t __len = _len;\
	dynarrReserve(dynarr, __len, ret_ok);\
	if (!ret_ok) {\
		break;\
	}\
	(dynarr)->len = __len;\
	for (__i = 0; __i < __len; ++__i) {\
		(dynarr)->buf[__i] = (def_val);\
	}\
} while (0)

#define	dynarrInsert(dynarr, idx, val, ret_ok)\
do {\
	size_t __i, __idx;\
	dynarrReserve(dynarr, (dynarr)->len + 1, ret_ok);\
	if (!ret_ok) {\
		break;\
	}\
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

#define	dynarrClearData(dynarr) ((dynarr)->len = 0)

#define	dynarrFreeMemory(dynarr)\
do {\
	free((dynarr)->buf);\
	dynarrInitZero(dynarr);\
} while (0)

#define	dynarrSwap(a1, a2)\
do {\
	size_t __v;\
	if ((void*)a1 == (void*)a2) {\
		break;\
	}\
	__v = (size_t)(a1)->buf;\
	(a1)->buf = (a2)->buf;\
	*(size_t*)&((a2)->buf) = __v;\
	__v = (a1)->len;\
	(a1)->len = (a2)->len;\
	(a2)->len = __v;\
	__v = (a1)->capacity;\
	(a1)->capacity = (a2)->capacity;\
	(a2)->capacity = __v;\
} while (0)

#define	dynarrFind(dynarr, ret_idx, val)\
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
