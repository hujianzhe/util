//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_UUID_H
#define	UTIL_C_SYSLIB_UUID_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#ifndef	uuid_t
		#define	uuid_t	UUID
	#endif
	typedef	char uuid_string_t[37];
	#pragma comment(lib, "Ole32.lib")
	#pragma comment(lib, "Rpcrt4.lib")
#else
	#include <uuid/uuid.h>
	#ifndef _UUID_STRING_T
		typedef char uuid_string_t[37];
	#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll uuid_t* uuidCreate(uuid_t* uuid);
__declspec_dll char* uuidToString(const uuid_t* uuid, uuid_string_t uuid_string);
__declspec_dll uuid_t* uuidFromString(uuid_t* uuid, const uuid_string_t uuid_string);

#ifdef	__cplusplus
}
#endif

#endif
