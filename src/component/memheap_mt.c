//
// Created by hujianzhe
//

#include "../../inc/component/memheap_mt.h"
#include "../../inc/datastruct/memheap.h"
#include <stdlib.h>
#include <string.h>

typedef struct MemHeapMt_t {
	Semaphore_t seminit;
	Semaphore_t semlock;
	ShareMemMap_t mm;
	struct MemHeap_t* layout;
	short is_open;
	size_t namelen;
	char name_ext[1];
} MemHeapMt_t;

#ifdef	__cplusplus
extern "C" {
#endif

MemHeapMt_t* memheapmtCreate(size_t len, const char* name) {
	int sem_init_ok = 0, sem_lock_ok = 0, mm_ok = 0, mm_addr_ok = 0;
	size_t namelen = strlen(name);
	void* mm_addr;
	MemHeapMt_t* memheap = (MemHeapMt_t*)malloc(sizeof(MemHeapMt_t) + namelen + 5);
	if (!memheap) {
		return NULL;
	}
	strcpy(memheap->name_ext, name);
	if (!semaphoreCreate(&memheap->seminit, strcat(memheap->name_ext, "init"), 1)) {
		goto err;
	}
	sem_init_ok = 1;

	semaphoreWait(&memheap->seminit);
	memheap->name_ext[namelen] = 0;
	if (!semaphoreCreate(&memheap->semlock, strcat(memheap->name_ext, "lock"), 1)) {
		goto err;
	}
	sem_lock_ok = 1;
	memheap->name_ext[namelen] = 0;
	if (!memoryCreateMapping(&memheap->mm, strcat(memheap->name_ext, "mem"), len)) {
		goto err;
	}
	mm_ok = 1;
	if (!memoryDoMapping(&memheap->mm, NULL, &mm_addr)) {
		goto err;
	}
	mm_addr_ok = 1;
	memheap->layout = memheapSetup(mm_addr, len);
	if (!memheap->layout) {
		goto err;
	}
	semaphorePost(&memheap->seminit);

	memheap->namelen = namelen;
	memheap->is_open = 0;
	return memheap;
err:
	if (sem_init_ok) {
		semaphorePost(&memheap->seminit);
		semaphoreClose(&memheap->seminit);
		semaphoreUnlink(strcat(strcpy(memheap->name_ext, name), "init"));
	}
	if (sem_lock_ok) {
		semaphoreClose(&memheap->semlock);
		semaphoreUnlink(strcat(strcpy(memheap->name_ext, name), "lock"));
	}
	if (mm_ok) {
		if (mm_addr_ok) {
			memoryUndoMapping(&memheap->mm);
		}
		memoryCloseMapping(&memheap->mm);
	}
	free(memheap);
	return NULL;
}

MemHeapMt_t* memheapmtOpen(const char* name) {
	int sem_init_ok = 0, sem_lock_ok = 0, mm_ok = 0, mm_addr_ok = 0;
	void* mm_addr;
	Semaphore_t seminit;
	size_t namelen = strlen(name);
	MemHeapMt_t* memheap = (MemHeapMt_t*)malloc(sizeof(MemHeapMt_t) + namelen + 5);
	if (!memheap) {
		return NULL;
	}
	strcpy(memheap->name_ext, name);
	if (!semaphoreOpen(&seminit, strcat(memheap->name_ext, "init"))) {
		goto err;
	}
	sem_init_ok = 1;

	semaphoreWait(&seminit);
	memheap->name_ext[namelen] = 0;
	if (!semaphoreOpen(&memheap->semlock, strcat(memheap->name_ext, "lock"))) {
		goto err;
	}
	sem_lock_ok = 1;
	memheap->name_ext[namelen] = 0;
	if (!memoryOpenMapping(&memheap->mm, strcat(memheap->name_ext, "mem"))) {
		goto err;
	}
	mm_ok = 1;
	if (!memoryDoMapping(&memheap->mm, NULL, &mm_addr)) {
		goto err;
	}
	mm_addr_ok = 1;
	memheap->layout = memheapSetupAddr(mm_addr);
	if (!memheap->layout) {
		goto err;
	}
	semaphorePost(&seminit);
	semaphoreClose(&seminit);

	memheap->namelen = namelen;
	memheap->is_open = 1;
	return memheap;
err:
	if (sem_init_ok) {
		semaphorePost(&seminit);
		semaphoreClose(&seminit);
	}
	if (sem_lock_ok) {
		semaphoreClose(&memheap->semlock);
	}
	if (mm_ok) {
		if (mm_addr_ok) {
			memoryUndoMapping(&memheap->mm);
		}
		memoryCloseMapping(&memheap->mm);
	}
	free(memheap);
	return NULL;
}

size_t memheapmtSetupUsableRange(struct MemHeapMt_t* memheap, void** out_buf, size_t alignment) {
	return memheapSetupUsableRange(memheap->layout, out_buf, alignment);
}

void* memheapmtAlloc(MemHeapMt_t* memheap, size_t nbytes) {
	void* addr;
	semaphoreWait(&memheap->semlock);
	addr = memheapAlloc(memheap->layout, nbytes);
	semaphorePost(&memheap->semlock);
	return addr;
}

void* memheapmtAlignAlloc(struct MemHeapMt_t* memheap, size_t nbytes, size_t alignment) {
	void* addr;
	if (0 == alignment || (alignment & (alignment - 1))) {
		return NULL;
	}
	semaphoreWait(&memheap->semlock);
	addr = memheapAlignAlloc(memheap->layout, nbytes, alignment);
	semaphorePost(&memheap->semlock);
	return addr;
}

void* memheapmtTryResize(struct MemHeapMt_t* memheap, void* addr, size_t nbytes) {
	semaphoreWait(&memheap->semlock);
	addr = memheapTryResize(memheap->layout, addr, nbytes);
	semaphorePost(&memheap->semlock);
	return addr;
}

void memheapmtFree(MemHeapMt_t* memheap, void* addr) {
	if (addr) {
		semaphoreWait(&memheap->semlock);
		memheapFree(memheap->layout, addr);
		semaphorePost(&memheap->semlock);
	}
}

void memheapmtFreeAll(struct MemHeapMt_t* memheap) {
	semaphoreWait(&memheap->semlock);
	memheapFreeAll(memheap->layout);
	semaphorePost(&memheap->semlock);
}

void memheapmtClose(MemHeapMt_t* memheap) {
	if (memheap) {
		memoryUndoMapping(&memheap->mm);
		memoryCloseMapping(&memheap->mm);
		semaphoreClose(&memheap->semlock);
		if (!memheap->is_open) {
			semaphoreClose(&memheap->seminit);
			memheap->name_ext[memheap->namelen] = 0;
			semaphoreUnlink(strcat(memheap->name_ext, "init"));

			memheap->name_ext[memheap->namelen] = 0;
			semaphoreUnlink(strcat(memheap->name_ext, "lock"));
		}
		free(memheap);
	}
}

#ifdef	__cplusplus
}
#endif
