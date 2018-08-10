//
// Created by hujianzhe
//

#ifndef	UTIL_C_SYSLIB_ATOMIC_H
#define	UTIL_C_SYSLIB_ATOMIC_H

#include "../platform_define.h"

#if defined(_WIN32) || defined(_WIN64)
	typedef CHAR volatile					Atom8_t;
	typedef SHORT volatile					Atom16_t;
	typedef	LONG volatile					Atom32_t;
	typedef LONGLONG volatile				Atom64_t;
	#define	_xchg8(addr, val8)				InterlockedExchange8(addr, val8)
	#define	_xchg16(addr, val16)			InterlockedExchange16(addr, val16)
	#define	_xchg32(addr, val32)			InterlockedExchange(addr, val32)
	#define	_xchg64(addr, val64)			InterlockedExchange64(addr, val64)
	#define	_cmpxchg16(addr, val16, cmp16)	InterlockedCompareExchange16(addr, val16, cmp16)
	#define	_cmpxchg32(addr, val32, cmp32)	InterlockedCompareExchange(addr, val32, cmp32)
	#define	_cmpxchg64(addr, val64, cmp64)	InterlockedCompareExchange64(addr, val64, cmp64)
	#define	_xadd32(addr, val32)			InterlockedExchangeAdd(addr, val32)
	#define	_xadd64(addr, val64)			InterlockedExchangeAdd64(addr, val64)
	#ifdef	_WIN64
		#define	_xchgsize					_xchg64
		#define	_cmpxchgsize				_cmpxchg64
		#define	_xaddsize					_xadd64
	#else
		#define	_xchgsize					_xchg32
		#define	_cmpxchgsize				_cmpxchg32
		#define	_xaddsize					_xadd32
	#endif
#else
	typedef signed char volatile			Atom8_t;
	typedef	short volatile					Atom16_t;
	typedef	int volatile					Atom32_t;
	typedef long long volatile				Atom64_t;
	#define	_xchg8(addr, val8)				__sync_lock_test_and_set((signed char volatile*)(addr), (signed char)(val8))
	#define	_xchg16(addr, val16)			__sync_lock_test_and_set((short volatile*)(addr), (short)(val16))
	#define	_xchg32(addr, val32)			__sync_lock_test_and_set((int volatile*)(addr), (int)(val32))
	#define	_xchg64(addr, val64)			__sync_lock_test_and_set((long long volatile*)(addr), (long long)(val64))
	#define	_xchgsize(addr, val)			__sync_lock_test_and_set((ssize_t volatile*)(addr), (ssize_t)(val))
	#define	_cmpxchg16(addr, val16, cmp16)	__sync_val_compare_and_swap((short volatile*)(addr), (short)(cmp16), (short)(val16))
	#define	_cmpxchg32(addr, val32, cmp32)	__sync_val_compare_and_swap((int volatile*)(addr), (int)(cmp32), (int)(val32))
	#define	_cmpxchg64(addr, val64, cmp64)	__sync_val_compare_and_swap((long long volatile*)(addr), (long long)(cmp64), (long long)(val64))
	#define	_cmpxchgsize(addr, val, cmp)	__sync_val_compare_and_swap((ssize_t volatile*)(addr), (ssize_t)(cmp), (ssize_t)(val))
	#define	_xadd32(addr, val32)			__sync_fetch_and_add((int volatile*)(addr), (int)(val32))
	#define	_xadd64(addr, val64)			__sync_fetch_and_add((long long volatile*)(addr), (long long)(val64))
	#define	_xaddsize(addr, val)			__sync_fetch_and_add((ssize_t volatile*)(addr), (ssize_t)(val))
#endif

typedef	Atom8_t								AtomBool_t;

#endif