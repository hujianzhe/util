//
// Created by hujianzhe
//

#include "../../inc/datastruct/memheap.h"
#include "../../inc/datastruct/list.h"

typedef struct MemHeapBlock_t {
	union {
		struct MemHeap_t* heap;
		UnsignedPtr_t heapoff;
	};
	union {
		ListNode_t node;
		struct {
			UnsignedPtr_t prevoff;
			UnsignedPtr_t nextoff;
		};
	};
	UnsignedPtr_t uselen;
	UnsignedPtr_t leftlen;
} MemHeapBlock_t;

#define	memheapblock_ptr(block)	((unsigned char*)((block) + 1))
#define	ptr_memheapblock(ptr)	((MemHeapBlock_t*)(((unsigned char*)(ptr)) - sizeof(MemHeapBlock_t)))
#define	PTR_VALUE_MAX			((UnsignedPtr_t)(~0))

typedef struct MemHeap_t {
	ListNode_t node;
	UnsignedPtr_t len;
	union {
		List_t blocklist;
		UnsignedPtr_t tailoff;
	};
	MemHeapBlock_t guard_block;
} MemHeap_t;

#ifdef	__cplusplus
extern "C" {
#endif

UnsignedPtr_t memheapLength(struct MemHeap_t* memheap) { return memheap->len; }

UnsignedPtr_t shmheapLength(struct MemHeap_t* memheap) { return memheap->len; }

MemHeap_t* memheapSetup(void* addr, UnsignedPtr_t len) {
	MemHeap_t* memheap;
	if (len < sizeof(MemHeap_t)) {
		return (MemHeap_t*)0;
	}
	memheap = (MemHeap_t*)addr;
	memheap->len = len;
	listInit(&memheap->blocklist);
	listPushNodeBack(&memheap->blocklist, &memheap->guard_block.node);
	memheap->guard_block.heap = memheap;
	memheap->guard_block.uselen = 0;
	memheap->guard_block.leftlen = len - sizeof(MemHeap_t);
	return memheap;
}

void* memheapAlloc(MemHeap_t* memheap, UnsignedPtr_t nbytes) {
	UnsignedPtr_t realbytes;
	ListNode_t* cur;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (cur = memheap->blocklist.tail; cur; cur = cur->prev) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)cur;
		if (block->leftlen >= realbytes) {
			MemHeapBlock_t* new_block = (MemHeapBlock_t*)(memheapblock_ptr(block) + block->uselen);
			new_block->heap = memheap;
			new_block->uselen = nbytes;
			new_block->leftlen = block->leftlen - realbytes;
			listInsertNodeBack(&memheap->blocklist, cur, &new_block->node);
			block->leftlen = 0;
			return memheapblock_ptr(new_block);
		}
	}
	return (void*)0;
}

void* memheapAlignAlloc(struct MemHeap_t* memheap, UnsignedPtr_t nbytes, UnsignedPtr_t alignment) {
	ListNode_t* cur;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	alignment -= 1;
	for (cur = memheap->blocklist.tail; cur; cur = cur->prev) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)cur;
		UnsignedPtr_t block_useptr = (UnsignedPtr_t)(memheapblock_ptr(block));
		UnsignedPtr_t block_useptr_end = block_useptr + block->uselen + block->leftlen;
		UnsignedPtr_t new_useptr;
		if (block->leftlen < nbytes + sizeof(MemHeapBlock_t)) {
			continue;
		}
		new_useptr = block_useptr + block->uselen + sizeof(MemHeapBlock_t);
		if (block_useptr_end - new_useptr < alignment) {
			continue;
		}
		new_useptr = (new_useptr + alignment) & (~alignment);
		if (nbytes <= block_useptr_end - new_useptr) {
			MemHeapBlock_t* new_block = ptr_memheapblock(new_useptr);
			new_block->heap = memheap;
			new_block->uselen = nbytes;
			new_block->leftlen = block_useptr_end - new_useptr - nbytes;
			listInsertNodeBack(&memheap->blocklist, cur, &new_block->node);
			block->leftlen = ((UnsignedPtr_t)new_block) - block_useptr - block->uselen;
			return (void*)new_useptr;
		}
	}
	return (void*)0;
}

void memheapFree(void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr), *prev_block;
		MemHeap_t* memheap = block->heap;
		prev_block = (MemHeapBlock_t*)(block->node.prev);
		prev_block->leftlen += sizeof(MemHeapBlock_t) + block->uselen + block->leftlen;
		listRemoveNode(&memheap->blocklist, &block->node);
	}
}

static void __insertback(MemHeap_t* memheap, MemHeapBlock_t* node, MemHeapBlock_t* new_node) {
	UnsignedPtr_t base = (UnsignedPtr_t)memheap;
	new_node->prevoff = (UnsignedPtr_t)node - base;
	new_node->nextoff = node->nextoff;
	if (node->nextoff)
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = (UnsignedPtr_t)new_node - base;
	node->nextoff = (UnsignedPtr_t)new_node - base;
	if (memheap->tailoff == new_node->prevoff)
		memheap->tailoff = (UnsignedPtr_t)new_node - base;
}

static void __remove(MemHeap_t* memheap, MemHeapBlock_t* node) {
	UnsignedPtr_t base = (UnsignedPtr_t)memheap;
	if (node->prevoff)
		((MemHeapBlock_t*)(base + node->prevoff))->nextoff = node->nextoff;
	if (node->nextoff)
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = node->prevoff;
	if (memheap->tailoff + base == (UnsignedPtr_t)node)
		memheap->tailoff = node->prevoff;
}

MemHeap_t* shmheapSetup(void* addr, UnsignedPtr_t len) {
	MemHeap_t* memheap;
	if (len < sizeof(MemHeap_t)) {
		return (MemHeap_t*)0;
	}
	memheap = (MemHeap_t*)addr;
	memheap->len = len;
	memheap->tailoff = (UnsignedPtr_t)&memheap->guard_block - (UnsignedPtr_t)memheap;
	memheap->guard_block.heapoff = memheap->tailoff;
	memheap->guard_block.prevoff = 0;
	memheap->guard_block.nextoff = 0;
	memheap->guard_block.uselen = 0;
	memheap->guard_block.leftlen = len - sizeof(MemHeap_t);
	return memheap;
}

void* shmheapAlloc(MemHeap_t* memheap, UnsignedPtr_t nbytes) {
	UnsignedPtr_t realbytes, curoff, prevoff;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (curoff = memheap->tailoff; curoff; curoff = prevoff) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)(curoff + (UnsignedPtr_t)memheap);
		prevoff = block->prevoff;
		if (block->leftlen >= realbytes) {
			UnsignedPtr_t newoff = curoff + sizeof(MemHeapBlock_t) + block->uselen;
			MemHeapBlock_t* newblock = (MemHeapBlock_t*)(newoff + (UnsignedPtr_t)memheap);
			newblock->heapoff = newoff;
			newblock->uselen = nbytes;
			newblock->leftlen = block->leftlen - realbytes;
			__insertback(memheap, block, newblock);
			block->leftlen = 0;
			return memheapblock_ptr(newblock);
		}
	}
	return (void*)0;
}

void shmheapFree(void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr), *prev_block;
		MemHeap_t* memheap = (MemHeap_t*)((UnsignedPtr_t)block - block->heapoff);
		prev_block = (MemHeapBlock_t*)(block->prevoff + (UnsignedPtr_t)memheap);
		prev_block->leftlen += sizeof(MemHeapBlock_t) + block->uselen + block->leftlen;
		__remove(memheap, block);
	}
}

#ifdef	__cplusplus
}
#endif
