//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPILER_DEFINE_H
#define	UTIL_C_COMPILER_DEFINE_H

#ifndef STATIC_ASSERT
	#define	STATIC_ASSERT(exp,msg)					extern char __STATIC_ASSERT__[(exp) ? 1 : -1]
#endif
STATIC_ASSERT(sizeof(char) == 1, "");
STATIC_ASSERT(sizeof(signed char) == 1, "");
STATIC_ASSERT(sizeof(unsigned char) == 1, "");
STATIC_ASSERT(sizeof(short) == 2, "");
STATIC_ASSERT(sizeof(unsigned short) == 2, "");
STATIC_ASSERT(sizeof(int) == 4, "");
STATIC_ASSERT(sizeof(unsigned int) == 4, "");
STATIC_ASSERT(sizeof(long long) == 8, "");
STATIC_ASSERT(sizeof(unsigned long long) == 8, "");

#define	field_sizeof(type, field)					sizeof(((type*)0)->field)
#define	pod_offsetof(type, field)					((char*)(&((type *)0)->field) - (char*)(0))
#define pod_container_of(address, type, field)		((type *)((char*)(address) - (char*)(&((type *)0)->field)))

#ifdef _MSC_VER
	#pragma warning(disable:4200)
	#pragma warning(disable:4018)
	#pragma warning(disable:4244)
	#pragma warning(disable:4267)
	#pragma warning(disable:4800)
	#pragma warning(disable:4819)
	#pragma warning(disable:4996)

	#define	embed_asm(exp)						__asm {exp}
	#define	__declspec_align(alignment)			__declspec(align(alignment))
	#define	__declspec_dllexport				__declspec(dllexport)
	#define	__declspec_dllimport				__declspec(dllimport)
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
	#ifndef	__linux__
		#define	__forceinline						inline __attribute__((always_inline))
	#else
		#define	__forceinline
	#endif

	#define	embed_asm(exp)						asm __volatile__(exp)
	#define	__declspec_align(alignment)			__attribute__ ((aligned(alignment)))
	#define	__declspec_dllexport
	#define	__declspec_dllimport
	#define	__declspec_code_seg(name)			__attribute__((section(name)))
	#define	__declspec_data_seg(name)			__attribute__((section(name)))
	#define	__declspec_bss_seg(name)			__attribute__((section(name)))
	#define	__declspec_const_seg(name)			__attribute__((section(name)))

#else
	#define	__forceinline
	#error "Unknown Compiler"
#endif

#endif
