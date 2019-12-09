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

__declspec_dll struct MemHeap_t* memheapSetup(void* addr, ptrlen_t len, void(*fn_lock_or_unlock)(int));
__declspec_dll void* memheapAlloc(struct MemHeap_t* memheap, ptrlen_t nbytes);
__declspec_dll void* memheapAlignAlloc(struct MemHeap_t* memheap, ptrlen_t nbytes, ptrlen_t alignment);
__declspec_dll void memheapFree(struct MemHeap_t* memheap, void* addr);

__declspec_dll struct MemHeap_t* shmheapSetup(void* addr, ptrlen_t len, void(*fn_lock_or_unlock)(int));
__declspec_dll void* shmheapAlloc(struct MemHeap_t* memheap, ptrlen_t nbytes);
__declspec_dll void shmheapFree(struct MemHeap_t* memheap, void* addr);

#ifdef	__cplusplus
}
#endif

#endif