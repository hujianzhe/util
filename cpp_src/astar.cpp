//
// Created by hujianzhe on 20-11-9.
//

#include "../cpp_inc/astar.h"
#include <algorithm>

namespace util {
//////////////////////////////////////////////////////////////////////////////

void AStarGridBase::init() {
	size_t cnt = 0;
	for (int x = 0; x < m_xsize; ++x) {
		for (int y = 0; y < m_ysize; ++y) {
			if (isStaticObstacle(x, y)) {
				continue;
			}
			++cnt;
		}
	}
	m_nodes.resize(cnt);
	cnt = 0;
	for (int x = 0; x < m_xsize; ++x) {
		for (int y = 0; y < m_ysize; ++y) {
			if (isStaticObstacle(x, y)) {
				continue;
			}
			Node* node = &m_nodes[cnt++];
			m_nodes_map[x + y * m_xsize] = node;
			node->x = x;
			node->y = y;
		}
	}
}

AStarGridBase::Node* AStarGridBase::tryOpenNode(int x, int y, const Walkable& walkable) {
	if (x < 0 || x >= m_xsize || y < 0 || y >= m_ysize) {
		return NULL;
	}
	Node* node = m_nodes_map[x + y * m_xsize];
	if (!node) {
		return NULL;
	}
	if (node->version == m_curVersion) {
		return NULL;
	}
	node->version = m_curVersion;
	if (!walkable.canMove(node)) {
		return NULL;
	}
	return node;
}

bool AStarGridBase::findPath(const Point& sp, const Point& ep, std::list<Point>& poslist, const Walkable& walkable) {
	if (sp.x == ep.x && sp.y == ep.y) {
		return true;
	}
	if (sp.x < 0 || sp.y < 0 || sp.x >= m_xsize || sp.y >= m_ysize ||
		ep.x < 0 || ep.y < 0 || ep.x >= m_xsize || ep.y >= m_ysize)
	{
		return false;
	}
	Node* cur_node = m_nodes_map[sp.x + sp.y * m_xsize];
	if (!cur_node) {
		return false;
	}
	if (walkable.earlyFinish(cur_node)) {
		poslist.push_back(sp);
		return true;
	}
	Node* end_node = m_nodes_map[ep.x + ep.y * m_xsize];
	if (!end_node || !walkable.canMove(end_node)) {
		return false;
	}

	if (0 == ++m_curVersion) {
		for (size_t i = 0; i < m_nodes.size(); ++i) {
			m_nodes[i].version = 0;
		}
		++m_curVersion;
	}
	cur_node->from = NULL;
	cur_node->version = m_curVersion;
	m_openheap.resize(0);
	Node* start_node = cur_node;
	Node* next_node = NULL;
	size_t cur_search_num = 0;
	if (walkable.max_search_num != -1) {
		m_openheap.reserve(walkable.max_search_num);
	}
	while (true) {
		Node* vec_open_nodes[8];
		unsigned int vec_open_nodes_cnt = 0;
		Node* open_node = NULL;
		if (cur_search_num >= walkable.max_search_num) {
			return false;
		}

		open_node = tryOpenNode(cur_node->x - 1, cur_node->y, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x + 1, cur_node->y, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x, cur_node->y - 1, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x, cur_node->y + 1, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x - 1, cur_node->y - 1, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x + 1, cur_node->y - 1, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x - 1, cur_node->y + 1, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}
		open_node = tryOpenNode(cur_node->x + 1, cur_node->y + 1, walkable);
		if (open_node) {
			vec_open_nodes[vec_open_nodes_cnt++] = open_node;
		}

		if (vec_open_nodes_cnt) {
			for (unsigned int i = 0; i < vec_open_nodes_cnt; ++i) {
				open_node = vec_open_nodes[i];
				open_node->g = walkable.G(open_node, cur_node, start_node);
				open_node->f = walkable.H(open_node, end_node) + open_node->g;
				open_node->from = cur_node;
				m_openheap.push_back(open_node);
				std::push_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
			}
			cur_search_num += vec_open_nodes_cnt;
		}
		if (m_openheap.empty()) {
			return false;
		}

		next_node = static_cast<Node*>(m_openheap.front());
		std::pop_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
		m_openheap.pop_back();

		if (next_node->x == ep.x && next_node->y == ep.y) {
			break;
		}
		if (walkable.earlyFinish(next_node)) {
			break;
		}
		cur_node = next_node;
	}

	merge(next_node, poslist);
	return true;
}

void AStarGridBase::merge(const Node* end_node, std::list<Point>& poslist) {
	std::list<Point> templist;
	templist.push_front(Point(end_node->x, end_node->y));
	const Node* prev_node = end_node;
	int dx = 0;
	int dy = 0;
	for (const Node* node = static_cast<const Node*>(end_node->from); node; node = static_cast<const Node*>(node->from)) {
		int new_dx = node->x - prev_node->x;
		int new_dy = node->y - prev_node->y;
		if (new_dx != dx || new_dy != dy) {
			dx = new_dx;
			dy = new_dy;
			templist.push_front(Point(node->x, node->y));
		} else {
			templist.front().x = node->x;
			templist.front().y = node->y;
		}
		prev_node = node;
	}
	poslist.splice(poslist.end(), templist);
}

//////////////////////////////////////////////////////////////////////////////

AStarAdjacencyBase::Node* AStarAdjacencyBase::addNode(int id) {
	Node* node = &m_nodes[id];
	node->id = id;
	return node;
}

AStarAdjacencyBase::Node* AStarAdjacencyBase::getNode(int id) {
	std::map<int, Node>::iterator it = m_nodes.find(id);
	return it != m_nodes.end() ? &it->second : NULL;
}

bool AStarAdjacencyBase::tryOpenNode(Node* node, const Walkable& walkable) {
	if (node->version == m_curVersion) {
		return false;
	}
	node->version = m_curVersion;
	if (!walkable.canMove(node)) {
		return false;
	}
	return true;
}

bool AStarAdjacencyBase::findPath(int sid, int eid, std::list<int>& idlist, const Walkable& walkable) {
	if (sid == eid) {
		return true;
	}
	Node* cur_node = getNode(sid);
	if (!cur_node) {
		return false;
	}
	if (walkable.earlyFinish(cur_node)) {
		idlist.push_back(sid);
		return true;
	}
	Node* end_node = getNode(eid);
	if (!end_node || !walkable.canMove(end_node)) {
		return false;
	}

	if (0 == ++m_curVersion) {
		for (std::map<int, Node>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
			it->second.version = 0;
		}
		++m_curVersion;
	}
	cur_node->from = NULL;
	cur_node->version = m_curVersion;
	m_openheap.resize(0);
	Node* start_node = cur_node;
	Node* next_node = NULL;
	size_t cur_search_num = 0;
	if (walkable.max_search_num != -1) {
		m_openheap.reserve(walkable.max_search_num);
	}
	while (true) {
		std::vector<Node*> vec_open_nodes;
		if (cur_search_num >= walkable.max_search_num) {
			return false;
		}

		for (std::list<Node*>::const_iterator it = cur_node->adjs.begin(); it != cur_node->adjs.end(); ++it) {
			Node* open_node = *it;
			if (!tryOpenNode(open_node, walkable)) {
				continue;
			}
			vec_open_nodes.push_back(open_node);
		}
		if (!vec_open_nodes.empty()) {
			for (size_t i = 0; i < vec_open_nodes.size(); ++i) {
				Node* open_node = vec_open_nodes[i];
				open_node->g = walkable.G(open_node, cur_node, start_node);
				open_node->f = walkable.H(open_node, end_node) + open_node->g;
				open_node->from = cur_node;
				m_openheap.push_back(open_node);
				std::push_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
			}
			cur_search_num += vec_open_nodes.size();
		}
		if (m_openheap.empty()) {
			return false;
		}

		next_node = static_cast<Node*>(m_openheap.front());
		std::pop_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
		m_openheap.pop_back();

		if (next_node->id == end_node->id) {
			break;
		}
		if (walkable.earlyFinish(next_node)) {
			break;
		}
		cur_node = next_node;
	}
	std::list<int> templist;
	for (const Node* node = next_node; node; node = static_cast<const Node*>(node->from)) {
		templist.push_front(node->id);
	}
	idlist.splice(idlist.end(), templist);
	return true;
}
}
