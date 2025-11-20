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
template <typename UserDataType>
class AStarPathFinder {
public:
	AStarPathFinder() :
		m_arrived(false),
		m_max_search_num(-1),
		m_prev_track_idx(-1)
	{
		m_current.g = m_current.h = 0;
		m_current.user_data = nullptr;
	}

	struct ProcTrack {
		int g, h;
		const UserDataType* user_data;
	};

	size_t search_num() const { return m_tracks.size(); }
	bool left_search_num_enough() const { return m_tracks.size() < m_max_search_num; }
	bool arrived() const { return m_arrived; }
	void set_arrived() { m_arrived = true; }

	ProcTrack* setStart(const UserDataType* start_udata, size_t max_search_num = -1) {
		m_prev_track_idx = -1;
		m_tracks.clear();
		m_openheap.clear();
		m_closeset.clear();
		m_closeset.insert(start_udata);
		m_arrived = false;
		m_max_search_num = max_search_num;
		if (m_max_search_num != -1) {
			m_tracks.reserve(m_max_search_num);
			m_openheap.reserve(m_max_search_num);
		}
		m_current.g = 0;
		m_current.h = 0;
		m_current.user_data = start_udata;
		return &m_current;
	}

	void pushCandidate(int g, int h, const UserDataType* user_data) {
		m_tracks.push_back({g, g + h, m_prev_track_idx, user_data});
		m_openheap.push_back(m_tracks.size() - 1);
		std::push_heap(m_openheap.begin(), m_openheap.end(), OpenHeapCompare(m_tracks));
	}

	ProcTrack* popCandidate() {
		if (m_openheap.empty()) {
			return nullptr;
		}
		m_prev_track_idx = m_openheap.front();
		std::pop_heap(m_openheap.begin(), m_openheap.end(), OpenHeapCompare(m_tracks));
		m_openheap.pop_back();
		const ProcTrackListNode& t = m_tracks[m_prev_track_idx];
		m_closeset.insert(t.user_data);
		m_current.g = t.g;
		m_current.h = t.f - t.g;
		m_current.user_data = t.user_data;
		return &m_current;
	}

	bool isDetected(const UserDataType* user_data) const {
		return m_closeset.find(user_data) != m_closeset.end();
	}

	const UserDataType* popTrack() {
		if (-1 == m_prev_track_idx) {
			return nullptr;
		}
		const ProcTrackListNode& t = m_tracks[m_prev_track_idx];
		m_prev_track_idx = t.from_idx;
		return t.user_data;
	}

private:
	AStarPathFinder(const AStarPathFinder&) = delete;
	AStarPathFinder& operator=(const AStarPathFinder&) = delete;

	struct ProcTrackListNode {
		int g, f;
		size_t from_idx;
		const UserDataType* user_data;
	};
	struct OpenHeapCompare {
		const std::vector<ProcTrackListNode>& tracks;
		OpenHeapCompare(const std::vector<ProcTrackListNode>& t) : tracks(t) {}
		bool operator()(size_t a, size_t b) const {
			return tracks[a].f > tracks[b].f;
		}
	};

private:
	bool m_arrived;
	size_t m_max_search_num;
	size_t m_prev_track_idx;
	ProcTrack m_current;
	std::vector<ProcTrackListNode> m_tracks;
	std::vector<size_t> m_openheap;
	std::unordered_set<const UserDataType*> m_closeset;
};
}

#endif
