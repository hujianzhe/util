//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPILER_DEFINE_H
#define	UTIL_C_COMPILER_DEFINE_H

#define	compiler_check(exp)						typedef char __compile_check__[(exp) ? 1 : -1]

#define	field_sizeof(type, field)				sizeof(((type*)0)->field)
#define	field_offset(type, field)				((char*)(&((type *)0)->field) - (char*)(0))
#define field_container(address, type, field)	((type *)((char*)(address) - (char*)(&((type *)0)->field)))

typedef struct class_field_reflect_desc_t {
	const char* name;
	unsigned int offset;
	unsigned int len;
} class_field_reflect_desc_t;
#define	field_reflect_init(type, field)			{ #field, field_offset(type, field), field_sizeof(type, field) }
#define	class_reflect_declare(type, suffix)		extern struct class_field_reflect_desc_t type##_##suffix##_reflect_desc[]
#define	class_reflect_define(type, suffix)		struct class_field_reflect_desc_t type##_##suffix##_reflect_desc[]

#define	__ADD_LABEL_PREFIX__(_prefix, _label)	_prefix##_##_label

#ifndef __cplusplus
typedef	char	bool;
#define	true	1
#define	false	0
#endif

#ifdef _MSC_VER
	#if	_MSC_VER >= 1900
		#define	__CPP_VERSION	2011
	#else
		#define	__CPP_VERSION	1998
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
	#define	variable_align(alignment)			__declspec(align(alignment))
	#define	embed_asm(exp)						__asm {exp}

#elif	defined(__GNUC__) || defined(__GNUG__)
	#if	__cplusplus > 199711L
		#define	__CPP_VERSION	2011
	#else
		#define	__CPP_VERSION	1998
	#endif
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
	#define	variable_align(alignment)			__attribute__ ((aligned(alignment)))
	#define	embed_asm(exp)						asm __volatile__(exp)

#else
	#error "Unknown Compiler"
#endif

#endif