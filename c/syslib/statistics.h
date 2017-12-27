//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_STATISTICS_H
#define	UTIL_C_SYSLIB_STATISTICS_H

#include "platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	//typedef char HOST_NAME[MAX_COMPUTERNAME_LENGTH + 1];
	#ifndef	uuid_t
	#define	uuid_t	uuid_t
	#endif
	typedef	char uuid_string_t[37];
	#pragma comment(lib, "Ole32.lib")
	#pragma comment(lib, "Rpcrt4.lib")
#else
	#include <sys/statvfs.h>
	#include <sys/utsname.h>
	#include <pwd.h>
	#include <uuid/uuid.h>
	#ifdef  __linux__
		#include <bits/local_lim.h>
		#include <bits/posix1_lim.h>
		#include <bits/posix2_lim.h>
		//typedef char HOST_NAME[HOST_NAME_MAX + 1];
	#elif defined(__FreeBSD__) || defined(__APPLE__)
		#include <sys/sysctl.h>
		//typedef char HOST_NAME[_POSIX_HOST_NAME_MAX + 1];
	#endif
	#ifndef _UUID_STRING_T
	typedef char uuid_string_t[37];
	#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* uuid */
EXEC_RETURN uuid_Create(uuid_t* uuid);
EXEC_RETURN uuid_GetString(uuid_t* uuid, uuid_string_t* uuid_string);
/* processor */
int processor_IsLittleEndian(void);
size_t processor_Count(void);
/* name */
char* current_Username(char* buffer, size_t nbytes);
char* host_Name(char* buf, size_t len);
/* memory */
EXEC_RETURN page_Size(unsigned long* page_size, unsigned long* granularity);
EXEC_RETURN memory_Size(unsigned long long* total);
/* disk */
EXEC_RETURN partition_Size(const char* dev_path, unsigned long long* total_mb, unsigned long long* free_mb, unsigned long long* availabel_mb, unsigned long long* b_size);

#ifdef	__cplusplus
}
#endif

#endif
