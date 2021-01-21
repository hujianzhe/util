//
// Created by hujianzhe
//

#include "../../inc/datastruct/arrheap.h"

#ifdef	__cplusplus
extern "C" {
#endif

static void* _memcopy_(void* dst, const void* src, ptrlen_t n) {
	for (; n; --n)
		((unsigned char*)dst)[n - 1] = ((unsigned char*)src)[n - 1];
	return dst;
}

static void _swap_(void* one, void* two, ptrlen_t n) {
	char* p1 = (char*)one;
	char* p2 = (char*)two;
	while (n--) {
		char d = *p1;
		*p1 = *p2;
		*p2 = d;
		++p1, ++p2;
	}
}

SortHeap_t* sortheapInit(SortHeap_t* h, void* buf, ptrlen_t bufsize, ptrlen_t esize, int(*cmp)(const void*, const void*)) {
	h->ecnt = 0;
	h->esize = esize;
	h->bufsize = bufsize;
	h->buf = (unsigned char*)buf;
	h->cmp = cmp;
	return h;
}

int sortheapIsFull(SortHeap_t* h) { return h->bufsize <= h->ecnt * h->esize + h->esize; }
int sortheapIsEmpty(SortHeap_t* h) { return h->ecnt == 0; }
const void* sortheapTop(SortHeap_t* h) { return &h->buf[h->esize]; }

SortHeap_t* sortheapInsert(SortHeap_t* h, void* ptr_data) {
	ptrlen_t i = ++h->ecnt;
	ptr_data = _memcopy_(h->buf + i * h->esize, ptr_data, h->esize);
	while (i >>= 1) {
		void* ptr_parent = h->buf + i * h->esize;
		if (h->cmp(ptr_data, ptr_parent) > 0) {
			break;
		}
		_swap_(ptr_parent, ptr_data, h->esize);
		ptr_data = ptr_parent;
	}
	return h;
}

void sortheapPop(SortHeap_t* h) {
	ptrlen_t i;
	void* ptr_select_data;
	if (0 == h->ecnt) {
		return;
	}
	ptr_select_data = _memcopy_(h->buf + h->esize, h->buf + h->ecnt * h->esize, h->esize);
	h->ecnt--;
	i = 1;
	while ((i <<= 1) <= h->ecnt) {
		unsigned char* ptr_data = h->buf + i * h->esize;
		if (i + 1 <= h->ecnt) {
			unsigned char* ptr_rdata = ptr_data + h->esize;
			if (h->cmp(ptr_rdata, ptr_data) < 0) {
				ptr_data = ptr_rdata;
				i += 1;
			}
		}
		if (h->cmp(ptr_select_data, ptr_data) <= 0) {
			continue;
		}
		_swap_(ptr_select_data, ptr_data, h->esize);
		ptr_select_data = ptr_data;
	}
}

#ifdef	__cplusplus
}
#endif
