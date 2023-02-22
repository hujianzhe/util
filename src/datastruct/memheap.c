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

typedef struct ShmHeap_t {
	UnsignedPtr_t len;
	UnsignedPtr_t tailoff;
	MemHeapBlock_t guard_block;
} ShmHeap_t;

static UnsignedPtr_t __shmheapblock_leftlen(ShmHeap_t* shmheap, MemHeapBlock_t* block) {
	if (block->nextoff) {
		return (UnsignedPtr_t)shmheap + block->nextoff - (UnsignedPtr_t)block - sizeof(*block) - block->uselen;
	}
	else {
		return shmheap->len - shmheap->tailoff - sizeof(*block) - block->uselen;
	}
}

static void __insertback(ShmHeap_t* shmheap, MemHeapBlock_t* node, MemHeapBlock_t* new_node) {
	UnsignedPtr_t base = (UnsignedPtr_t)shmheap;
	new_node->prevoff = (UnsignedPtr_t)node - base;
	new_node->nextoff = node->nextoff;
	if (node->nextoff) {
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = (UnsignedPtr_t)new_node - base;
	}
	node->nextoff = (UnsignedPtr_t)new_node - base;
	if (shmheap->tailoff == new_node->prevoff) {
		shmheap->tailoff = (UnsignedPtr_t)new_node - base;
	}
}

static void __remove(ShmHeap_t* shmheap, MemHeapBlock_t* node) {
	UnsignedPtr_t base = (UnsignedPtr_t)shmheap;
	if (node->prevoff) {
		((MemHeapBlock_t*)(base + node->prevoff))->nextoff = node->nextoff;
	}
	if (node->nextoff) {
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = node->prevoff;
	}
	if (shmheap->tailoff + base == (UnsignedPtr_t)node) {
		shmheap->tailoff = node->prevoff;
	}
}

#ifdef	__cplusplus
extern "C" {
#endif

UnsignedPtr_t shmheapLength(ShmHeap_t* shmheap) { return shmheap->len; }
void* shmheapStartAddr(ShmHeap_t* shmheap) { return (void*)(shmheap + 1); }

ShmHeap_t* shmheapSetup(void* addr, UnsignedPtr_t len) {
	ShmHeap_t* shmheap;
	if (len < sizeof(ShmHeap_t)) {
		return (ShmHeap_t*)0;
	}
	shmheap = (ShmHeap_t*)addr;
	shmheap->len = len;
	shmheap->tailoff = (UnsignedPtr_t)&shmheap->guard_block - (UnsignedPtr_t)shmheap;
	shmheap->guard_block.prevoff = 0;
	shmheap->guard_block.nextoff = 0;
	shmheap->guard_block.uselen = 0;
	return shmheap;
}

void* shmheapAlloc(ShmHeap_t* shmheap, UnsignedPtr_t nbytes) {
	UnsignedPtr_t realbytes, curoff, prevoff;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (curoff = shmheap->tailoff; curoff; curoff = prevoff) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)(curoff + (UnsignedPtr_t)shmheap);
		prevoff = block->prevoff;
		if (__shmheapblock_leftlen(shmheap, block) >= realbytes) {
			UnsignedPtr_t newoff = curoff + sizeof(MemHeapBlock_t) + block->uselen;
			MemHeapBlock_t* newblock = (MemHeapBlock_t*)(newoff + (UnsignedPtr_t)shmheap);
			newblock->uselen = nbytes;
			__insertback(shmheap, block, newblock);
			return memheapblock_ptr(newblock);
		}
	}
	return (void*)0;
}

void* shmheapRealloc(ShmHeap_t* shmheap, void* addr, UnsignedPtr_t nbytes) {
	if (!addr) {
		return shmheapAlloc(shmheap, nbytes);
	}
	else if (!nbytes) {
		shmheapFree(shmheap, addr);
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
		if (__shmheapblock_leftlen(shmheap, block) + block->uselen >= nbytes) {
			block->uselen = nbytes;
			return addr;
		}
		new_addr = shmheapAlloc(shmheap, nbytes);
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
		shmheapFree(shmheap, addr);
		return new_addr;
	}
}

void shmheapFree(ShmHeap_t* shmheap, void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr);
		__remove(shmheap, block);
	}
}

#ifdef	__cplusplus
}
#endif
