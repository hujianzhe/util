//
// Created by hujianzhe
//

#ifndef UTIL_CPP_CPP_COMPILER_DEFINE_H
#define	UTIL_CPP_CPP_COMPILER_DEFINE_H

#ifndef __cplusplus
	#error "Compiler isn't a C++"
#endif

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

#if __CPP_VERSION >= 2011
	#undef	STATIC_ASSERT
	#define	STATIC_ASSERT	static_assert
#else
	#define	override
#endif

#endif
