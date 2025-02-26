//
// Created by hujianzhe
//

#include "../../inc/sysapi/statistics.h"
#include <errno.h>

#ifdef	__cplusplus
extern "C" {
#endif

long memoryPageSize(void) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
#else
	return sysconf(_SC_PAGESIZE);
#endif
}

unsigned long long memorySize(void) {
#if defined(_WIN32) || defined(_WIN64)
	MEMORYSTATUSEX statex = { 0 };
	statex.dwLength = sizeof(statex);
	if (GlobalMemoryStatusEx(&statex)) {
		return statex.ullTotalPhys;
		//*avail = statex.ullAvailPhys;
		//return TRUE;
	}
	return 0;
	//return FALSE;
#elif __linux__
	unsigned long page_size, total_page;
	if ((page_size = sysconf(_SC_PAGESIZE)) == -1)
		return 0;
	if ((total_page = sysconf(_SC_PHYS_PAGES)) == -1)
		return 0;
	//if((free_page = sysconf(_SC_AVPHYS_PAGES)) == -1)
	//return FALSE;
	return (unsigned long long)total_page * (unsigned long long)page_size;
	//*avail = (unsigned long long)free_page * (unsigned long long)page_size;
	//return TRUE;
#elif __APPLE__
	int64_t value;
	size_t len = sizeof(value);
	return sysctlbyname("hw.memsize", &value, &len, NULL, 0) != -1 ? value : 0;
	//*avail = 0;// sorry...
	//return TRUE;
#endif
}

size_t processorCount(void) {
#if defined(_WIN32) || defined(_WIN64)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1;
#else
	/*long count = sysconf(_SC_NPROCESSORS_CONF);*/
	long count = sysconf(_SC_NPROCESSORS_ONLN);
	if (count <= 0) {
		return 1;
	}
	return count;
#endif
}

char* systemCurrentLoginUsername(char* buffer, size_t nbytes) {
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

char* systemHostname(char* buf, size_t len) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwLen = len;
	return GetComputerNameA(buf, &dwLen) ? buf : NULL;
#else
	return gethostname(buf, len) == 0 ? buf : NULL;
#endif
}

BOOL diskPartitionSize(const char* dev_path, unsigned long long* total_bytes, unsigned long long* free_bytes, unsigned long long* availabel_bytes, unsigned long long* block_bytes) {
#if defined(_WIN32) || defined(_WIN64)
	ULARGE_INTEGER t, f, a;
	if (!GetDiskFreeSpaceExA(dev_path, &a, &t, &f)) {
		return FALSE;
	}
	if (block_bytes) {
		DWORD spc, bps, nfc, tnc;
		if (!GetDiskFreeSpaceA(dev_path, &spc, &bps, &nfc, &tnc))
			return FALSE;
		*block_bytes = spc * bps;
	}
	if (total_bytes)
		*total_bytes = t.QuadPart;
	if (free_bytes)
		*free_bytes = f.QuadPart;
	if (availabel_bytes)
		*availabel_bytes = a.QuadPart;
	return TRUE;
#else
	struct statvfs disk_info = { 0 };
	if (!statvfs(dev_path, &disk_info)) {
		unsigned long f_bsize = disk_info.f_bsize;
		if (total_bytes) {
			*total_bytes = disk_info.f_blocks;
			*total_bytes *= f_bsize;
		}
		if (free_bytes) {
			*free_bytes = disk_info.f_bfree;
			*free_bytes *= f_bsize;
		}
		if (availabel_bytes) {
			*availabel_bytes = disk_info.f_bavail;
			*availabel_bytes *= f_bsize;
		}
		if (block_bytes)
			*block_bytes = f_bsize;
		return TRUE;
	}
	return FALSE;
#endif
}

/*
#if defined(_WIN32) || defined(_WIN64)
	#pragma comment(lib, "Advapi32.lib")
	#pragma comment(lib, "Crypt32.lib")
#endif
#if defined(_WIN32) || defined(_WIN64)
typedef struct __WIN32_CRYPT_CTX {
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	DWORD dwLen;
} __WIN32_CRYPT_CTX;

static BOOL __win32_crypt_init(struct __WIN32_CRYPT_CTX* ctx) {
	ctx->hProv = ctx->hHash = 0;
	return CryptAcquireContext(&ctx->hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
}
static BOOL __win32_crypt_hash_create(struct __WIN32_CRYPT_CTX* ctx, DWORD Algid) {
	DWORD dwDataLen = sizeof(ctx->dwLen);
	if (!CryptCreateHash(ctx->hProv, Algid, 0, 0, &ctx->hHash)) {
		return FALSE;
	}
	return CryptGetHashParam(ctx->hHash, HP_HASHSIZE, (BYTE*)(&ctx->dwLen), &dwDataLen, 0);
}
static BOOL __win32_crypt_update(struct __WIN32_CRYPT_CTX* ctx, const BYTE* data, DWORD len) {
	return CryptHashData(ctx->hHash, (const BYTE*)data, len, 0);
}
static BOOL __win32_crypt_final(unsigned char* data, struct __WIN32_CRYPT_CTX* ctx) {
	return CryptGetHashParam(ctx->hHash, HP_HASHVAL, data, &ctx->dwLen, 0);
}
static BOOL __win32_crypt_clean(struct __WIN32_CRYPT_CTX* ctx) {
	if (ctx->hHash && !CryptDestroyHash(ctx->hHash)) {
		return FALSE;
	}
	return CryptReleaseContext(ctx->hProv, 0);
}
#endif
*/

#ifdef	__cplusplus
}
#endif
