//
// Created by hujianzhe on 17-2-25.
//

#include "../string_util.h"
#include "http_frame.h"
#include <stdio.h>
#include <sstream>
#include <vector>

namespace Util {
const char* HttpFrame::statusDesc(int status_code) {
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
std::string HttpFrame::uriQuery(const std::string& uri) {
	size_t pos = uri.find("?");
	if (std::string::npos == pos || uri.size() - 1 <= pos) {
		return "";
	}
	return uri.substr(pos + 1);
}
void HttpFrame::parseQuery(const std::string& qs, std::map<std::string, std::string>& kv) {
	if (qs.empty()) {
		return;
	}
	std::vector<std::string> parts = Util::string::split(qs, '&');
	for (size_t i = 0; i < parts.size(); ++i) {
		std::vector<std::string> datas = Util::string::split(parts[i], '=');
		if (datas.size() < 2) {
			continue;
		}
		kv[datas[0]] = datas[1];
	}
}

HttpFrame::HttpFrame(size_t frame_length_limit) :
	m_frameLengthLimit(frame_length_limit),
	m_frameLength(0),
	m_data(NULL),
	m_dataLength(0),
	m_statusCode(0)
{
	memset(m_method, 0, sizeof(m_method));
}
HttpFrame::HttpFrame(size_t frame_length_limit, const char* method, const char* uri) :
	m_frameLengthLimit(frame_length_limit),
	m_frameLength(0),
	m_data(NULL),
	m_dataLength(0),
	m_uri(uri),
	m_statusCode(0)
{
	if (method) {
		strncpy(m_method, method, sizeof(m_method));
		m_method[sizeof(m_method) - 1] = 0;
	}
	else {
		memset(m_method, 0, sizeof(m_method));
	}
}
HttpFrame::HttpFrame(size_t frame_length_limit, const char* method, const char* uri, size_t urilen) :
	m_frameLengthLimit(frame_length_limit),
	m_frameLength(0),
	m_data(NULL),
	m_dataLength(0),
	m_uri(uri, urilen),
	m_statusCode(0)
{
	if (method) {
		strncpy(m_method, method, sizeof(m_method));
		m_method[sizeof(m_method) - 1] = 0;
	}
	else {
		memset(m_method, 0, sizeof(m_method));
	}
}
HttpFrame::HttpFrame(size_t frame_length_limit, int status_code) :
	m_frameLengthLimit(frame_length_limit),
	m_frameLength(0),
	m_data(NULL),
	m_dataLength(0),
	m_statusCode(status_code)
{
	memset(m_method, 0, sizeof(m_method));
}

const std::string& HttpFrame::getHeader(const std::string& key) const {
	static const std::string null_string;
	header_const_iter it = m_headers.find(key);
	return it != m_headers.end() ? it->second : null_string;
}
void HttpFrame::setHeader(const std::string& key, const std::string& value) {
	if (key.empty() || value.empty()) {
		return;
	}
	m_headers[key] = value;
}
void HttpFrame::setContentLengthHeader(size_t len) {
	char number[21];
	sprintf(number, "%zu", len);
	m_headers["Content-Length"] = number;
}
void HttpFrame::setAccessControlAllowOriginHeader(const std::string& origin) {
	m_headers["Access-Control-Allow-Origin"] = origin;
}

int HttpFrame::parseHeader(const char* data, size_t len) {
	const char* key, *p;
	if (m_frameLengthLimit < len) {
		key = strnstr(data, "\r\n\r\n", m_frameLengthLimit);
		if (!key) {
			return PARSE_OVERRANGE;
		}
	}
	else {
		key = strnstr(data, "\r\n\r\n", len);
		if (!key) {
			return PARSE_INCOMPLETION;
		}
	}
	m_frameLength = key - data + 4;

	if (strncmp(data, "HTTP", 4)) {
		m_statusCode = 0;
		for (p = data, key = data; *p != '\r'; ++p) {
			if (' ' != *p) {
				continue;
			}
			if ('\0' == m_method[0]) {
				if (p - key >= sizeof(m_method)) {
					return PARSE_INVALID;
				}
				strncpy(m_method, key, p - key);
			}
			else {
				m_uri = std::string(key, p - key);
			}
			key = p + 1;
		}
	}
	else {
		const char* p;
		for (p = data; *p != '\r' && *p != ' '; ++p);
		if ('\r' == *p) {
			return PARSE_INVALID;
		}
		int stscode = 0;
		for (++p; *p != ' '; ++p) {
			if (!isdigit(*p)) {
				return PARSE_INVALID;
			}
			stscode *= 10;
			stscode += *p - '0';
		}
		m_statusCode = stscode;
		m_method[0] = 0;
		m_uri.clear();
	}

	m_headers.clear();
	const char* headerline = strstr(p, "\r\n") + 2;
	while (strncmp(headerline, "\r\n", 2)) {
		p = headerline;
		while (*p != ' ' && *p != ':') { ++p; }
		std::string key(headerline, p - headerline);
		while (*p == ':' || *p == ' ') { ++p; }
		headerline = p;
		while (*p != '\r') { ++p; }
		std::string value(headerline, p - headerline);
		headerline = p + 2;
		if (!key.empty() && !value.empty()) {
			m_headers.insert(std::make_pair(key, value));
		}
	}

	return PARSE_OK;
}

int HttpFrame::parseContentLengthBody(const char* data, size_t len) {
	m_data = NULL;
	m_dataLength = m_frameLength = 0;
	header_iter it = m_headers.find("Content-Length");
	if (it == m_headers.end() || it->second.empty()) {
		return PARSE_BODY_NOT_EXIST;
	}
	unsigned int content_length = 0;
	for (size_t i = 0; i < it->second.size(); ++i) {
		if (!isdigit(it->second[i])) {
			return PARSE_INVALID;
		}
		content_length *= 10;
		content_length += it->second[i] - '0';
	}
	if (content_length > m_frameLengthLimit) {
		return PARSE_OVERRANGE;
	}
	if (content_length > len) {
		return PARSE_INCOMPLETION;
	}
	if (content_length) {
		m_data = data;
		m_dataLength = content_length;
	}
	m_frameLength = content_length;
	return PARSE_OK;
}

int HttpFrame::parseNextChunkedBody(const char* data, size_t len) {
	m_data = NULL;
	m_dataLength = m_frameLength = 0;
	header_iter it = m_headers.find("Transfer-Encoding");
	if (it == m_headers.end() || it->second != "chunked") {
		return PARSE_BODY_NOT_EXIST;
	}
	const char* key;
	if (m_frameLengthLimit < len) {
		key = strnstr(data, "\r\n", m_frameLengthLimit);
		if (!key) {
			return PARSE_OVERRANGE;
		}
	}
	else {
		key = strnstr(data, "\r\n", len);
		if (!key) {
			return PARSE_INCOMPLETION;
		}
	}
	unsigned int chunked_length = 0;
	for (const char* p = data; p < key; ++p) {
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
			return PARSE_INVALID;
		}
	}
	if (key - data + 2 + chunked_length + 2 > m_frameLengthLimit) {
		return PARSE_OVERRANGE;
	}
	if (key - data + 2 + chunked_length + 2 > len) {
		return PARSE_INCOMPLETION;
	}
	if (data[key - data + 2 + chunked_length] != '\r' || data[key - data + 2 + chunked_length + 1] != '\n') {
		return PARSE_INVALID;
	}
	if (chunked_length) {
		m_data = key + 2;
		m_dataLength = chunked_length;
	}
	m_frameLength = key - data + 2 + chunked_length + 2;
	return PARSE_OK;
}

bool HttpFrame::buildResponseHeader(std::string& s) {
	const char* status_desc = statusDesc(m_statusCode);
	if ('\0' == *status_desc) {
		return false;
	}
	std::stringstream ss;
	ss << "HTTP/1.1 " << m_statusCode << ' ' << status_desc << "\r\n";
	s = ss.str();
	if (s.size() + 2 > m_frameLengthLimit) {
		s.clear();
		return false;
	}
	for (header_iter iter = m_headers.begin(); iter != m_headers.end(); ++iter) {
		if (iter->first.empty() || iter->second.empty()) {
			continue;
		}
		if (s.size() + iter->first.size() + iter->second.size() + 6 > m_frameLengthLimit) {
			return false;
		}
		s += iter->first; s += ": "; s += iter->second; s += "\r\n";
	}
	s += "\r\n";
	if (s.size() > m_frameLengthLimit) {
		s.clear();
		return false;
	}
	return true;
}
}
