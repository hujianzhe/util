//
// Created by hujianzhe
//

#include "../../inc/datastruct/sort.h"

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

SortInsertTopN_t* sortInsertTopN(void* top, void* new_, SortInsertTopN_t* arg) {
	unsigned char* p = (unsigned char*)top;
	unsigned char* pp = p;
	ptrlen_t i;
	for (i = 0; i < arg->ecnt; ++i, p += arg->esize) {
		if (arg->cmp(p, new_) == new_)
			break;
	}
	if (i < arg->ecnt) {
		pp += arg->esize * arg->ecnt;
		if (arg->ecnt >= arg->N) {
			pp -= arg->esize;
			arg->has_discard = 1;
			if (arg->discard_bak)
				__byte_copy(arg->discard_bak, pp, arg->esize);
		}
		else {
			arg->ecnt++;
			arg->has_discard = 0;
		}
		while (pp != p) {
			__byte_copy(pp, pp - arg->esize, arg->esize);
			pp -= arg->esize;
		}
		__byte_copy(p, new_, arg->esize);
		arg->insert_ok = 1;
	}
	else if (arg->ecnt < arg->N) {
		__byte_copy(p, new_, arg->esize);
		arg->ecnt++;
		arg->insert_ok = 1;
		arg->has_discard = 0;
	}
	else {
		arg->insert_ok = 0;
		arg->has_discard = 0;
	}
	return arg;
}

#ifdef	__cplusplus
}
#endif
