//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SORT_H
#define	UTIL_C_DATASTRUCT_SORT_H

#include "list.h"

typedef struct SortInsertTopN_t {
	/* insert parameter */
	UnsignedPtr_t ecnt;
	UnsignedPtr_t esize;
	UnsignedPtr_t N;
	const void*(*cmp)(const void*, const void*);
	/* insert result */
	int insert_ok;
	int has_discard;
	void* discard_bak;
} SortInsertTopN_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void sortMergeOrder(void* p_, UnsignedPtr_t icnt, const void* p1_, UnsignedPtr_t icnt1, const void* p2_, UnsignedPtr_t icnt2, UnsignedPtr_t esize, const void*(*cmp)(const void*, const void*));
__declspec_dll SortInsertTopN_t* sortInsertTopN(void* top, void* new_, SortInsertTopN_t* arg);

#ifdef	__cplusplus
}
#endif

#endif
