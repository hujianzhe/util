//
// Created by hujianzhe on 17-2-25.
//

#ifndef	UTIL_CPP_PROTOCOL_HTTP_FRAME_H
#define	UTIL_CPP_PROTOCOL_HTTP_FRAME_H

#include <stddef.h>
#include <string>
#include "../cpp_compiler_define.h"
#include <map>

namespace Util {
class HttpFrame {
public:
	enum {
		PARSE_OVERRANGE,
		PARSE_INCOMPLETION,
		PARSE_INVALID,
		PARSE_BODY_NOT_EXIST,
		PARSE_OK
	};
	static const char* statusDesc(int status_code);
	static std::string uriQuery(const std::string& uri);
	static void parseQuery(const std::string& qs, std::map<std::string, std::string>& kv);

	HttpFrame(size_t frame_length_limit);
	HttpFrame(size_t frame_length_limit, const char* method, const char* uri);
	HttpFrame(size_t frame_length_limit, const char* method, const char* uri, size_t urilen);
	HttpFrame(size_t frame_length_limit, int status_code);

	int parseHeader(const char* data, size_t len);
	int parseContentLengthBody(const char* data, size_t len);
	int parseNextChunkedBody(const char* data, size_t len);
	bool buildResponseHeader(std::string& s);

	const char* method(void) const { return m_method; }
	std::string uri(void) const { return m_uri; }
	
	int statusCode(void) const { return m_statusCode; }
	const char* statusDescription(void) const { return statusDesc(m_statusCode); }

	const std::map<std::string, std::string>& headers(void) const { return m_headers; }
	const std::string& getHeader(const std::string& key) const;
	void setHeader(const std::string& key, const std::string& value);
	void setContentLengthHeader(size_t len);
	void setAccessControlAllowOriginHeader(const std::string& origin = "*");

	const char* data(void) const { return m_data; }
	size_t dataLength(void) const { return m_dataLength; }
	size_t frameLength(void) const { return m_frameLength; }
	size_t frameLengthLimit(void) const { return m_frameLengthLimit; }

private:
	const size_t m_frameLengthLimit;
	size_t m_frameLength;

	const char* m_data;
	size_t m_dataLength;

	char m_method[8];
	std::string m_uri;
	int m_statusCode;
	std::map<std::string, std::string> m_headers;
	typedef std::map<std::string, std::string>::iterator header_iter;
	typedef std::map<std::string, std::string>::const_iterator header_const_iter;
};
}

#endif // UTIL_HTTP_PROTOCOL_H
