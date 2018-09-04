//
// Created by hujianzhe
//

#include "url.h"

#ifdef	__cplusplus
extern "C" {
#endif

int urlParsePrepare(URL_t* url, const char* str) {
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

	const char* p = str, *alpha;
	while (*p && *p != ':')
		++p;
	if ('\0' == *p || p[1] != '/' || p[2] != '/')
		return 0;
	schema = str;
	schemalen = p - str;
	p += 3;
	str = p;
	while (*p && *p != '/')
		++p;
	if ('\0' == *p)
		return 0;
	path = p;

	port = (char*)0;
	alpha = (char*)0;
	for (p = str; p < path; ++p) {
		if ('@' == *p) {
			user = str;
			for (; str < p; ++str) {
				if (':' == *str)
					break;
			}
			if (str != p) {
				userlen = str - user;
				pwd = str + 1;
				pwdlen = p - pwd;
			}
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
	if (port)
		hostlen = port - host;
	else
		hostlen = path - host;

	p = path;
	while(*p && *p != '?' && *p != '#')
		++p;
	pathlen = p - path;
	query = (char*)0;
	if ('?' == *p) {
		query = ++p;
		while (*p && *p != '#')
			++p;
		querylen = p - query;
	}
	if ('#' == *p) {
		fragment = ++p;
		while (*p)
			++p;
		fragmentlen = p - fragment;
	}
	/* save parse result, return need space */
}

void urlParse(URL_t* url, void* buf) {

}

#ifdef	__cplusplus
}
#endif
