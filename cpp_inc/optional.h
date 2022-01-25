//
// Created by hujianzhe on 22-1-24
//

#ifndef	UTIL_CPP_OPTIONAL_H
#define	UTIL_CPP_OPTIONAL_H

#include "cpp_compiler_define.h"
#if	__CPP_VERSION >= 2017
#include <optional>
#else
#include <stdio.h>
#include <exception>
#include <stdexcept>

namespace std {
class bad_optional_access : public std::exception {
public:
	virtual const char* what(void) const throw() {
		return "bad_optional_access";
	}
};

typedef struct nullopt_t {} nullopt_t;
extern const nullopt_t nullopt;

template <typename T>
class optional {
public:
	typedef T value_type;

	optional() : m_hasValue(false) {}
	optional(nullopt_t v) : m_hasValue(false) {}

	template <typename U>
	optional(const U& v) : m_value(v), m_hasValue(true) {}

	template <typename U>
	optional(const optional<U>& other) :
		m_hasValue(other.m_hasValue)
	{
		if (m_hasValue) {
			m_value = other.m_value;
		}
	}

	template <typename U>
	optional& operator=(const optional<U>& other) {
		if (this == &other) {
			return *this;
		}
		reset();
		m_hasValue = other.m_hasValue;
		if (other.m_hasValue) {
			m_value = other.m_value;
		}
		return *this;
	}

	optional& operator=(nullopt_t v) {
		reset();
		return *this;
	}

	void reset() {
		if (!m_hasValue) {
			return;
		}
		m_hasValue = false;
		m_value.~T();
	}

	bool has_value() const { return m_hasValue; }
	operator bool() const { return m_hasValue; }

	T& value() {
		if (m_hasValue) {
			return m_value;
		}
		throw bad_optional_access();
	}
	const T& value() const {
		if (m_hasValue) {
			return m_value;
		}
		throw bad_optional_access();
	}

	template <typename U>
	T value_or(const U& v) const {
		if (m_hasValue) {
			return m_value;
		}
		return v;
	}

	T& operator*() { return m_value; }
	const T& operator*() const { return m_value; }

	T* operator->() {
		return m_hasValue ? &m_value : (T*)0;
	}
	const T* operator->() const {
		return m_hasValue ? &m_value : (const T*)0;
	}

private:
	T m_value;
	bool m_hasValue;
};
}
#endif

#endif
