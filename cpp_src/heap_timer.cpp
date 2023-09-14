//
// Created by hujianzhe on 20-11-9.
//

#include "../cpp_inc/heap_timer.h"
#include <algorithm>

namespace util {
HeapTimerEvent::HeapTimerEvent(const fn_callback& f):
	m_id(0),
	m_valid(true),
	m_timestamp(0),
	m_timer(NULL),
	m_callback(f)
{}

void HeapTimerEvent::doCallback() {
	if (m_callback) {
		m_callback(shared_from_this());
	}
}

HeapTimer::HeapTimer():
	m_idseq(0)
{}

HeapTimerEvent::sptr HeapTimer::addTimerEvent(const HeapTimerEvent::fn_callback& f, int64_t timestamp) {
	HeapTimerEvent::sptr e(new HeapTimerEvent(f));
	if (!addTimerEvent(e, timestamp)) {
		return HeapTimerEvent::sptr();
	}
	return e;
}

bool HeapTimer::addTimerEvent(HeapTimerEvent::sptr e, int64_t timestamp) {
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

void HeapTimer::delTimerEvent(HeapTimerEvent::sptr e) {
	if (e->m_timer != this) {
		return;
	}
	m_events.erase(e->m_id);
	e->m_id = 0;
	e->m_timer = NULL;
	e->m_valid = false;
}

void HeapTimer::doReset() {
	m_eventHeap.clear();
	m_events.clear();
	m_idseq = 0;
}

HeapTimerEvent::sptr HeapTimer::popTimeEvent(int64_t timestamp) {
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
		e->m_timer = NULL;
		return e;
	}
	return HeapTimerEvent::sptr();
}

int64_t HeapTimer::getNextTimestamp() {
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
}