//
// Created by hujianzhe on 23-4-20
//

#ifndef UTIL_CPP_COROUTINE_DEFAULT_SCHE_H
#define UTIL_CPP_COROUTINE_DEFAULT_SCHE_H

#include "coroutine_helper.h"
#include <map>
#include <list>
#include <array>
#include <mutex>
#include <chrono>
#include <climits>
#include <functional>
#include <condition_variable>

namespace util {
class CoroutineDefaultSche : public CoroutineScheBase {
private:
    enum {
        ST_RUN = 0,
        ST_EXIT
    };
    enum {
        MAX_PEAK_CNT = 100
    };

public:
    typedef std::function<CoroutinePromise<void>(const std::any&)> EntryFunc;

    CoroutineDefaultSche()
        :CoroutineScheBase()
        ,m_status(ST_RUN)
    {
        m_peak_events.reserve(MAX_PEAK_CNT);
    }

    bool check_exit() { return ST_EXIT == m_status; }

    void doExit() {
        if (m_status.exchange(ST_EXIT) == ST_RUN) {
            m_cv.notify_one();
        }
    }

    long long get_current_ts_msec() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void readyExec(const EntryFunc& func, const std::any& param) {
        readyExecUtil(func, param, 0);
    }
    void readyExecTimeout(const EntryFunc& func, const std::any& param, long long tlen_msec) {
        if (!func) {
            return;
        }
        readyExecUtil(func, param, get_current_ts_msec() + tlen_msec);
    }
    void readyExecUtil(const EntryFunc& func, const std::any& param, long long ts_msec) {
        if (!func) {
            return;
        }
        std::lock_guard<std::mutex> guard(m_mtx);
        bool is_empty = check_need_wake_up();
        m_events.emplace_back(Event(func, param, ts_msec));
        if (is_empty) {
            m_cv.notify_one();
        }
    }

    CoroutineAwaiter blockPointUtil(long long ts_msec) {
        CoroutineAwaiter awaiter;
        if (regAwaiter(awaiter)) {
            addTimeoutEvent(Event(awaiter.id(), std::any(), ts_msec));
        }
        return awaiter;
    }
    CoroutineAwaiter blockPointTimeout(long long tlen_msec) {
        CoroutineAwaiter awaiter;
        if (regAwaiter(awaiter)) {
            addTimeoutEvent(Event(awaiter.id(), std::any(), get_current_ts_msec() + tlen_msec));
        }
        return awaiter;
    }

    CoroutineAwaiter sleepTimeout(long long tlen_msec) {
        CoroutineAwaiter awaiter;
        addTimeoutEvent(Event(this_coroutine(), get_current_ts_msec() + tlen_msec));
        return awaiter;
    }
    CoroutineAwaiter sleepUtil(long long ts_msec) {
        CoroutineAwaiter awaiter;
        addTimeoutEvent(Event(this_coroutine(), ts_msec));
        return awaiter;
    }

    void readyResume(int32_t id, const std::any& param) {
        if (CoroutineAwaiter::INVALID_AWAITER_ID == id) {
            return;
        }
        std::lock_guard<std::mutex> guard(m_mtx);
        bool is_empty = check_need_wake_up();
        m_events.emplace_back(Event(id, param, 0));
        if (is_empty) {
            m_cv.notify_one();
        }
    }

    void doSche(int idle_msec) {
        idle_msec = calculateWaitTimelen(get_current_ts_msec(), idle_msec);
        doPeakEvent(idle_msec);
        if (ST_RUN != m_status) {
            return;
        }
        handlePeakEvent(get_current_ts_msec());
        handleTimeoutEvents(get_current_ts_msec());
    }

    void scheDestroy() {
        std::unordered_set<CoroutineNode*> next_set;
        for (auto it = m_timeout_events.begin(); it != m_timeout_events.end(); ) {
            std::list<Event>& evlist = it->second;
            for (auto it = evlist.begin(); it != evlist.end(); ) {
                if (it->sleep_co_node) {
                    next_set.insert(it->sleep_co_node);
                }
                it = evlist.erase(it);
            }
            it = m_timeout_events.erase(it);
        }
        for (auto it = m_block_points.begin(); it != m_block_points.end(); ) {
            CoroutineNode* co_node = it->second;
            CoroutineNode* parent = onDestroy(co_node);
            if (parent) {
                next_set.insert(parent);
            }
            it = m_block_points.erase(it);
        }
        while (!next_set.empty()) {
            std::unordered_set<CoroutineNode*> tmp_set;
            for (auto it = next_set.begin(); it != next_set.end(); ) {
                CoroutineNode* co_node = *it;
                CoroutineNode* parent = onDestroy(co_node);
                if (parent) {
                    tmp_set.insert(parent);
                }
                it = next_set.erase(it);
            }
            next_set.swap(tmp_set);
        }
    }

private:
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
        Event(CoroutineNode* sleep_co_node, long long ts)
            :resume_id(CoroutineAwaiter::INVALID_AWAITER_ID)
            ,ts(ts)
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

private:
    bool check_need_wake_up() {
        return m_events.empty() && ST_RUN == m_status;
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

    void addTimeoutEvent(Event& e) {
        auto result_pair = m_timeout_events.try_emplace(e.ts, std::list<Event>());
        std::list<Event>& evlist = result_pair.first->second;
        evlist.emplace_back();
        evlist.back().swap(e);
    }

    void addTimeoutEvent(Event&& e) {
        auto result_pair = m_timeout_events.try_emplace(e.ts, std::list<Event>());
        std::list<Event>& evlist = result_pair.first->second;
        evlist.emplace_back(e);
    }

    void doPeakEvent(int wait_msec) {
        m_peak_events.clear();
        std::unique_lock lk(m_mtx);
        if (wait_msec >= 0) {
            m_cv.wait_for(lk, std::chrono::milliseconds(wait_msec), [this] { return !check_need_wake_up(); });
        }
        else {
            m_cv.wait(lk, [this] { return !check_need_wake_up(); });
        }
        if (ST_RUN != m_status) {
            return;
        }
        while (!m_events.empty() && m_peak_events.size() < MAX_PEAK_CNT) {
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
                e.func(e.param);
            }
            else {
                doResumeById(e.resume_id, e.param);
            }
            e.reset();
        }
        m_peak_events.clear();
    }

    void handleTimeoutEvents(long long cur_ts) {
        size_t cnt = 0;
        for (auto iter = m_timeout_events.begin(); cnt < MAX_PEAK_CNT && iter != m_timeout_events.end(); ) {
            if (cur_ts < iter->first) {
                break;
            }
            std::list<Event>& evlist = iter->second;
            while (!evlist.empty() && cnt < MAX_PEAK_CNT) {
                Event& e = evlist.front();
                if (e.func) {
                    e.func(e.param);
                }
                else if (e.sleep_co_node) {
                    doResume(e.sleep_co_node, std::any());
                }
                else {
                    doResumeById(e.resume_id, e.param);
                }
                evlist.pop_front();
                cnt++;
            }
            if (evlist.empty()) {
                iter = m_timeout_events.erase(iter);
            }
        }
    }

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::atomic_int m_status;
    std::list<Event> m_events;
    std::vector<Event> m_peak_events;
    std::map<long long, std::list<Event> > m_timeout_events;
};
}

#endif