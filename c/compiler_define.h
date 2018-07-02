//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPILER_DEFINE_H
#define	UTIL_C_COMPILER_DEFINE_H

#define	COMPILER_TIME_CHECK(exp)					typedef char __COMPILER_TIME_CHECK__[(exp) ? 1 : -1]

COMPILER_TIME_CHECK(sizeof(char) == 1);
COMPILER_TIME_CHECK(sizeof(signed char) == 1);
COMPILER_TIME_CHECK(sizeof(unsigned char) == 1);
COMPILER_TIME_CHECK(sizeof(short) == 2);
COMPILER_TIME_CHECK(sizeof(unsigned short) == 2);
COMPILER_TIME_CHECK(sizeof(int) == 4);
COMPILER_TIME_CHECK(sizeof(unsigned int) == 4);
COMPILER_TIME_CHECK(sizeof(long long) == 8);
COMPILER_TIME_CHECK(sizeof(unsigned long long) == 8);

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
