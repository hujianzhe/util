//
// Created by hujianzhe
//

#include "../../inc/datastruct/url.h"

#ifdef	__cplusplus
extern "C" {
#endif

unsigned int urlParsePrepare(URL_t* url, const char* str) {
	const char* schema;
	unsigned int schemalen;
	const char* user;
	unsigned int userlen;
	const char* pwd;
	unsigned int pwdlen;
	const char* host;
	unsigned int hostlen;
	const char* path;
	unsigned int pathlen;
	const char* query;
	unsigned int querylen;
	const char* fragment;
	unsigned int fragmentlen;
	const char* port;
	unsigned short port_number;

	unsigned int buflen;
	const char* p = str, *alpha;
	while (*p && *p != ':')
		++p;
	if ('\0' == *p || p[1] != '/' || p[2] != '/')
		return 0;
	schema = str;
	schemalen = (unsigned int)(p - str);
	p += 3;
	str = p;
	while (*p && *p != '/')
		++p;
	if ('\0' == *p)
		return 0;
	path = p;

	port = (char*)0;
	alpha = (char*)0;
	user = (char*)0;
	pwd = (char*)0;
	userlen = 0;
	pwdlen = 0;
	for (p = str; p < path; ++p) {
		if ('@' == *p) {
			user = str;
			for (; str < p; ++str) {
				if (':' == *str)
					break;
			}
			if (str != p) {
				userlen = (unsigned int)(str - user);
				pwd = str + 1;
				pwdlen = (unsigned int)(p - pwd);
			}
			else
				userlen = (unsigned int)(p - user);
			alpha = p;
			port = (char*)0;
		}
		else if (':' == *p)
			port = p;
	}
	if (alpha)
		host = alpha + 1;
	else
		host = str;
	if (port) {
		hostlen = (unsigned int)(port - host);
		if (0 == hostlen)
			return 0;
		if (path - port > 6)
			return 0;
		++port;
		for (port_number = 0; port < path; ++port) {
			if (*port < '0' || *port > '9')
				return 0;
			port_number *= 10;
			port_number += *port - '0';
		}
		if (0 == port_number)
			port_number = 80;
	}
	else {
		hostlen = (unsigned int)(path - host);
		if (0 == hostlen)
			return 0;
		port_number = 80;
	}

	p = path;
	while (*p && *p != '?' && *p != '#')
		++p;
	pathlen = (unsigned int)(p - path);

	if ('?' == *p) {
		query = ++p;
		while (*p && *p != '#')
			++p;
		querylen = (unsigned int)(p - query);
	}
	else {
		query = (char*)0;
		querylen = 0;
	}

	if ('#' == *p) {
		fragment = ++p;
		while (*p)
			++p;
		fragmentlen = (unsigned int)(p - fragment);
	}
	else {
		fragment = (char*)0;
		fragmentlen = 0;
	}
	/* save parse result */
	url->schema = schema;
	url->schemalen = schemalen;
	url->user = user;
	url->userlen = userlen;
	url->pwd = pwd;
	url->pwdlen = pwdlen;
	url->host = host;
	url->hostlen = hostlen;
	url->path = path;
	url->pathlen = pathlen;
	url->query = query;
	url->querylen = querylen;
	url->fragment = fragment;
	url->fragmentlen = fragmentlen;
	url->port = port_number;
	/* return buffer space */
	buflen = 1;
	buflen += schemalen + hostlen + pathlen + 3;
	if (userlen)
		buflen += userlen + 1;
	if (pwdlen)
		buflen += pwdlen + 1;
	if (querylen)
		buflen += querylen + 1;
	if (fragmentlen)
		buflen += fragmentlen + 1;
	return buflen;
}

static char* copy(char* dst, const char* src, unsigned int n) {
	unsigned int i;
	for (i = 0; i < n; ++i)
		dst[i] = src[i];
	return dst;
}

URL_t* urlParseFinish(URL_t* url, char* buf) {
	char *schema;
	char *user;
	char *pwd;
	char *host;
	char *path;
	char *query;
	char *fragment;
	char *t;

	schema = buf;
	copy(schema, url->schema, url->schemalen);
	schema[url->schemalen] = 0;

	host = schema + url->schemalen + 1;
	copy(host, url->host, url->hostlen);
	host[url->hostlen] = 0;

	path = host + url->hostlen + 1;
	copy(path, url->path, url->pathlen);
	path[url->pathlen] = 0;

	t = path + url->pathlen;

	if (url->userlen) {
		user = ++t;
		t += url->userlen;
		copy(user, url->user, url->userlen);
		*t = 0;
	}
	else
		user = t;

	if (url->pwdlen) {
		pwd = ++t;
		t += url->pwdlen;
		copy(pwd, url->pwd, url->pwdlen);
		*t = 0;
	}
	else
		pwd = t;

	if (url->querylen) {
		query = ++t;
		t += url->querylen;
		copy(query, url->query, url->querylen);
		*t = 0;
	}
	else
		query = t;

	if (url->fragmentlen) {
		fragment = ++t;
		t += url->fragmentlen;
		copy(fragment, url->fragment, url->fragmentlen);
		*t = 0;
	}
	else
		fragment = t;

	url->schema = schema;
	url->user = user;
	url->pwd = pwd;
	url->host = host;
	url->path = path;
	url->query = query;
	url->fragment = fragment;
	return url;
}

#define char_isdigit(c)		((unsigned int)(((int)(c)) - '0') < 10u)
#define	char_isalpha(c)		((unsigned int)((((int)(c)) | 0x20) - 'a') < 26u)
#define char_alphadelta(c)	((((int)(c)) | 0x20) - 'a')

unsigned int urlEncode(const char* src, unsigned int srclen, char* dst) {
	static const char hex2char[] = "0123456789ABCDEF";
	unsigned int i, dstlen;
	for (dstlen = 0, i = 0; i < srclen; ++i) {
		char c = src[i];
		if (char_isdigit(c) || char_isalpha(c)) {
			if (dst) {
				dst[dstlen] = c;
			}
			++dstlen;
		}
		else if (c == ' ') {
			if (dst) {
				dst[dstlen] = '+';
			}
			++dstlen;
		}
		else {
			if (dst) {
				dst[dstlen] = '%';
				dst[dstlen + 1] = hex2char[((unsigned char)c) >> 4];
				dst[dstlen + 2] = hex2char[c & 0xf];
			}
			dstlen += 3;
		}
	}
	if (dst) {
		dst[dstlen] = 0;
	}
	return dstlen;
}

unsigned int urlDecode(const char* src, unsigned int srclen, char* dst) {
	unsigned int i, dstlen;
	for (dstlen = 0, i = 0; i < srclen; ++i) {
		char c = src[i];
		if (c == '+') {
			if (dst) {
				dst[dstlen] = ' ';
			}
			++dstlen;
		}
		else if (c == '%') {
			if (dst) {
				char ch = src[i + 1];
				char cl = src[i + 2];
				dst[dstlen] = (char)((char_isdigit(ch) ? ch - '0' : char_alphadelta(ch) - 'A' + 10) << 4);
				dst[dstlen] |= (char)(char_isdigit(cl) ? cl - '0' : char_alphadelta(cl) - 'A' + 10);
			}
			i += 2;
			++dstlen;
		}
		else {
			if (dst) {
				dst[dstlen] = c;
			}
			++dstlen;
		}
	}
	if (dst) {
		dst[dstlen] = 0;
	}
	return dstlen;
}

#ifdef	__cplusplus
}
#endif
