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

class HeapTimerEvent final {
friend class HeapTimer;
public:
	HeapTimerEvent(const HeapTimerFunction& f = nullptr) :
		m_ptrSched(nullptr),
		m_timer(nullptr),
		m_timestamp(0),
		m_func(f)
	{}

	~HeapTimerEvent() { detach(); }

	int64_t timestamp() const { return m_timestamp; }

	const HeapTimerFunction& func() const { return m_func; }
	void set_func(const HeapTimerFunction& f) { m_func = f; }

	void callback() {
		if (m_func) {
			m_func(m_timer, this);
		}
	}

	bool sched() const { return m_ptrSched != nullptr; }

	void detach() {
		if (m_ptrSched) {
			m_ptrSched->e = nullptr;
			m_ptrSched = nullptr;
			m_timer = nullptr;
		}
	}

private:
	HeapTimerEvent(const HeapTimerEvent&) {}
	HeapTimerEvent& operator=(const HeapTimerEvent&) { return *this; }
	struct ScheNode {
		HeapTimerEvent* e;
		int64_t t;
	};

private:
	ScheNode* m_ptrSched;
	HeapTimer* m_timer;
	int64_t m_timestamp;
	HeapTimerFunction m_func;
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
				e->m_ptrSched->e = e;
				e->m_ptrSched->t = timestamp;
				std::make_heap(m_nodes.begin(), m_nodes.end(), heapCompare);
			}
		}
		else {
			HeapTimerEvent::ScheNode* node = new HeapTimerEvent::ScheNode();
			try {
				node->e = e;
				node->t = timestamp;
				m_nodes.push_back(node);
				std::push_heap(m_nodes.begin(), m_nodes.end(), heapCompare);
			}
			catch (...) {
				delete node;
				return false;
			}
			e->m_timestamp = timestamp;
			e->m_timer = this;
			e->m_ptrSched = node;
		}
		return true;
	}

	void clearEvents() {
		for (size_t i = 0; i < m_nodes.size(); ++i) {
			HeapTimerEvent::ScheNode* node = m_nodes[i];
			HeapTimerEvent* e = node->e;
			if (e) {
				e->m_timer = nullptr;
				e->m_ptrSched = nullptr;
			}
			delete node;
		}
		m_nodes.clear();
	}

	HeapTimerEvent* popTimeoutEvent(int64_t timestamp) {
		while (!m_nodes.empty()) {
			HeapTimerEvent::ScheNode* node = m_nodes.front();
			HeapTimerEvent* e = node->e;
			if (e && e->m_timestamp > timestamp) {
				break;
			}
			std::pop_heap(m_nodes.begin(), m_nodes.end(), heapCompare);
			m_nodes.pop_back();
			delete node;
			if (!e) {
				continue;
			}
			e->m_ptrSched = nullptr;
			return e;
		}
		return nullptr;
	}

	int64_t getNextTimestamp() {
		while (!m_nodes.empty()) {
			HeapTimerEvent::ScheNode* node = m_nodes.front();
			HeapTimerEvent* e = node->e;
			if (e) {
				return e->m_timestamp;
			}
			std::pop_heap(m_nodes.begin(), m_nodes.end(), heapCompare);
			m_nodes.pop_back();
			delete node;
		}
		return -1;
	}

private:
	HeapTimer(const HeapTimer&) {}
	HeapTimer& operator=(const HeapTimer&) { return *this; }

	static bool heapCompare(HeapTimerEvent::ScheNode* ap, HeapTimerEvent::ScheNode* bp) {
		return ap->t > bp->t;
	}

private:
	std::vector<HeapTimerEvent::ScheNode*> m_nodes;
};
}

#endif
