//
// Created by hujianzhe
//

#include "../../inc/sysapi/module.h"
#include <string.h>
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

static void module_empty_probe() {}

void* moduleAddress(const void* symbol_addr) {
#if defined(_WIN32) || defined(_WIN64)
	HMODULE m;
	if (!symbol_addr) {
		symbol_addr = (const void*)module_empty_probe;
	}
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PTCHAR)symbol_addr, &m)) {
		return (void*)m;
	}
	return NULL;
#else
	Dl_info dl_info;
	if (!symbol_addr) {
		symbol_addr = (const void*)module_empty_probe;
	}
	if (dladdr((void*)symbol_addr, &dl_info)) {
		return dlopen(dl_info.dli_fname, RTLD_NOW);
	}
	return NULL;
#endif
}

size_t modulePathLength(void* md) {
#if defined(_WIN32) || defined(_WIN64)
	size_t n = MAX_PATH;
	char* buf = NULL;
	do {
		DWORD ret;
		char* newbuf = (char*)realloc(buf, n);
		if (!newbuf) {
			break;
		}
		buf = newbuf;
		ret = GetModuleFileNameA((HMODULE)md, buf, n);
		if (!ret) {
			break;
		}
		if (ret < n) {
			free(buf);
			return ret;
		}
		n += MAX_PATH;
	} while (1);
	free(buf);
	return 0;
#elif	__linux__
	struct link_map *lm;
	if (dlinfo(md, RTLD_DI_LINKMAP, &lm)) {
		return 0;
	}
	return strlen(lm->l_name);
#else
	return 0;
#endif
}

size_t moduleFillPath(void* md, char* buf, size_t n) {
#if defined(_WIN32) || defined(_WIN64)
	DWORD ret = GetModuleFileNameA((HMODULE)md, buf, n);
	if (!ret) {
		return 0;
	}
	if (n > ret) {
		buf[ret] = 0;
		return ret;
	}
	else {
		buf[n - 1] = 0;
		return n - 1;
	}
#elif	__linux__
	struct link_map *lm;
	if (dlinfo(md, RTLD_DI_LINKMAP, &lm)) {
		return 0;
	}
	strncpy(buf, lm->l_name, n);
	buf[n - 1] = 0;
	return n - 1;
#else
	return 0;
#endif
}

#ifdef	__cplusplus
}
#endif
