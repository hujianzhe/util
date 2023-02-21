//
// Created by hujianzhe
//

#include "../../inc/component/memheap_mt.h"
#include "../../inc/datastruct/memheap.h"
#include <stdlib.h>
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

MemHeapMt_t* memheapmtCreate(MemHeapMt_t* memheap, size_t len, const char* name) {
	int ok;
	size_t namelen = strlen(name);
	char* name_ext = (char*)malloc(namelen + 5);
	memheap->initok = 0;
	if (!name_ext)
		return NULL;
	strcpy(name_ext, name);
	strcat(name_ext, "init");
	if (!semaphoreCreate(&memheap->seminit, name_ext, 1)) {
		free(name_ext);
		return NULL;
	}
	semaphoreWait(&memheap->seminit);
	do {
		void* addr;
		ok = 0;
		name_ext[namelen] = 0;
		if (!semaphoreCreate(&memheap->semlock, strcat(name_ext, "lock"), 1))
			break;
		ok = 1;
		name_ext[namelen] = 0;
		if (!memoryCreateMapping(&memheap->mm, strcat(name_ext, "mem"), len))
			break;
		ok = 2;
		if (!memoryDoMapping(&memheap->mm, NULL, 0, len, &addr))
			break;
		memheap->ptr = (struct MemHeap_t*)addr;
		if (!memheap->ptr)
			break;
		shmheapSetup(memheap->ptr, len);
		ok = 3;
	} while (0);
	semaphorePost(&memheap->seminit);
	if (ok < 3) {
		semaphoreClose(&memheap->seminit);
		semaphoreUnlink(strcat(strcpy(name_ext, name), "init"));
		if (ok > 0) {
			semaphoreClose(&memheap->semlock);
			semaphoreUnlink(strcat(strcpy(name_ext, name), "lock"));
		}
		if (ok > 1) {
			memoryCloseMapping(&memheap->mm);
		}
		free(name_ext);
		return NULL;
	}
	memheap->len = len;
	memheap->name_ext = name_ext;
	memheap->namelen = namelen;
	memheap->initok = 1;
	memheap->is_open = 0;
	return memheap;
}

MemHeapMt_t* memheapmtOpen(MemHeapMt_t* memheap, size_t len, const char* name) {
	int ok;
	size_t namelen = strlen(name);
	char* name_ext = (char*)malloc(namelen + 5);
	memheap->initok = 0;
	if (!name_ext)
		return NULL;
	strcpy(name_ext, name);
	strcat(name_ext, "init");
	if (!semaphoreOpen(&memheap->seminit, name_ext)) {
		free(name_ext);
		return NULL;
	}
	semaphoreWait(&memheap->seminit);
	do {
		void* addr;
		ok = 0;
		name_ext[namelen] = 0;
		if (!semaphoreOpen(&memheap->semlock, strcat(name_ext, "lock")))
			break;
		ok = 1;
		name_ext[namelen] = 0;
		if (!memoryOpenMapping(&memheap->mm, strcat(name_ext, "mem")))
			break;
		ok = 2;
		if (!memoryDoMapping(&memheap->mm, NULL, 0, len, &addr))
			break;
		memheap->ptr = (struct MemHeap_t*)addr;
		if (!memheap->ptr)
			break;
		ok = 3;
	} while (0);
	semaphorePost(&memheap->seminit);
	semaphoreClose(&memheap->seminit);
	if (ok < 3) {
		free(name_ext);
		if (ok > 0)
			semaphoreClose(&memheap->semlock);
		if (ok > 1)
			memoryCloseMapping(&memheap->mm);
		return NULL;
	}
	memheap->len = len;
	memheap->name_ext = name_ext;
	memheap->namelen = namelen;
	memheap->initok = 1;
	memheap->is_open = 1;
	return memheap;
}

void* memheapmtAlloc(MemHeapMt_t* memheap, size_t nbytes) {
	void* addr;
	semaphoreWait(&memheap->semlock);
	addr = shmheapAlloc(memheap->ptr, nbytes);
	semaphorePost(&memheap->semlock);
	return addr;
}

void memheapmtFree(MemHeapMt_t* memheap, void* addr) {
	if (addr) {
		semaphoreWait(&memheap->semlock);
		shmheapFree(addr);
		semaphorePost(&memheap->semlock);
	}
}

void memheapmtClose(MemHeapMt_t* memheap) {
	if (memheap->initok) {
		memheap->initok = 0;
		memoryUndoMapping(&memheap->mm, memheap->ptr, memheap->len);
		memoryCloseMapping(&memheap->mm);
		semaphoreClose(&memheap->semlock);
		if (!memheap->is_open) {
			semaphoreClose(&memheap->seminit);

			memheap->name_ext[memheap->namelen] = 0;
			semaphoreUnlink(strcat(memheap->name_ext, "init"));
			memheap->name_ext[memheap->namelen] = 0;
			semaphoreUnlink(strcat(memheap->name_ext, "lock"));
		}
		free(memheap->name_ext);
	}
}

#ifdef	__cplusplus
}
#endif
