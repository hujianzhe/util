//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_DYNARR_H
#define	UTIL_C_CRT_DYNARR_H

#include <stddef.h>
#include <stdlib.h>

#define	dynarrReserve(dynarr, type, _capacity, ret_ok)\
do {\
	size_t __cap = _capacity;\
	if ((dynarr)->capacity < __cap) {\
		type* __p = (type*)realloc((dynarr)->buf, sizeof(type) * __cap);\
		if (!__p) {\
			ret_ok = 0;\
			break;\
		}\
		(dynarr)->buf = __p;\
		(dynarr)->capacity = __cap;\
	}\
	ret_ok = 1;\
} while (0)

#define	dynarrResize(dynarr, type, _len, def_val, ret_ok)\
do {\
	size_t __i;\
	size_t __len = _len;\
	dynarrReserve(dynarr, type, __len, ret_ok);\
	if (!ret_ok) {\
		break;\
	}\
	(dynarr)->len = __len;\
	for (__i = 0; __i < __len; ++__i) {\
		(dynarr)->buf[__i] = (def_val);\
	}\
} while (0)

#define	dynarrInsert(dynarr, type, idx, val, ret_ok)\
do {\
	size_t __i, __idx;\
	type* __p;\
	dynarrReserve(dynarr, type, (dynarr)->len + 1, ret_ok);\
	if (!ret_ok) {\
		break;\
	}\
	__p = (dynarr)->buf;\
	__idx = idx;\
	for (__i = (dynarr)->len; __i > __idx; --__i) {\
		__p[__i] = __p[__i - 1];\
	}\
	__p[__i] = val;\
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
	(dynarr)->buf = NULL;\
	(dynarr)->len = 0;\
	(dynarr)->capacity = 0;\
} while (0)

#define	dynarrSwap(a1, a2, type)\
do {\
	type* __p;\
	size_t __v;\
	if ((void*)a1 == (void*)a2) {\
		break;\
	}\
	__p = (a1)->buf;\
	(a1)->buf = (a2)->buf;\
	(a2)->buf = __p;\
	__v = (a1)->len;\
	(a1)->len = (a2)->len;\
	(a2)->len = __v;\
	__v = (a1)->capacity;\
	(a1)->capacity = (a2)->capacity;\
	(a2)->capacity = __v;\
} while (0)

#define	DynArr_t(type)\
struct {\
	type* buf;\
	size_t len;\
	size_t capacity;\
}

#endif
