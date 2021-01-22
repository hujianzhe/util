//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_GRAPH_H
#define	UTIL_C_DATASTRUCT_GRAPH_H

#include "hashtable.h"
#include "list.h"

typedef struct Graph_t {
	List_t vnodelist;
	int dfs_visit;
} Graph_t;

struct GraphEdge_t;
typedef struct GraphNode_t {
	ListNode_t node;
	Graph_t* graph;
	List_t edgelist[2];
	ptrlen_t degree[2];
	struct {
		struct GraphEdge_t* visit_from_edgenode;
		int dfs_visit;
	};
} GraphNode_t;

typedef struct GraphEdge_t {
	union {
		GraphNode_t* v[2];
		struct {
			GraphNode_t* v_start;
			GraphNode_t* v_end;
		};
	};
	ListNode_t edgelistnode[2];
	ListNode_t viewnode;
} GraphEdge_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll Graph_t* graphInit(Graph_t* g);
__declspec_dll Graph_t* graphAddNode(Graph_t* g, GraphNode_t* v);
__declspec_dll void graphRemoveNode(Graph_t* g, GraphNode_t* v, List_t* unlink_edgelist);

__declspec_dll GraphEdge_t* graphLinkEdge(GraphEdge_t* e);
__declspec_dll void graphUnlinkEdge(GraphEdge_t* e);
__declspec_dll void graphUnlinkNode(GraphNode_t* v, List_t* unlink_edgelist);

__declspec_dll GraphNode_t* graphDFSFirst(Graph_t* g, GraphNode_t* v);
__declspec_dll GraphNode_t* graphDFSNext(GraphNode_t* v);

#ifdef	__cplusplus
}
#endif

#endif
