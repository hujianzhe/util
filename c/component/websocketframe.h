//
// Created by hujianzhe on 18-8-17.
//

#ifndef	UTIL_C_COMPONENT_WEBSOCKETFRAME_H
#define	UTIL_C_COMPONENT_WEBSOCKETFRAME_H

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

int websocketframeDecodeHandshake(char* data, unsigned int datalen, char** key, unsigned int* keylen);
int websocketframeEncodeHandshake(const char* key, unsigned int keylen, char txtbuf[162]);
int websocketframeDecode(unsigned char* buf, unsigned long long len,
		unsigned char** data, unsigned long long* datalen, int* is_fin, int* type);
unsigned int websocketframeHeadLength(unsigned long long datalen);
void websocketframeEncode(void* headbuf, int is_fin, int type, unsigned long long datalen);

#ifdef __cplusplus
}
#endif

#endif
