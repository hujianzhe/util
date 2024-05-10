//
// Created by hujianzhe
//

#include "../../inc/sysapi/module.h"

#ifdef	__cplusplus
extern "C" {
#endif

void* moduleGetAddress(const void* symbol_addr) {
#if defined(_WIN32) || defined(_WIN64)
	HMODULE m;
	if (!symbol_addr) {
		symbol_addr = (const void*)moduleGetAddress;
	}
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PTCHAR)symbol_addr, &m)) {
		return (void*)m;
	}
	return NULL;
#else
	Dl_info dl_info;
	if (dladdr((void*)symbol_addr, &dl_info)) {
		return dlopen(dl_info.dli_fname, RTLD_NOW);
	}
	return NULL;
#endif
}

#ifdef	__cplusplus
}
#endif
