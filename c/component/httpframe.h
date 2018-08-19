//
// Created by hujianzhe on 18-8-18.
//

#ifndef	UTIL_C_COMPONENT_HTTPFRAME_H
#define	UTIL_C_COMPONENT_HTTPFRAME_H

#include "../datastruct/hashtable.h"

typedef struct HttpFrameHeaderField_t {
	const char* key;
	const char* value;
	hashtable_node_t m_hashnode;
} HttpFrameHeaderField_t;

typedef struct HttpFrame_t {
	int status_code;
	char method[8];
	char* url;
	hashtable_t headers;
	hashtable_node_t* m_bulks[11];
} HttpFrame_t;

#ifdef __cplusplus
extern "C" {
#endif

const char* httpframeStatusDesc(int status_code);
int httpframeDecode(HttpFrame_t* frame, char* buf, unsigned int len);
const char* httpframeGetHeader(HttpFrame_t* frame, const char* key);
void httpframeFree(HttpFrame_t* frame);

#ifdef __cplusplus
}
#endif

#endif
