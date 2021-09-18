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

static void httpframe_clear_headers(Hashtable_t* tbl) {
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(tbl); cur; cur = next) {
		next = hashtableNextNode(cur);
		free(pod_container_of(cur, HttpFrameHeaderField_t, m_hashnode));
	}
}

static void httpframe_clear_multipart_form_data(List_t* list) {
	ListNode_t* cur, *next;
	for (cur = list->head; cur; cur = next) {
		HttpMultipartFormData_t* form_data = pod_container_of(cur, HttpMultipartFormData_t, listnode);
		next = cur->next;
		httpframe_clear_headers(&form_data->headers);
		free(form_data);
	}
}

static char EMPTY_STRING[] = "";

HttpFrame_t* httpframeInit(HttpFrame_t* frame) {
	frame->status_code = 0;
	frame->method[0] = 0;
	frame->query = frame->uri = EMPTY_STRING;
	frame->pathlen = 0;
	hashtableInit(&frame->headers, frame->m_bulks,
		sizeof(frame->m_bulks) / sizeof(frame->m_bulks[0]),
		hashtableDefaultKeyCmpStr, hashtableDefaultKeyHashStr);
	frame->multipart_form_data_boundary = EMPTY_STRING;
	listInit(&frame->multipart_form_datalist);
	frame->content_length = 0;
	return frame;
}

HttpFrame_t* httpframeReset(HttpFrame_t* frame) {
	if (frame) {
		httpframe_clear_headers(&frame->headers);
		httpframe_clear_multipart_form_data(&frame->multipart_form_datalist);
		if (frame->uri != EMPTY_STRING)
			free(frame->uri);
		httpframeInit(frame);
	}
	return frame;
}

const char* httpframeGetHeader(Hashtable_t* headers, const char* key) {
	HashtableNodeKey_t hkey;
	HashtableNode_t* node;
	hkey.ptr = key;
	node = hashtableSearchKey(headers, hkey);
	return node ? pod_container_of(node, HttpFrameHeaderField_t, m_hashnode)->value : NULL;
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

static int httpframe_save_special_header(HttpFrame_t* frame) {
	HashtableNode_t* cur;
	for (cur = hashtableFirstNode(&frame->headers); cur; cur = hashtableNextNode(cur)) {
		HttpFrameHeaderField_t* field = pod_container_of(cur, HttpFrameHeaderField_t, m_hashnode);
		const char *k = field->key, *v = field->value;
		if (0 == strcmp(k, "Content-Type")) {
			static const char MULTIPART_FORM_DATA[] = "multipart/form-data";

			v = strstr(v, MULTIPART_FORM_DATA);
			if (v) {
				static const char BOUNDARY[] = "boundary=";
				v += sizeof(MULTIPART_FORM_DATA) - 1;
				v = strstr(v, BOUNDARY);
				if (!v) {
					return 0;
				}
				v += sizeof(BOUNDARY) - 1;
				frame->multipart_form_data_boundary = v;
				return 1;
			}
		}
		if (0 == strcmp(k, "Content-Length")) {
			if (sscanf(v, "%u", &frame->content_length) != 1) {
				return 0;
			}
		}
	}
	return 1;
}

static int parse_and_add_header(Hashtable_t* tbl, const char* h) {
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
			char *pkv;
			HashtableNode_t* exist_node;
			HttpFrameHeaderField_t* field;

			field = (HttpFrameHeaderField_t*)malloc(sizeof(HttpFrameHeaderField_t) + keylen + valuelen + 2);
			if (!field)
				return 0;

			pkv = (char*)(field + 1);
			memcpy(pkv, key, keylen);
			pkv[keylen] = 0;
			field->key = pkv;
			memcpy(pkv + keylen + 1, value, valuelen);
			pkv[keylen + valuelen + 1] = 0;
			field->value = pkv + keylen + 1;

			field->m_hashnode.key.ptr = field->key;
			exist_node = hashtableInsertNode(tbl, &field->m_hashnode);
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

	if (!parse_and_add_header(&frame->headers, h) || !httpframe_save_special_header(frame))
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

static HttpMultipartFormData_t* new_http_multipart_form_data(const void* data, unsigned int datalen, const char* headers) {
	HttpMultipartFormData_t* form_data = (HttpMultipartFormData_t*)malloc(sizeof(HttpMultipartFormData_t) + datalen);
	if (form_data) {
		hashtableInit(&form_data->headers, form_data->m_headerbulks,
			sizeof(form_data->m_headerbulks) / sizeof(form_data->m_headerbulks[0]),
			hashtableDefaultKeyCmpStr, hashtableDefaultKeyHashStr);
		if (headers) {
			if (!parse_and_add_header(&form_data->headers, headers)) {
				free(form_data);
				return NULL;
			}
		}

		form_data->datalen = datalen;
		if (datalen && data)
			memcpy(form_data->data, data, datalen);
		form_data->data[datalen] = 0;
	}
	return form_data;
}

int httpframeDecodeMultipartFormData(const char* boundary, unsigned char* buf, unsigned int len, HttpMultipartFormData_t** form_data) {
	unsigned int boundarylen, datalen;
	unsigned char *s, *e, *data;

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
		const char* h;
		e = (unsigned char*)strStr((char*)s, len, "\r\n\r\n", 4);
		if (!e)
			return 0;
		else if (e == s || !memchr(s + 2, ':', e - s - 2))
			h = NULL;
		else
			h = (char*)s + 2;
		e += 4;
		len -= e - s;
		data = s = e;
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

		datalen = e - data;
		if (0 == datalen && !h) {
			data = NULL;
			*form_data = NULL;
		}
		else {
			*form_data = new_http_multipart_form_data(data, datalen, h);
			if (NULL == *form_data)
				return -1;
		}

		s = e + 4 + boundarylen;
		if (s[0] == '-' && s[1] == '-' && s[2] == '\r' && s[3] == '\n')
			return e + 8 + boundarylen - buf;
		else
			return e + 2 - buf;
	}
	else if (s[0] == '-' && s[1] == '-') {
		if (s[2] != '\r' || s[3] != '\n')
			return -1;
		*form_data = NULL;
		// *form_data = new_http_multipart_form_data(NULL, 0, NULL);
		return s + 4 - buf;
	}
	else {
		return -1;
	}
}

HttpFrame_t* httpframeDecodeMultipartFormDataList(HttpFrame_t* frame, unsigned char* buf, unsigned int content_length) {
	unsigned int decodelen = 0;
	List_t list;
	listInit(&list);
	while (decodelen < content_length) {
		HttpMultipartFormData_t* form_data;
		int res = httpframeDecodeMultipartFormData(frame->multipart_form_data_boundary, buf + decodelen, content_length - decodelen, &form_data);
		if (res < 0) {
			break;
		}
		else if (0 == res) {
			break;
		}
		else if (form_data) {
			listPushNodeBack(&list, &form_data->listnode);
		}
		decodelen += res;
	}
	if (decodelen < content_length) {
		httpframe_clear_multipart_form_data(&list);
		return NULL;
	}
	else {
		listAppend(&frame->multipart_form_datalist, &list);
	}
	return frame;
}

#ifdef __cplusplus
}
#endif
