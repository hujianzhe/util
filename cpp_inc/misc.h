//
// Created by hujianzhe on 22-3-16
//

#ifndef	UTIL_CPP_MISC_H
#define	UTIL_CPP_MISC_H

#include <string.h>

namespace util {
template <typename T>
struct cstruct_wrap : public T {
	virtual ~cstruct_wrap() {}

	T* c_ptr() { return this; }
	const T* c_ptr() const { return this; }

	void bzero() {
		memset((T*)this, 0, sizeof(T));
	}
};

template <typename T>
void delete_fn(void* p) { delete (T*)p; }
template <typename T>
void delete_fn(T* p) { delete p; }
template <typename T>
void delete_arr_fn(T* p) { delete [] p; }
template <typename T>
void delete_arr_fn(void* p) { delete [] (T*)p; }
template <typename T>
void free_fn(T* p) { free((void*)p); }
}

#endif
