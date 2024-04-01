//
// Created by hujianzhe
//

#include "../../inc/datastruct/url.h"

#ifdef	__cplusplus
extern "C" {
#endif

URL_t* urlParse(URL_t* url, const char* str, UnsignedPtr_t slen) {
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
	unsigned short portlen;
	unsigned short port_number;
	const char* p;
	UnsignedPtr_t si = 0;
	/* prepare parse */
	p = str;
	while (si < slen && *p && *p != '/') {
		++p;
		++si;
	}
	if (si >= slen || '\0' == *p) {
		return (URL_t*)0;
	}
	/* parse schema */
	if (si > 0 && si + 1 < slen && p[-1] == ':' && p[1] == '/') {
		schemalen = (unsigned int)(p - str - 1);
		if (!schemalen) {
			return (URL_t*)0;
		}
		schema = str;
		/* find path start */
		p += 2;
		si += 2;
		while (si < slen && *p && *p != '/') {
			++p;
			++si;
		}
		if (si >= slen || '\0' == *p) {
			return (URL_t*)0;
		}
		path = p;
	}
	else {
		if (si > 0) {
			return (URL_t*)0;
		}
		schema = (char*)0;
		schemalen = 0;
		/* find path start */
		path = p;
	}
	/* prepare parse user,pwd,host,port */
	if (schemalen) {
		const char* ps = schema + schemalen + 3;
		const char* first_colon = (char*)0;
		const char* first_at = (char*)0;
		for (--p; p >= ps; --p) {
			if (':' == *p) {
				if (!first_colon) {
					first_colon = p;
					if (first_at) {
						break;
					}
				}
			}
			else if ('@' == *p) {
				if (!first_at) {
					first_at = p;
					if (first_colon) {
						break;
					}
				}
			}
		}
		if (first_colon < first_at) {
			first_colon = (char*)0;
		}
		/* parse host */
		if (!first_colon && !first_at) {
			hostlen = path - ps;
			if (!hostlen) {
				return (URL_t*)0;
			}
			host = ps;
		}
		else if (first_colon && first_at) {
			hostlen = first_colon - first_at - 1;
			if (!hostlen) {
				return (URL_t*)0;
			}
			host = first_at + 1;
		}
		else if (first_colon) {
			hostlen = first_colon - ps;
			if (!hostlen) {
				return (URL_t*)0;
			}
			host = ps;
		}
		else {
			hostlen = path - first_at - 1;
			if (!hostlen) {
				return (URL_t*)0;
			}
			host = first_at + 1;
		}
		/* parse port */
		if (first_colon) {
			portlen = (unsigned short)(path - first_colon - 1);
			if (portlen) {
				const char* sp = first_colon + 1;
				port = sp;
				for (port_number = 0; sp < path; ++sp) {
					if (*sp < '0' || *sp > '9') {
						return (URL_t*)0;
					}
					port_number *= 10;
					port_number += *sp - '0';
				}
			}
			else {
				port = (char*)0;
				port_number = 80;
			}
		}
		else {
			port = (char*)0;
			portlen = 0;
			port_number = 80;
		}
		/* parse user,pwd */
		if (first_at) {
			for (p = ps; p < first_at && ':' != *p; ++p);
			userlen = p - ps;
			if (userlen) {
				user = ps;
			}
			else {
				user = (char*)0;
			}
			if (':' == *p) {
				pwdlen = first_at - p - 1;
				if (pwdlen) {
					pwd = p + 1;
				}
				else {
					pwd = (char*)0;
				}
			}
			else {
				pwd = (char*)0;
				pwdlen = 0;
			}
		}
		else {
			user = (char*)0;
			userlen = 0;
			pwd = (char*)0;
			pwdlen = 0;
		}
	}
	else {
		user = (char*)0;
		pwd = (char*)0;
		host = (char*)0;
		port = (char*)0;
		userlen = 0;
		pwdlen = 0;
		hostlen = 0;
		portlen = 0;
		port_number = 0;
	}
	/* parse path length */
	p = path;
	while (si < slen && *p && *p != '?' && *p != '#') {
		++p;
		++si;
	}
	pathlen = (unsigned int)(p - path);
	if (si < slen) {
		/* parse query */
		if ('?' == *p) {
			query = ++p;
			++si;
			while (si < slen && *p && *p != '#') {
				++p;
				++si;
			}
			querylen = (unsigned int)(p - query);
			if (!querylen) {
				query = (char*)0;
			}
		}
		else {
			query = (char*)0;
			querylen = 0;
		}
		/* parse fragment */
		if (si < slen && '#' == *p) {
			fragment = ++p;
			++si;
			while (si < slen && *p) {
				++p;
				++si;
			}
			fragmentlen = (unsigned int)(p - fragment);
			if (!fragmentlen) {
				fragment = (char*)0;
			}
		}
		else {
			fragment = (char*)0;
			fragmentlen = 0;
		}
	}
	else {
		query = (char*)0;
		querylen = 0;
		fragment = (char*)0;
		fragmentlen = 0;
	}
	/* save parse result */
	url->schema = schema;
	url->schema_slen = schemalen;
	url->user = user;
	url->user_slen = userlen;
	url->pwd = pwd;
	url->pwd_slen = pwdlen;
	url->host = host;
	url->host_slen = hostlen;
	url->port = port;
	url->port_slen = portlen;
	url->path = path;
	url->path_slen = pathlen;
	url->query = query;
	url->query_slen = querylen;
	url->fragment = fragment;
	url->fragment_slen = fragmentlen;
	url->port_number = port_number;
	return url;
	/* return buffer space */
	/*
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
	*/
}

#define char_isdigit(c)		((unsigned int)(((int)(c)) - '0') < 10u)
#define	char_isalpha(c)		((unsigned int)((((int)(c)) | 0x20) - 'a') < 26u)

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
				int ch = src[i + 1];
				int cl = src[i + 2];
				dst[dstlen] = (char)((char_isdigit(ch) ? ch - '0' : (ch | 0x20) - 'a' + 10) << 4);
				dst[dstlen] |= (char)(char_isdigit(cl) ? cl - '0' : (cl | 0x20) - 'a' + 10);
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
