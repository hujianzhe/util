//
// Created by hujianzhe on 23-4-20
//

#ifndef UTIL_CPP_COROUTINE_HELPER_H
#define UTIL_CPP_COROUTINE_HELPER_H

#include <any>
#include <atomic>
#include <cstdint>
#if __has_include(<coroutine>)
#include <coroutine>
#elif	__has_include(<experimental/coroutine>)
// for Apple Clang
#include <experimental/coroutine>
namespace std {
	typedef experimental::suspend_never suspend_never;
	typedef experimental::suspend_always suspend_always;
	template <typename T>
	using coroutine_handle = experimental::coroutine_handle<T>;
}
#endif
#include <exception>
#include <stdexcept>
#include <list>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace util {
template <typename T>
struct CoroutinePromise;

class CoroutineScheBase;
class CoroutineScheBaseImpl;
class CoroutineAwaiter;
class CoroutineAwaiterAnyone;
class CoroutinePromiseBase;
class CoroutineNode;

class CoroutineNode {
template <typename T>
friend struct CoroutinePromise;

friend class CoroutineScheBase;
friend class CoroutineScheBaseImpl;
friend class CoroutineAwaiter;
friend class CoroutineAwaiterAnyone;
friend class CoroutinePromiseBase;
public:
    ~CoroutineNode() {
		m_handle.destroy();
    }

    bool done() const { return m_handle.done(); }

    size_t ident() const { return m_ident; }
    const std::any& getAny() const { return m_value; }

private:
    CoroutineNode()
        :m_awaiting(false)
        ,m_ident(0)
        ,m_parent(nullptr)
        ,m_awaiter(nullptr)
        ,m_awaiter_anyone(nullptr)
		,m_awaiter_anyone_suspend(nullptr)
        ,m_promise_base(nullptr)
		,m_unhandle_exception_ptr(nullptr)
    {}

private:
    bool m_awaiting;
	std::coroutine_handle<void> m_handle;
    size_t m_ident;
    std::any m_value;
    CoroutineNode* m_parent;
    CoroutineAwaiter* m_awaiter;
    CoroutineAwaiterAnyone* m_awaiter_anyone;
    CoroutineAwaiterAnyone* m_awaiter_anyone_suspend;
    CoroutinePromiseBase* m_promise_base;
	std::exception_ptr m_unhandle_exception_ptr;
};

class CoroutineScheBase {
template <typename T>
friend struct CoroutinePromise;

friend class CoroutineAwaiter;
friend class CoroutinePromiseBase;
friend class CoroutineAwaiterAnyone;
public:
	CoroutineScheBase()
		:m_current_co_node(nullptr)
		,m_unhandle_exception_cnt(0)
		,m_fn_unhandled_exception(nullptr)
	{}

	virtual ~CoroutineScheBase() {}

	const CoroutineNode* current_co_node() const { return m_current_co_node; }

	void setUnhandledException(void(*fn)(std::exception_ptr), size_t max_cnt) {
		if (m_unhandle_exception_cnt > max_cnt) {
			m_unhandle_exception_cnt = max_cnt;
		}
		m_unhandle_exceptions.resize(max_cnt);
		m_fn_unhandled_exception = fn;
	}

protected:
	static inline thread_local CoroutineScheBase* p = nullptr;

private:
	CoroutineScheBase(const CoroutineScheBase&) = delete;
	CoroutineScheBase& operator=(const CoroutineScheBase&) = delete;

	void handleReturn() {
		m_current_co_node = m_current_co_node->m_parent;
	}

	void saveUnhandleException(CoroutineNode* co_node, std::exception_ptr eptr) {
		co_node->m_unhandle_exception_ptr = eptr;
		if (m_unhandle_exception_cnt >= m_unhandle_exceptions.size()) {
			return;
		}
		m_unhandle_exceptions[m_unhandle_exception_cnt++] = eptr;
	}

	void awaitMaybeThrowUnhandleException(CoroutineNode* co_node) {
		std::exception_ptr eptr = co_node->m_unhandle_exception_ptr;
		if (!eptr) {
			return;
		}
		co_node->m_unhandle_exception_ptr = nullptr;
		for (size_t n = m_unhandle_exception_cnt; n > 0; --n) {
			if (eptr == m_unhandle_exceptions[n - 1]) {
				m_unhandle_exceptions[n - 1] = nullptr;
				if (n == m_unhandle_exception_cnt) {
					--m_unhandle_exception_cnt;
				}
				break;
			}
		}
		std::rethrow_exception(eptr);
	}

protected:
	CoroutineNode* m_current_co_node;
	size_t m_unhandle_exception_cnt;
	void(*m_fn_unhandled_exception)(std::exception_ptr);
	std::vector<std::exception_ptr> m_unhandle_exceptions;
};

class CoroutineAwaiter {
friend class CoroutineScheBaseImpl;
public:
    enum {
        INVALID_AWAITER_ID = 0
    };

    CoroutineAwaiter& operator=(const CoroutineAwaiter&) = delete;
    CoroutineAwaiter()
        :m_id(INVALID_AWAITER_ID)
        ,m_await_ready(false)
    {}
	CoroutineAwaiter(int32_t id)
		:m_id(id)
		,m_await_ready(false)
	{}

    bool await_ready() const { return m_await_ready; }
    std::any await_resume() {
		CoroutineScheBase::p->m_current_co_node->m_awaiter = nullptr;
        m_await_ready = true;
        return m_value;
    }
    void await_suspend(std::coroutine_handle<void> h) {
		CoroutineScheBase::p->m_current_co_node->m_awaiter = this;
		CoroutineScheBase::p->m_current_co_node = CoroutineScheBase::p->m_current_co_node->m_parent;
    }

    int32_t id() const { return m_id; };
    const std::any& getAny() const { return m_value; }

	void invalid() {
		m_id = INVALID_AWAITER_ID;
		m_await_ready = true;
		m_value.reset();
	}

public:
    static int32_t gen_id() {
        static std::atomic_int32_t SEQ;
        int32_t v;
        do {
            v = ++SEQ;
        } while (INVALID_AWAITER_ID == v);
        return v;
    }

private:
    int32_t m_id;
    bool m_await_ready;
    std::any m_value;
};

class CoroutinePromiseBase {
friend class CoroutineScheBaseImpl;
friend class CoroutineAwaiterAnyone;
public:
    struct promise_type {
        CoroutineNode* co_node = nullptr;

        std::suspend_never initial_suspend() noexcept {
            co_node->m_parent = CoroutineScheBase::p->m_current_co_node;
            CoroutineScheBase::p->m_current_co_node = co_node;
            return {};
        }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {
			CoroutineScheBase::p->saveUnhandleException(co_node, std::current_exception());
			CoroutineScheBase::p->handleReturn();
        }
    };
    bool await_ready() const {
		return m_co_node->m_handle.done();
    }
    void await_suspend(std::coroutine_handle<void> h) {
        m_co_node->m_awaiting = true;
        CoroutineScheBase::p->m_current_co_node = CoroutineScheBase::p->m_current_co_node->m_parent;
    }

    const std::any& getAny() const { return m_co_node->m_value; }

protected:
    CoroutinePromiseBase& operator=(const CoroutinePromiseBase&) = delete;
	CoroutinePromiseBase(const CoroutinePromiseBase& other)
        :m_delete_co_node(other.m_delete_co_node)
        ,m_co_node(other.m_co_node)
    {
        other.m_delete_co_node = false;
		if (m_delete_co_node) {
        	m_co_node->m_promise_base = this;
		}
    }
    CoroutinePromiseBase()
        :m_delete_co_node(true)
        ,m_co_node(new CoroutineNode())
    {
        m_co_node->m_promise_base = this;
    }
    virtual ~CoroutinePromiseBase() {
        if (!m_delete_co_node) {
            return;
        }
        if (!m_co_node->done()) {
            m_co_node->m_promise_base = nullptr;
            return;
        }
        delete m_co_node;
    }

protected:
    mutable bool m_delete_co_node;
    CoroutineNode* m_co_node;
};

class CoroutineAwaiterAnyone {
friend class CoroutineScheBaseImpl;
public:
    CoroutineAwaiterAnyone& operator=(const CoroutineAwaiterAnyone&) = delete;
    CoroutineAwaiterAnyone(const CoroutineAwaiterAnyone&) = delete;
    CoroutineAwaiterAnyone() {}

    ~CoroutineAwaiterAnyone() {
		clearRunCoroutines();
        for (auto co_node : m_free_nodes) {
            delete co_node;
        }
    }

    bool await_ready() {
		for (auto co_node : m_run_nodes) {
            co_node->m_awaiter_anyone = this;
        }
        if (m_down_nodes.empty()) {
            return m_run_nodes.empty();
        }
        return true;
    }
    CoroutineNode* await_resume() {
		CoroutineScheBase::p->m_current_co_node->m_awaiter_anyone_suspend = nullptr;
		auto beg_it = m_down_nodes.begin();
		if (beg_it != m_down_nodes.end()) {
			CoroutineNode* co_node = *beg_it;
			m_down_nodes.erase(beg_it);
			CoroutineScheBase::p->awaitMaybeThrowUnhandleException(co_node);
			return co_node;
		}
		return nullptr;
    }
    void await_suspend(std::coroutine_handle<void> h) {
		CoroutineScheBase::p->m_current_co_node->m_awaiter_anyone_suspend = this;
        CoroutineScheBase::p->m_current_co_node = CoroutineScheBase::p->m_current_co_node->m_parent;
    }

    void add(const CoroutinePromiseBase& cpb) {
        CoroutineNode* co_node = cpb.m_co_node;
        co_node->m_promise_base = nullptr;
        cpb.m_delete_co_node = false;
        if (co_node->done()) {
            if (m_down_nodes.insert(co_node).second) {
                m_free_nodes.emplace_back(co_node);
            }
            return;
        }
        m_run_nodes.insert(co_node);
    }
    void addWithIdentity(const CoroutinePromiseBase& cpb, size_t ident) {
        cpb.m_co_node->m_ident = ident;
        add(cpb);
    }

    bool allDone() const {
        return m_down_nodes.empty() && m_run_nodes.empty();
    }

private:
    void clearRunCoroutines() {
        for (auto it = m_run_nodes.begin(); it != m_run_nodes.end(); ) {
            (*it)->m_awaiter_anyone = nullptr;
			(*it)->m_parent = nullptr;
            it = m_run_nodes.erase(it);
        }
    }

private:
    std::vector<CoroutineNode*> m_free_nodes;
    std::unordered_set<CoroutineNode*> m_run_nodes;
    std::unordered_set<CoroutineNode*> m_down_nodes;
};

template <typename T>
struct CoroutinePromise : public CoroutinePromiseBase {
friend class CoroutinePromiseBase;
    struct promise_type : public CoroutinePromiseBase::promise_type {
        CoroutinePromise get_return_object() {
            return { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        void return_value(const T& v) {
            co_node->m_value = v;
			CoroutineScheBase::p->handleReturn();
        }
    };
    T await_resume() const {
		CoroutineScheBase::p->awaitMaybeThrowUnhandleException(m_co_node);
		return std::any_cast<T>(m_co_node->m_value);
	}

    CoroutinePromise(std::coroutine_handle<promise_type> handle) {
        handle.promise().co_node = m_co_node;
		m_co_node->m_handle = handle;
    }
};

template <>
struct CoroutinePromise<void> : public CoroutinePromiseBase {
friend class CoroutinePromiseBase;
    struct promise_type : public CoroutinePromiseBase::promise_type {
        CoroutinePromise get_return_object() {
            return { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        void return_void() {
			CoroutineScheBase::p->handleReturn();
        }
    };
    void await_resume() const {
		CoroutineScheBase::p->awaitMaybeThrowUnhandleException(m_co_node);
	}

    CoroutinePromise(std::coroutine_handle<promise_type> handle) {
        handle.promise().co_node = m_co_node;
		m_co_node->m_handle = handle;
    }
};

class CoroutineScheBaseImpl : public CoroutineScheBase {
private:
    void doResumeImpl(CoroutineNode* co_node) {
        bool last_free = true;
        CoroutineNode* last, *cur = co_node;
		CoroutineScheBase::p = this;
        do {
            CoroutineNode* parent = cur->m_parent;
            last = cur;
            m_current_co_node = cur;
			cur->m_handle.resume();
            if (!cur->m_handle.done()) {
                last_free = false;
                break;
            }
            if (!cur->m_awaiting) {
				CoroutineAwaiterAnyone* aw_anyone = cur->m_awaiter_anyone;
				if (aw_anyone) {
					aw_anyone->m_run_nodes.erase(cur);
					aw_anyone->m_down_nodes.insert(cur);
					aw_anyone->m_free_nodes.emplace_back(cur);
					cur->m_awaiter_anyone = nullptr;
					if (parent->m_awaiter_anyone_suspend != aw_anyone) {
						last_free = false;
						break;
					}
				}
                else {
                    if (cur->m_promise_base) {
                        cur->m_promise_base->m_delete_co_node = false;
                        cur->m_promise_base = nullptr;
                    }
                    break;
                }
            }
            cur = parent;
        } while (cur);
        m_current_co_node = nullptr;
		CoroutineScheBase::p = nullptr;
        if (last->m_promise_base) {
            return;
        }
        if (last_free) {
            delete last;
        }
    }

protected:
	template <typename T = std::any>
	void doResume(CoroutineNode* co_node, const T& v) {
		if (co_node->m_awaiter) {
			co_node->m_awaiter->m_value = v;
		}
		doResumeImpl(co_node);
	}

	bool checkBusy() const {
		if (m_unhandle_exception_cnt > 0) {
			return true;
		}
		if (!m_ready_resumes.empty()) {
			return true;
		}
		return false;
	}

	void readyResume(CoroutineNode* co_node, const std::any& param) {
		m_ready_resumes.emplace_back(std::pair{co_node, param});
	}

	void doSche(size_t peak_cnt) {
		for (size_t i = 0; i < m_unhandle_exception_cnt; ++i) {
			if (!m_unhandle_exceptions[i]) {
				continue;
			}
			if (m_fn_unhandled_exception) { /* avoid fn change to nullptr */
				try {
					m_fn_unhandled_exception(m_unhandle_exceptions[i]);
				} catch (...) {}
			}
			m_unhandle_exceptions[i] = nullptr;
		}
		m_unhandle_exception_cnt = 0;

		for (auto it = m_ready_resumes.begin(); it != m_ready_resumes.end() && peak_cnt; ) {
			doResume(it->first, it->second);
			it = m_ready_resumes.erase(it);
			--peak_cnt;
		}
	}

    static CoroutineNode* onDestroy(CoroutineNode* co_node) {
        if (co_node->m_promise_base) {
            co_node->m_promise_base->m_delete_co_node = false;
        }
        CoroutineNode* parent;
        if (co_node->m_awaiter_anyone) {
			parent = co_node->m_parent;
            co_node->m_awaiter_anyone->clearRunCoroutines();
        }
        else if (!co_node->m_awaiting) {
            parent = nullptr;
        }
		else {
			parent = co_node->m_parent;
		}
        delete co_node;
        return parent;
    }

	void scheDestroy(std::unordered_set<CoroutineNode*>& top_set) {
		CoroutineScheBase::p = nullptr;
		for (auto it = m_lock_datas.begin(); it != m_lock_datas.end(); ) {
			LockData& lock_data = it->second;
			for (auto it = lock_data.m_wait_infos.begin(); it != lock_data.m_wait_infos.end(); ) {
				top_set.insert(it->co_node);
				it = lock_data.m_wait_infos.erase(it);
			}
			it = m_lock_datas.erase(it);
		}
		for (auto it = m_ready_resumes.begin(); it != m_ready_resumes.end(); ) {
			top_set.insert(it->first);
			it = m_ready_resumes.erase(it);
		}
		while (!top_set.empty()) {
            std::unordered_set<CoroutineNode*> tmp_set;
            for (auto it = top_set.begin(); it != top_set.end(); ) {
                CoroutineNode* parent = onDestroy(*it);
                if (parent && top_set.find(parent) == top_set.end()) { /* avoid anyone awaiter */
                    tmp_set.insert(parent);
                }
                it = top_set.erase(it);
            }
            top_set.swap(tmp_set);
        }
	}

protected:
	class LockOwner {
	public:
		LockOwner() : m_used(false), m_unique(true) {}
		LockOwner(const std::string& v) : m_used(false), m_unique(v.empty()), m_v(v) {}

		bool used() const { return m_used; }
		void set_used(bool v) { m_used = v; }

		bool equal(const LockOwner& other) const {
			if (m_unique || other.m_unique) {
				return false;
			}
			return m_v == other.m_v;
		}

	private:
		bool m_used;
		bool m_unique;
		std::string m_v;
	};

	class LockData {
	friend class CoroutineScheBaseImpl;
	public:
		LockData()
			:m_enter_times(0)
			,m_ptr_name(nullptr)
		{}

		bool try_acquire(const LockOwner& owner) {
			if (!m_owner.used()) {
				m_owner = owner;
				m_owner.set_used(true);
				m_enter_times = 1;
				return true;
			}
			if (owner.equal(m_owner)) {
				m_enter_times++;
				return true;
			}
			return false;
		}

		bool acquire(CoroutineNode* current_co_node, const LockOwner& owner) {
			if (try_acquire(owner)) {
				return true;
			}
			m_wait_infos.emplace_back(WaitInfo{current_co_node, owner});
			return false;
		}

		void release(CoroutineScheBaseImpl* sc) {
			if (m_enter_times > 1) {
				m_enter_times--;
				return;
			}
			if (m_wait_infos.empty()) {
				m_owner.set_used(false);
				return;
			}
			WaitInfo& wait_info = m_wait_infos.front();
			CoroutineNode* co_node = wait_info.co_node;
			m_owner = wait_info.owner;
			m_owner.set_used(true);
			m_wait_infos.pop_front();

			sc->readyResume(co_node, std::any());
			for (auto it = m_wait_infos.begin(); it != m_wait_infos.end(); ) {
				if (!m_owner.equal(it->owner)) {
					return;
				}
				co_node = it->co_node;
				m_enter_times++;
				it = m_wait_infos.erase(it);

				sc->readyResume(co_node, std::any());
			}
		}

	private:
		typedef struct WaitInfo {
			CoroutineNode* co_node;
			LockOwner owner;
		} WaitInfo;

	private:
		LockOwner m_owner;
		size_t m_enter_times;
		const std::string* m_ptr_name;
		std::list<WaitInfo> m_wait_infos;
	};

	LockData* get_or_new_lock(const std::string& name) {
		auto result = m_lock_datas.insert({name, LockData()});
		LockData* lock_data = &result.first->second;
		if (result.second) {
			lock_data->m_ptr_name = &result.first->first;
		}
		return lock_data;
	}

public:
	class LockGuard {
	public:
		LockGuard() : m_lockData(nullptr) {}
		LockGuard(const std::string& str_owner) : m_lockData(nullptr), m_strOwner(str_owner) {}
		~LockGuard() { unlock(); }
		LockGuard(const LockGuard&) = delete;
		LockGuard& operator=(const LockGuard&) = delete;

		CoroutineAwaiter lock(const std::string& name) {
			if (m_lockData) {
				throw std::logic_error("coroutine already locked");
			}
			CoroutineScheBaseImpl* sc = (CoroutineScheBaseImpl*)CoroutineScheBase::p;
			m_lockData = sc->get_or_new_lock(name);
			CoroutineAwaiter awaiter;
			if (m_lockData->acquire(sc->m_current_co_node, LockOwner(m_strOwner))) {
				awaiter.invalid();
			}
			return awaiter;
		}

		bool try_lock(const std::string& name) {
			if (m_lockData) {
				throw std::logic_error("coroutine already locked");
			}
			CoroutineScheBaseImpl* sc = (CoroutineScheBaseImpl*)CoroutineScheBase::p;
			LockData* lock_data = sc->get_or_new_lock(name);
			if (lock_data->try_acquire(LockOwner(m_strOwner))) {
				m_lockData = lock_data;
				return true;
			}
			return false;
		}

		void unlock() {
			if (!m_lockData) {
				return;
			}
			CoroutineScheBaseImpl* sc = (CoroutineScheBaseImpl*)CoroutineScheBase::p;
			if (!sc) { /* on sche destroy */
				return;
			}
			LockData* lock_data = m_lockData;
			m_lockData = nullptr;
			lock_data->release(sc);
			if (lock_data->m_owner.used()) {
				return;
			}
			sc->m_lock_datas.erase(*(lock_data->m_ptr_name));
		}

	private:
		LockData* m_lockData;
		std::string m_strOwner;
	};

private:
	std::unordered_map<std::string, LockData> m_lock_datas;
	std::list<std::pair<CoroutineNode*, std::any> > m_ready_resumes;
};
}

#endif
