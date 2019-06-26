//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SORT_H
#define	UTIL_C_DATASTRUCT_SORT_H

#include "list.h"

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void sortMergeOrder(void* p_, ptrlen_t icnt, const void* p1_, ptrlen_t icnt1, const void* p2_, ptrlen_t icnt2, ptrlen_t esize, const void*(*cmp)(const void*, const void*));
__declspec_dll int sortInsertTopN(void* p_, ptrlen_t icnt, ptrlen_t topn, const void* new_, ptrlen_t esize, int(*less)(const void*, const void*));

#ifdef	__cplusplus
}
#endif

#endif
