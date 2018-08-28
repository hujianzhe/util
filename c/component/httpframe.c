//
// Created by hujianzhe on 18-8-18.
//

#include "../syslib/string.h"
#include "httpframe.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static int header_keycmp(struct HashtableNode_t* node, const void* key) {
	const char* sk = *(const char**)key;
	return strcmp(pod_container_of(node, HttpFrameHeaderField_t, m_hashnode)->key, sk);
}

static unsigned int header_keyhash(const void* key) {
	const char* sk = *(const char**)key;
	return strhash_bkdr(sk);
}

const char* httpframeGetHeader(HttpFrame_t* frame, const char* key) {
	HashtableNode_t* node = hashtableSearchKey(&frame->headers, &key);
	if (node)
		return pod_container_of(node, HttpFrameHeaderField_t, m_hashnode)->value;
	else
		return NULL;
}

const char* httpframeStatusDesc(int status_code) {
	switch (status_code) {
		case 100:
			return "Continue";
		case 101:
			return "Switching Protocols";
		case 102:
			return "Processing";
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 202:
			return "Accepted";
		case 203:
			return "Non-Authoritative Information";
		case 204:
			return "No Content";
		case 205:
			return "Reset Content";
		case 206:
			return "Partial Content";
		case 207:
			return "Multi-Status";
		case 300:
			return "Multiple Choices";
		case 301:
			return "Moved Permanently";
		case 302:
			return "Found";
		case 303:
			return "See Other";
		case 304:
			return "Not Modified";
		case 305:
			return "Use Proxy";
		case 306:
			return "Switch Proxy";
		case 307:
			return "Temporary Redirect";
		case 400:
			return "Bad Request";
		case 401:
			return "Unauthorized";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 406:
			return "Not Acceptable";
		case 407:
			return "Proxy Authentication Required";
		case 408:
			return "Request Timeout";
		case 409:
			return "Conflict";
		case 410:
			return "Gone";
		case 411:
			return "Length Required";
		case 412:
			return "Precondition Failed";
		case 413:
			return "Request Entity Too Large";
		case 414:
			return "Request URI Too Long";
		case 415:
			return "Unsupported media type";
		case 416:
			return "Requested Range Not Satisfiable";
		case 417:
			return "Expectation Failed";
		case 422:
			return "Unprocessable Entity";
		case 423:
			return "Locked";
		case 424:
			return "Failed Dependency";
		case 425:
			return "Unordered Collection";
		case 426:
			return "Upgrade Required";
		case 449:
			return "Retry With";
		case 451:
			return "Unavailable For Legal Reasons";
		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 502:
			return "Bad Gateway";
		case 503:
			return "Service Unavailable";
		case 504:
			return "Gateway Timeout";
		case 505:
			return "HTTP Version Not Supported";
		case 506:
			return "Variant Also Negotiates";
		case 507:
			return "Insufficient Storage";
		case 509:
			return "Bandwidth Limit Exceeded";
		case 510:
			return "Not Extended";
		case 600:
			return "Unparseable Response Headers";
		default:
			return "";
	}
}

int httpframeDecode(HttpFrame_t* frame, char* buf, unsigned int len) {
	const char *h, *e;

	frame->status_code = 0;
	frame->method[0] = 0;
	frame->query = frame->uri = NULL;
	hashtableInit(&frame->headers, frame->m_bulks,
			sizeof(frame->m_bulks) / sizeof(frame->m_bulks[0]),
			header_keycmp, header_keyhash);

	e = strnstr(buf, "\r\n\r\n", len);
	if (!e)
		return 0;

	if (strncmp(buf, "HTTP", 4)) {
		const char *s, *e;
		for (s = e = buf; '\r' != *e; ++e) {
			if (' ' != *e)
				continue;
			if ('\0' == frame->method[0]) {
				if (e - s >= sizeof(frame->method))
					return -1;
				strncpy(frame->method, s, e - s);
			}
			else {
				unsigned int i, query_pos = -1;
				frame->uri = (char*)malloc(e - s + 1);
				if (!frame->uri)
					return -1;
				for (i = 0; i < e - s; ++i) {
					frame->uri[i] = s[i];
					if (-1 == query_pos && '?' == s[i])
						query_pos = i + 1;
				}
				frame->uri[e - s] = 0;
				if (query_pos != -1)
					frame->query = frame->uri + query_pos;
				else
					frame->query = frame->uri + (e - s);
			}
			s = e + 1;
		}
		h = e + 2;
	}
	else {
		const char *s;
		for (s = buf; ' ' != *s && '\r' != *s; ++s);
		if ('\r' == *s)
			return -1;
		for (++s; ' ' != *s; ++s) {
			if (!isdigit(*s))
				return -1;
			frame->status_code *= 10;
			frame->status_code += *s - '0';
		}
		while ('\r' != *s)
			++s;
		h = s + 2;
	}

	while (!('\r' == h[0] && '\n' == h[1])) {
		const char *key, *value;
		unsigned int keylen, valuelen;
		const char *p;
		for (p = key = h; ' ' != *p && ':' != *p; ++p);
		keylen = p - key;
		for (++p; ' ' == *p || ':' == *p; ++p);
		for (value = p; '\r' != *p; ++p);
		valuelen = p - value;
		h = p + 2;
		if (keylen && valuelen) {
			char *p;
			HashtableNode_t* exist_node;
			HttpFrameHeaderField_t* field;

			field = (HttpFrameHeaderField_t*)malloc(sizeof(HttpFrameHeaderField_t) + keylen + valuelen + 2);
			if (!field)
				return -1;

			p = (char*)(field + 1);
			memcpy(p, key, keylen);
			p[keylen] = 0;
			field->key = p;
			memcpy(p + keylen + 1, value, valuelen);
			p[keylen + valuelen + 1] = 0;
			field->value = p + keylen + 1;

			field->m_hashnode.key = &field->key;
			exist_node = hashtableInsertNode(&frame->headers, &field->m_hashnode);
			if (exist_node != &field->m_hashnode) {
				hashtableReplaceNode(exist_node, &field->m_hashnode);
				free(pod_container_of(exist_node, HttpFrameHeaderField_t, m_hashnode));
			}
		}
	}

	return e - buf + 4;
}

int httpframeDecodeChunked(char* buf, unsigned int len, unsigned char** data, unsigned int* datalen) {
	unsigned int chunked_length, frame_length;
	const char* p;
	char* e = strnstr(buf, "\r\n", len);
	if (!e)
		return 0;

	chunked_length = 0;
	for (p = buf; p < e; ++p) {
		if (isdigit(*p)) {
			chunked_length *= 16;
			chunked_length += *p - '0';
		}
		else if (*p >= 'a' && *p <= 'f') {
			chunked_length *= 16;
			chunked_length += *p - 'a' + 10;
		}
		else if (*p >= 'A' && *p <= 'F') {
			chunked_length *= 16;
			chunked_length += *p - 'A' + 10;
		}
		else {
			return -1;
		}
	}

	frame_length = e - buf + 2 + chunked_length + 2;
	if (frame_length > len)
		return 0;

	*datalen = chunked_length;
	if (*datalen)
		*data = (unsigned char*)(e + 2);
	else
		*data = NULL;
	return frame_length;
}

void httpframeEncodeChunked(unsigned int datalen, char txtbuf[11]) {
	txtbuf[0] = 0;
	sprintf(txtbuf, "%x\r\n", datalen);
}

void httpframeFree(HttpFrame_t* frame) {
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(&frame->headers); cur; cur = next) {
		next = hashtableNextNode(cur);
		free(pod_container_of(cur, HttpFrameHeaderField_t, m_hashnode));
	}
	hashtableInit(&frame->headers, frame->m_bulks,
			sizeof(frame->m_bulks) / sizeof(frame->m_bulks[0]),
			header_keycmp, header_keyhash);
	free(frame->uri);
	frame->query = frame->uri = NULL;
	frame->status_code = 0;
	frame->method[0] = 0;
}

#ifdef __cplusplus
}
#endif
