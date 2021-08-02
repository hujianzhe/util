//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_ARRHEAP_H
#define	UTIL_C_DATASTRUCT_ARRHEAP_H

#include "../compiler_define.h"

#define	arrheapInsert(type, buf, len, cmp)\
do {\
	UnsignedPtr_t __ci;\
	if (len <= 1) {\
		break;\
	}\
	__ci = len - 1;\
	while (__ci > 0) {\
		UnsignedPtr_t __pi = ((__ci - 1) >> 1);\
		if (cmp(&buf[__pi], &buf[__ci]) > 0) {\
			type __temp = buf[__pi];\
			buf[__pi] = buf[__ci];\
			buf[__ci] = __temp;\
			__ci = __pi;\
		} else break;\
	}\
} while (0)

#define	arrheapPop(type, buf, len, cmp)\
do {\
	UnsignedPtr_t __pi, __ci;\
	if (len <= 1) {\
		break;\
	}\
	buf[0] = buf[len - 1];\
	__pi = 0;\
	__ci = (__pi << 1) + 1;\
	while (__ci < len - 1) {\
		if (__ci + 1 < len - 1) {\
			if (cmp(&buf[__ci], &buf[__ci + 1]) > 0) {\
				__ci += 1;\
			}\
		}\
		if (cmp(&buf[__pi], &buf[__ci]) > 0) {\
			type __temp = buf[__pi];\
			buf[__pi] = buf[__ci];\
			buf[__ci] = __temp;\
			__pi = __ci;\
			__ci = (__pi << 1) + 1;\
		} else break;\
	}\
} while (0)

#endif
