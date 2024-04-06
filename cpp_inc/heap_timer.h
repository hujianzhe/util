//
// Created by hujianzhe on 20-11-9.
//

#ifndef	UTIL_CPP_HEAP_TIMER_H
#define	UTIL_CPP_HEAP_TIMER_H

#include <vector>
#include <algorithm>
#include <functional>

namespace util {
class HeapTimer;
class HeapTimerEvent;
typedef std::function<void(HeapTimer*, HeapTimerEvent*)> HeapTimerFunction;

class HeapTimerEvent {
friend class HeapTimer;
public:
	HeapTimerEvent(const HeapTimerFunction& f) :
		m_ptrSched(nullptr),
		m_timer(nullptr),
		m_timestamp(0),
		m_callback(f)
	{}

	virtual ~HeapTimerEvent() { detach(); }

	static void callback(HeapTimer* t, HeapTimerEvent* e) {
		if (e->m_callback) {
			e->m_callback(t, e);
		}
	}

	bool sched() const { return m_ptrSched != nullptr; }
	void detach() {
		if (m_ptrSched) {
			*m_ptrSched = nullptr;
			m_ptrSched = nullptr; 
			m_timer = nullptr;
		}
	}

	int64_t timestamp() const { return m_timestamp; }

private:
	HeapTimerEvent(const HeapTimerEvent&) {}
	HeapTimerEvent& operator=(const HeapTimerEvent&) { return *this; }

private:
	HeapTimerEvent** m_ptrSched;
	HeapTimer* m_timer;
	int64_t m_timestamp;
	HeapTimerFunction m_callback;
};

class HeapTimer {
public:
	HeapTimer() {}

	virtual ~HeapTimer() { clearEvents(); }

	HeapTimerEvent* setEvent(const HeapTimerFunction& f, int64_t timestamp) {
		if (timestamp < 0) {
			return nullptr;
		}
		HeapTimerEvent* e = new HeapTimerEvent(f);
		try {
			if (!setEvent(e, timestamp)) {
				delete e;
				return nullptr;
			}
			return e;
		}
		catch (...) {
			delete e;
			return nullptr;
		}
	}

	bool setEvent(HeapTimerEvent* e, int64_t timestamp) {
		if (timestamp < 0) {
			return false;
		}
		if (e->m_ptrSched) {
			if (e->m_timer != this) {
				return false;
			}
			if (e->m_timestamp != timestamp) {
				e->m_timestamp = timestamp;
				std::make_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			}
		}
		else {
			e->m_timestamp = timestamp;
			HeapTimerEvent** ep = new HeapTimerEvent*(e);
			try {
				m_eventHeap.push_back(ep);
				std::push_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			}
			catch (...) {
				delete ep;
				return false;
			}
			e->m_timer = this;
			e->m_ptrSched = ep;
		}
		return true;
	}

	void clearEvents() {
		for (size_t i = 0; i < m_eventHeap.size(); ++i) {
			HeapTimerEvent** ep = m_eventHeap[i];
			HeapTimerEvent* e = *ep;
			if (e) {
				e->m_timer = nullptr;
				e->m_ptrSched = nullptr;
			}
			delete ep;
		}
		m_eventHeap.clear();
	}

	HeapTimerEvent* popTimeoutEvent(int64_t timestamp) {
		while (!m_eventHeap.empty()) {
			HeapTimerEvent** ep = m_eventHeap.front();
			HeapTimerEvent* e = *ep;
			if (e && e->m_timestamp > timestamp) {
				break;
			}
			std::pop_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			m_eventHeap.pop_back();
			delete ep;
			if (!e) {
				continue;
			}
			e->m_timer = nullptr;
			e->m_ptrSched = nullptr;
			return e;
		}
		return nullptr;
	}

	int64_t getNextTimestamp() {
		while (!m_eventHeap.empty()) {
			HeapTimerEvent** ep = m_eventHeap.front();
			HeapTimerEvent* e = *ep;
			if (e) {
				return e->m_timestamp;
			}
			std::pop_heap(m_eventHeap.begin(), m_eventHeap.end(), heapCompare);
			m_eventHeap.pop_back();
			delete ep;
		}
		return -1;
	}

private:
	HeapTimer(const HeapTimer&) {}
	HeapTimer& operator=(const HeapTimer&) { return *this; }

	static bool heapCompare(HeapTimerEvent** ap, HeapTimerEvent** bp) {
		return (*ap ? (*ap)->m_timestamp : 0) > ( *bp ? (*bp)->m_timestamp : 0);
	}

private:
	std::vector<HeapTimerEvent**> m_eventHeap;
};
}

#endif
