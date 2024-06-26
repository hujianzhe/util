//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_MODULE_H
#define	UTIL_C_SYSLIB_MODULE_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <libloaderapi.h>
#else
	#if	__linux__
	#include <link.h>
	#endif
	#include <dlfcn.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
	#define	moduleLoad(path)						((void*)LoadLibraryA(path))
	#define	moduleSymbolAddress(md, symbol_name)	((void*)GetProcAddress((HMODULE)md, symbol_name))
	#define	moduleUnload(md)						(md ? FreeLibrary((HMODULE)md) : TRUE)
#else
	#define	moduleLoad(path)						dlopen(path, RTLD_NOW)
	#define	moduleSymbolAddress(md, symbol_name)	dlsym(md, symbol_name)
	#define	moduleUnload(md)						(md ? (dlclose(md) == 0) : 1)
#endif

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll void* moduleAddress(const void* symbol_addr);
__declspec_dll size_t modulePathLength(void* md);
__declspec_dll size_t moduleFillPath(void* md, char* buf, size_t n);

#ifdef	__cplusplus
}
#endif

#endif
