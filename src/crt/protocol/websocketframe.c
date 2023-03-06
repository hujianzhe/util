//
// Created by hujianzhe on 18-8-17.
//

#include "../../../inc/datastruct/base64.h"
#include "../../../inc/datastruct/sha1.h"
#include "../../../inc/datastruct/memfunc.h"
#include "../../../inc/crt/protocol/websocketframe.h"
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

char* websocketframeComputeSecAccept(const char* sec_key, unsigned int sec_keylen, char sec_accept[60]) {
	static const char WEB_SOCKET_MAGIC_KEY[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	SHA1_CTX sha1_ctx;
	unsigned char sha1_key[20];
	unsigned char* pk = (unsigned char*)malloc(sec_keylen + sizeof(WEB_SOCKET_MAGIC_KEY) - 1);
	if (!pk) {
		return NULL;
	}
	memcpy(pk, sec_key, sec_keylen);
	memcpy(pk + sec_keylen, WEB_SOCKET_MAGIC_KEY, sizeof(WEB_SOCKET_MAGIC_KEY) - 1);
	SHA1Init(&sha1_ctx);
	SHA1Update(&sha1_ctx, (unsigned char*)pk, sec_keylen + sizeof(WEB_SOCKET_MAGIC_KEY) - 1);
	SHA1Final(sha1_key, &sha1_ctx);
	free(pk);
	base64Encode(sha1_key, sizeof(sha1_key), sec_accept);
	return sec_accept;
}

int websocketframeDecodeHandshakeRequest(const char* data, unsigned int datalen, const char** sec_key, unsigned int* sec_keylen, const char** sec_protocol, unsigned int* sec_protocol_len) {
	const char *ks, *ke;
	const char* e = strStr(data, datalen, "\r\n\r\n", 4);
	if (!e) {
		return 0;
	}
	ks = strStr(data, e - data, "Sec-WebSocket-Key:", 18);
	if (!ks) {
		return -1;
	}
	for (ks += 18; ks < e && *ks <= 32; ++ks);
	if (ks >= e) {
		return -1;
	}
	ke = strChr(ks, e - ks + 1, '\r');
	if (!ke) {
		return -1;
	}
	*sec_key = ks;
	*sec_keylen = ke - ks;
	*sec_protocol = NULL;
	*sec_protocol_len = 0;
	do {
		ks = strStr(data, e - data, "Sec-WebSocket-Protocol:", 23);
		if (!ks) {
			break;
		}
		for (ks += 23; ks < e && *ks <= 32; ++ks);
		if (ks >= e) {
			break;
		}
		ke = strChr(ks, e - ks + 1, '\r');
		if (!ke) {
			break;
		}
		*sec_protocol = ks;
		*sec_protocol_len = ke - ks;
	} while (0);
	return e - data + 4;
}

char* websocketframeEncodeHandshakeResponse(const char* sec_accept, unsigned int sec_accept_len, char buf[162]) {
	buf[0] = 0;
	strcat(buf, "HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: ");
	strncat(buf, sec_accept, sec_accept_len);
	strcat(buf, "\r\n\r\n");
	return buf;
}

char* websocketframeEncodeHandshakeResponseWithProtocol(const char* sec_accept, unsigned int sec_accept_len, const char* sec_protocol, unsigned int sec_protocol_len) {
	char* buf;
	size_t buflen = 162;
	if (sec_protocol && sec_protocol_len > 0) {
		buflen += (24 + sec_protocol_len + 2); /* Sec-WebSocket-Protocol: %s\r\n */
	}
	buf = (char*)malloc(buflen);
	if (!buf) {
		return NULL;
	}
	buf[0] = 0;
	strcat(buf, "HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: ");
	strncat(buf, sec_accept, sec_accept_len);
	if (sec_protocol && sec_protocol_len > 0) {
		strcat(buf, "\r\nSec-WebSocket-Protocol: ");
		strncat(buf, sec_protocol, sec_protocol_len);
	}
	strcat(buf, "\r\n\r\n");
	return buf;
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
		payload_len = memFromBE16(*(unsigned short*)&buf[header_size]);
	}
	else if (payload_len == 127) {
		ext_payload_filed_len = 8;
		if (len < header_size + ext_payload_filed_len + mask_len)
			return 0;
		payload_len = memFromBE64(*(unsigned long long*)&data[header_size]);
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

unsigned int websocketframeEncodeHeadLength(unsigned long long datalen) {
	if (datalen < 126)
		return 2;
	else if (datalen <= 0xffff)
		return 4;
	else
		return 10;
}

void websocketframeEncode(void* headbuf, int is_fin, int prev_is_fin, int type, unsigned long long datalen) {
	unsigned char* phead = (unsigned char*)headbuf;
	if (prev_is_fin && is_fin) {
		phead[0] = type | 0x80;
	}
	else if (prev_is_fin) {
		phead[0] = type;
	}
	else if (is_fin) {
		phead[0] = WEBSOCKET_CONTINUE_FRAME | 0x80;
	}
	else {
		phead[0] = WEBSOCKET_CONTINUE_FRAME;
	}

	if (datalen < 126) {
		phead[1] = datalen;
	}
	else if (datalen <= 0xffff) {
		phead[1] = 126;
		*(unsigned short*)&phead[2] = memToBE16(datalen);
	}
	else {
		phead[1] = 127;
		*(unsigned long long*)&phead[2] = memToBE64(datalen);
	}
}

#ifdef __cplusplus
}
#endif
