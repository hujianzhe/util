//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPILER_DEFINE_H
#define	UTIL_C_COMPILER_DEFINE_H

#define	__MACRO_TOSTRING(m)							#m
#define	__MACRO_APPEND(m1, m2)						m1##m2

#define	MACRO_TOSTRING(m)							__MACRO_TOSTRING(m)
#define	MACRO_APPEND(m1, m2)						__MACRO_APPEND(m1, m2)

#ifndef STATIC_ASSERT
	#define	STATIC_ASSERT(exp,msg)					typedef char MACRO_APPEND(__static_assert_line, __LINE__)[(exp) ? 1 : -1]
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
	#pragma warning(disable:6255)
	#pragma warning(disable:26451)

	#define	__declspec_align(alignment)				__declspec(align(alignment))
	#define	__declspec_code_seg(name)				__declspec(code_seg(name))
	#define	__declspec_data_seg(name)				__pragma(data_seg(name))
	#define	__declspec_bss_seg(name)				__pragma(bss_seg(name))
	#define	__declspec_const_seg(name)				__pragma(const_seg(name))

	#define	__declspec_dllexport					__declspec(dllexport)
	#define	__declspec_dllimport					__declspec(dllimport)

	#ifdef	DECLSPEC_DLL_EXPORT
		#define	__declspec_dll						__declspec_dllexport
	#elif	DECLSPEC_DLL_IMPORT
		#define	__declspec_dll						__declspec_dllimport
	#else
		#define	__declspec_dll
	#endif

	#define	__declspec_noinline						__declspec(noinline)

	#ifdef	_WIN64
		typedef __int64								SignedPtr_t;
		typedef	unsigned __int64					UnsignedPtr_t;
	#elif	_WIN32
		typedef	__int32								SignedPtr_t;
		typedef	unsigned __int32					UnsignedPtr_t;
	#endif
	STATIC_ASSERT(sizeof(SignedPtr_t) == sizeof(void*), "");
	STATIC_ASSERT(sizeof(UnsignedPtr_t) == sizeof(void*), "");

#elif	defined(__GNUC__) || defined(__GNUG__)
	#ifndef NDEBUG	/* ANSI define */
		#ifndef _DEBUG
			#define	_DEBUG	/* same as VC */
		#endif
	#else
		#undef	_DEBUG	/* same as VC */
	#endif

	#define	__declspec_align(alignment)				__attribute__ ((aligned(alignment)))
	#define	__declspec_code_seg(name)				__attribute__((section(name)))
	#define	__declspec_data_seg(name)				__attribute__((section(name)))
	#define	__declspec_bss_seg(name)				__attribute__((section(name)))
	#define	__declspec_const_seg(name)				__attribute__((section(name)))

	#define	__declspec_dllexport
	#define	__declspec_dllimport
	#define	__declspec_dll
	#define	__declspec_noinline						__attribute__ ((noinline))

	STATIC_ASSERT(sizeof(long) == sizeof(void*), "");
	STATIC_ASSERT(sizeof(unsigned long) == sizeof(void*), "");
	typedef	long									SignedPtr_t;
	typedef	unsigned long							UnsignedPtr_t;
	#ifdef	__clang__
		#if	__has_feature(address_sanitizer)
			#ifndef	__SANITIZE_ADDRESS__
			#define	__SANITIZE_ADDRESS__
			#endif
		#endif
	#endif

#else
	#define	__declspec_noinline
	#error "Unknown Compiler"
#endif

#endif
