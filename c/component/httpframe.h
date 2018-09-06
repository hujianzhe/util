//
// Created by hujianzhe on 18-8-18.
//

#ifndef	UTIL_C_COMPONENT_HTTPFRAME_H
#define	UTIL_C_COMPONENT_HTTPFRAME_H

#include "../datastruct/hashtable.h"

typedef struct HttpFrameHeaderField_t {
	HashtableNode_t m_hashnode;
	const char* key;
	const char* value;
} HttpFrameHeaderField_t;

typedef struct HttpFrame_t {
	int status_code;
	char method[8];
	char* uri;
	const char* query;
	Hashtable_t headers;
	HashtableNode_t* m_bulks[11];
} HttpFrame_t;

#ifdef __cplusplus
extern "C" {
#endif

UTIL_LIBAPI const char* httpframeStatusDesc(int status_code);
UTIL_LIBAPI int httpframeDecode(HttpFrame_t* frame, char* buf, unsigned int len);
UTIL_LIBAPI int httpframeDecodeChunked(char* buf, unsigned int len, unsigned char** data, unsigned int* datalen);
UTIL_LIBAPI void httpframeEncodeChunked(unsigned int datalen, char txtbuf[11]);
UTIL_LIBAPI const char* httpframeGetHeader(HttpFrame_t* frame, const char* key);
UTIL_LIBAPI void httpframeFree(HttpFrame_t* frame);

#ifdef __cplusplus
}
#endif

#endif
