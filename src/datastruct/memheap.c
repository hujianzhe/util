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
#define	PTR_VALUE_MAX			((ptrlen_t)(~0))

typedef struct MemHeap_t {
	ListNode_t node;
	void* addr;
	ptrlen_t len;
	void(*fn_lock_or_unlock)(int do_lock);
	List_t blocklist;
	MemHeapBlock_t guard_block;
} MemHeap_t;

#ifdef	__cplusplus
extern "C" {
#endif

MemHeap_t* memheapSetup(void* addr, ptrlen_t len, void(*fn_lock_or_unlock)(int)) {
	MemHeap_t* memheap;
	if (len < sizeof(MemHeap_t)) {
		return (MemHeap_t*)0;
	}
	memheap = (MemHeap_t*)addr;
	memheap->addr = addr;
	memheap->len = len;
	memheap->fn_lock_or_unlock = fn_lock_or_unlock;
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
	memheap->fn_lock_or_unlock(1);
	for (cur = memheap->blocklist.tail; cur; cur = cur->prev) {
		MemHeapBlock_t* block = (MemHeapBlock_t*)cur;
		if (block->leftlen >= realbytes) {
			MemHeapBlock_t* new_block = (MemHeapBlock_t*)(memheapblock_ptr(block) + block->uselen);
			new_block->uselen = nbytes;
			new_block->leftlen = block->leftlen - realbytes;
			listInsertNodeBack(&memheap->blocklist, cur, &new_block->node);
			block->leftlen = 0;
			memheap->fn_lock_or_unlock(0);
			return memheapblock_ptr(new_block);
		}
	}
	memheap->fn_lock_or_unlock(0);
	return (void*)0;
}

void* memheapAlignAlloc(struct MemHeap_t* memheap, ptrlen_t nbytes, ptrlen_t alignment) {
	ListNode_t* cur;
	if (PTR_VALUE_MAX - sizeof(MemHeapBlock_t) < nbytes) {
		return (void*)0;
	}
	alignment -= 1;
	memheap->fn_lock_or_unlock(1);
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
			memheap->fn_lock_or_unlock(0);
			return (void*)new_useptr;
		}
	}
	memheap->fn_lock_or_unlock(0);
	return (void*)0;
}

void memheapFree(MemHeap_t* memheap, void* addr) {
	if (addr) {
		MemHeapBlock_t* block = ptr_memheapblock(addr), *prev_block;
		memheap->fn_lock_or_unlock(1);
		prev_block = (MemHeapBlock_t*)(block->node.prev);
		prev_block->leftlen += sizeof(MemHeapBlock_t) + block->uselen + block->leftlen;
		listRemoveNode(&memheap->blocklist, &block->node);
		memheap->fn_lock_or_unlock(0);
	}
}

#ifdef	__cplusplus
}
#endif