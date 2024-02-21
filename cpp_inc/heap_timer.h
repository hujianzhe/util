//
// Created by hujianzhe on 20-11-9.
//

#ifndef	UTIL_CPP_HEAP_TIMER_H
#define	UTIL_CPP_HEAP_TIMER_H

#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>

namespace util {
class HeapTimer;

class HeapTimerEvent : public std::enable_shared_from_this<HeapTimerEvent> {
friend class HeapTimer;
public:
	typedef std::shared_ptr<HeapTimerEvent> sptr;
	typedef std::function<void(sptr)> fn_callback;

	HeapTimerEvent(const fn_callback& f) :
		m_id(0),
		m_valid(true),
		m_timestamp(0),
		m_timer(nullptr),
		m_callback(f)
	{}

	virtual ~HeapTimerEvent() {}

	void doCallback() {
		if (m_callback) {
			m_callback(shared_from_this());
		}
	}

	int64_t timestamp() const { return m_timestamp; }

private:
	int m_id;
	bool m_valid;
	int64_t m_timestamp;
	HeapTimer* m_timer;
	fn_callback m_callback;
};

class HeapTimer {
public:
	typedef std::shared_ptr<HeapTimer> sptr;

	HeapTimer():
		m_idseq(0)
	{}

	virtual ~HeapTimer() {
		doReset();
	}

	HeapTimerEvent::sptr addTimerEvent(const HeapTimerEvent::fn_callback& f, int64_t timestamp) {
		HeapTimerEvent::sptr e(new HeapTimerEvent(f));
		if (!addTimerEvent(e, timestamp)) {
			return HeapTimerEvent::sptr();
		}
		return e;
	}

	bool addTimerEvent(HeapTimerEvent::sptr e, int64_t timestamp) {
		if (!e->m_valid) {
			return false;
		}
		if (timestamp < 0) {
			return false;
		}

		if (e->m_timer) {
			if (e->m_timer != this) {
				return false;
			}
			if (e->m_timestamp != timestamp) {
				e->m_timestamp = timestamp;
				std::make_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			}
			return true;
		}
		while ((++m_idseq) == 0);
		if (!m_events.insert(std::make_pair(m_idseq, e)).second) {
			return false;
		}
		e->m_id = m_idseq;
		e->m_timer = this;
		e->m_timestamp = timestamp;
		m_eventHeap.push_back(e);
		std::push_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
		return true;
	}

	void delTimerEvent(HeapTimerEvent::sptr e) {
		if (e->m_timer != this) {
			return;
		}
		m_events.erase(e->m_id);
		e->m_id = 0;
		e->m_timer = nullptr;
		e->m_valid = false;
	}

	void doReset() {
		for (auto iter = m_events.begin(); iter != m_events.end(); ) {
			iter->second->m_timer = nullptr;
			iter = m_events.erase(iter);
		}
		m_eventHeap.clear();
		m_idseq = 0;
	}

	HeapTimerEvent::sptr popTimeEvent(int64_t timestamp) {
		while (!m_eventHeap.empty()) {
			HeapTimerEvent::sptr e = m_eventHeap.front();
			if (e->m_timestamp > timestamp && e->m_timer) {
				break;
			}
			std::pop_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			m_eventHeap.pop_back();
			if (!e->m_timer) {
				continue;
			}
			m_events.erase(e->m_id);
			e->m_id = 0;
			e->m_timer = nullptr;
			return e;
		}
		return HeapTimerEvent::sptr();
	}

	int64_t getNextTimestamp() {
		while (!m_eventHeap.empty()) {
			HeapTimerEvent::sptr e = m_eventHeap.front();
			if (e->m_timer) {
				return e->m_timestamp;
			}
			std::pop_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			m_eventHeap.pop_back();
		}
		return -1;
	}

private:
	static bool heapCompare(HeapTimerEvent::sptr a, HeapTimerEvent::sptr b) { return a->m_timestamp > b->m_timestamp; }

private:
	int m_idseq;
	std::vector<HeapTimerEvent::sptr> m_eventHeap;
	std::unordered_map<int, HeapTimerEvent::sptr> m_events;
};
}

#endif
