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

namespace std17 {
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

	optional() { set_value(false); }
	~optional() { reset(); }

	optional(nullopt_t v) { set_value(false); }

	template <typename U>
	optional(const U& v) {
		set_value(true);
		new (m_value) T(v);
	}

	optional(const optional& other) {
		set_value(other.has_value());
		if (other.has_value()) {
			new (m_value) T(*reinterpret_cast<T*>(other.m_value));
		}
	}

	template <typename U>
	optional(const optional<U>& other) {
		set_value(other.has_value());
		if (other.has_value()) {
			new (m_value) T(*reinterpret_cast<T*>(other.m_value));
		}
	}

	optional& operator=(const optional& other) {
		if (this == &other) {
			return *this;
		}
		reset();
		set_value(other.has_value());
		if (other.has_value()) {
			new (m_value) T(*reinterpret_cast<T*>(other.m_value));
		}
		return *this;
	}

	template <typename U>
	optional& operator=(const optional<U>& other) {
		if (this == &other) {
			return *this;
		}
		reset();
		set_value(other.has_value());
		if (other.has_value()) {
			new (m_value) T(*reinterpret_cast<T*>(other.m_value));
		}
		return *this;
	}

	optional& operator=(nullopt_t v) {
		reset();
		return *this;
	}

	void reset() {
		if (!has_value()) {
			return;
		}
		set_value(false);
		(*reinterpret_cast<T*>(m_value)).~T();
	}

	inline bool has_value() const { return m_value[sizeof(T)]; }
	inline operator bool() const { return m_value[sizeof(T)]; }

	T& value() {
		if (has_value()) {
			return *reinterpret_cast<T*>(m_value);
		}
		throw bad_optional_access();
	}
	const T& value() const {
		if (has_value()) {
			return *reinterpret_cast<T*>(m_value);
		}
		throw bad_optional_access();
	}

	template <typename U>
	T value_or(const U& v) const {
		if (has_value()) {
			return *reinterpret_cast<T*>(m_value);
		}
		return v;
	}

	T& operator*() { return *reinterpret_cast<T*>(m_value); }
	const T& operator*() const { return *reinterpret_cast<T*>(m_value); }

	T* operator->() {
		return has_value() ? reinterpret_cast<T*>(m_value) : (T*)0;
	}
	const T* operator->() const {
		return has_value() ? reinterpret_cast<T*>(m_value) : (const T*)0;
	}

private:
	void set_value(bool v) { m_value[sizeof(T)] = v; }

	struct __align {
		char __r;
		T __v;
	};

private:
	__declspec_align(sizeof(__align) - sizeof(T)) mutable char m_value[sizeof(T) + 1];
};
}
#endif

#endif
