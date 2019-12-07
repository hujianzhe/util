//
// Created by hujianzhe
//

#include "../../inc/datastruct/memheap.h"
#include "../../inc/datastruct/list.h"

typedef struct MemHeapBlock_t {
	ListNode_t node;
	ptrlen_t uselen;
	ptrlen_t leftlen;
} MemHeapBlock_t;

#define	memheapblock_ptr(block)	((unsigned char*)((block) + 1))
#define	ptr_memheapblock(ptr)	((MemHeapBlock_t*)(((unsigned char*)(ptr)) - sizeof(MemHeapBlock_t)))

typedef struct MemHeap_t {
	ListNode_t node;
	void* addr;
	ptrlen_t len;
	List_t blocklist;
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
	memheap->addr = addr;
	memheap->len = len;
	listInit(&memheap->blocklist);
	listPushNodeBack(&memheap->blocklist, &memheap->guard_block.node);
	memheap->guard_block.uselen = 0;
	memheap->guard_block.leftlen = len - sizeof(MemHeap_t);
	return memheap;
}

void* memheapAlloc(MemHeap_t* memheap, ptrlen_t nbytes) {
	ptrlen_t realbytes = nbytes + sizeof(MemHeapBlock_t);
	ListNode_t* cur, *prev;
	if (realbytes < nbytes) {
		return (void*)0;
	}
	for (cur = memheap->blocklist.tail; cur; cur = prev) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)cur;
		prev = cur->prev;
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

void memheapFree(MemHeap_t* memheap, void* addr) {
	MemHeapBlock_t* block, *prev_block;
	if ((ptrlen_t)addr < ((ptrlen_t)memheap) + sizeof(MemHeap_t) + sizeof(MemHeapBlock_t)) {
		return;
	}
	block = ptr_memheapblock(addr);
	prev_block = (MemHeapBlock_t*)(block->node.prev);
	prev_block->leftlen += sizeof(*block) + block->uselen + block->leftlen;
	listRemoveNode(&memheap->blocklist, &block->node);
}

#ifdef	__cplusplus
}
#endif