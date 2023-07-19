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
	typedef struct Point {
		int x, y;

		Point(int px, int py) : x(px), y(py) {}
	} Point;

	typedef struct Pos {
		int x, y;
		int g, f;
		int version;
		struct Pos* from;

		Pos() :
			x(0), y(0),
			g(0), f(0),
			version(0),
			from(NULL)
		{}
		virtual ~Pos() {}
	} Pos;

	typedef struct Walkable {
		virtual bool canMove(const Pos* pos) const { return true; }
		virtual bool earlyFinish(const Pos* pos) const { return false; }
		virtual ~Walkable() {}
	} Walkable;
	static Walkable def_walkable;

	AStarBase() :
		m_curVersion(0),
		m_maxSearchPoint(1024)
	{}

	virtual ~AStarBase() {}

	size_t maxSearchPoint() { return m_maxSearchPoint; }
	void maxSearchPoint(size_t v) {
		m_maxSearchPoint = v;
		m_openheap.reserve(v);
	}

	virtual void init() = 0;

protected:
	virtual int H(const Pos* openpos, const Pos* endpos) {
		int delta_x = endpos->x - openpos->x;
		int delta_y = endpos->y - openpos->y;
		if (delta_x < 0) {
			delta_x = -delta_x;
		}
		if (delta_y < 0) {
			delta_y = -delta_y;
		}
		return delta_x + delta_y;
	}
	virtual int G(const Pos* openpos, const Pos* curpos, const Pos* startpos) { return 0; }

	static bool openheapCompare(const Pos* a, const Pos* b) { return a->f > b->f; }

protected:
	int m_curVersion;
	size_t m_maxSearchPoint;
	std::vector<Pos*> m_openheap;
};

//////////////////////////////////////////////////////////////////////////////

class AStarGridBase : public AStarBase {
public:
	AStarGridBase(int xsize, int ysize) :
		AStarBase(),
		m_xsize(xsize),
		m_ysize(ysize),
		m_posmap(xsize * ysize)
	{}

	int xsize() { return m_xsize; }
	int ysize() { return m_ysize; }

	virtual void init();

	bool findPath(int sx, int sy, int ex, int ey, std::list<AStarBase::Point>& poslist, const AStarBase::Walkable& walkable = AStarBase::def_walkable);

private:
	virtual bool isStaticObstacle(int x, int y) { return false; }
	AStarBase::Pos* tryOpenPos(int x, int y, const AStarBase::Walkable& walkable);

	static void merge(const AStarBase::Pos* endpos, std::list<AStarBase::Point>& poslist);

private:
	int m_xsize;
	int m_ysize;
	std::vector<Pos> m_poses;
	std::vector<Pos*> m_posmap;
};

//////////////////////////////////////////////////////////////////////////////

class AStarAdjPointBase : public AStarBase {
public:
	typedef struct Pos : public AStarBase::Pos {
		int id;
		std::list<struct Pos*> adjs;

		Pos() :
			AStarBase::Pos(),
			id(0)
		{}
	} Pos;

	virtual void init() = 0;

	bool findPath(int sid, int eid, std::list<int>& idlist, const AStarBase::Walkable& walkable = AStarBase::def_walkable);

protected:
	AStarAdjPointBase::Pos* addPos(int id);
	AStarAdjPointBase::Pos* getPos(int id);

private:
	AStarBase::Pos* tryOpenPos(AStarAdjPointBase::Pos* pos, const AStarBase::Walkable& walkable);

private:
	std::map<int, Pos> m_poses;
};
}

#endif
