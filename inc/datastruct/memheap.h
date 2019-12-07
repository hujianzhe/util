//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_MEMHEAP_H
#define	UTIL_C_DATASTRUCT_MEMHEAP_H

#include "../compiler_define.h"

struct MemHeap_t;

extern struct MemHeap_t* memheapSetup(void* addr, ptrlen_t len);
extern void* memheapAlloc(struct MemHeap_t* memheap, ptrlen_t nbytes);
extern void memheapFree(struct MemHeap_t* memheap, void* addr);

#endif