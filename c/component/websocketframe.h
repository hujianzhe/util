//
// Created by hujianzhe on 18-8-17.
//

#ifndef	UTIL_C_COMPONENT_WEBSOCKETFRAME_H
#define	UTIL_C_COMPONENT_WEBSOCKETFRAME_H

#ifdef __cplusplus
extern "C" {
#endif

int websocketframeDecodeHandshake(char* data, unsigned int datalen, char** key, unsigned int* keylen);
int websocketframeEncodeHandshake(const char* key, unsigned int keylen, char* response);
int websocketframeDecode(unsigned char* buf, unsigned long long len,
		unsigned char** data, unsigned long long* datalen, int* is_fin, int* type);
unsigned int websocketframeHeadLength(unsigned long long datalen);
void websocketframeEncode(void* headbuf, int is_fin, int type, unsigned long long datalen);

#ifdef __cplusplus
}
#endif

#endif
