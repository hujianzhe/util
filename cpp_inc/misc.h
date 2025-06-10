//
// Created by hujianzhe on 22-3-16
//

#ifndef	UTIL_CPP_MISC_H
#define	UTIL_CPP_MISC_H

#include <string.h>
#include <vector>
#include <algorithm>
#include <utility>

namespace util {
template <typename T, void(*Deleter)(T*)>
struct cstruct_raii : public T {
	cstruct_raii() {}
	cstruct_raii(int ch) {
		memset((T*)this, ch, sizeof(T));
	}
	~cstruct_raii() {
		Deleter(this);
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

template <typename T, typename Alloc, typename U>
bool std_vector_erase_value_unordered(::std::vector<T, Alloc>* v, const U& u) {
	typename ::std::vector<T, Alloc>::iterator it = ::std::find(v->begin(), v->end(), u);
	if (it == v->end()) {
		return false;
	}
	::std::swap(*it, v->back());
	v->pop_back();
	return true;
}
template <typename T, typename Alloc>
bool std_vector_erase_idx_unordered(::std::vector<T, Alloc>* v, size_t idx) {
	if (idx >= v->size()) {
		return false;
	}
	::std::swap((*v)[idx], v->back());
	v->pop_back();
	return true;
}
}

#endif
