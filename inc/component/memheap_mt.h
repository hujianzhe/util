//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_MEMHEAP_MT_H
#define	UTIL_C_COMPONENT_MEMHEAP_MT_H

#include "../../inc/sysapi/ipc.h"
#include "../../inc/sysapi/mmap.h"

typedef struct MemHeapMt_t {
	Semaphore_t seminit;
	Semaphore_t semlock;
	MemoryMapping_t mm;
	struct MemHeap_t* ptr;
	size_t len;
	short initok;
	short is_open;
	char* name_ext;
	size_t namelen;
} MemHeapMt_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll MemHeapMt_t* memheapmtCreate(MemHeapMt_t* memheap, size_t len, const char* name);
__declspec_dll MemHeapMt_t* memheapmtOpen(MemHeapMt_t* memheap, size_t len, const char* name);
__declspec_dll void* memheapmtAlloc(MemHeapMt_t* memheap, size_t nbytes);
__declspec_dll void memheapmtFree(MemHeapMt_t* memheap, void* addr);
__declspec_dll void memheapmtClose(MemHeapMt_t* memheap);

#ifdef	__cplusplus
}
#endif

#endif