//
// Created by hujianzhe on 23-9-14.
//

#ifndef	UTIL_CPP_HEAP_TIMER_H
#define	UTIL_CPP_HEAP_TIMER_H

#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace util {
class HeapTimer;

class HeapTimerEvent : public std::enable_shared_from_this<HeapTimerEvent> {
friend class HeapTimer;
public:
	typedef std::shared_ptr<HeapTimerEvent> sptr;
	typedef std::function<void(sptr)> fn_callback;

	HeapTimerEvent(const fn_callback& f);
	virtual ~HeapTimerEvent() {}

	void doCallback();

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

	HeapTimer();
	virtual ~HeapTimer() {
		doReset();
	}

	HeapTimerEvent::sptr addTimerEvent(const HeapTimerEvent::fn_callback& f, int64_t timestamp);
	bool addTimerEvent(HeapTimerEvent::sptr e, int64_t timestamp);
	void delTimerEvent(HeapTimerEvent::sptr e);
	void doReset();

	HeapTimerEvent::sptr popTimeEvent(int64_t timestamp);
	int64_t getNextTimestamp();

private:
	static bool heapCompare(HeapTimerEvent::sptr a, HeapTimerEvent::sptr b) { return a->m_timestamp > b->m_timestamp; }

private:
	int m_idseq;
	std::vector<HeapTimerEvent::sptr> m_eventHeap;
	std::unordered_map<int, HeapTimerEvent::sptr> m_events;
};
}

#endif
