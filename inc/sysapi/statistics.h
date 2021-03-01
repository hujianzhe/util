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

__declspec_dll size_t processorCount(void);
__declspec_dll char* systemCurrentLoginUsername(char* buffer, size_t nbytes);
__declspec_dll char* systemHostname(char* buf, size_t len);
__declspec_dll BOOL diskPartitionSize(const char* dev_path, unsigned long long* total_bytes, unsigned long long* free_bytes, unsigned long long* availabel_bytes, unsigned long long* block_bytes);

#ifdef	__cplusplus
}
#endif

#endif
