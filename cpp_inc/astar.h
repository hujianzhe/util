//
// Created by hujianzhe on 20-11-9.
//

#ifndef	UTIL_CPP_ASTAR_H
#define	UTIL_CPP_ASTAR_H

#include <cstddef>
#include <algorithm>
#include <cmath>
#include <functional>
#include <list>
#include <unordered_set>
#include <vector>

namespace util {
class AStarWalkImpl;

class AStarNodeBase {
friend class AStarWalkImpl;
public:
	AStarNodeBase() : from(NULL), g(0), h(0), f(0) {}
	virtual ~AStarNodeBase() {}

	static bool openheapCompare(const AStarNodeBase* a, const AStarNodeBase* b) { return a->f > b->f; }

public:
	AStarNodeBase* from;
	int g, h, f;
};

template <typename Element>
class AStarNode : public AStarNodeBase {
public:
	AStarNode() : AStarNodeBase() {}
	AStarNode(const Element& v) : AStarNodeBase(), e(v) {}

public:
	Element e;
};

class AStarWalkImpl {
public:
	AStarWalkImpl() : m_cur_search_num(0), m_max_search_num(-1) {}
	virtual ~AStarWalkImpl() {}

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
			return NULL;
		}
		AStarNodeBase* next_node = m_openheap.front();
		std::pop_heap(m_openheap.begin(), m_openheap.end(), AStarNodeBase::openheapCompare);
		m_openheap.pop_back();
		return next_node;
	}

protected:
	size_t m_cur_search_num;
	size_t m_max_search_num;
	std::vector<AStarNodeBase*> m_openheap;
};

template <typename NodeId_t, typename NodeHash_t = std::hash<NodeId_t> >
class AStarWalkBase : public AStarWalkImpl {
public:
	void doStart(const NodeId_t& start_node_id) {
		m_closeset.clear();
		m_closeset.insert(start_node_id);
		m_openheap.resize(0);
		m_cur_search_num = 0;
	}

	bool useNodeId(const NodeId_t& id) {
		return m_closeset.insert(id).second;
	}

private:
	std::unordered_set<NodeId_t, NodeHash_t> m_closeset;
};

//////////////////////////////////////////////////////////////////////////////

template <typename T>
struct AStarPoint2D {
	union {
		T v[2];
		struct {
			T x;
			T y;
		};
	};

	AStarPoint2D() : x(0), y(0) {}
	AStarPoint2D(T px, T py) : x(px), y(py) {}

	bool operator==(const AStarPoint2D& p) const {
		return this->x == p.x && this->y == p.y;
	}
	bool operator!=(const AStarPoint2D& p) const {
		return this->x != p.x || this->y != p.y;
	}
	T getManhattanDistance(const AStarPoint2D& p) const {
		T deltaX = this->x - p.x;
		T deltaY = this->y - p.y;
		return std::abs(this->x - p.x) + std::abs(this->y - p.y);
	}
};

template <typename T, typename Point2D = AStarPoint2D<T> >
class AStarNode_Grid2D : public AStarNodeBase {
public:
	AStarNode_Grid2D() : util::AStarNodeBase() {}
	AStarNode_Grid2D(T x, T y) : util::AStarNodeBase(), p(x, y) {}

public:
	static void merge(const AStarNode_Grid2D* end_node, std::list<Point2D>& poslist) {
		if (!end_node) {
			return;
		}
		std::list<Point2D> templist;
		templist.push_front(end_node->p);
		const AStarNode_Grid2D* prev_node = end_node;
		T dx = 0;
		T dy = 0;
		for (const AStarNode_Grid2D* node = (const AStarNode_Grid2D*)end_node->from;
			node; node = (const AStarNode_Grid2D*)node->from)
		{
			int new_dx = node->p.x - prev_node->p.x;
			int new_dy = node->p.y - prev_node->p.y;
			if (new_dx != dx || new_dy != dy) {
				dx = new_dx;
				dy = new_dy;
				templist.push_front(node->p);
			}
			else {
				templist.front() = node->p;
			}
			prev_node = node;
		}
		poslist.splice(poslist.end(), templist);
	}

public:
	Point2D p;
};
}

namespace std {
	template <class T>
	struct hash<util::AStarPoint2D<T> > {
		typedef util::AStarPoint2D<T> argument_type;
		typedef size_t result_type;

		result_type operator()(const argument_type& p) const noexcept { return p.x + p.y; }
	};
}

#endif
