//
// Created by hujianzhe
//

#ifndef UTIL_CPP_CPP_COMPILER_DEFINE_H
#define	UTIL_CPP_CPP_COMPILER_DEFINE_H

#ifdef __cplusplus

#include "../inc/compiler_define.h"

#ifdef _MSC_VER
	#ifndef	__CPP_VERSION
		#define	__CPP_VERSION	_MSVC_LANG
	#endif
#else
	#ifndef	__CPP_VERSION
		#define	__CPP_VERSION	__cplusplus
	#endif
#endif

#endif

#endif
