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
UTIL_LIBAPI void rand48Seed(Rand48_t* ctx, int seedval);
UTIL_LIBAPI int rand48_l(Rand48_t* ctx);
#define	rand48_ul(ctx)	((unsigned int)rand48_l(ctx))
UTIL_LIBAPI int rand48Range(Rand48_t* ctx, int start, int end);
/* mt19937 */
typedef struct RandMT19937_t { unsigned long long x[312]; int k; } RandMT19937_t;
UTIL_LIBAPI void mt19937Seed(RandMT19937_t* ctx, int seedval);
UTIL_LIBAPI unsigned long long mt19937_ull(RandMT19937_t* ctx);
#define	mt19937_ll(ctx)	((long long)mt19937_ull(ctx))
UTIL_LIBAPI long long mt19937Range(RandMT19937_t* ctx, long long start, long long end);

#ifdef	__cplusplus
}
#endif

#endif