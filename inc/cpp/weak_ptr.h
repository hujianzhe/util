//
// Created by hujianzhe on 20-9-4.
//

#ifndef	UTIL_CPP_WEAK_PTR_H
#define	UTIL_CPP_WEAK_PTR_H

#ifdef	__cplusplus

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <memory>
#else
#include "shared_ptr.h"
#include <exception>
namespace std {
class bad_weak_ptr : public std::exception {
public:
	virtual const char* what(void) const throw() {
		return "bad_weak_ptr";
	}
};

template <typename T>
class weak_ptr {
public:
	weak_ptr(void) : m_ptr((T*)0), m_refcnt((sp_refcnt*)0) {}

	template <typename U>
	weak_ptr(const shared_ptr<U>& sp) :
		m_ptr(sp.m_ptr),
		m_refcnt(sp.m_refcnt)
	{
		if (m_refcnt)
			m_refcnt->incr_weak(1);
	}

	template <typename U>
	weak_ptr(const weak_ptr<U>& wp) :
		m_ptr(wp.m_ptr),
		m_refcnt(wp.m_refcnt)
	{
		if (m_refcnt)
			m_refcnt->incr_weak(1);
	}

	template <typename U>
	weak_ptr& operator=(const weak_ptr<U>& other) {
		if (this != &other) {
			if (m_refcnt) {
				sp_refcnt::wp_release(m_refcnt);
			}
			m_ptr = other.m_ptr;
			m_refcnt = other.m_refcnt;
			if (other.m_refcnt)
				other.m_refcnt->incr_weak(1);
		}
		return *this;
	}

	~weak_ptr(void) {
		if (m_refcnt) {
			sp_refcnt::wp_release(m_refcnt);
		}
	}

	void reset(void) {
		if (m_refcnt) {
			sp_refcnt::wp_release(m_refcnt);
			m_refcnt = (sp_refcnt*)0;
			m_ptr = (T*)0;
		}
	}

	bool expired(void) const {
		return m_refcnt ? (m_refcnt->count_share() > 0) : false;
	}

	shared_ptr<T> lock(void) const {
		if (!m_ptr || !m_refcnt || !m_refcnt->lock()) {
			m_ptr = (T*)0;
			return shared_ptr<T>();
		}
		else {
			shared_ptr<T> p;
			p.m_ptr = m_ptr;
			p.m_refcnt = m_refcnt;
			return p;
		}
	}

private:
	mutable T* m_ptr;
	sp_refcnt* m_refcnt;
};
}
#endif

#endif

#endif
