//
// Created by hujianzhe
//

#include "../../inc/sysapi/misc.h"
#include <string.h>

#ifdef	__cplusplus
extern "C" {
#endif

void* alignMalloc(size_t nbytes, size_t alignment) {
#if defined(_WIN32) || defined(_WIN64)
	return alignment ? _aligned_malloc(nbytes, alignment) : NULL;
#elif	defined(__linux__)
	return alignment ? memalign(alignment, nbytes) : NULL;
#else
	if (alignment <= 0) {
		return NULL;
	}
	size_t padsize = alignment > sizeof(size_t) ? alignment : sizeof(size_t);
	size_t ptr = (size_t)malloc(nbytes + padsize);
	if (NULL == (void*)ptr) {
		return NULL;
	}
	size_t new_ptr = (ptr + sizeof(size_t) + alignment - 1) & ~(alignment - 1);
	*(((size_t*)new_ptr) - 1) = new_ptr - ptr;
	return (void*)new_ptr;
#endif
}

void alignFree(const void* ptr) {
#if defined(_WIN32) || defined(_WIN64)
	_aligned_free((void*)ptr);
#elif	defined(__linux__)
	free((void*)ptr);
#else
	if (ptr) {
		size_t off = *(((size_t*)ptr) - 1);
		free((char*)ptr - off);
	}
#endif
}

#ifdef	__cplusplus
}
#endif
