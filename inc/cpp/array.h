//
// Created by hujianzhe on 20-9-1
//

#ifndef	UTIL_CPP_ARRAY_H
#define UTIL_CPP_ARRAY_H

#ifdef	__cplusplus
#include "cpp_compiler_define.h"
#if	__CPP_VERSION >= 2011
#include <array>
#else
#include <stdio.h>
#include <exception>
#include <stdexcept>
namespace std {
template <typename T, size_t N>
struct array {
// public:
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	void fill(const T& v) {
		for (size_t i = 0; i < N; ++i) {
			e[i] = v;
		}
	}
	void swap(array<T, N>& other) {
		for (size_t i = 0; i < N; ++i) {
			T t = e[i];
			e[i] = other[i];
			other[i] = t;
		}
	}

	T& operator[](size_t pos) { return e[pos]; }
	const T& operator[](size_t pos) const { return e[pos]; }
	T& at(size_t pos) {
		if (pos >= N) {
			char err[128];
			snprintf(err, sizeof(err), "array::at: __n (which is %zu) >= _Nm (which is %zu)", pos, N);
			throw std::out_of_range("std::array pos >= N");
		}
		return e[pos];
	}
	const T& at(size_t pos) const {
		if (pos >= N) {
			char err[128];
			snprintf(err, sizeof(err), "array::at: __n (which is %zu) >= _Nm (which is %zu)", pos, N);
			throw std::out_of_range("std::array pos >= N");
		}
		return e[pos];
	}
	T& front(void) { return at(0); }
	const T& front(void) const { return at(0); }
	T& back(void) { return at(N - 1); }
	const T& back(void) const { return at(N - 1); }
	T* data(void) { return e; }
	const T* data(void) const  { return e; }

	bool empty(void) const { return N == 0; }
	size_t size(void) const { return N; }
	size_t max_size(void) const { return N; }

// private:
	T e[N?N:1];
};

template <typename T, size_t N>
bool operator==(const array<T, N>& lhs, const array<T, N>& rhs) {
	for (size_t i = 0; i < N; ++i) {
		if (lhs[i] != rhs[i])
			return false;
	}
	return true;
}
template <typename T, size_t N>
bool operator!=(const array<T, N>& lhs, const array<T, N>& rhs) { return !(lhs == rhs); }
}
#endif

#endif

#endif
