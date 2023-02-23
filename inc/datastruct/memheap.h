//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_MEMHEAP_H
#define	UTIL_C_DATASTRUCT_MEMHEAP_H

#include "../compiler_define.h"

struct ShmHeap_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll UnsignedPtr_t shmheapLength(struct ShmHeap_t* h);
__declspec_dll void* shmheapStartAddr(struct ShmHeap_t* shmheap);

__declspec_dll struct ShmHeap_t* shmheapSetup(void* addr, UnsignedPtr_t len);
__declspec_dll void* shmheapAlloc(struct ShmHeap_t* shmheap, UnsignedPtr_t nbytes);
__declspec_dll void* shmheapAlignAlloc(struct ShmHeap_t* shmheap, UnsignedPtr_t nbytes, UnsignedPtr_t alignment);
__declspec_dll void* shmheapRealloc(struct ShmHeap_t* shmheap, void* addr, UnsignedPtr_t nbytes);
__declspec_dll void shmheapFree(struct ShmHeap_t* shmheap, void* addr);
__declspec_dll void shmheapFreeAll(struct ShmHeap_t* shmheap);

#ifdef	__cplusplus
}
#endif

#endif
