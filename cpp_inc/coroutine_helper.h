//
// Created by hujianzhe on 23-4-20
//

#ifndef UTIL_CPP_COROUTINE_HELPER_H
#define UTIL_CPP_COROUTINE_HELPER_H

#include <any>
#include <atomic>
#include <cstdint>
#include <coroutine>
#include <exception>
#include <list>
#include <memory>
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

friend class CoroutineScheBaseImpl;
friend class CoroutineAwaiter;
friend class CoroutineAwaiterAnyone;
friend class CoroutinePromiseBase;
public:
    ~CoroutineNode() {
        std::coroutine_handle<void>::from_address(m_co_addr).destroy();
    }

    bool done() const { return m_co_addr ? std::coroutine_handle<void>::from_address(m_co_addr).done() : true; }

    size_t ident() const { return m_ident; }
    const std::any& getAny() const { return m_value; }

private:
    CoroutineNode()
        :m_awaiting(false)
        ,m_co_addr(nullptr)
        ,m_ident(0)
        ,m_parent(nullptr)
        ,m_awaiter(nullptr)
        ,m_awaiter_anyone(nullptr)
        ,m_promise_base(nullptr)
    {}

private:
    bool m_awaiting;
    void* m_co_addr;
    size_t m_ident;
    std::any m_value;
    CoroutineNode* m_parent;
    CoroutineAwaiter* m_awaiter;
    CoroutineAwaiterAnyone* m_awaiter_anyone;
    CoroutinePromiseBase* m_promise_base;
};

class CoroutineScheBase {
friend class CoroutineAwaiter;
friend class CoroutinePromiseBase;
friend class CoroutineAwaiterAnyone;
public:
	CoroutineScheBase()
		:m_current_co_node(nullptr)
		,m_unhandled_exception(nullptr)
	{}

	virtual ~CoroutineScheBase() {}

	const CoroutineNode* current_co_node() const { return m_current_co_node; }
	void set_unhandled_exception(void(*fn)(std::exception_ptr)) { m_unhandled_exception = fn; }

protected:
	static inline thread_local CoroutineScheBase* p = nullptr;

protected:
	CoroutineNode* m_current_co_node;
	void(*m_unhandled_exception)(std::exception_ptr);
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
        std::exception_ptr current_exception_ptr;

        std::suspend_never initial_suspend() noexcept {
            co_node->m_parent = CoroutineScheBase::p->m_current_co_node;
            CoroutineScheBase::p->m_current_co_node = co_node;
            return {};
        }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {
            current_exception_ptr = std::current_exception();
            handleReturn();
        }

        void handleReturn() {
            CoroutineScheBase::p->m_current_co_node = CoroutineScheBase::p->m_current_co_node->m_parent;
        }
    };
    bool await_ready() const {
        return std::coroutine_handle<void>::from_address(m_co_node->m_co_addr).done();
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
        m_co_node->m_promise_base = this;
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
    CoroutineAwaiterAnyone()
        :m_resume_node(nullptr)
    {}

    ~CoroutineAwaiterAnyone() {
        for (auto co_node : m_run_nodes) {
            if (!co_node->done()) {
                co_node->m_awaiter_anyone = nullptr;
                continue;
            }
            delete co_node;
        }
        for (auto co_node : m_free_nodes) {
            delete co_node;
        }
    }

    bool await_ready() const {
        if (m_down_nodes.empty()) {
            return m_run_nodes.empty();
        }
        return true;
    }
    CoroutineNode* await_resume() {
        if (!m_down_nodes.empty()) {
            m_resume_node = *(m_down_nodes.begin());
            m_down_nodes.erase(m_down_nodes.begin());
        }
        else if (m_resume_node) {
            m_run_nodes.erase(m_resume_node);
            m_resume_node->m_awaiter_anyone = nullptr;
            m_free_nodes.emplace_back(m_resume_node);
        }
        return m_resume_node;
    }
    void await_suspend(std::coroutine_handle<void> h) {
        for (auto co_node : m_run_nodes) {
            co_node->m_awaiter_anyone = this;
        }
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
    void onScheDestroy() {
        for (auto it = m_run_nodes.begin(); it != m_run_nodes.end(); ) {
            (*it)->m_awaiter_anyone = nullptr;
            it = m_run_nodes.erase(it);
        }
    }

private:
    CoroutineNode* m_resume_node;
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
            handleReturn();
        }
    };
    T await_resume() const { return getValue(); }

    T getValue() const { return std::any_cast<const T&>(m_co_node->m_value); }

    CoroutinePromise(std::coroutine_handle<promise_type> handle) {
        handle.promise().co_node = m_co_node;
        m_co_node->m_co_addr = handle.address();
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
            handleReturn();
        }
    };
    void await_resume() const {}

    CoroutinePromise(std::coroutine_handle<promise_type> handle) {
        handle.promise().co_node = m_co_node;
        m_co_node->m_co_addr = handle.address();
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
            auto co_handle = std::coroutine_handle<CoroutinePromiseBase::promise_type>::from_address(cur->m_co_addr);
            co_handle.resume();
            if (!co_handle.done()) {
                last_free = false;
                break;
            }
            if (!cur->m_awaiting) {
                if (!cur->m_awaiter_anyone) {
                    if (cur->m_promise_base) {
                        cur->m_promise_base->m_delete_co_node = false;
                        cur->m_promise_base = nullptr;
                    }
                    if (co_handle.promise().current_exception_ptr && m_unhandled_exception) {
                        m_unhandled_exception(co_handle.promise().current_exception_ptr);
                    }
                    break;
                }
                cur->m_awaiter_anyone->m_resume_node = cur;
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

    static CoroutineNode* onDestroy(CoroutineNode* co_node) {
        CoroutineNode* parent = co_node->m_parent;
        if (co_node->m_promise_base) {
            co_node->m_promise_base->m_delete_co_node = false;
        }
        if (co_node->m_awaiter_anyone) {
            co_node->m_awaiter_anyone->onScheDestroy();
        }
        else if (!co_node->m_awaiting) {
            parent = nullptr;
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
		while (!top_set.empty()) {
            std::unordered_set<CoroutineNode*> tmp_set;
            for (auto it = top_set.begin(); it != top_set.end(); ) {
                CoroutineNode* parent = onDestroy(*it);
                if (parent) {
                    tmp_set.insert(parent);
                }
                it = top_set.erase(it);
            }
            top_set.swap(tmp_set);
        }
	}

public:
	static std::shared_ptr<int> new_scope() {
		return std::make_shared<int>(0);
	}

protected:
	class LockData {
	friend class CoroutineScheBaseImpl;
	public:
		LockData()
			:m_scope_enter_times(0)
			,m_ptr_name(nullptr)
		{}

		std::shared_ptr<int> scope() const { return m_scope; }

	private:
		typedef struct WaitInfo {
			CoroutineNode* co_node;
			std::shared_ptr<int> scope;
		} WaitInfo;

	private:
		std::shared_ptr<int> m_scope;
		size_t m_scope_enter_times;
		const std::string* m_ptr_name;
		std::list<WaitInfo> m_wait_infos;
	};

	class LockGuardImpl {
	public:
		LockGuardImpl(const std::shared_ptr<int>& scope) : m_scope(scope), m_data(nullptr) {}
		LockGuardImpl(const LockGuardImpl&) = delete;
		LockGuardImpl& operator=(const LockGuardImpl&) = delete;

		std::shared_ptr<int> scope() const { return m_scope; }

	protected:
		CoroutineAwaiter lock(const std::string& name) {
			CoroutineScheBaseImpl* sc = (CoroutineScheBaseImpl*)CoroutineScheBase::p;
			m_data = sc->lock_acquire(m_scope, name);
			CoroutineAwaiter awaiter;
			if (m_data->scope() == m_scope) {
				awaiter.invalid();
			}
			return awaiter;
		}

		bool try_lock(const std::string& name) {
			CoroutineScheBaseImpl* sc = (CoroutineScheBaseImpl*)CoroutineScheBase::p;
			LockData* lock_data = sc->lock_acquire(m_scope, name);
			if (lock_data->scope() != m_scope) {
				return false;
			}
			m_data = lock_data;
			return true;
		}

	protected:
		std::shared_ptr<int> m_scope;
		LockData* m_data;
	};

	LockData* lock_acquire(std::shared_ptr<int> scope, const std::string& name) {
		auto result = m_lock_datas.insert({name, LockData()});
		LockData* lock_data = &result.first->second;
		if (result.second) {
			lock_data->m_scope = scope;
			lock_data->m_scope_enter_times = 1;
			lock_data->m_ptr_name = &result.first->first;
			return lock_data;
		}
		if (lock_data->m_scope == scope) {
			++lock_data->m_scope_enter_times;
			return lock_data;
		}
		lock_data->m_wait_infos.emplace_back(LockData::WaitInfo{m_current_co_node, scope});
		return lock_data;
	}

	CoroutineNode* lock_release(LockData* lock_data) {
		if (lock_data->m_scope_enter_times > 1) {
			--lock_data->m_scope_enter_times;
			return nullptr;
		}
		if (lock_data->m_wait_infos.empty()) {
			m_lock_datas.erase(*(lock_data->m_ptr_name));
			return nullptr;
		}
		LockData::WaitInfo& wait_info = lock_data->m_wait_infos.front();
		CoroutineNode* co_node = wait_info.co_node;
		lock_data->m_scope = wait_info.scope;
		lock_data->m_wait_infos.pop_front();
		return co_node;
	}

	void lock_release_all(LockData* lock_data, std::vector<CoroutineNode*>& ret_co_nodes) {
		if (lock_data->m_scope_enter_times > 1) {
			--lock_data->m_scope_enter_times;
			return;
		}
		if (lock_data->m_wait_infos.empty()) {
			m_lock_datas.erase(*(lock_data->m_ptr_name));
			return;
		}
		ret_co_nodes.reserve(lock_data->m_wait_infos.size());
		for (auto it = lock_data->m_wait_infos.begin(); it != lock_data->m_wait_infos.end(); ) {
			ret_co_nodes.push_back(it->co_node);
			it = lock_data->m_wait_infos.erase(it);
		}
		m_lock_datas.erase(*(lock_data->m_ptr_name));
	}

private:
	std::unordered_map<std::string, LockData> m_lock_datas;
};
}

#endif
