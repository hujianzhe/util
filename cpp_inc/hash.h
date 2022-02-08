//
// Created by hujianzhe on 22-2-7
//

#ifndef	UTIL_CPP_HASH_H
#define	UTIL_CPP_HASH_H

#include "cpp_compiler_define.h"
#if	__CPP_VERSION >= 2011
#include <functional>
#else
#include "unique_ptr.h"
#include "shared_ptr.h"
#include <string>

namespace std11 {
template <typename T, typename Y = void>
struct hash {
	size_t operator()(T v) const {
		return (size_t)v;
	};
};

template <>
struct hash<char*> {
	size_t operator()(const char* __s) const {
		unsigned long __h = 0;
		for ( ; *__s; ++__s) {
			__h = 5 * __h + *__s;
		}
		return size_t(__h);
	}
};

template<>
struct hash<const char*> {
	size_t operator()(const char* __s) const {
		unsigned long __h = 0;
		for ( ; *__s; ++__s) {
			__h = 5 * __h + *__s;
		}
		return size_t(__h);
	}
};

template <>
struct hash<std::string> {
	size_t operator()(const std::string& v) const {
		const char* __s = v.data();
		unsigned long __h = 0;
		for (size_t i = 0; i < v.size(); ++__s, ++i) {
			__h = 5 * __h + *__s;
		}
		return size_t(__h);
	}
};
}
#endif

#endif
