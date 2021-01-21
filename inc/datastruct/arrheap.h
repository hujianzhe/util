//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_ARRHEAP_H
#define	UTIL_C_DATASTRUCT_ARRHEAP_H

#include "../compiler_define.h"

typedef struct SortHeap_t {
	ptrlen_t ecnt;
	ptrlen_t esize;
	ptrlen_t bufsize;
	unsigned char* buf;
	int(*cmp)(const void*, const void*);
} SortHeap_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll SortHeap_t* sortheapInit(SortHeap_t* h, void* buf, ptrlen_t bufsize, ptrlen_t esize, int(*cmp)(const void*, const void*));
__declspec_dll int sortheapIsFull(SortHeap_t* h);
__declspec_dll int sortheapIsEmpty(SortHeap_t* h);
__declspec_dll const void* sortheapTop(SortHeap_t* h);
__declspec_dll SortHeap_t* sortheapInsert(SortHeap_t* h, void* ptr_data);
__declspec_dll void sortheapPop(SortHeap_t* h);

#ifdef	__cplusplus
}
#endif

#endif
