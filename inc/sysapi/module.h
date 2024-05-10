//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MODULE_H
#define	UTIL_C_SYSLIB_MODULE_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <libloaderapi.h>
#else
	#include <dlfcn.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
	#define	moduleLoad(path)							((void*)LoadLibraryA(path))
	#define	moduleSymbolAddress(module, symbol_name)	((void*)GetProcAddress((HMODULE)module, symbol_name))
	#define	moduleUnload(module)						(module ? FreeLibrary((HMODULE)module) : TRUE)
#else
	#define	moduleLoad(path)							dlopen(path, RTLD_NOW)
	#define	moduleSymbolAddress(module, symbol_name)	dlsym(module, symbol_name)
	#define	moduleUnload(module)						(module ? (dlclose(module) == 0) : 1)
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void* moduleGetSelf(void);

#ifdef	__cplusplus
}
#endif

#endif
