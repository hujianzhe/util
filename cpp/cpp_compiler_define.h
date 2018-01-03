//
// Created by hujianzhe
//

#ifndef UTIL_CPP_CPP_COMPILER_DEFINE_H
#define	UTIL_CPP_CPP_COMPILER_DEFINE_H

#include "../c/compiler_define.h"

#ifdef _MSC_VER
	#if	_MSC_VER >= 1900
		#define	__CPP_VERSION	2011
	#else
		#define	__CPP_VERSION	1998
	#endif

#elif	defined(__GNUC__) || defined(__GNUG__)
	#if	__cplusplus > 199711L
		#define	__CPP_VERSION	2011
	#else
		#define	__CPP_VERSION	1998
	#endif

#else
	#error "Unknown Compiler"
#endif

#endif // !UTIL_CPP_CPP_COMPILER_DEFINE_H
