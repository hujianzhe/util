//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_DYNBUF_H
#define	UTIL_C_CRT_DYNBUF_H

#include "../compiler_define.h"
#include <stddef.h>

typedef struct DynBuf_t {
	size_t sizeof_type;
	size_t size;
	size_t capcity;
	char* buf;
} DynBuf_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll DynBuf_t* dynbufInitSizeOfType(DynBuf_t* dynbuf, size_t sizeof_type);
__declspec_dll DynBuf_t* dynbufSetCapcity(DynBuf_t* dynbuf, size_t capcity);
__declspec_dll DynBuf_t* dynbufSetSize(DynBuf_t* dynbuf, size_t size);
__declspec_dll DynBuf_t* dynbufClear(DynBuf_t* dynbuf);
__declspec_dll DynBuf_t* dynbufInsert(DynBuf_t* dynbuf, size_t offset, const void* data, size_t datalen);
__declspec_dll DynBuf_t* dynbufRemove(DynBuf_t* dynbuf, size_t start, size_t end);
__declspec_dll DynBuf_t* dynbufCopy(DynBuf_t* dynbuf, size_t offset, const void* data, size_t datalen);

#ifdef __cplusplus
}
#endif

#endif // !UTIL_C_CRT_DYNBUF_H
