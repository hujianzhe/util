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
	#define	xchg8(addr, val8)				InterlockedExchange8(addr, val8)
	#define	xchg16(addr, val16)				InterlockedExchange16(addr, val16)
	#define	xchg32(addr, val32)				InterlockedExchange(addr, val32)
	#define	xchg64(addr, val64)				InterlockedExchange64(addr, val64)
	#define	cmpxchg16(addr, val16, cmp16)	InterlockedCompareExchange16(addr, val16, cmp16)
	#define	cmpxchg32(addr, val32, cmp32)	InterlockedCompareExchange(addr, val32, cmp32)
	#define	cmpxchg64(addr, val64, cmp64)	InterlockedCompareExchange64(addr, val64, cmp64)
	#define	xadd32(addr, val32)				InterlockedExchangeAdd(addr, val32)
	#define	xadd64(addr, val64)				InterlockedExchangeAdd64(addr, val64)
	#ifdef	_WIN64
		#define	xchgsize					_xchg64
		#define	cmpxchgsize					_cmpxchg64
		#define	xaddsize					_xadd64
		typedef	Atom64_t					AtomSSize_t;
	#else
		#define	xchgsize					_xchg32
		#define	cmpxchgsize					_cmpxchg32
		#define	xaddsize					_xadd32
		typedef	Atom32_t					AtomSSize_t;
	#endif
	#define	memoryBarrier()					MemoryBarrier()
	#define	memoryBarrierAcquire()
	#define	memoryBarrierRelease()
#else
	typedef signed char volatile			Atom8_t;
	typedef	short volatile					Atom16_t;
	typedef	int volatile					Atom32_t;
	typedef long long volatile				Atom64_t;
	typedef	ssize_t volatile				AtomSSize_t;
	#define	xchg8(addr, val8)				__sync_lock_test_and_set((signed char volatile*)(addr), (signed char)(val8))
	#define	xchg16(addr, val16)				__sync_lock_test_and_set((short volatile*)(addr), (short)(val16))
	#define	xchg32(addr, val32)				__sync_lock_test_and_set((int volatile*)(addr), (int)(val32))
	#define	xchg64(addr, val64)				__sync_lock_test_and_set((long long volatile*)(addr), (long long)(val64))
	#define	xchgsize(addr, val)				__sync_lock_test_and_set((ssize_t volatile*)(addr), (ssize_t)(val))
	#define	cmpxchg16(addr, val16, cmp16)	__sync_val_compare_and_swap((short volatile*)(addr), (short)(cmp16), (short)(val16))
	#define	cmpxchg32(addr, val32, cmp32)	__sync_val_compare_and_swap((int volatile*)(addr), (int)(cmp32), (int)(val32))
	#define	cmpxchg64(addr, val64, cmp64)	__sync_val_compare_and_swap((long long volatile*)(addr), (long long)(cmp64), (long long)(val64))
	#define	cmpxchgsize(addr, val, cmp)		__sync_val_compare_and_swap((ssize_t volatile*)(addr), (ssize_t)(cmp), (ssize_t)(val))
	#define	xadd32(addr, val32)				__sync_fetch_and_add((int volatile*)(addr), (int)(val32))
	#define	xadd64(addr, val64)				__sync_fetch_and_add((long long volatile*)(addr), (long long)(val64))
	#define	xaddsize(addr, val)				__sync_fetch_and_add((ssize_t volatile*)(addr), (ssize_t)(val))
	#define	memoryBarrier()					__sync_synchronize()
	#define memoryBarrierAcquire()			do { volatile int v; __sync_lock_test_and_set(&v, 0); } while (0)
	#define memoryBarrierRelease()			do { volatile int v; __sync_lock_release(&v); } while (0)
#endif

typedef	Atom8_t								AtomBool_t;

#endif
