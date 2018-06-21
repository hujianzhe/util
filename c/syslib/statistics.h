//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_STATISTICS_H
#define	UTIL_C_SYSLIB_STATISTICS_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	//typedef char HOST_NAME[MAX_COMPUTERNAME_LENGTH + 1];
#else
	#include <sys/statvfs.h>
	#include <sys/utsname.h>
	#include <pwd.h>
	#ifdef  __linux__
		#include <bits/local_lim.h>
		#include <bits/posix1_lim.h>
		#include <bits/posix2_lim.h>
		//typedef char HOST_NAME[HOST_NAME_MAX + 1];
	#elif defined(__FreeBSD__) || defined(__APPLE__)
		#include <sys/sysctl.h>
		//typedef char HOST_NAME[_POSIX_HOST_NAME_MAX + 1];
	#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/* processor */
int processor_IsLittleEndian(void);
size_t processor_Count(void);
/* name */
char* current_Username(char* buffer, size_t nbytes);
char* host_Name(char* buf, size_t len);
/* memory */
long memory_PageSize(void);
BOOL memory_Size(unsigned long long* total);
/* disk */
BOOL partition_Size(const char* dev_path, unsigned long long* total_mb, unsigned long long* free_mb, unsigned long long* availabel_mb, unsigned long long* b_size);

#ifdef	__cplusplus
}
#endif

#endif
