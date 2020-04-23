//
// Created by hujianzhe
//

#include "../../inc/component/rpc_core.h"
#include "../../inc/sysapi/atomic.h"
#include "../../inc/sysapi/time.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static RpcItem_t* rpc_reg_item(RBTree_t* rpc_reg_tree, RpcItem_t* item) {
	RBTreeNode_t* exist_node = rbtreeInsertNode(rpc_reg_tree, &item->m_treenode);
	if (exist_node != &item->m_treenode) {
		return NULL;
	}
	item->m_has_reg = 1;
	return item;
}

static RpcItem_t* rpc_get_item(RBTree_t* rpc_reg_tree, int rpcid) {
	RBTreeNode_t* node = rbtreeSearchKey(rpc_reg_tree, (const void*)(size_t)rpcid);
	return node ? pod_container_of(node, RpcItem_t, m_treenode) : NULL;
}

static void rpc_remove_node(RBTree_t* rpc_reg_tree, RpcItem_t* item) {
	rbtreeRemoveNode(rpc_reg_tree, &item->m_treenode);
	item->m_has_reg = 0;
}

static int __keycmp(const void* node_key, const void* key) {
	return ((int)(size_t)node_key) - (int)((size_t)key);
}

/*****************************************************************************************/

static Atom32_t RPCID = 0;
int rpcGenId(void) {
	int id;
	while (0 == (id = _xadd32(&RPCID, 1) + 1));
	return id;
}

RpcItem_t* rpcItemSet(RpcItem_t* item, int rpcid, long long timeout_msec, void* timeout_ev) {
	item->m_treenode.key = (const void*)(size_t)rpcid;
	item->m_has_reg = 0;
	item->timeout_msec = timeout_msec;
	item->timeout_ev = timeout_ev;
	item->id = rpcid;
	item->ret_msg = NULL;
	item->fiber = NULL;
	return item;
}

/*****************************************************************************************/

RpcAsyncCore_t* rpcAsyncCoreInit(RpcAsyncCore_t* rpc) {
	rbtreeInit(&rpc->reg_tree, __keycmp);
	return rpc;
}

void rpcAsyncCoreDestroy(RpcAsyncCore_t* rpc) {
	// TODO
}

RpcItem_t* rpcAsyncCoreRegItem(RpcAsyncCore_t* rpc, RpcItem_t* item, void* req_arg, void(*ret_callback)(RpcItem_t*)) {
	if (rpc_reg_item(&rpc->reg_tree, item)) {
		item->async_req_arg = req_arg;
		item->async_callback = ret_callback;
		return item;
	}
	return NULL;
}

RpcItem_t* rpcAsyncCoreUnregItem(RpcAsyncCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_node(&rpc->reg_tree, item);
	}
	return item;
}

static void rpc_async_callback(RpcAsyncCore_t* rpc, RpcItem_t* item, void* ret_msg) {
	rpc_remove_node(&rpc->reg_tree, item);
	item->ret_msg = ret_msg;
	item->async_callback(item);
}

RpcItem_t* rpcAsyncCoreCallback(RpcAsyncCore_t* rpc, int rpcid, void* ret_msg) {
	RpcItem_t* item = rpc_get_item(&rpc->reg_tree, rpcid);
	if (item) {
		rpc_async_callback(rpc, item, ret_msg);
	}
	return item;
}

RpcItem_t* rpcAsyncCoreCancel(RpcAsyncCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_async_callback(rpc, item, NULL);
	}
	return item;
}

void rpcAsyncCoreCancelAll(RpcAsyncCore_t* rpc, RBTree_t* item_set) {
	RBTreeNode_t* rbnode;
	rbtreeInit(item_set, __keycmp);
	rbtreeSwap(item_set, &rpc->reg_tree);
	for (rbnode = rbtreeFirstNode(item_set); rbnode; rbnode = rbtreeNextNode(rbnode)) {
		RpcItem_t* item = pod_container_of(rbnode, RpcItem_t, m_treenode);
		item->m_has_reg = 0;
		item->ret_msg = NULL;
		item->async_callback(item);
	}
}

/*****************************************************************************************/

static void RpcFiberProcEntry(Fiber_t* fiber) {
	RpcFiberCore_t* rpc = (RpcFiberCore_t*)fiber->arg;
	while (1) {
		rpc->cur_fiber = fiber;
		if (rpc->new_msg) {
			void* msg = rpc->new_msg;
			rpc->new_msg = NULL;
			rpc->msg_handler(rpc, msg);
		}
		if (fiber != rpc->msg_fiber)
			rpc->ret_flag = 1;
		fiberSwitch(fiber, rpc->sche_fiber);
	}
}

static void do_fiber_switch(RpcFiberCore_t* rpc, Fiber_t* dst_fiber) {
	Fiber_t* cur_fiber = rpc->cur_fiber;
	fiberSwitch(cur_fiber, dst_fiber);
	rpc->cur_fiber = cur_fiber;
}

RpcFiberCore_t* rpcFiberCoreInit(RpcFiberCore_t* rpc, Fiber_t* sche_fiber, size_t stack_size) {
	rpc->msg_fiber = fiberCreate(sche_fiber, stack_size, RpcFiberProcEntry);
	if (!rpc->msg_fiber) {
		free(rpc);
		return NULL;
	}
	rpc->msg_fiber->arg = rpc;
	rpc->cur_fiber = sche_fiber;
	rpc->sche_fiber = sche_fiber;
	rpc->ret_flag = 0;
	rpc->stack_size = stack_size;
	rpc->new_msg = NULL;
	rpc->reply_item = NULL;
	rpc->msg_handler = NULL;
	rbtreeInit(&rpc->reg_tree, __keycmp);
	return rpc;
}

void rpcFiberCoreDestroy(RpcFiberCore_t* rpc) {
	fiberFree(rpc->msg_fiber);
}

RpcItem_t* rpcFiberCoreRegItem(RpcFiberCore_t* rpc, RpcItem_t* item) {
	if (rpc->cur_fiber == rpc->sche_fiber)
		return NULL;
	if (!rpc_reg_item(&rpc->reg_tree, item))
		return NULL;
	if (rpc->cur_fiber == rpc->msg_fiber) {
		Fiber_t* new_fiber = fiberCreate(rpc->sche_fiber, rpc->stack_size, RpcFiberProcEntry);
		if (!new_fiber) {
			rpc_remove_node(&rpc->reg_tree, item);
			return NULL;
		}
		new_fiber->arg = rpc;
		rpc->msg_fiber = new_fiber;
	}
	item->fiber = rpc->cur_fiber;
	return item;
}

RpcItem_t* rpcFiberCoreUnregItem(RpcFiberCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_node(&rpc->reg_tree, item);
	}
	return item;
}

RpcItem_t* rpcFiberCoreYield(RpcFiberCore_t* rpc) {
	do_fiber_switch(rpc, rpc->sche_fiber);
	return rpc->reply_item;
}

static void rpc_fiber_resume(RpcFiberCore_t* rpc, RpcItem_t* item, void* ret_msg) {
	item->ret_msg = ret_msg;
	rpc->reply_item = item;
	do_fiber_switch(rpc, item->fiber);
	rpc->reply_item = NULL;
	if (rpc->ret_flag) {
		fiberFree(item->fiber);
		item->fiber = NULL;
		rpc->ret_flag = 0;
	}
}

RpcItem_t* rpcFiberCoreResume(RpcFiberCore_t* rpc, int rpcid, void* ret_msg) {
	RpcItem_t* item = rpc_get_item(&rpc->reg_tree, rpcid);
	if (item) {
		rpc_remove_node(&rpc->reg_tree, item);
		rpc_fiber_resume(rpc, item, ret_msg);
	}
	return item;
}

void rpcFiberCoreResumeMsg(RpcFiberCore_t* rpc, void* new_msg) {
	if (rpc->cur_fiber != rpc->sche_fiber)
		return;
	rpc->new_msg = new_msg;
	do_fiber_switch(rpc, rpc->msg_fiber);
}

RpcItem_t* rpcFiberCoreCancel(RpcFiberCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_node(&rpc->reg_tree, item);
		rpc_fiber_resume(rpc, item, NULL);
	}
	return item;
}

void rpcFiberCoreCancelAll(RpcFiberCore_t* rpc, RBTree_t* item_set) {
	RBTreeNode_t* rbnode;
	rbtreeInit(item_set, __keycmp);
	rbtreeSwap(item_set, &rpc->reg_tree);
	for (rbnode = rbtreeFirstNode(item_set); rbnode; rbnode = rbtreeNextNode(rbnode)) {
		RpcItem_t* item = pod_container_of(rbnode, RpcItem_t, m_treenode);
		item->m_has_reg = 0;
		rpc_fiber_resume(rpc, item, NULL);
	}
}

#ifdef __cplusplus
}
#endif