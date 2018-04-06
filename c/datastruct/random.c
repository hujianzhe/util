//
// Created by hujianzhe
//

#include "random.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* linear congruential */
#define	N					16
#define MASK				((1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)				((unsigned)(x) & MASK)
#define HIGH(x)				LOW((x) >> N)
#define MUL(x, y, z)		{ int l = (long)(x) * (long)(y); (z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)			((int)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)		(z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0					0x330E
#define X1					0xABCD
#define X2					0x1234
#define A0					0xE66D
#define A1					0xDEEC
#define A2					0x5
#define C					0xB
#define SET3(x, x0, x1, x2)	((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
void rand48_seed(Rand48_t* ctx, int seedval) {
	ctx->x[0] = X0;
	ctx->x[1] = LOW(seedval);
	ctx->x[2] = HIGH(seedval);

	ctx->a[0] = A0;
	ctx->a[1] = A1;
	ctx->a[2] = A2;

	ctx->c = C;
}
int rand48_l(Rand48_t* ctx) {
	unsigned int p[2], q[2], r[2], carry0, carry1;

	MUL(ctx->a[0], ctx->x[0], p);
	ADDEQU(p[0], ctx->c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(ctx->a[0], ctx->x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(ctx->a[1], ctx->x[0], r);
	ctx->x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
				ctx->a[0] * ctx->x[2] + ctx->a[1] * ctx->x[1] + ctx->a[2] * ctx->x[0]);
	ctx->x[1] = LOW(p[1] + r[0]);
	ctx->x[0] = LOW(p[0]);
	return (((int)(ctx->x[2]) << (N - 1)) + (ctx->x[1] >> 1));
}
int rand48_range(Rand48_t* ctx, int start, int end) {
	/* [start, end) */
	return rand48_l(ctx) % (end - start) + start;
}

/* mt19937 */
void mt19937_seed(RandMT19937_t* ctx, int seedval) {
	int i;
	unsigned long long* x = ctx->x;
	x[0] = seedval;
	for (i = 1; i < sizeof(ctx->x) / sizeof(ctx->x[0]); i++)
		x[i] = 6364136223846793005ULL * (x[i - 1] ^ (x[i - 1] >> (64 - 2))) + i;
	ctx->k = 0;
}
unsigned long long mt19937_ull(RandMT19937_t* ctx) {
	int k = ctx->k;
	unsigned long long* x = ctx->x;
	unsigned long long y, z;

	/* z = (x^u_k | x^l_(k+1))*/
	z = (x[k] & 0xffffffff80000000ULL) | (x[(k + 1) % 312] & 0x7fffffffULL);
	/* x_(k+n) = x_(k+m) |+| z*A */
	x[k] = x[(k + 156) % 312] ^ (z >> 1) ^ (!(z & 1ULL) ? 0ULL : 0xb5026f5aa96619e9ULL);
	/* Tempering */
	y = x[k];
	y ^= (y >> 29) & 0x5555555555555555ULL;
	y ^= (y << 17) & 0x71d67fffeda60000ULL;
	y ^= (y << 37) & 0xfff7eee000000000ULL;
	y ^= y >> 43;

	ctx->k = (k + 1) % 312;
	return y;
}
long long mt19937_range(RandMT19937_t* ctx, long long start, long long end) {
	/* [start, end) */
	return mt19937_ll(ctx) % (end - start) + start;
}

#ifdef	__cplusplus
}
#endif