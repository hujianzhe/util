//
// Created by hujianzhe
//

#include "statistics.h"
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* processor */
int processor_IsLittleEndian(void) {
	unsigned short v = 0x0001;
	return *((unsigned char*)&v);
}

size_t processor_Count(void) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1;
#else
	long count = sysconf(_SC_NPROCESSORS_CONF);
	if (count <= 0)
		count = 1;
	return count;
#endif
}

/* name */
char* current_Username(char* buffer, size_t nbytes) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD len = nbytes;
	return GetUserNameA(buffer, &len) ? buffer : NULL;
#else
	struct passwd pwd, *res = NULL;
	int r = getpwuid_r(getuid(), &pwd, buffer, nbytes, &res);
	if (!res && r) {
		errno = r;
		return NULL;
	}
	return buffer;
#endif
}

char* host_Name(char* buf, size_t len) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwLen = len;
	return GetComputerNameA(buf, &dwLen) ? buf : NULL;
#else
	return gethostname(buf, len) == 0 ? buf : NULL;
#endif
}

/* memory */
long memory_PageSize(void) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
#else
	return sysconf(_SC_PAGESIZE);
#endif
}
BOOL memory_Size(unsigned long long* total) {
#if defined(_WIN32) || defined(_WIN64)
	MEMORYSTATUSEX statex = {0};
	statex.dwLength = sizeof(statex);
	if (GlobalMemoryStatusEx(&statex)) {
		*total = statex.ullTotalPhys;
		//*avail = statex.ullAvailPhys;
		return TRUE;
	}
	return FALSE;
#elif __linux__
	unsigned long page_size, total_page;
	if ((page_size = sysconf(_SC_PAGESIZE)) == -1)
		return FALSE;
	if ((total_page = sysconf(_SC_PHYS_PAGES)) == -1)
		return FALSE;
	//if((free_page = sysconf(_SC_AVPHYS_PAGES)) == -1)
		//return FALSE;
	*total = (unsigned long long)total_page * (unsigned long long)page_size;
	//*avail = (unsigned long long)free_page * (unsigned long long)page_size;
	return TRUE;
#elif __APPLE__
	int64_t value;
	size_t len = sizeof(value);
	if (sysctlbyname("hw.memsize", &value, &len, NULL, 0) == -1)
		return FALSE;
	*total = value;
	//*avail = 0;// sorry...
	return TRUE;
#endif
}

/* disk */
BOOL partition_Size(const char* dev_path, unsigned long long* total_mb, unsigned long long* free_mb, unsigned long long* availabel_mb, unsigned long long* b_size) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD spc,bps,nfc,tnc;
	ULARGE_INTEGER t,f,a;
	if (GetDiskFreeSpaceExA(dev_path, &a, &t, &f) && GetDiskFreeSpaceA(dev_path, &spc, &bps, &nfc, &tnc)) {
		*total_mb = t.QuadPart >> 20;
		*free_mb = f.QuadPart >> 20;
		*availabel_mb = a.QuadPart >> 20;
		*b_size = spc * bps;
		return TRUE;
	}
	return FALSE;
#else
	struct statvfs disk_info = {0};
	if (!statvfs(dev_path,&disk_info)) {
		*total_mb = (disk_info.f_blocks * disk_info.f_bsize) >> 20;
		*free_mb = (disk_info.f_bfree * disk_info.f_bsize) >> 20;
		*availabel_mb = (disk_info.f_bavail * disk_info.f_bsize) >> 20;
		*b_size = disk_info.f_bsize;
		return TRUE;
	}
	return FALSE;
#endif
}

#ifdef	__cplusplus
}
#endif
