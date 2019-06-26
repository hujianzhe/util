//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SORT_H
#define	UTIL_C_DATASTRUCT_SORT_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void sortMergeOrder(void* p_, ptrlen_t icnt, const void* p1_, ptrlen_t icnt1, const void* p2_, ptrlen_t icnt2, ptrlen_t esize, const void*(*cmp)(const void*, const void*));

#ifdef	__cplusplus
}
#endif

#endif
