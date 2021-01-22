//
// Created by hujianzhe
//

#include "../../inc/datastruct/graph.h"

#ifdef	__cplusplus
extern "C" {
#endif

Graph_t* graphInit(Graph_t* g) {
	listInit(&g->vnodelist);
	g->dfs_visit = 0;
	return g;
}

Graph_t* graphAddNode(Graph_t* g, GraphNode_t* v) {
	int i;
	for (i = 0; i < 2; ++i) {
		listInit(&v->edgelist[i]);
		v->degree[i] = 0;
	}
	v->dfs_visit = 0;
	v->graph = g;
	listPushNodeBack(&g->vnodelist, &v->node);
	return g;
}

void graphRemoveNode(Graph_t* g, GraphNode_t* v) {
	listRemoveNode(&g->vnodelist, &v->node);
}

GraphEdge_t* graphLinkEdge(GraphEdge_t* e) {
	if (e->v[0] && e->v[1]) {
		int i;
		for (i = 0; i < 2; ++i) {
			GraphNode_t* v = e->v[i];
			listInsertNodeBack(&v->edgelist[i], v->edgelist[i].tail, &e->edgelistnode[i]);
			v->degree[i]++;
		}
	}
	return e;
}

void graphUnlinkEdge(GraphEdge_t* e) {
	if (e->v[0] && e->v[1]) {
		int i;
		for (i = 0; i < 2; ++i) {
			GraphNode_t* v = e->v[i];
			listRemoveNode(&v->edgelist[i], &e->edgelistnode[i]);
			v->degree[i]--;
			e->v[i] = (GraphNode_t*)0;
		}
	}
}

List_t graphUnlinkNode(GraphNode_t* v) {
	List_t free_edgelist;
	int i;
	listInit(&free_edgelist);
	for (i = 0; i < 2; ++i) {
		List_t* edgelist = &v->edgelist[i];
		ListNode_t* cur, *next;
		for (cur = edgelist->head; cur; cur = next) {
			GraphEdge_t* edge = pod_container_of(cur, GraphEdge_t, edgelistnode[i]);
			next = cur->next;
			graphUnlinkEdge(edge);
			listPushNodeBack(&free_edgelist, &edge->viewnode);
		}
		v->degree[i] = 0;
	}
	return free_edgelist;
}

GraphNode_t* graphDFSFirst(Graph_t* g, GraphNode_t* v) {
	if (0 == ++g->dfs_visit) {
		ListNode_t* cur;
		for (cur = g->vnodelist.head; cur; cur = cur->next) {
			GraphNode_t* vnode = pod_container_of(cur, GraphNode_t, node);
			vnode->dfs_visit = 0;
		}
		++g->dfs_visit;
	}
	v->dfs_visit = g->dfs_visit;
	v->visit_from_edgenode = (GraphEdge_t*)0;
	return v;
}

GraphNode_t* graphDFSNext(GraphNode_t* v) {
	ListNode_t* cur = v->edgelist[0].head;
	int dfs_visit = v->graph->dfs_visit;
	while (1) {
		GraphEdge_t* edge;
		for (; cur; cur = cur->next) {
			GraphNode_t* next_v;
			edge = pod_container_of(cur, GraphEdge_t, edgelistnode[0]);
			next_v = edge->v[1];
			if (next_v->dfs_visit == dfs_visit)
				continue;
			next_v->dfs_visit = dfs_visit;
			next_v->visit_from_edgenode = edge;
			return next_v;
		}
		if (!v->visit_from_edgenode)
			return (GraphNode_t*)0;
		cur = v->visit_from_edgenode->edgelistnode[0].next;
		edge = v->visit_from_edgenode;
		v = edge->v[0];
	}
}

#ifdef	__cplusplus
}
#endif
