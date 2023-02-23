//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_MEMHEAP_H
#define	UTIL_C_DATASTRUCT_MEMHEAP_H

#include "../compiler_define.h"

struct MemHeap_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll UnsignedPtr_t memheapLength(struct MemHeap_t* h);
__declspec_dll void* memheapStartAddr(struct MemHeap_t* memheap);

__declspec_dll struct MemHeap_t* memheapSetup(void* addr, UnsignedPtr_t len);
__declspec_dll void* memheapAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes);
__declspec_dll void* memheapAlignAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes, UnsignedPtr_t alignment);
__declspec_dll void* memheapRealloc(struct MemHeap_t* memheap, void* addr, UnsignedPtr_t nbytes);
__declspec_dll void memheapFree(struct MemHeap_t* memheap, void* addr);
__declspec_dll void memheapFreeAll(struct MemHeap_t* memheap);

#ifdef	__cplusplus
}
#endif

#endif
