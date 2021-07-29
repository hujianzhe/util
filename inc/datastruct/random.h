//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RANDOM_H
#define	UTIL_C_DATASTRUCT_RANDOM_H

#include "../compiler_define.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* linear congruential */
typedef struct Rand48_t { unsigned int x[3], a[3], c; } Rand48_t;
__declspec_dll void rand48Seed(Rand48_t* ctx, int seedval);
__declspec_dll int rand48_l(Rand48_t* ctx);
#define	rand48_ul(ctx)	((unsigned int)rand48_l(ctx))
__declspec_dll int rand48Range(Rand48_t* ctx, int start, int end);
/* mt19937 */
typedef struct RandMT19937_t { unsigned long long x[312]; int k; } RandMT19937_t;
__declspec_dll void mt19937Seed(RandMT19937_t* ctx, int seedval);
__declspec_dll unsigned long long mt19937_ull(RandMT19937_t* ctx);
#define	mt19937_ll(ctx)	((long long)mt19937_ull(ctx))
__declspec_dll long long mt19937Range(RandMT19937_t* ctx, long long start, long long end);
/* random string */
__declspec_dll char* randAlphabetNumber(int seedval, char* s, UnsignedPtr_t length);

#ifdef	__cplusplus
}
#endif

#endif
