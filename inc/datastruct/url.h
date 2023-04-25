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
	const char* port;
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
	unsigned short portlen;
	unsigned short port_number;
} URL_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll URL_t* urlParse(URL_t* url, const char* str);
__declspec_dll unsigned int urlEncode(const char* src, unsigned int srclen, char* dst);
__declspec_dll unsigned int urlDecode(const char* src, unsigned int srclen, char* dst);

#ifdef	__cplusplus
}
#endif

#endif
