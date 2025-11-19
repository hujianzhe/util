//
// Created by hujianzhe on 20-11-9.
//

#ifndef	UTIL_CPP_ASTAR_H
#define	UTIL_CPP_ASTAR_H

#include <cstddef>
#include <vector>
#include <algorithm>
#include <unordered_set>

namespace util {
class AStarNodeBase {
public:
	virtual ~AStarNodeBase() {}

	static bool openheapCompare(const AStarNodeBase* a, const AStarNodeBase* b) { return a->f > b->f; }

protected:
	AStarNodeBase() : from(NULL), g(0), h(0), f(0) {}

public:
	const AStarNodeBase* from;
	int g, h, f;
};

class AStarPathFinder {
public:
	AStarPathFinder() : m_max_search_num(-1) { reset(); }
	virtual ~AStarPathFinder() {}

	void setMaxSearchNum(size_t num) {
		m_max_search_num = num;
		if (m_max_search_num != -1) {
			m_openheap.reserve(m_max_search_num);
		}
	}

	bool reachMaxSearch() const {
		return m_cur_search_num >= m_max_search_num && m_max_search_num != -1;
	}

	bool addOpenHeap(AStarNodeBase* node, AStarNodeBase* from_node) {
		if (m_cur_search_num < m_max_search_num) {
			m_cur_search_num++;
		}
		else if (m_max_search_num != -1) {
			return false;
		}
		node->f = node->h + node->g;
		node->from = from_node;
		m_openheap.push_back(node);
		std::push_heap(m_openheap.begin(), m_openheap.end(), AStarNodeBase::openheapCompare);
		return true;
	}

	AStarNodeBase* popOpenHeap() {
		if (m_openheap.empty()) {
			return nullptr;
		}
		AStarNodeBase* next_node = m_openheap.front();
		std::pop_heap(m_openheap.begin(), m_openheap.end(), AStarNodeBase::openheapCompare);
		m_openheap.pop_back();
		return next_node;
	}

	void reset() {
		m_closeset.clear();
		m_openheap.resize(0);
		m_cur_search_num = 0;
	}

	bool addCloseSet(const AStarNodeBase* node) {
		return m_closeset.insert(node).second;
	}

private:
	AStarPathFinder(const AStarPathFinder&) = delete;
	AStarPathFinder& operator=(const AStarPathFinder&) = delete;

protected:
	size_t m_max_search_num;
	size_t m_cur_search_num;
	std::vector<AStarNodeBase*> m_openheap;
	std::unordered_set<const AStarNodeBase*> m_closeset;
};
}

#endif
