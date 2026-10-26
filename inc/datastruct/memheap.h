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

__declspec_dll UnsignedPtr_t memheapSetupUsableRange(const struct MemHeap_t* memheap, void** out_buf, UnsignedPtr_t alignment) noexcept;
__declspec_dll struct MemHeap_t* memheapSetupAddr(void* addr) noexcept;

__declspec_dll struct MemHeap_t* memheapSetup(void* addr, UnsignedPtr_t len) noexcept;
__declspec_dll void* memheapAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes) noexcept;
__declspec_dll void* memheapAlignAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes, UnsignedPtr_t alignment) noexcept;
__declspec_dll void* memheapTryResize(struct MemHeap_t* memheap, void* addr, UnsignedPtr_t nbytes) noexcept;
__declspec_dll void memheapFree(struct MemHeap_t* memheap, void* addr) noexcept;
__declspec_dll void memheapFreeAll(struct MemHeap_t* memheap) noexcept;

#ifdef	__cplusplus
}
#endif

#endif
