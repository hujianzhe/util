//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPILER_DEFINE_H
#define	UTIL_C_COMPILER_DEFINE_H

#define	compiler_check(exp)							typedef char __compile_check__[(exp) ? 1 : -1]

#define	field_sizeof(type, field)					sizeof(((type*)0)->field)
#define	pod_offsetof(type, field)					((char*)(&((type *)0)->field) - (char*)(0))
#define pod_container_of(address, type, field)		((type *)((char*)(address) - (char*)(&((type *)0)->field)))

#ifndef __cplusplus
typedef	unsigned char	bool;
#define	true			1
#define	false			0
#endif
#define	undefined		-1

#ifdef _MSC_VER
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
	#define	embed_asm(exp)						__asm {exp}
	#define	__declspec_align(alignment)			__declspec(align(alignment))

	#define	__declspec_code_seg(name)			__declspec(code_seg(name))
	#define	__declspec_data_seg(name)			__pragma(data_seg(name))
	#define	__declspec_bss_seg(name)			__pragma(bss_seg(name))
	#define	__declspec_const_seg(name)			__pragma(const_seg(name))

#elif	defined(__GNUC__) || defined(__GNUG__)
	#ifndef NDEBUG	/* ANSI define */
		#ifndef _DEBUG
			#define	_DEBUG	/* same as VC */
		#endif
	#else
		#undef	_DEBUG	/* same as VC */
	#endif
	#ifndef	_XOPEN_SOURCE
		#define	_XOPEN_SOURCE
	#endif
	#ifdef	__linux__
		#ifndef	_GNU_SOURCE
			#define _GNU_SOURCE
		#endif
	#endif
	#define	embed_asm(exp)						asm __volatile__(exp)
	#define	__declspec_align(alignment)			__attribute__ ((aligned(alignment)))

	#define	__declspec_code_seg(name)			__attribute__((section(name)))
	#define	__declspec_data_seg(name)			__attribute__((section(name)))
	#define	__declspec_bss_seg(name)			__attribute__((section(name)))
	#define	__declspec_const_seg(name)			__attribute__((section(name)))

#else
	#error "Unknown Compiler"
#endif

#endif
