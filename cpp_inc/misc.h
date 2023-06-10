//
// Created by hujianzhe on 22-3-16
//

#include <string.h>

namespace util {
template <typename T>
struct cstruct_wrap : public T {
	virtual ~cstruct_wrap() {}

	void bzero() {
		T* base = this;
		memset(base, 0, sizeof(T));
	}

	void copy(const void* p, size_t n) {
		T* base = this;
		if (base == p) {
			return;
		}
		memmove(base, p, n > sizeof(T) ? sizeof(T) : n);
	}
	void copy(const T* t) {
		T* base = this;
		if (base == t) {
			return;
		}
		memmove(base, t, sizeof(T));
	}
	void copy(const T& t) { copy(&t); }
};
}
