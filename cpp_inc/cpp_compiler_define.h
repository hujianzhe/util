//
// Created by hujianzhe
//

#ifndef UTIL_CPP_CPP_COMPILER_DEFINE_H
#define	UTIL_CPP_CPP_COMPILER_DEFINE_H

#ifdef __cplusplus

#include "../inc/compiler_define.h"

namespace std {}

#ifdef _MSC_VER
	#if		_MSVC_LANG > 201402L
		#define	__CPP_VERSION	2017
		namespace std17 = ::std;
		namespace std14 = ::std;
		namespace std11 = ::std;
	#elif	_MSVC_LANG > 201103L
		#define	__CPP_VERSION	2014
		namespace std14 = ::std;
		namespace std11 = ::std;
	#elif	_MSVC_LANG > 199711L
		#define	__CPP_VERSION	2011
		namespace std11 = ::std;
	#else
		#define	__CPP_VERSION	1998
	#endif

#elif	defined(__GNUC__) || defined(__GNUG__)
	#if		__cplusplus > 201402L
		#define	__CPP_VERSION	2017
		namespace std17 = ::std;
		namespace std14 = ::std;
		namespace std11 = ::std;
	#elif	__cplusplus > 201103L
		#define	__CPP_VERSION	2014
		namespace std14 = ::std;
		namespace std11 = ::std;
	#elif	__cplusplus > 199711L
		#define	__CPP_VERSION	2011
		namespace std11 = ::std;
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
	#define constexpr
	#define	override
	#define noexcept
#endif

#endif

#endif
