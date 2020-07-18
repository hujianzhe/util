//
// Created by hujianzhe on 18-8-18.
//

#ifndef	UTIL_C_COMPONENT_HTTPFRAME_H
#define	UTIL_C_COMPONENT_HTTPFRAME_H

#include "../compiler_define.h"
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
	char* query;
	unsigned int pathlen;
	Hashtable_t headers;
	HashtableNode_t* m_bulks[11];
} HttpFrame_t;

#define	HTTP_SIMPLE_RESP_FMT \
"HTTP/1.1 %u %s\r\n"\
"Access-Control-Allow-Origin: *\r\n"\
"Content-Length:%u\r\n"\
"\r\n"\
"%s"\

#define	HTTP_SIMPLE_RESP_VALUE(status_code, strdata, strdatalen) \
status_code, httpframeStatusDesc(status_code), strdatalen, strdata

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll const char* httpframeStatusDesc(int status_code);
__declspec_dll int httpframeDecodeHeader(HttpFrame_t* frame, char* buf, unsigned int len);
__declspec_dll int httpframeDecodeChunked(char* buf, unsigned int len, unsigned char** data, unsigned int* datalen);
__declspec_dll void httpframeEncodeChunked(unsigned int datalen, char txtbuf[11]);
__declspec_dll const char* httpframeGetHeader(HttpFrame_t* frame, const char* key);
__declspec_dll HttpFrame_t* httpframeReset(HttpFrame_t* frame);

#ifdef __cplusplus
}
#endif

#endif
