//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_URL_H
#define	UTIL_C_DATASTRUCT_URL_H

#include "../compiler_define.h"

typedef struct URL_t {
	const char* schema;
	const char* user;
	const char* pwd;
	const char* host;
	const char* path;
	const char* query;
	const char* fragment;

	unsigned int schemalen;
	unsigned int userlen;
	unsigned int pwdlen;
	unsigned int hostlen;
	unsigned int pathlen;
	unsigned int querylen;
	unsigned int fragmentlen;
	unsigned short port;
} URL_t;

#ifdef	__cplusplus
extern "C" {
#endif

UTIL_LIBAPI unsigned int urlParsePrepare(URL_t* url, const char* str);
UTIL_LIBAPI URL_t* urlParseFinish(URL_t* url, char* buf);

#ifdef	__cplusplus
}
#endif

#endif
