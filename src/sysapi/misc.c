//
// Created by hujianzhe
//

#include "../../inc/sysapi/misc.h"
#include <stdarg.h>
#include <stdio.h>
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

char* strFormat(const char* format, ...) {
	char test_buf;
	char* buf;
	int len;
	va_list varg;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len < 0)
		return NULL;
	buf = (char*)malloc(len + 1);
	if (!buf)
		return NULL;
	va_start(varg, format);
	len = vsnprintf(buf, len + 1, format, varg);
	va_end(varg);
	if (len < 0) {
		free(buf);
		return NULL;
	}
	buf[len] = 0;
	return buf;
}

unsigned int iobufSharedCopy(const Iobuf_t* iov, unsigned int iovcnt, unsigned int* iov_i, unsigned int* iov_off, void* buf, unsigned int n) {
	unsigned int off = 0;
	unsigned char* ptr_buf = (unsigned char*)buf;
	while (*iov_i < iovcnt) {
		unsigned int leftsize = n - off;
		char* iovptr = ((char*)iobufPtr(iov + *iov_i)) + *iov_off;
		unsigned int iovleftsize = iobufLen(iov + *iov_i) - *iov_off;
		if (iovleftsize > leftsize) {
			memcpy(ptr_buf + off, iovptr, leftsize);
			*iov_off += leftsize;
			off += leftsize;
			break;
		}
		else {
			memcpy(ptr_buf + off, iovptr, iovleftsize);
			*iov_off = 0;
			(*iov_i)++;
			off += iovleftsize;
		}
	}
	return off;
}

#ifdef	__cplusplus
}
#endif