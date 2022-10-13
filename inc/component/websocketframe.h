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

#define	WEBSOCKET_MAX_ENCODE_HEADLENGTH	10

#define	WEBSOCKET_SIMPLE_HTTP_HANDSHAKE_REQUEST_FMT	\
"GET %s HTTP/1.1\r\n"	\
"Upgrade: websocket\r\n"	\
"Connection: Upgrade\r\n"	\
"Sec-WebSocket-Version: 13\r\n"	\
"Sec-WebSocket-Key: %s\r\n"	\
"\r\n"	\

#define	WEBSOCKET_SIMPLE_HTTP_HANDSHAKE_REQUEST_WITH_PROTOCOL_FMT	\
"GET %s HTTP/1.1\r\n"	\
"Upgrade: websocket\r\n"	\
"Connection: Upgrade\r\n"	\
"Sec-WebSocket-Version: 13\r\n"	\
"Sec-WebSocket-Key: %s\r\n"	\
"Sec-WebSocket-Protocol: %s\r\n"	\
"\r\n"	\

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll int websocketframeDecodeHandshakeRequest(const char* data, unsigned int datalen,
		const char** key, unsigned int* keylen, const char** sec_protocol, unsigned int* sec_protocol_len);
__declspec_dll char* websocketframeEncodeHandshakeResponse(const char* key, unsigned int keylen, char txtbuf[162]);
__declspec_dll char* websocketframeEncodeHandshakeResponseWithProtocol(const char* key, unsigned int keylen, const char* sec_protocol, unsigned int sec_protocol_len);
__declspec_dll int websocketframeDecode(unsigned char* buf, unsigned long long len,
		unsigned char** data, unsigned long long* datalen, int* is_fin, int* type);
__declspec_dll unsigned int websocketframeEncodeHeadLength(unsigned long long datalen);
__declspec_dll void websocketframeEncode(void* headbuf, int is_fin, int prev_is_fin, int type, unsigned long long datalen);

#ifdef __cplusplus
}
#endif

#endif
