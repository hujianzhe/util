//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_URL_H
#define	UTIL_C_DATASTRUCT_URL_H

typedef struct URL_t {
	const char* schema;
	const char* user;
	const char* pwd;
	const char* host;
	unsigned short port;
	const char* path;
	const char* query;
	const char* fragment;
} URL_t;

#ifdef	__cplusplus
extern "C" {
#endif

int urlParsePrepare(URL_t* url, const char* str);
void urlParse(URL_t* url, void* buf);

#ifdef	__cplusplus
}
#endif

#endif
