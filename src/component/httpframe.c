//
// Created by hujianzhe on 18-8-18.
//

#include "../../inc/datastruct/hash.h"
#include "../../inc/datastruct/memfunc.h"
#include "../../inc/component/httpframe.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static int header_keycmp(const void* node_key, const void* key) {
	return strcmp((const char*)node_key, (const char*)key);
}

static unsigned int header_keyhash(const void* key) {
	return hashBKDR((const char*)key);
}

static char EMPTY_STRING[] = "";

HttpFrame_t* httpframeInit(HttpFrame_t* frame) {
	frame->status_code = 0;
	frame->method[0] = 0;
	frame->query = frame->uri = EMPTY_STRING;
	frame->pathlen = 0;
	hashtableInit(&frame->headers, frame->m_bulks,
		sizeof(frame->m_bulks) / sizeof(frame->m_bulks[0]),
		header_keycmp, header_keyhash);
	frame->multipart_form_data_boundary = EMPTY_STRING;
	return frame;
}

HttpFrame_t* httpframeReset(HttpFrame_t* frame) {
	if (frame) {
		httpframeClearHeader(frame);
		if (frame->uri != EMPTY_STRING)
			free(frame->uri);
		httpframeInit(frame);
	}
	return frame;
}

const char* httpframeGetHeader(HttpFrame_t* frame, const char* key) {
	HashtableNode_t* node = hashtableSearchKey(&frame->headers, key);
	if (node)
		return pod_container_of(node, HttpFrameHeaderField_t, m_hashnode)->value;
	else
		return NULL;
}

HttpFrame_t* httpframeClearHeader(HttpFrame_t* frame) {
	if (frame) {
		HashtableNode_t *cur, *next;
		for (cur = hashtableFirstNode(&frame->headers); cur; cur = next) {
			next = hashtableNextNode(cur);
			free(pod_container_of(cur, HttpFrameHeaderField_t, m_hashnode));
		}
		hashtableInit(&frame->headers, frame->m_bulks,
			sizeof(frame->m_bulks) / sizeof(frame->m_bulks[0]),
			header_keycmp, header_keyhash);
		frame->multipart_form_data_boundary = EMPTY_STRING;
	}
	return frame;
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

static int parse_and_add_header(HttpFrame_t* frame, const char* h) {
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
				return 0;

			p = (char*)(field + 1);
			memcpy(p, key, keylen);
			p[keylen] = 0;
			field->key = p;
			memcpy(p + keylen + 1, value, valuelen);
			p[keylen + valuelen + 1] = 0;
			field->value = p + keylen + 1;

			field->m_hashnode.key = field->key;
			exist_node = hashtableInsertNode(&frame->headers, &field->m_hashnode);
			if (exist_node != &field->m_hashnode) {
				hashtableReplaceNode(exist_node, &field->m_hashnode);
				free(pod_container_of(exist_node, HttpFrameHeaderField_t, m_hashnode));
			}
		}
	}
	return 1;
}

int httpframeDecodeHeader(HttpFrame_t* frame, char* buf, unsigned int len) {
	const char *h, *e;

	httpframeInit(frame);

	e = strStr(buf, len, "\r\n\r\n", -1);
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
				frame->method[e - s] = '\0';
			}
			else {
				unsigned int i, query_pos = -1, frag_pos = -1;
				frame->uri = (char*)malloc(e - s + 1);
				if (!frame->uri)
					return -1;
				for (i = 0; i < e - s; ++i) {
					frame->uri[i] = s[i];
					if ('?' == s[i] && -1 == query_pos)
						query_pos = i + 1;
					if ('#' == s[i] && -1 == frag_pos)
						frag_pos = i + 1;
				}
				frame->uri[e - s] = 0;
				if (query_pos != -1) {
					frame->query = frame->uri + query_pos;
					if (query_pos)
						frame->pathlen = query_pos - 1;
					if (frag_pos != -1)
						frame->uri[frag_pos - 1] = 0;
				}
				else {
					frame->query = frame->uri + (e - s);
					frame->pathlen = e - s;
				}
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

	if (!parse_and_add_header(frame, h))
		return -1;

	return e - buf + 4;
}

int httpframeDecodeChunked(char* buf, unsigned int len, unsigned char** data, unsigned int* datalen) {
	unsigned int chunked_length, frame_length;
	const char* p;
	char* e = strStr(buf, len, "\r\n", -1);
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

const char* httpframeMultipartFormDataBoundary(HttpFrame_t* frame) {
	static const char MULTIPART_FORM_DATA[] = "multipart/form-data";
	static const char BOUNDARY[] = "boundary=";
	const char* content_type;
	const char* multipart_form_data;
	const char* boundary;

	content_type = httpframeGetHeader(frame, "Content-Type");
	if (!content_type)
		return NULL;

	multipart_form_data = strstr(content_type, MULTIPART_FORM_DATA);
	if (!multipart_form_data)
		return NULL;
	multipart_form_data += sizeof(MULTIPART_FORM_DATA) - 1;

	boundary = strstr(multipart_form_data, BOUNDARY);
	if (!boundary)
		return NULL;
	boundary += sizeof(BOUNDARY) - 1;

	return boundary;
}

int httpframeDecodeMultipartFormData(HttpFrame_t* frame, unsigned char* buf, unsigned int len, unsigned char** data, unsigned int* datalen) {
	const char* boundary;
	int boundarylen;
	unsigned char *s, *e;

	boundary = frame->multipart_form_data_boundary;
	if (!boundary || '\0' == *boundary)
		return -1;
	boundarylen = strlen(boundary);
	
	if (len < 6 + boundarylen)
		return 0;
	if (buf[0] != '-' || buf[1] != '-')
		return -1;
	s = buf + 2 + boundarylen;
	len -= boundarylen + 2;
	if (s[0] == '\r' && s[1] == '\n') {
		const char* h = (char*)s + 2;
		e = (char*)strStr((char*)s, len, "\r\n\r\n", 4);
		if (!e)
			return 0;
		e += 4;
		len -= e - s;
		*data = s = e;
		while (1) {
			e = (unsigned char*)memSearch(s, len, "\r\n--", 4);
			if (!e)
				return 0;
			len -= e - s;
			if (len < 4 + boundarylen)
				return 0;
			if (0 == memcmp(e + 4, boundary, boundarylen))
				break;
			s = e + 4;
			len -= 4;
		}
		if (len < 8 + boundarylen)
			return 0;

		if (!parse_and_add_header(frame, h))
			return -1;

		*datalen = e - *data;
		if (e == *data)
			*data = NULL;
		s = e + 4 + boundarylen;
		if (s[0] == '-' && s[1] == '-' && s[2] == '\r' && s[3] == '\n')
			return e + 8 + boundarylen - buf;
		else
			return e + 2 - buf;
	}
	else if (s[0] == '-' && s[1] == '-') {
		if (s[2] != '\r' || s[3] != '\n')
			return -1;
		*datalen = 0;
		*data = NULL;
		return s + 4 - buf;
	}
	else {
		return -1;
	}
}

#ifdef __cplusplus
}
#endif
