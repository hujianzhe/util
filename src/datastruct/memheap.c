//
// Created by hujianzhe
//

#include "../../inc/datastruct/memheap.h"
#include "../../inc/datastruct/list.h"

typedef struct MemHeapBlock_t {
	union {
		ListNode_t node;
		struct {
			ptrlen_t prevoff;
			ptrlen_t nextoff;
		};
	};
	ptrlen_t uselen;
	ptrlen_t leftlen;
} MemHeapBlock_t;

#define	memheapblock_ptr(block)	((unsigned char*)((block) + 1))
#define	ptr_memheapblock(ptr)	((MemHeapBlock_t*)(((unsigned char*)(ptr)) - sizeof(MemHeapBlock_t)))
#define	PTR_VALUE_MAX			((ptrlen_t)(~0))

typedef struct MemHeap_t {
	ListNode_t node;
	ptrlen_t len;
	union {
		List_t blocklist;
		ptrlen_t tailoff;
	};
	MemHeapBlock_t guard_block;
} MemHeap_t;

#ifdef	__cplusplus
extern "C" {
#endif

MemHeap_t* memheapSetup(void* addr, ptrlen_t len) {
	MemHeap_t* memheap;
	if (len < sizeof(MemHeap_t)) {
		return (MemHeap_t*)0;
	}
	memheap = (MemHeap_t*)addr;
	memheap->len = len;
	listInit(&memheap->blocklist);
	listPushNodeBack(&memheap->blocklist, &memheap->guard_block.node);
	memheap->guard_block.uselen = 0;
	memheap->guard_block.leftlen = len - sizeof(MemHeap_t);
	return memheap;
}

void* memheapAlloc(MemHeap_t* memheap, ptrlen_t nbytes) {
	ptrlen_t realbytes;
	ListNode_t* cur;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (cur = memheap->blocklist.tail; cur; cur = cur->prev) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)cur;
		if (block->leftlen >= realbytes) {
			MemHeapBlock_t* new_block = (MemHeapBlock_t*)(memheapblock_ptr(block) + block->uselen);
			new_block->uselen = nbytes;
			new_block->leftlen = block->leftlen - realbytes;
			listInsertNodeBack(&memheap->blocklist, cur, &new_block->node);
			block->leftlen = 0;
			return memheapblock_ptr(new_block);
		}
	}
	return (void*)0;
}

void* memheapAlignAlloc(struct MemHeap_t* memheap, ptrlen_t nbytes, ptrlen_t alignment) {
	ListNode_t* cur;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	alignment -= 1;
	for (cur = memheap->blocklist.tail; cur; cur = cur->prev) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)cur;
		ptrlen_t block_useptr = (ptrlen_t)(memheapblock_ptr(block));
		ptrlen_t block_useptr_end = block_useptr + block->uselen + block->leftlen;
		ptrlen_t new_useptr;
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
			new_block->uselen = nbytes;
			new_block->leftlen = block_useptr_end - new_useptr - nbytes;
			listInsertNodeBack(&memheap->blocklist, cur, &new_block->node);
			block->leftlen = ((ptrlen_t)new_block) - block_useptr - block->uselen;
			return (void*)new_useptr;
		}
	}
	return (void*)0;
}

void memheapFree(MemHeap_t* memheap, void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr), *prev_block;
		prev_block = (MemHeapBlock_t*)(block->node.prev);
		prev_block->leftlen += sizeof(MemHeapBlock_t) + block->uselen + block->leftlen;
		listRemoveNode(&memheap->blocklist, &block->node);
	}
}

static void __insertback(MemHeap_t* memheap, MemHeapBlock_t* node, MemHeapBlock_t* new_node) {
	ptrlen_t base = (ptrlen_t)memheap;
	new_node->prevoff = (ptrlen_t)node - base;
	new_node->nextoff = node->nextoff;
	if (node->nextoff)
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = (ptrlen_t)new_node - base;
	node->nextoff = (ptrlen_t)new_node - base;
	if (memheap->tailoff == new_node->prevoff)
		memheap->tailoff = (ptrlen_t)new_node - base;
}

static void __remove(MemHeap_t* memheap, MemHeapBlock_t* node) {
	ptrlen_t base = (ptrlen_t)memheap;
	if (node->prevoff)
		((MemHeapBlock_t*)(base + node->prevoff))->nextoff = node->nextoff;
	if (node->nextoff)
		((MemHeapBlock_t*)(base + node->nextoff))->prevoff = node->prevoff;
	if (memheap->tailoff + base == (ptrlen_t)node)
		memheap->tailoff = node->prevoff;
}

MemHeap_t* shmheapSetup(void* addr, ptrlen_t len) {
	MemHeap_t* memheap;
	if (len < sizeof(MemHeap_t)) {
		return (MemHeap_t*)0;
	}
	memheap = (MemHeap_t*)addr;
	memheap->len = len;
	memheap->tailoff = (ptrlen_t)&memheap->guard_block - (ptrlen_t)memheap;
	memheap->guard_block.prevoff = 0;
	memheap->guard_block.nextoff = 0;
	memheap->guard_block.uselen = 0;
	memheap->guard_block.leftlen = len - sizeof(MemHeap_t);
	return memheap;
}

void* shmheapAlloc(MemHeap_t* memheap, ptrlen_t nbytes) {
	ptrlen_t realbytes, curoff, prevoff;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	realbytes = nbytes + sizeof(MemHeapBlock_t);
	for (curoff = memheap->tailoff; curoff; curoff = prevoff) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)(curoff + (ptrlen_t)memheap);
		prevoff = block->prevoff;
		if (block->leftlen >= realbytes) {
			ptrlen_t newoff = curoff + sizeof(MemHeapBlock_t) + block->uselen;
			MemHeapBlock_t* newblock = (MemHeapBlock_t*)(newoff + (ptrlen_t)memheap);
			newblock->uselen = nbytes;
			newblock->leftlen = block->leftlen - realbytes;
			__insertback(memheap, block, newblock);
			block->leftlen = 0;
			return memheapblock_ptr(newblock);
		}
	}
	return (void*)0;
}

void shmheapFree(MemHeap_t* memheap, void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr), *prev_block;
		prev_block = (MemHeapBlock_t*)(block->prevoff + (ptrlen_t)memheap);
		prev_block->leftlen += sizeof(MemHeapBlock_t) + block->uselen + block->leftlen;
		__remove(memheap, block);
	}
}

#ifdef	__cplusplus
}
#endif