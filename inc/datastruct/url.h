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

	unsigned int schema_slen;
	unsigned int user_slen;
	unsigned int pwd_slen;
	unsigned int host_slen;
	unsigned int path_slen;
	unsigned int query_slen;
	unsigned int fragment_slen;
	unsigned short port_slen;
	unsigned short port_number;
} URL_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll URL_t* urlParse(URL_t* url, const char* str, UnsignedPtr_t slen);
__declspec_dll unsigned int urlEncode(const char* src, unsigned int srclen, char* dst);
__declspec_dll unsigned int urlDecode(const char* src, unsigned int srclen, char* dst);

#ifdef	__cplusplus
}
#endif

#endif
