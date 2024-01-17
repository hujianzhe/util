//
// Created by hujianzhe on 20-11-9.
//

#ifndef	UTIL_CPP_ASTAR_H
#define	UTIL_CPP_ASTAR_H

#include <stddef.h>
#include <list>
#include <vector>
#include <map>

namespace util {
class AStarBase {
public:
	typedef struct Node {
		int g, f;
		int version;
		struct Node* from;

		Node() :
			g(0), f(0),
			version(0),
			from(NULL)
		{}
		virtual ~Node() {}
	} Node;

	AStarBase() :
		m_curVersion(0)
	{}

	virtual ~AStarBase() {}

	virtual void init() = 0;

protected:
	typedef struct Walkable {
		Walkable() : max_search_num(-1) {}
		virtual ~Walkable() {}

		size_t max_search_num;
	} Walkable;

	static bool openheapCompare(const Node* a, const Node* b) { return a->f > b->f; }

protected:
	int m_curVersion;
	std::vector<Node*> m_openheap;
};

//////////////////////////////////////////////////////////////////////////////

class AStarGridBase : public AStarBase {
public:
	AStarGridBase(int xsize, int ysize) :
		AStarBase(),
		m_xsize(xsize),
		m_ysize(ysize),
		m_nodes_map(xsize * ysize)
	{}

	typedef struct Point {
		int x, y;

		Point(int px, int py) : x(px), y(py) {}
	} Point;

	typedef struct Node : public AStarBase::Node {
		int x, y;

		Node() : AStarBase::Node(), x(0), y(0) {}
	} Node;

	typedef struct Walkable : public AStarBase::Walkable {
		virtual bool canMove(const Node* node) const { return true; }
		virtual bool earlyFinish(const Node* node) const { return false; }
		virtual int G(const Node* open_node, const Node* cur_node, const Node* start_node) const { return 0; }
		virtual int H(const Node* open_node, const Node* end_node) const {
			int delta_x = end_node->x - open_node->x;
			int delta_y = end_node->y - open_node->y;
			if (delta_x < 0) {
				delta_x = -delta_x;
			}
			if (delta_y < 0) {
				delta_y = -delta_y;
			}
			return delta_x + delta_y;
		}
	} Walkable;

	int xsize() { return m_xsize; }
	int ysize() { return m_ysize; }

	virtual void init();

	bool findPath(const Point& sp, const Point& ep, std::list<Point>& poslist, const Walkable& walkable = Walkable());

private:
	virtual bool isStaticObstacle(int x, int y) { return false; }
	Node* tryOpenNode(int x, int y, const Walkable& walkable);

	static void merge(const Node* endpos, std::list<Point>& poslist);

private:
	int m_xsize;
	int m_ysize;
	std::vector<Node> m_nodes;
	std::vector<Node*> m_nodes_map;
};

//////////////////////////////////////////////////////////////////////////////

class AStarAdjacencyBase : public AStarBase {
public:
	typedef struct Node : public AStarBase::Node {
		int id;
		std::list<struct Node*> adjs;

		Node() : AStarBase::Node(), id(0) {}
	} Node;

	typedef struct Walkable : public AStarBase::Walkable {
		virtual bool canMove(const Node* node) const { return true; }
		virtual bool earlyFinish(const Node* node) const { return false; }
		virtual int G(const Node* open_node, const Node* cur_node, const Node* start_node) const { return 0; }
		virtual int H(const Node* open_node, const Node* end_node) const { return 0; }
	} Walkable;

	virtual void init() = 0;

	bool findPath(int sid, int eid, std::list<int>& idlist, const Walkable& walkable = Walkable());

protected:
	Node* addNode(int id);
	Node* getNode(int id);

private:
	bool tryOpenNode(Node* node, const Walkable& walkable);

private:
	std::map<int, Node> m_nodes;
};
}

#endif
