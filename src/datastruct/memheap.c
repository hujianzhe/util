//
// Created by hujianzhe
//

#include "../../inc/datastruct/memheap.h"

typedef struct MemHeapBlock_t {
	UnsignedPtr_t prevoff;
	UnsignedPtr_t nextoff;
	UnsignedPtr_t uselen;
} MemHeapBlock_t;

#define	memheapblock_ptr(block)	((unsigned char*)((block) + 1))
#define	ptr_memheapblock(ptr)	((MemHeapBlock_t*)(((unsigned char*)(ptr)) - sizeof(MemHeapBlock_t)))
#define	PTR_VALUE_MAX			((UnsignedPtr_t)(~0))

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

UnsignedPtr_t memheapLength(MemHeap_t* memheap) { return memheap->len; }
void* memheapStartAddr(MemHeap_t* memheap) { return (void*)(memheap + 1); }

MemHeap_t* memheapSetup(void* addr, UnsignedPtr_t len) {
	MemHeap_t* memheap;
	if (len < sizeof(MemHeap_t)) {
		return (MemHeap_t*)0;
	}
	memheap = (MemHeap_t*)addr;
	memheap->len = len;
	memheap->tailoff = (UnsignedPtr_t)&memheap->guard_block - (UnsignedPtr_t)memheap;
	memheap->guard_block.prevoff = 0;
	memheap->guard_block.nextoff = 0;
	memheap->guard_block.uselen = 0;
	return memheap;
}

void* memheapAlloc(MemHeap_t* memheap, UnsignedPtr_t nbytes) {
	if (sizeof(void*) > 4) {
		return memheapAlignAlloc(memheap, nbytes, 16);
	}
	else {
		return memheapAlignAlloc(memheap, nbytes, 8);
	}
}

void* memheapAlignAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes, UnsignedPtr_t alignment) {
	UnsignedPtr_t realbytes, curoff, prevoff, mask;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	mask = alignment - 1;
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (curoff = memheap->tailoff; curoff; curoff = prevoff) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)(curoff + (UnsignedPtr_t)memheap);
		UnsignedPtr_t leftlen = __heapblock_leftlen(memheap, block);
		prevoff = block->prevoff;
		if (leftlen >= realbytes) {
			UnsignedPtr_t newoff = curoff + sizeof(MemHeapBlock_t) + block->uselen;
			UnsignedPtr_t ptr = newoff + (UnsignedPtr_t)memheap + sizeof(MemHeapBlock_t);
			UnsignedPtr_t newptr = (ptr + mask) & (~mask);
			realbytes += newptr - ptr;
			if (leftlen < realbytes) {
				continue;
			}
			MemHeapBlock_t* newblock = ptr_memheapblock(newptr);
			newblock->uselen = nbytes;
			__insertback(memheap, block, newblock);
			return (void*)newptr;
		}
	}
	return (void*)0;
}

void* memheapRealloc(MemHeap_t* memheap, void* addr, UnsignedPtr_t nbytes) {
	if (!addr) {
		return memheapAlloc(memheap, nbytes);
	}
	else if (!nbytes) {
		memheapFree(memheap, addr);
		return (void*)0;
	}
	else {
		char* new_p, *old_p;
		void* new_addr;
		MemHeapBlock_t* block = ptr_memheapblock(addr);
		if (block->uselen >= nbytes) {
			block->uselen = nbytes;
			return addr;
		}
		if (__heapblock_leftlen(memheap, block) + block->uselen >= nbytes) {
			block->uselen = nbytes;
			return addr;
		}
		new_addr = memheapAlloc(memheap, nbytes);
		if (!new_addr) {
			return (void*)0;
		}
		new_p = (char*)new_addr;
		old_p = (char*)addr;
		nbytes = block->uselen;
		while (nbytes--) {
			*new_p = *old_p;
			++new_p;
			++old_p;
		}
		memheapFree(memheap, addr);
		return new_addr;
	}
}

void memheapFree(MemHeap_t* memheap, void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr);
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
