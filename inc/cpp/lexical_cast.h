//
// Created by hujianzhe on 19-10-21.
//

#ifndef UTIL_CPP_LEXICAL_CAST_H
#define	UTIL_CPP_LEXICAL_CAST_H

#ifdef __cplusplus

#include "cpp_compiler_define.h"
#include <sstream>
#include <typeinfo>

namespace util {
class bad_lexical_cast : public std::bad_cast {
public:
	virtual const char* what(void) const throw() {
		return "bad lexical cast: source type could not be interpreted as target type";
	}
};

template <typename T, typename F>
T lexical_cast(const F& f) {
	std::stringstream ss;
	ss << f;
	if (!ss)
		throw bad_lexical_cast();
	T t;
	ss >> t;
	if (!ss)
		throw bad_lexical_cast();
	return t;
}

template <typename T, typename F>
T lexical_cast_nothrow(const F& f) {
	std::stringstream ss;
	ss << f;
	if (!ss)
		return T();
	T t;
	ss >> t;
	if (!ss)
		return T();
	return t;
}

template <typename F, typename T>
bool lexical_cast(const F& f, T& t) {
	std::stringstream ss;
	ss << f;
	if (!ss)
		return false;
	ss >> t;
	if (!ss)
		return false;
	return true;
}
}

#endif

#endif // !UTIL_CPP_LEXICAL_CAST_H