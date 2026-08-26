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
		m_destination_peek(false),
		m_search_num(0),
		m_max_search_num(-1),
		m_prev_track_idx(-1)
	{
		m_current.g = m_current.h = 0;
	}

	struct ProcTrack {
		int g, h;
		UserDataType user_data;
	};

	size_t search_num() const { return m_search_num; }
	void set_max_search_num(size_t v) { m_max_search_num = v; }
	bool search_num_enough() const { return m_search_num < m_max_search_num; }
	bool arrived() const { return m_arrived; }
	const ProcTrack* current_track() const { return &m_current; }

	const ProcTrack* beginIter(const UserDataType& start) {
		m_prev_track_idx = -1;
		m_tracks.clear();
		m_openheap.clear();
		m_closeset.clear();
		m_destination_peek = false;
		m_arrived = false;
		m_search_num = 0;
		if (m_search_num >= m_max_search_num) {
			return nullptr;
		}
		m_closeset.insert(start);
		if (m_max_search_num != -1) {
			m_tracks.reserve(m_max_search_num);
			m_openheap.reserve(m_max_search_num);
		}
		m_current.g = 0;
		m_current.h = 0;
		m_current.user_data = start;
		return &m_current;
	}

	const ProcTrack* nextIter() {
		if (m_openheap.empty() || m_search_num >= m_max_search_num) {
			return nullptr;
		}
		++m_search_num;
		m_prev_track_idx = m_openheap.front();
		std::pop_heap(m_openheap.begin(), m_openheap.end(), OpenHeapCompare(m_tracks));
		m_openheap.pop_back();
		const ProcTrackListNode& t = m_tracks[m_prev_track_idx];
		m_current.g = t.g;
		m_current.h = t.f - t.g;
		m_current.user_data = t.user_data;
		return &m_current;
	}

	void arrivedDestination(const UserDataType& user_data) {
		m_destination = user_data;
		m_arrived = true;
		m_openheap.clear();
	}

	void insert(int g, int h, const UserDataType& user_data) {
		if (!m_closeset.insert(user_data).second) {
			return;
		}
		m_tracks.push_back({g, g + h, m_prev_track_idx, user_data});
		m_openheap.push_back(m_tracks.size() - 1);
		std::push_heap(m_openheap.begin(), m_openheap.end(), OpenHeapCompare(m_tracks));
	}

	bool exist(const UserDataType& user_data) const {
		return m_closeset.find(user_data) != m_closeset.end();
	}

	bool backtrace_pop(UserDataType* ret) {
		if (!m_destination_peek && m_arrived) {
			m_destination_peek = true;
			*ret = m_destination;
			return true;
		}
		if (-1 == m_prev_track_idx) {
			return false;
		}
		const ProcTrackListNode& t = m_tracks[m_prev_track_idx];
		m_prev_track_idx = t.from_idx;
		*ret = t.user_data;
		return true;
	}

private:
	AStarPathFinder(const AStarPathFinder&) = delete;
	AStarPathFinder& operator=(const AStarPathFinder&) = delete;

	struct ProcTrackListNode {
		int g, f;
		size_t from_idx;
		UserDataType user_data;
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
	bool m_destination_peek;
	size_t m_search_num;
	size_t m_max_search_num;
	size_t m_prev_track_idx;
	UserDataType m_destination;
	ProcTrack m_current;
	std::vector<ProcTrackListNode> m_tracks;
	std::vector<size_t> m_openheap;
	std::unordered_set<UserDataType> m_closeset;
};
}

#endif
