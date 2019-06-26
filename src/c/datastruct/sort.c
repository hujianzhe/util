//
// Created by hujianzhe
//

#include "../../../inc/c/datastruct/sort.h"

#ifdef	__cplusplus
extern "C" {
#endif

static void* __byte_copy(void* dst, const void* src, ptrlen_t sz) {
	unsigned char* pdst = (unsigned char*)dst;
	unsigned char* psrc = (unsigned char*)src;
	while (sz--) {
		*pdst = *psrc;
		++pdst;
		++psrc;
	}
	return dst;
}

void sortMergeOrder(void* p_, ptrlen_t icnt, const void* p1_, ptrlen_t icnt1, const void* p2_, ptrlen_t icnt2, ptrlen_t esize, const void*(*cmp)(const void*, const void*)) {
	unsigned char* p = (unsigned char*)p_;
	const unsigned char* p1 = (const unsigned char*)p1_;
	const unsigned char* p2 = (const unsigned char*)p2_;
	ptrlen_t i = 0, i1 = 0, i2 = 0;
	while (i < icnt && i1 < icnt1 && i2 < icnt2) {
		if (cmp(p1, p2) == p1) {
			__byte_copy(p, p1, esize);
			p1 += esize;
			++i1;
		}
		else {
			__byte_copy(p, p2, esize);
			p2 += esize;
			++i2;
		}
		p += esize;
		++i;
	}
	if (i >= icnt)
		return;
	else if (i1 < icnt1) {
		for (; i < icnt && i1 < icnt1; ++i1, p1 += esize, p += esize) {
			__byte_copy(p, p1, esize);
		}
	}
	else {
		for (; i < icnt && i2 < icnt2; ++i2, p2 += esize, p += esize) {
			__byte_copy(p, p2, esize);
		}
	}
}

int sortInsertTopN(void* p_, ptrlen_t icnt, ptrlen_t topn, const void* new_, ptrlen_t esize, int(*less)(const void*, const void*)) {
	unsigned char* p = (unsigned char*)p_;
	unsigned char* pp = p;
	unsigned char* pnew = (unsigned char*)new_;
	ptrlen_t i;
	for (i = 0; i < icnt; ++i, p += esize) {
		if (!less(p, pnew))
			break;
	}
	if (i < icnt) {
		pp += esize * icnt;
		if (icnt >= topn)
			pp -= esize;
		while (pp != p) {
			__byte_copy(pp, pp - esize, esize);
			pp -= esize;
		}
		__byte_copy(p, pnew, esize);
		return icnt < topn;
	}
	else if (icnt < topn) {
		__byte_copy(p, pnew, esize);
		return 1;
	}
	return 0;
}

#ifdef	__cplusplus
}
#endif
