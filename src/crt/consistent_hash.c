//
// Created by hujianzhe
//

#include "../../inc/crt/consistent_hash.h"
#include <stdlib.h>

typedef struct VirtualNode_t {
	RBTreeNode_t m_treenode;
	void* value;
} VirtualNode_t;
static int __consthash_keycmp(const void* node_key, const void* key) {
	if ((size_t)key < (size_t)node_key)
		return -1;
	else if ((size_t)key > (size_t)node_key)
		return 1;
	else
		return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

void consistenthashInit(ConsistentHash_t* ch) {
	rbtreeInit(ch, __consthash_keycmp);
}

int consistenthashReg(ConsistentHash_t* ch, unsigned int key, void* value) {
	VirtualNode_t* vc = (VirtualNode_t*)malloc(sizeof(VirtualNode_t));
	if (vc) {
		RBTreeNode_t* exist_node;
		vc->m_treenode.key = (void*)(size_t)key;
		vc->value = value;
		exist_node = rbtreeInsertNode(ch, &vc->m_treenode);
		if (exist_node != &vc->m_treenode) {
			rbtreeReplaceNode(exist_node, &vc->m_treenode);
			free(pod_container_of(exist_node, VirtualNode_t, m_treenode));
		}
		return 1;
	}
	return 0;
}

void* consistenthashSelect(ConsistentHash_t* ch, unsigned int key) {
	RBTreeNode_t* exist_node = rbtreeUpperBoundKey(ch, (void*)(size_t)key);
	if (!exist_node) {
		exist_node = rbtreeFirstNode(ch);
		if (!exist_node) {
			return NULL;
		}
	}
	return pod_container_of(exist_node, VirtualNode_t, m_treenode)->value;
}

void consistenthashDelValue(ConsistentHash_t* ch, const void* value) {
	RBTreeNode_t* cur, *next;
	for (cur = rbtreeFirstNode(ch); cur; cur = next) {
		VirtualNode_t* vc = pod_container_of(cur, VirtualNode_t, m_treenode);
		next = rbtreeNextNode(cur);
		if (vc->value != value)
			continue;
		rbtreeRemoveNode(ch, cur);
		free(vc);
	}
}

void consistenthashDelKey(ConsistentHash_t* ch, unsigned int key) {
	RBTreeNode_t* exist_node = rbtreeRemoveKey(ch, (void*)(size_t)key);
	if (exist_node) {
		free(pod_container_of(exist_node, VirtualNode_t, m_treenode));
	}
}

void consistenthashFree(ConsistentHash_t* ch) {
	RBTreeNode_t* cur, *next;
	for (cur = rbtreeFirstNode(ch); cur; cur = next) {
		VirtualNode_t* vc = pod_container_of(cur, VirtualNode_t, m_treenode);
		next = rbtreeNextNode(cur);
		rbtreeRemoveNode(ch, cur);
		free(vc);
	}
}

#ifdef __cplusplus
}
#endif