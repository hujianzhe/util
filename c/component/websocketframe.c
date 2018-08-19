//
// Created by hujianzhe on 18-8-17.
//

#include "../syslib/alloca.h"
#include "../syslib/encrypt.h"
#include "../syslib/socket.h"
#include "../syslib/string.h"
#include "websocketframe.h"

#ifdef __cplusplus
extern "C" {
#endif

int websocketframeDecodeHandshake(char* data, unsigned int datalen, char** key, unsigned int* keylen) {
	char *ks, *ke;
	const char* e = strnstr(data, "\r\n\r\n", datalen);
	if (!e)
		return 0;

	ks = strnstr(data, "Sec-WebSocket-Key:", e - data);
	if (!ks)
		return -1;
	for (ks += 18; *ks <= 32; ++ks);
	ke = strchr(ks, '\r');
	if (!ke)
		return -1;
	*key = ks;
	*keylen = ke - ks;

	return e - data + 4;
}

int websocketframeEncodeHandshake(const char* key, unsigned int keylen, char response[162]) {
	static const char WEB_SOCKET_MAGIC_KEY[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char sha1_key[CC_SHA1_DIGEST_LENGTH];
	char base64_key[CC_SHA1_DIGEST_LENGTH * 3];
	char* pk = (char*)alloca(keylen + sizeof(WEB_SOCKET_MAGIC_KEY));
	memcpy(pk, key, keylen);
	memcpy(pk + keylen, WEB_SOCKET_MAGIC_KEY, sizeof(WEB_SOCKET_MAGIC_KEY));
	if (!cryptSHA1Encode(pk, strlen(pk), sha1_key))
		return 0;
	if (!cryptBase64Encode(sha1_key, sizeof(sha1_key), base64_key))
		return 0;
	response[0] = 0;
	strcat(response, "HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: ");
	strcat(response, base64_key);
	strcat(response, "\r\n\r\n");
	return 1;
}

int websocketframeDecode(unsigned char* buf, unsigned long long len,
		unsigned char** data, unsigned long long* datalen, int* is_fin, int* type)
{
	unsigned char frame_is_fin, frame_type;
	unsigned int mask_len = 0, ext_payload_filed_len = 0;
	unsigned char* payload_data = NULL;
	unsigned long long payload_len;
	const unsigned int header_size = 2;

	if (len < header_size)
		return 0;

	frame_is_fin = buf[0] >> 7;
	frame_type = buf[0] & 0x0f;
	if (buf[1] >> 7)
		mask_len = 4;

	payload_len = buf[1] & 0x7f;
	if (payload_len < 126) {
		if (len < header_size + ext_payload_filed_len + mask_len)
			return 0;
	}
	else if (payload_len == 126) {
		ext_payload_filed_len = 2;
		if (len < header_size + ext_payload_filed_len + mask_len)
			return 0;
		payload_len = ntohs(*(unsigned short*)&buf[header_size]);
	}
	else if (payload_len == 127) {
		ext_payload_filed_len = 8;
		if (len < header_size + ext_payload_filed_len + mask_len)
			return 0;
		payload_len = ntohll(*(unsigned long long*)&data[header_size]);
	}
	else
		return -1;

	if (len < header_size + ext_payload_filed_len + mask_len + payload_len)
		return 0;

	payload_data = &buf[header_size + ext_payload_filed_len + mask_len];
	if (mask_len) {
		unsigned long long i;
		unsigned char* mask = &buf[header_size + ext_payload_filed_len];
		for (i = 0; i < payload_len; ++i)
			payload_data[i] ^= mask[i % 4];
	}

	*is_fin = frame_is_fin;
	*type = frame_type;
	*datalen = payload_len;
	*data = payload_len ? payload_data : NULL;
	return header_size + ext_payload_filed_len + mask_len + payload_len;
}

unsigned int websocketframeHeadLength(unsigned long long datalen) {
	if (datalen < 126)
		return 2;
	else if (datalen <= 0xffff)
		return 4;
	else
		return 10;
}

void websocketframeEncode(void* headbuf, int is_fin, int type, unsigned long long datalen) {
	unsigned char* phead = (unsigned char*)headbuf;
	phead[0] = type | (is_fin ? 0x80 : 0);
	if (datalen < 126)
		phead[1] = datalen;
	else if (datalen <= 0xffff) {
		phead[1] = 126;
		*(unsigned short*)&phead[2] = htons(datalen);
	}
	else {
		phead[1] = 127;
		*(unsigned long long*)&phead[2] = htonll(datalen);
	}
}

#ifdef __cplusplus
}
#endif
