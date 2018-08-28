//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_RANDOM_H
#define	UTIL_C_DATASTRUCT_RANDOM_H

#ifdef	__cplusplus
extern "C" {
#endif

/* linear congruential */
typedef struct Rand48_t { unsigned int x[3], a[3], c; } Rand48_t;
void rand48Seed(Rand48_t* ctx, int seedval);
int rand48_l(Rand48_t* ctx);
#define	rand48_ul(ctx)	((unsigned int)rand48_l(ctx))
int rand48Range(Rand48_t* ctx, int start, int end);
/* mt19937 */
typedef struct RandMT19937_t { unsigned long long x[312]; int k; } RandMT19937_t;
void mt19937Seed(RandMT19937_t* ctx, int seedval);
unsigned long long mt19937_ull(RandMT19937_t* ctx);
#define	mt19937_ll(ctx)	((long long)mt19937_ull(ctx))
long long mt19937Range(RandMT19937_t* ctx, long long start, long long end);

#ifdef	__cplusplus
}
#endif

#endif