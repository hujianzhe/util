//
// Created by hujianzhe on 23-4-26.
//

#ifndef UTIL_CPP_DEFAULT_DELETE_H
#define	UTIL_CPP_DEFAULT_DELETE_H

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <memory>
#else
namespace std11 {
template<typename T>
struct default_delete {
	void operator()(T* ptr) const { delete ptr; }
};

template<typename T>
struct default_delete<T[]> {
	void operator()(T *ptr) const { delete[] ptr; }
};
}
#endif

#endif
