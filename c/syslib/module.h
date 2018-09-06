//
// Created by hujianzhe
//

#ifndef UTIL_C_SYSLIB_MODULE_H
#define	UTIL_C_SYSLIB_MODULE_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	#define	moduleLoad(path)							(path ? (void*)LoadLibraryA(path) : (void*)GetModuleHandleA(NULL))
	#define	moduleSymbolAddress(module, symbol_name)	GetProcAddress(module, symbol_name)
	#define	moduleUnload(module)						(module ? FreeLibrary(module) : TRUE)
#else
	#include <dlfcn.h>
	#define	moduleLoad(path)							dlopen(path, RTLD_NOW)
	#define	moduleSymbolAddress(module, symbol_name)	dlsym(module, symbol_name)
	#define	moduleUnload(module)						(module ? (dlclose(module) == 0) : 1)
#endif

#endif
