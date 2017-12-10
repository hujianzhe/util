//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPILER_DEFINE_H
#define	UTIL_C_COMPILER_DEFINE_H

#if defined(_WIN32) || defined(_WIN64)
	#ifdef _MSC_VER
		#ifdef	_WIN64
			typedef long long	ssize_t;
		#else
			typedef long 		ssize_t;
		#endif
		#pragma warning(disable:4200)
		#pragma warning(disable:4018)
		#pragma warning(disable:4244)
		#pragma warning(disable:4267)
		#pragma warning(disable:4800)
		#pragma warning(disable:4819)
		#ifndef _CRT_SECURE_NO_WARNINGS
			#define	_CRT_SECURE_NO_WARNINGS
		#endif
		#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
			#define	_WINSOCK_DEPRECATED_NO_WARNINGS
		#endif
	#endif
#else
	#ifdef	__APPLE__
		#ifndef	_XOPEN_SOURCE
			#define	_XOPEN_SOURCE
		#endif
	#elif	__linux__
		#ifndef	_GNU_SOURCE
			#define _GNU_SOURCE
		#endif
	#endif
#endif

#endif