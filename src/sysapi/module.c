//
// Created by hujianzhe
//

#include "../../inc/sysapi/module.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#else
static void empty_dladdr() {}
#endif

void* moduleGetSelf(void) {
#if defined(_WIN32) || defined(_WIN64)
	return (void*)GetModuleHandle(NULL);
#else
	Dl_info dl_info;
	if (dladdr((void*)empty_dladdr, &dl_info)) {
		return dlopen(dl_info.dli_fname, RTLD_NOW);
	}
	return NULL;
#endif
}

#ifdef	__cplusplus
}
#endif
