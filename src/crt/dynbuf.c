//
// Created by hujianzhe
//

#include "../../inc/crt/dynbuf.h"
#include "../../inc/datastruct/memfunc.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

DynBuf_t* dynbufInitSizeOfType(DynBuf_t* dynbuf, size_t sizeof_type) {
	dynbuf->sizeof_type = sizeof_type;
	dynbuf->size = 0;
	dynbuf->capcity = 0;
	dynbuf->buf = NULL;
	return dynbuf;
}

DynBuf_t* dynbufSetCapcity(DynBuf_t* dynbuf, size_t capcity) {
	if (capcity) {
		char* buf = (char*)realloc(dynbuf->buf, capcity);
		if (!buf)
			return NULL;
		dynbuf->buf = buf;
		dynbuf->capcity = capcity;
		if (dynbuf->size > capcity)
			dynbuf->size = capcity;
	}
	else {
		dynbufClear(dynbuf);
	}
	return dynbuf;
}

DynBuf_t* dynbufSetSize(DynBuf_t* dynbuf, size_t size) {
	if (dynbuf->size == size)
		return dynbuf;
	else if (size > dynbuf->capcity)
		dynbufSetCapcity(dynbuf, size);
	dynbuf->size = size;
	return dynbuf;
}

DynBuf_t* dynbufClear(DynBuf_t* dynbuf) {
	free(dynbuf->buf);
	return dynbufInitSizeOfType(dynbuf, dynbuf->sizeof_type);
}

DynBuf_t* dynbufInsert(DynBuf_t* dynbuf, size_t offset, const void* data, size_t datalen) {
	size_t old_size, total_size;
	if (0 == datalen)
		return dynbuf;
	old_size = dynbuf->size;
	if (offset >= old_size)
		total_size = offset + datalen;
	else
		total_size = old_size + datalen;
	if (dynbuf->capcity < total_size) {
		char* buf = (char*)realloc(dynbuf->buf, total_size);
		if (!buf)
			return NULL;
		dynbuf->buf = buf;
		dynbuf->capcity = total_size;
	}
	dynbuf->size = total_size;
	if (offset < old_size)
		memCopy(dynbuf->buf + offset + datalen, dynbuf->buf + offset, old_size - offset);
	memCopy(dynbuf->buf + offset, data, datalen);
	return dynbuf;
}

DynBuf_t* dynbufRemove(DynBuf_t* dynbuf, size_t start, size_t end) {
	if (start >= end || start >= dynbuf->size)
		return dynbuf;
	if (end > dynbuf->size)
		end = dynbuf->size;
	memCopy(dynbuf->buf + start, dynbuf->buf + end, dynbuf->size - end);
	dynbuf->size -= end - start;
	return dynbuf;
}

DynBuf_t* dynbufCopy(DynBuf_t* dynbuf, size_t offset, const void* data, size_t datalen) {
	if (!datalen || !data)
		return dynbuf;
	else if (dynbuf->capcity < offset + datalen && !dynbufSetCapcity(dynbuf, offset + datalen))
		return NULL;
	memCopy(dynbuf->buf + offset, data, datalen);
	if (offset + datalen > dynbuf->size)
		dynbuf->size = offset + datalen;
	return dynbuf;
}

int dynbufPrintf(DynBuf_t* dynbuf, size_t offset, const char* format, ...) {
	size_t old_size;
	char test_buf;
	int len;
	va_list varg;
	va_start(varg, format);
	len = vsnprintf(&test_buf, 0, format, varg);
	va_end(varg);
	if (len < 0)
		return len;
	old_size = dynbuf->size;
	if (offset + len + 1 > dynbuf->size) {
		dynbufSetSize(dynbuf, offset + len + 1);
	}
	va_start(varg, format);
	len = vsnprintf(dynbuf->buf + offset, len + 1, format, varg);
	va_end(varg);
	if (len < 0) {
		dynbuf->size = old_size;
		return len;
	}
	dynbuf->buf[offset + len] = 0;
	return len;
}

#ifdef __cplusplus
}
#endif