//
// Created by hujianzhe
//

#include "../../inc/datastruct/graph.h"

#ifdef	__cplusplus
extern "C" {
#endif

Graph_t* graphInit(Graph_t* g) {
	listInit(&g->vnodelist);
	return g;
}

Graph_t* graphAddNode(Graph_t* g, GraphNode_t* v) {
	int i;
	for (i = 0; i < 2; ++i) {
		listInit(&v->edgelist[i]);
		v->degree[i] = 0;
	}
	v->graph = g;
	listPushNodeBack(&g->vnodelist, &v->node);
	return g;
}

void graphRemoveNode(Graph_t* g, GraphNode_t* v) {
	listRemoveNode(&g->vnodelist, &v->node);
}

GraphNode_t* graphnodeInit(GraphNode_t* v) {
	int i;
	for (i = 0; i < 2; ++i) {
		listInit(&v->edgelist[i]);
		v->degree[i] = 0;
	}
	v->visited = 0;
	return v;
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
	ListNode_t* cur;
	for (cur = g->vnodelist.head; cur; cur = cur->next) {
		GraphNode_t* vnode = pod_container_of(cur, GraphNode_t, node);
		vnode->last_visit_edgenode = (ListNode_t*)0;
		vnode->visited = 0;
	}
	v->visited = 1;
	return v;
}

GraphNode_t* graphDFSNext(GraphNode_t* v) {
	ListNode_t* cur = v->edgelist[0].head;
	while (1) {
		GraphEdge_t* edge;
		for (; cur; cur = cur->next) {
			GraphNode_t* next_v;
			edge = pod_container_of(cur, GraphEdge_t, edgelistnode[0]);
			next_v = edge->v[1];
			if (next_v->visited)
				continue;
			next_v->visited = 1;
			next_v->last_visit_edgenode = cur;
			return next_v;
		}
		if (!v->last_visit_edgenode)
			return (GraphNode_t*)0;
		cur = v->last_visit_edgenode->next;
		edge = pod_container_of(v->last_visit_edgenode, GraphEdge_t, edgelistnode[0]);
		v = edge->v[0];
	}
}

#ifdef	__cplusplus
}
#endif
