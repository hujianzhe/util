//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_MEMHEAP_MT_H
#define	UTIL_C_COMPONENT_MEMHEAP_MT_H

#include "../../inc/sysapi/ipc.h"
#include "../../inc/sysapi/mmap.h"

struct MemHeapMt_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll struct MemHeapMt_t* memheapmtCreate(size_t len, const char* name, int prot_bits);
__declspec_dll struct MemHeapMt_t* memheapmtOpen(const char* name, int prot_bits);
__declspec_dll size_t memheapmtSetupUsableRange(const struct MemHeapMt_t* memheap, void** out_buf, size_t alignment);
__declspec_dll void* memheapmtAlloc(struct MemHeapMt_t* memheap, size_t nbytes);
__declspec_dll void* memheapmtAlignAlloc(struct MemHeapMt_t* memheap, size_t nbytes, size_t alignment);
__declspec_dll void* memheapmtTryResize(struct MemHeapMt_t* memheap, void* addr, size_t nbytes);
__declspec_dll void memheapmtFree(struct MemHeapMt_t* memheap, void* addr);
__declspec_dll void memheapmtFreeAll(struct MemHeapMt_t* memheap);
__declspec_dll void memheapmtClose(struct MemHeapMt_t* memheap);

#ifdef	__cplusplus
}
#endif

#endif