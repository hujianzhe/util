//
// Created by hujianzhe on 23-4-20
//

#ifndef UTIL_CPP_COROUTINE_DEFAULT_SCHE_H
#define UTIL_CPP_COROUTINE_DEFAULT_SCHE_H

#include "coroutine_helper.h"
#include <map>
#include <mutex>
#include <chrono>
#include <climits>
#include <functional>
#include <condition_variable>

namespace util {
class CoroutineDefaultSche : public CoroutineScheBaseImpl {
public:
    typedef std::function<CoroutinePromise<void>(const std::any&)> EntryFunc;

    CoroutineDefaultSche()
        :CoroutineScheBaseImpl()
		,m_handle_cnt(100)
        ,m_status(ST_RUN)
    {
        m_peak_events.reserve(m_handle_cnt);
    }

	static CoroutineDefaultSche* get() { return (CoroutineDefaultSche*)CoroutineScheBase::p; }

    bool check_exit() const { return ST_EXIT == m_status; }

	int handle_cnt() const { return m_handle_cnt; }
	void set_handle_cnt(int cnt) {
		if (cnt > 0) {
			m_handle_cnt = cnt;
		}
	}

    void doExit() {
        if (m_status.exchange(ST_EXIT) == ST_RUN) {
            m_cv.notify_one();
        }
    }

    long long get_current_ts_msec() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void readyExec(const EntryFunc& func, const std::any& param = std::any()) {
        readyExecUtil(func, 0, param);
    }
    void readyExecTimeout(const EntryFunc& func, long long tlen_msec, const std::any& param = std::any()) {
        if (!func) {
            return;
        }
        readyExecUtil(func, get_current_ts_msec() + tlen_msec, param);
    }
    void readyExecUtil(const EntryFunc& func, long long ts_msec, const std::any& param = std::any()) {
        if (!func) {
            return;
        }
		postEvent(Event(func, param, ts_msec));
    }

	CoroutineAwaiter blockPoint() {
		CoroutineAwaiter awaiter(CoroutineAwaiter::gen_id());
		if (!regAwaiter(awaiter.id())) {
			awaiter.invalid();
		}
		return awaiter;
	}
	CoroutineAwaiter blockPointUtil(long long ts_msec) {
		CoroutineAwaiter awaiter(CoroutineAwaiter::gen_id());
		BlockPointData* pdata = regAwaiter(awaiter.id());
		if (!pdata) {
			awaiter.invalid();
			return awaiter;
		}
		if (ts_msec >= 0) {
			pdata->timeout_event = addTimeoutEvent(Event(awaiter.id(), std::any(), ts_msec));
		}
		return awaiter;
	}
	CoroutineAwaiter blockPointTimeout(long long tlen_msec) {
		CoroutineAwaiter awaiter(CoroutineAwaiter::gen_id());
		BlockPointData* pdata = regAwaiter(awaiter.id());
		if (!pdata) {
			awaiter.invalid();
			return awaiter;
		}
		if (tlen_msec >= 0) {
			long long ts_msec = get_current_ts_msec() + tlen_msec;
			pdata->timeout_event = addTimeoutEvent(Event(awaiter.id(), std::any(), ts_msec));
		}
		return awaiter;
	}

    CoroutineAwaiter sleepTimeout(long long tlen_msec) {
        CoroutineAwaiter awaiter;
        addTimeoutEvent(Event(m_current_co_node, std::any(), get_current_ts_msec() + tlen_msec));
        return awaiter;
    }
    CoroutineAwaiter sleepUtil(long long ts_msec) {
        CoroutineAwaiter awaiter;
        addTimeoutEvent(Event(m_current_co_node, std::any(), ts_msec));
        return awaiter;
    }

	void readyResume(int32_t id, const std::any& param) {
        if (CoroutineAwaiter::INVALID_AWAITER_ID == id) {
            return;
        }
		postEvent(Event(id, param, 0));
    }

    void doSche(int idle_msec) {
        idle_msec = calculateWaitTimelen(get_current_ts_msec(), idle_msec);
        doPeakEvent(idle_msec);
        if (ST_RUN != m_status) {
            return;
        }
		CoroutineScheBaseImpl::doSche(m_handle_cnt);
        handlePeakEvent(get_current_ts_msec());
        handleTimeoutEvents(get_current_ts_msec());
    }

    void scheDestroy() {
        std::unordered_set<CoroutineNode*> top_set;
        for (auto it = m_timeout_events.begin(); it != m_timeout_events.end(); ) {
            std::list<Event>& evlist = it->second;
            for (auto it = evlist.begin(); it != evlist.end(); ) {
                if (it->sleep_co_node) {
                    top_set.insert(it->sleep_co_node);
                }
                it = evlist.erase(it);
            }
            it = m_timeout_events.erase(it);
        }
        for (auto it = m_block_points.begin(); it != m_block_points.end(); ) {
			top_set.insert(it->second.co_node);
            it = m_block_points.erase(it);
        }
		CoroutineScheBaseImpl::scheDestroy(top_set);
    }

private:
	enum {
        ST_RUN = 0,
        ST_EXIT
    };

    struct Event {
        int32_t resume_id;
        long long ts;
        EntryFunc func;
        std::any param;
        CoroutineNode* sleep_co_node;

        Event()
            :resume_id(CoroutineAwaiter::INVALID_AWAITER_ID)
            ,ts(0)
            ,sleep_co_node(nullptr)
        {}
        Event(const EntryFunc& fn, const std::any& param, long long ts)
            :resume_id(CoroutineAwaiter::INVALID_AWAITER_ID)
            ,ts(ts)
            ,func(fn)
            ,param(param)
            ,sleep_co_node(nullptr)
        {}
        Event(int32_t resume_id, const std::any& param, long long ts)
            :resume_id(resume_id)
            ,ts(ts)
            ,param(param)
            ,sleep_co_node(nullptr)
        {}
        Event(CoroutineNode* sleep_co_node, const std::any& param, long long ts)
            :resume_id(CoroutineAwaiter::INVALID_AWAITER_ID)
            ,ts(ts)
			,param(param)
            ,sleep_co_node(sleep_co_node)
        {}

        void swap(Event& other) {
            std::swap(sleep_co_node, other.sleep_co_node);
            std::swap(resume_id, other.resume_id);
            std::swap(ts, other.ts);
            func.swap(other.func);
            param.swap(other.param);
        }

        void reset() {
            EntryFunc().swap(func);
            param.reset();
        }
    };

	typedef struct BlockPointData {
		CoroutineNode* co_node;
		Event* timeout_event;
	} BlockPointData;

private:
    bool checkBusy() {
		if (!m_events.empty()) {
			return true;
		}
		if (CoroutineScheBaseImpl::checkBusy()) {
			return true;
		}
		return false;
    }

    int calculateWaitTimelen(long long cur_ts, int idle_timelen) {
        auto iter = m_timeout_events.begin();
        if (iter == m_timeout_events.end()) {
            return idle_timelen;
        }
        if (cur_ts >= iter->first) {
            return 0;
        }
        long long tlen = iter->first - cur_ts;
        if (idle_timelen <= 0) {
            return tlen > INT_MAX ? INT_MAX : (int)tlen;
        }
        return tlen < idle_timelen ? (int)tlen : idle_timelen;
    }

	Event* addTimeoutEvent(Event& e) {
        auto result_pair = m_timeout_events.try_emplace(e.ts, std::list<Event>());
        std::list<Event>& evlist = result_pair.first->second;
        evlist.emplace_back();
        evlist.back().swap(e);
		return &evlist.back();
    }

	Event* addTimeoutEvent(Event&& e) {
        auto result_pair = m_timeout_events.try_emplace(e.ts, std::list<Event>());
        std::list<Event>& evlist = result_pair.first->second;
        evlist.emplace_back(e);
		return &evlist.back();
    }

	void postEvent(const Event& e) {
		std::lock_guard<std::mutex> guard(m_mtx);
		bool is_busy = checkBusy();
		m_events.emplace_back(e);
		if (!is_busy) {
			m_cv.notify_one();
		}
	}

    void doPeakEvent(int wait_msec) {
        m_peak_events.clear();
        std::unique_lock lk(m_mtx);
        if (wait_msec >= 0) {
            m_cv.wait_for(lk, std::chrono::milliseconds(wait_msec), [this] { return ST_RUN != m_status || checkBusy(); });
        }
        else {
            m_cv.wait(lk, [this] { return ST_RUN != m_status || checkBusy(); });
        }
        if (ST_RUN != m_status) {
            return;
        }
        while (!m_events.empty() && m_peak_events.size() < m_handle_cnt) {
            size_t idx = m_peak_events.size();
            m_peak_events.resize(idx + 1);
            m_peak_events[idx].swap(m_events.front());
            m_events.pop_front();
        }
    }

    void handlePeakEvent(long long cur_ts) {
        for (size_t i = 0; i < m_peak_events.size(); ++i) {
            Event& e = m_peak_events[i];
            if (e.ts > 0 && cur_ts < e.ts) {
                addTimeoutEvent(e);
                continue;
            }
            if (e.func) {
				CoroutineScheBase::p = this;
                e.func(e.param);
            }
			else if (e.sleep_co_node) {
				doResume(e.sleep_co_node, e.param);
			}
			else if (e.resume_id != CoroutineAwaiter::INVALID_AWAITER_ID) {
				auto it = m_block_points.find(e.resume_id);
				if (it != m_block_points.end()) {
					BlockPointData& data = it->second;
					if (data.timeout_event) {
						data.timeout_event->resume_id = CoroutineAwaiter::INVALID_AWAITER_ID;
					}
					CoroutineNode* co_node = data.co_node;
					m_block_points.erase(it);
					doResume(co_node, e.param);
				}
            }
            e.reset();
        }
        m_peak_events.clear();
    }

    void handleTimeoutEvents(long long cur_ts) {
		int handle_cnt = m_handle_cnt;
        size_t cnt = 0;
        for (auto iter = m_timeout_events.begin(); cnt < handle_cnt && iter != m_timeout_events.end(); ) {
            if (cur_ts < iter->first) {
                break;
            }
            std::list<Event>& evlist = iter->second;
            while (!evlist.empty() && cnt < handle_cnt) {
                Event& e = evlist.front();
                if (e.func) {
					CoroutineScheBase::p = this;
                    e.func(e.param);
                }
                else if (e.sleep_co_node) {
                    doResume(e.sleep_co_node, e.param);
                }
                else if (e.resume_id != CoroutineAwaiter::INVALID_AWAITER_ID) {
					auto it = m_block_points.find(e.resume_id);
					if (it != m_block_points.end()) {
						CoroutineNode* co_node = it->second.co_node;
						m_block_points.erase(it);
						doResume(co_node, e.param);
					}
                }
                evlist.pop_front();
                cnt++;
            }
            if (evlist.empty()) {
                iter = m_timeout_events.erase(iter);
            }
        }
    }

	BlockPointData* regAwaiter(int32_t awaiter_id) {
		BlockPointData data;
		data.co_node = m_current_co_node;
		data.timeout_event = nullptr;
		auto result = m_block_points.insert({awaiter_id, data});
		if (!result.second) {
			return nullptr;
		}
		return &result.first->second;
	}

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
	int m_handle_cnt;
    std::atomic_int m_status;
    std::list<Event> m_events;
    std::vector<Event> m_peak_events;
    std::map<long long, std::list<Event> > m_timeout_events;
	std::unordered_map<int32_t, BlockPointData> m_block_points;
};
}

#endif
