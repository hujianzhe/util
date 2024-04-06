//
// Created by hujianzhe on 20-11-9.
//

#ifndef	UTIL_CPP_HEAP_TIMER_H
#define	UTIL_CPP_HEAP_TIMER_H

#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

namespace util {
class HeapTimer;

class HeapTimerEvent : public std::enable_shared_from_this<HeapTimerEvent> {
friend class HeapTimer;
public:
	typedef std::shared_ptr<HeapTimerEvent> sptr;
	typedef std::function<void(sptr)> fn_callback;

	HeapTimerEvent(const fn_callback& f) :
		m_sched(false),
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

	bool sched() const { return m_sched; }

	int64_t timestamp() const { return m_timestamp; }

private:
	bool m_sched;
	int64_t m_timestamp;
	HeapTimer* m_timer;
	fn_callback m_callback;
};

class HeapTimer {
public:
	typedef std::shared_ptr<HeapTimer> sptr;

	HeapTimer() {}
	HeapTimer(const HeapTimer&) = delete;
	HeapTimer& operator=(const HeapTimer&) = delete;
	virtual ~HeapTimer() { clearEvents(); }

	HeapTimerEvent::sptr setEvent(const HeapTimerEvent::fn_callback& f, int64_t timestamp) {
		HeapTimerEvent::sptr e(new HeapTimerEvent(f));
		if (!setEvent(e, timestamp)) {
			return HeapTimerEvent::sptr();
		}
		return e;
	}

	bool setEvent(HeapTimerEvent::sptr e, int64_t timestamp) {
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
		}
		else {
			e->m_timer = this;
			e->m_timestamp = timestamp;
			m_eventHeap.push_back(e);
			std::push_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
		}
		e->m_sched = true;
		return true;
	}

	void removeEvent(HeapTimerEvent::sptr e) {
		if (e->m_timer != this) {
			return;
		}
		e->m_sched = false;
	}

	void clearEvents() {
		for (HeapTimerEvent::sptr& e : m_eventHeap) {
			e->m_timer = nullptr;
			e->m_sched = false;
		}
		m_eventHeap.clear();
	}

	HeapTimerEvent::sptr popTimeoutEvent(int64_t timestamp) {
		while (!m_eventHeap.empty()) {
			HeapTimerEvent::sptr e = m_eventHeap.front();
			if (e->m_timestamp > timestamp && e->m_sched) {
				break;
			}
			std::pop_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			m_eventHeap.pop_back();
			e->m_timer = nullptr;
			if (!e->m_sched) {
				continue;
			}
			e->m_sched = false;
			return e;
		}
		return HeapTimerEvent::sptr();
	}

	int64_t getNextTimestamp() {
		while (!m_eventHeap.empty()) {
			HeapTimerEvent::sptr e = m_eventHeap.front();
			if (e->m_sched) {
				return e->m_timestamp;
			}
			std::pop_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			m_eventHeap.pop_back();
		}
		return -1;
	}

private:
	static bool heapCompare(const HeapTimerEvent::sptr& a, const HeapTimerEvent::sptr& b) {
		return a->m_timestamp > b->m_timestamp;
	}

private:
	std::vector<HeapTimerEvent::sptr> m_eventHeap;
};
}

#endif
