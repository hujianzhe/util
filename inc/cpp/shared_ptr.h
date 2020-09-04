//
// Created by hujianzhe on 20-9-3.
//

#ifndef	UTIL_CPP_SHARED_PTR_H
#define	UTIL_CPP_SHARED_PTR_H

#ifdef	__cplusplus

#include "cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <memory>
#else
#include "../sysapi/atomic.h"
#include "nullptr.h"
#include "type_traits.h"
namespace std {
template<typename T>
struct shared_ptr_default_delete {
	void operator()(T* ptr) const { delete ptr; }
};

template <typename T>
class enable_shared_from_this;
template <typename T>
class weak_ptr;

class sp_refcnt {
public:
	sp_refcnt(void) : m_share_refcnt (1), m_weak_refcnt(1) {}
	virtual ~sp_refcnt(void) {}

	int incr_share(int v) { return _xadd32(&m_share_refcnt, v); }
	int incr_weak(int v) { return _xadd32(&m_weak_refcnt, v); }
	int count_share(void) { return m_share_refcnt; }
	bool lock(void) {
		while (true) {
			Atom32_t tmp = _xadd32(&m_share_refcnt, 0);
			if (tmp < 1)
				return false;
			if (_cmpxchg32(&m_share_refcnt, tmp + 1, tmp) == tmp)
				return true;
		}
	}

	static void sp_release(sp_refcnt* refcnt) {
		if (refcnt->incr_share(-1) > 1)
			return;
		refcnt->destroy();
		if (refcnt->incr_weak(-1) <= 1)
			delete refcnt;
	}

	static void wp_release(sp_refcnt* refcnt) {
		if (refcnt->incr_weak(-1) > 1)
			return;
		if (refcnt->incr_share(0) < 1)
			delete refcnt;
	}

private:
	sp_refcnt(const sp_refcnt&) {}
	sp_refcnt& operator=(const sp_refcnt&) { return *this; }
	virtual void destroy(void) {}

private:
	Atom32_t m_share_refcnt;
	Atom32_t m_weak_refcnt;
};

template <typename T, typename Deleter>
class sp_impl : public sp_refcnt {
public:
	sp_impl(T* p, const Deleter& deleter) :
		m_ptr(p),
		m_deleter(deleter)
	{
	}

private:
	virtual void destroy(void) { m_deleter(m_ptr); }

private:
	T* m_ptr;
	Deleter m_deleter;
};

template <typename T, typename Deleter = shared_ptr_default_delete<T> >
class shared_ptr {
friend class enable_shared_from_this<T>;
friend class weak_ptr<T>;
public:
	explicit shared_ptr(T* p = (T*)0, const Deleter& deleter = Deleter()) :
		m_ptr(p)
	{
		if (p) {
			m_refcnt = new sp_impl<T, Deleter>(p, deleter);
			if (util::is_base_of<enable_shared_from_this<T>, T >::value) {
				enable_shared_from_this<T>* pe = (enable_shared_from_this<T>*)p;
				pe->m_refcnt = m_refcnt;
			}
		}
		else {
			m_refcnt = (sp_refcnt*)0;
		}
	}

	shared_ptr(const shared_ptr& other) :
		m_ptr(other.m_ptr),
		m_refcnt(other.m_refcnt)
	{
		if (m_refcnt) {
			m_refcnt->incr_share(1);
		}
	}

	template <typename U>
	shared_ptr(const shared_ptr<U>& other) :
		m_ptr(other.m_ptr),
		m_refcnt(other.m_refcnt)
	{
		if (m_refcnt) {
			m_refcnt->incr_share(1);
		}
	}

	shared_ptr& operator=(const shared_ptr& other) {
		if (this != &other) {
			if (m_refcnt) {
				sp_refcnt::sp_release(m_refcnt);
			}
			m_ptr = other.m_ptr;
			m_refcnt = other.m_refcnt;
			if (other.m_refcnt) {
				other.m_refcnt->incr_share(1);
			}
		}
		return *this;
	}

	template <typename U>
	shared_ptr& operator=(const shared_ptr<U>& other) {
		if (m_ptr != (T*)other.m_ptr) {
			if (m_refcnt) {
				sp_refcnt::sp_release(m_refcnt);
			}
			m_ptr = other.m_ptr;
			m_refcnt = other.m_refcnt;
			if (other.m_refcnt) {
				other.m_refcnt->incr_share(1);
			}
		}
		return *this;
	}

	~shared_ptr(void) {
		if (m_refcnt) {
			sp_refcnt::sp_release(m_refcnt);
		}
	}

	operator bool(void) const { return m_ptr != (T*)0; }
	T& operator*(void) { return *m_ptr; }
	const T& operator*(void) const { return *m_ptr; }
	T* operator->(void) { return m_ptr; }
	const T* operator->(void) const { return m_ptr; }

	void reset(T* p = (T*)0) {
		if (m_ptr == p)
			return;
		if (m_refcnt) {
			sp_refcnt::sp_release(m_refcnt);
		}
		m_ptr = p;
		if (p) {
			m_refcnt = new sp_impl<T, Deleter>(p, Deleter());
			if (util::is_base_of<enable_shared_from_this<T>, T >::value) {
				enable_shared_from_this<T>* pe = (enable_shared_from_this<T>*)p;
				pe->m_refcnt = m_refcnt;
			}
		}
		else {
			m_refcnt = (sp_refcnt*)0;
		}
	}
	template <typename D>
	void reset(T* p, const D& deleter) {
		if (m_ptr == p)
			return;
		if (m_refcnt) {
			sp_refcnt::sp_release(m_refcnt);
		}
		m_ptr = p;
		if (p) {
			m_refcnt = new sp_impl<T, D>(p, deleter);
			if (util::is_base_of<enable_shared_from_this<T>, T >::value) {
				enable_shared_from_this<T>* pe = (enable_shared_from_this<T>*)p;
				pe->m_refcnt = m_refcnt;
			}
		}
		else {
			m_refcnt = (sp_refcnt*)0;
		}
	}

	long int use_count(void) const { return m_refcnt ? m_refcnt->count_share() : 0; }
	T* get(void) const { return m_ptr; }

public:
	T* m_ptr;
	sp_refcnt* m_refcnt;
};

template <class T1, class D1, class T2, class D2>
bool operator==(const shared_ptr<T1, D1>& x, const shared_ptr<T2, D2>& y) { return x.get() == y.get(); }
template <class T1, class D, class T2>
bool operator==(const shared_ptr<T1, D>& x, const T2* y) { return x.get() == y; }
template <class T1, class D, class T2>
bool operator==(const T2* y, const shared_ptr<T1, D>& x) { return x.get() == y; }
template <class T, class D>
bool operator==(nullptr_t p, const shared_ptr<T, D>& x) { return x.get() == p; }
template <class T, class D>
bool operator==(const shared_ptr<T, D>& x, nullptr_t p) { return x.get() == p; }

template <class T1, class D1, class T2, class D2>
bool operator!=(const shared_ptr<T1, D1>& x, const shared_ptr<T2, D2>& y) { return x.get() != y.get(); }
template <class T1, class D, class T2>
bool operator!=(const shared_ptr<T1, D>& x, const T2* y) { return x.get() != y; }
template <class T1, class D, class T2>
bool operator!=(const T2* y, const shared_ptr<T1, D>& x) { return x.get() != y; }
template <class T, class D>
bool operator!=(nullptr_t p, const shared_ptr<T, D>& x) { return x.get() != p; }
template <class T, class D>
bool operator!=(const shared_ptr<T, D>& x, nullptr_t p) { return x.get() != p; }

template <typename T>
class enable_shared_from_this {
friend class shared_ptr<T>;
protected:
	shared_ptr<T> shared_from_this(void) const {
		shared_ptr<T> p;
		p.m_ptr = (T*)this;
		p.m_refcnt = m_refcnt;
		m_refcnt->incr_share(1);
		return p;
	}

	enable_shared_from_this(void) {}
	enable_shared_from_this(enable_shared_from_this&) {}
	enable_shared_from_this& operator=(const enable_shared_from_this&) { return *this; }
	~enable_shared_from_this(void) {}

private:
	sp_refcnt* m_refcnt;
};
}
#endif

#endif

#endif
