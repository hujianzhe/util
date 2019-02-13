//
// Created by hujianzhe on 18-8-17.
//

#ifndef	UTIL_C_COMPONENT_WEBSOCKETFRAME_H
#define	UTIL_C_COMPONENT_WEBSOCKETFRAME_H

#include "../compiler_define.h"

enum {
	WEBSOCKET_CONTINUE_FRAME	= 0,
	WEBSOCKET_TEXT_FRAME		= 1,
	WEBSOCKET_BINARY_FRAME		= 2,
	WEBSOCKET_CLOSE_FRAME		= 8,
	WEBSOCKET_PING_FRAME		= 9,
	WEBSOCKET_PONG_FRAME		= 10
};

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll int websocketframeDecodeHandshake(char* data, unsigned int datalen, char** key, unsigned int* keylen);
__declspec_dll int websocketframeEncodeHandshake(const char* key, unsigned int keylen, char txtbuf[162]);
__declspec_dll int websocketframeDecode(unsigned char* buf, unsigned long long len,
		unsigned char** data, unsigned long long* datalen, int* is_fin, int* type);
__declspec_dll unsigned int websocketframeHeadLength(unsigned long long datalen);
__declspec_dll void websocketframeEncode(void* headbuf, int is_fin, int type, unsigned long long datalen);

#ifdef __cplusplus
}
#endif

#endif
