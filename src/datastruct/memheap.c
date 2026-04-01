//
// Created by hujianzhe
//

#include "../../inc/datastruct/memheap.h"

typedef struct MemHeapBlock_t {
	UnsignedPtr_t prevoff;
	UnsignedPtr_t nextoff;
	UnsignedPtr_t uselen;
} MemHeapBlock_t;

#define	ptr_memheapblock__(ptr)	((MemHeapBlock_t*)(((unsigned char*)(ptr)) - sizeof(MemHeapBlock_t)))

typedef struct MemHeap_t {
	UnsignedPtr_t len;
	UnsignedPtr_t tailoff;
	MemHeapBlock_t guard_block;
} MemHeap_t;

static UnsignedPtr_t __heapblock_leftlen(MemHeap_t* memheap, MemHeapBlock_t* block) {
	if (block->nextoff) {
		return (UnsignedPtr_t)memheap + block->nextoff - (UnsignedPtr_t)block - sizeof(*block) - block->uselen;
	}
	else {
		return memheap->len - memheap->tailoff - sizeof(*block) - block->uselen;
	}
}

static void __insertback(MemHeap_t* memheap, MemHeapBlock_t* node, MemHeapBlock_t* new_node) {
	UnsignedPtr_t base = (UnsignedPtr_t)memheap;
	new_node->prevoff = (UnsignedPtr_t)node - base;
	new_node->nextoff = node->nextoff;
	if (node->nextoff) {
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = (UnsignedPtr_t)new_node - base;
	}
	node->nextoff = (UnsignedPtr_t)new_node - base;
	if (memheap->tailoff == new_node->prevoff) {
		memheap->tailoff = (UnsignedPtr_t)new_node - base;
	}
}

static void __remove(MemHeap_t* memheap, MemHeapBlock_t* node) {
	UnsignedPtr_t base = (UnsignedPtr_t)memheap;
	if (node->prevoff) {
		((MemHeapBlock_t*)(base + node->prevoff))->nextoff = node->nextoff;
	}
	if (node->nextoff) {
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = node->prevoff;
	}
	if (memheap->tailoff + base == (UnsignedPtr_t)node) {
		memheap->tailoff = node->prevoff;
	}
}

#ifdef	__cplusplus
extern "C" {
#endif

UnsignedPtr_t memheapSetupUsableRange(const MemHeap_t* memheap, void** out_buf, UnsignedPtr_t alignment) {
	UnsignedPtr_t ptr, newptr, end, mask;
	*out_buf = (void*)0;
	if (0 == alignment) {
		return 0;
	}
	mask = alignment - 1;
	if (alignment & mask) {
		return 0;
	}
	ptr = (UnsignedPtr_t)(&memheap->guard_block + 1);
	if ((UnsignedPtr_t)(~0) - mask < ptr) {
		return 0;
	}
	newptr = (ptr + mask) & (~mask);
	end = (UnsignedPtr_t)memheap + memheap->len;
	if (end <= newptr) {
		return 0;
	}
	*out_buf = (void*)newptr;
	return end - newptr;
}

MemHeap_t* memheapSetupAddr(void* addr) {
	const UnsignedPtr_t MAX_PTR_VALUE = (UnsignedPtr_t)(~0);
	UnsignedPtr_t addr_aligned, mask = sizeof(void*) - 1;
	if (MAX_PTR_VALUE - mask < (UnsignedPtr_t)addr) {
		return (void*)0;
	}
	addr_aligned = (((UnsignedPtr_t)addr) + mask) & (~mask);
	return (MemHeap_t*)addr_aligned;
}

MemHeap_t* memheapSetup(void* addr, UnsignedPtr_t len) {
	UnsignedPtr_t aligned_len;
	MemHeap_t* memheap = memheapSetupAddr(addr);
	if (!memheap) {
		return (MemHeap_t*)0;
	}
	aligned_len = (UnsignedPtr_t)memheap - (UnsignedPtr_t)addr;
	if (len < sizeof(MemHeap_t) + aligned_len) {
		return (MemHeap_t*)0;
	}
	memheap->len = len - aligned_len;
	memheap->tailoff = (UnsignedPtr_t)&memheap->guard_block - (UnsignedPtr_t)memheap;
	memheap->guard_block.prevoff = 0;
	memheap->guard_block.nextoff = 0;
	memheap->guard_block.uselen = 0;
	return memheap;
}

void* memheapAlloc(MemHeap_t* memheap, UnsignedPtr_t nbytes) {
	return memheapAlignAlloc(memheap, nbytes, sizeof(void*) + sizeof(void*));
}

void* memheapAlignAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes, UnsignedPtr_t alignment) {
	UnsignedPtr_t realbytes, curoff, prevoff, mask;
	const UnsignedPtr_t MAX_PTR_VALUE = (UnsignedPtr_t)(~0);
	if (MAX_PTR_VALUE - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	if (0 == alignment) {
		return (void*)0;
	}
	mask = alignment - 1;
	if (alignment & mask) {
		return (void*)0;
	}
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (curoff = memheap->tailoff; curoff; curoff = prevoff) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)(curoff + (UnsignedPtr_t)memheap);
		UnsignedPtr_t leftlen = __heapblock_leftlen(memheap, block);
		prevoff = block->prevoff;
		if (leftlen >= realbytes) {
			UnsignedPtr_t newptr, ptr;
			ptr = (UnsignedPtr_t)block + sizeof(MemHeapBlock_t) + block->uselen;
			if (MAX_PTR_VALUE - sizeof(MemHeapBlock_t) <= ptr) {
				continue;
			}
			ptr += sizeof(MemHeapBlock_t);
			if (MAX_PTR_VALUE - mask < ptr) {
				continue;
			}
			newptr = (ptr + mask) & (~mask);
			if (leftlen - realbytes >= newptr - ptr) {
				MemHeapBlock_t* newblock = ptr_memheapblock__(newptr);
				newblock->uselen = nbytes;
				__insertback(memheap, block, newblock);
				return (void*)newptr;
			}
		}
	}
	return (void*)0;
}

void* memheapTryResize(MemHeap_t* memheap, void* addr, UnsignedPtr_t nbytes) {
	MemHeapBlock_t* block = ptr_memheapblock__(addr);
	if (block->uselen >= nbytes) {
		block->uselen = nbytes;
		return addr;
	}
	if (__heapblock_leftlen(memheap, block) + block->uselen >= nbytes) {
		block->uselen = nbytes;
		return addr;
	}
	return (void*)0;
}

void memheapFree(MemHeap_t* memheap, void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock__(addr);
		__remove(memheap, block);
	}
}

void memheapFreeAll(MemHeap_t* memheap) {
	memheap->tailoff = (UnsignedPtr_t)&memheap->guard_block - (UnsignedPtr_t)memheap;
	memheap->guard_block.nextoff = 0;
}

#ifdef	__cplusplus
}
#endif
