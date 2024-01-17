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
	m_poses.resize(cnt);
	cnt = 0;
	for (int x = 0; x < m_xsize; ++x) {
		for (int y = 0; y < m_ysize; ++y) {
			if (isStaticObstacle(x, y)) {
				continue;
			}
			Pos* pos = &m_poses[cnt++];
			m_posmap[x + y * m_xsize] = pos;
			pos->x = x;
			pos->y = y;
		}
	}

	m_openheap.reserve(m_maxSearchPoint);
}

AStarGridBase::Pos* AStarGridBase::tryOpenPos(int x, int y, const AStarGridBase::Walkable& walkable) {
	if (x < 0 || x >= m_xsize || y < 0 || y >= m_ysize) {
		return NULL;
	}
	Pos* pos = m_posmap[x + y * m_xsize];
	if (!pos) {
		return NULL;
	}
	if (pos->version == m_curVersion) {
		return NULL;
	}
	pos->version = m_curVersion;
	if (!walkable.canMove(pos)) {
		return NULL;
	}
	return pos;
}

bool AStarGridBase::findPath(int sx, int sy, int ex, int ey, std::list<AStarGridBase::Point>& poslist, const AStarGridBase::Walkable& walkable) {
	if (sx == ex && sy == ey) {
		return true;
	}
	if (sx < 0 || sy < 0 || sx >= m_xsize || sy >= m_ysize ||
		ex < 0 || ey < 0 || ex >= m_xsize || ey >= m_ysize)
	{
		return false;
	}
	Pos* curpos = m_posmap[sx + sy * m_xsize];
	if (!curpos) {
		return false;
	}
	if (walkable.earlyFinish(curpos)) {
		poslist.push_back(Point(sx, sy));
		return true;
	}
	Pos* endpos = m_posmap[ex + ey * m_xsize];
	if (!endpos || !walkable.canMove(endpos)) {
		return false;
	}

	if (0 == ++m_curVersion) {
		for (size_t i = 0; i < m_poses.size(); ++i) {
			m_poses[i].version = 0;
		}
		++m_curVersion;
	}
	curpos->from = NULL;
	curpos->version = m_curVersion;
	m_openheap.resize(0);
	Pos* startpos = curpos;
	Pos* nextpos = NULL;
	size_t cur_search_point = 0;
	while (true) {
		Pos* vec_openpos[8];
		unsigned int vec_openpos_cnt = 0;
		Pos* openpos = NULL;
		if (cur_search_point >= m_maxSearchPoint) {
			return false;
		}

		openpos = tryOpenPos(curpos->x - 1, curpos->y, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x + 1, curpos->y, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x, curpos->y - 1, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x, curpos->y + 1, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x - 1, curpos->y - 1, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x + 1, curpos->y - 1, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x - 1, curpos->y + 1, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}
		openpos = tryOpenPos(curpos->x + 1, curpos->y + 1, walkable);
		if (openpos) {
			vec_openpos[vec_openpos_cnt++] = openpos;
		}

		if (vec_openpos_cnt) {
			for (unsigned int i = 0; i < vec_openpos_cnt; ++i) {
				openpos = vec_openpos[i];
				openpos->g = walkable.G(openpos, curpos, startpos);
				openpos->f = walkable.H(openpos, endpos) + openpos->g;
				openpos->from = curpos;
				m_openheap.push_back(openpos);
				std::push_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
			}
			cur_search_point += vec_openpos_cnt;
		}
		if (m_openheap.empty()) {
			return false;
		}

		nextpos = static_cast<Pos*>(m_openheap.front());
		std::pop_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
		m_openheap.pop_back();

		if (nextpos->x == ex && nextpos->y == ey) {
			break;
		}
		if (walkable.earlyFinish(nextpos)) {
			break;
		}
		curpos = nextpos;
	}

	merge(nextpos, poslist);
	return true;
}

void AStarGridBase::merge(const AStarGridBase::Pos* endpos, std::list<AStarGridBase::Point>& poslist) {
	std::list<Point> templist;
	templist.push_front(Point(endpos->x, endpos->y));
	const Pos* npos = endpos;
	int dx = 0;
	int dy = 0;
	for (const Pos* pos = static_cast<const Pos*>(endpos->from); pos; pos = static_cast<const Pos*>(pos->from)) {
		int new_dx = pos->x - npos->x;
		int new_dy = pos->y - npos->y;
		if (new_dx != dx || new_dy != dy) {
			dx = new_dx;
			dy = new_dy;
			templist.push_front(Point(pos->x, pos->y));
		} else {
			templist.front().x = pos->x;
			templist.front().y = pos->y;
		}
		npos = pos;
	}
	poslist.splice(poslist.end(), templist);
}

//////////////////////////////////////////////////////////////////////////////

AStarAdjPointBase::Pos* AStarAdjPointBase::addPos(int id) {
	Pos* pos = &m_poses[id];
	pos->id = id;
	return pos;
}

AStarAdjPointBase::Pos* AStarAdjPointBase::getPos(int id) {
	std::map<int, Pos>::iterator it = m_poses.find(id);
	return it != m_poses.end() ? &it->second : NULL;
}

bool AStarAdjPointBase::tryOpenPos(AStarAdjPointBase::Pos* pos, const AStarAdjPointBase::Walkable& walkable) {
	if (pos->version == m_curVersion) {
		return false;
	}
	pos->version = m_curVersion;
	if (!walkable.canMove(pos)) {
		return false;
	}
	return true;
}

bool AStarAdjPointBase::findPath(int sid, int eid, std::list<int>& idlist, const AStarAdjPointBase::Walkable& walkable) {
	if (sid == eid) {
		return true;
	}
	Pos* curpos = getPos(sid);
	if (!curpos) {
		return false;
	}
	if (walkable.earlyFinish(curpos)) {
		idlist.push_back(sid);
		return true;
	}
	Pos* endpos = getPos(eid);
	if (!endpos || !walkable.canMove(endpos)) {
		return false;
	}

	if (0 == ++m_curVersion) {
		for (std::map<int, Pos>::iterator it = m_poses.begin(); it != m_poses.end(); ++it) {
			it->second.version = 0;
		}
		++m_curVersion;
	}
	curpos->from = NULL;
	curpos->version = m_curVersion;
	m_openheap.resize(0);
	Pos* startpos = curpos;
	Pos* nextpos = NULL;
	size_t cur_search_point = 0;
	while (true) {
		std::vector<Pos*> vec_openpos;
		if (cur_search_point >= m_maxSearchPoint) {
			return false;
		}

		for (std::list<Pos*>::const_iterator it = curpos->adjs.begin(); it != curpos->adjs.end(); ++it) {
			Pos* openpos = *it;
			if (!tryOpenPos(openpos, walkable)) {
				continue;
			}
			vec_openpos.push_back(openpos);
		}
		if (!vec_openpos.empty()) {
			for (size_t i = 0; i < vec_openpos.size(); ++i) {
				Pos* openpos = vec_openpos[i];
				openpos->g = walkable.G(openpos, curpos, startpos);
				openpos->f = walkable.H(openpos, endpos) + openpos->g;
				openpos->from = curpos;
				m_openheap.push_back(openpos);
				std::push_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
			}
			cur_search_point += vec_openpos.size();
		}
		if (m_openheap.empty()) {
			return false;
		}

		nextpos = static_cast<Pos*>(m_openheap.front());
		std::pop_heap(m_openheap.begin(), m_openheap.end(), openheapCompare);
		m_openheap.pop_back();

		if (nextpos->id == endpos->id) {
			break;
		}
		if (walkable.earlyFinish(nextpos)) {
			break;
		}
		curpos = nextpos;
	}
	std::list<int> templist;
	for (const Pos* pos = nextpos; pos; pos = static_cast<const Pos*>(pos->from)) {
		templist.push_front(pos->id);
	}
	idlist.splice(idlist.end(), templist);
	return true;
}
}
