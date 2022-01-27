//
// Created by hujianzhe on 18-7-27
//

#ifndef	UTIL_CPP_NULLPTR_H
#define	UTIL_CPP_NULLPTR_H

#include "cpp_compiler_define.h"
#include <cstddef>

#if	__CPP_VERSION < 2011 && !defined(__clang__)
namespace std {
typedef struct nullptr_t {
	nullptr_t(void) : __p((void*)0) {}
	nullptr_t(const nullptr_t& p) : __p((void*)0) {}
	nullptr_t& operator=(const nullptr_t& p) {
		__p = (void*)0;
		return *this;
	}

	template <typename T>
		operator T*(void) const { return (T*)0; }
private:
	void* __p;
} nullptr_t;
}
#define	nullptr		(std::nullptr_t())
#endif

#endif
