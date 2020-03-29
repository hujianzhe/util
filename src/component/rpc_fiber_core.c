//
// Created by hujianzhe
//

#include "../../inc/component/rpc_fiber_core.h"
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

RpcItem_t* rpcItemInit(RpcItem_t* item, int rpcid) {
	item->m_treenode.key = (const void*)(size_t)rpcid;
	item->m_has_reg = 0;
	item->id = rpcid;
	item->ret_msg = NULL;
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

RpcItem_t* rpcAsyncCoreRegItem(RpcAsyncCore_t* rpc, RpcItem_t* item, long long timeout_msec, void* req_arg, void(*ret_callback)(RpcItem_t*)) {
	if (rpc_reg_item(&rpc->reg_tree, item)) {
		item->timeout_msec = timeout_msec;
		item->async_req_arg = req_arg;
		item->async_callback = ret_callback;
		return item;
	}
	return NULL;
}

RpcItem_t* rpcAsyncCoreRemoveItem(RpcAsyncCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_node(&rpc->reg_tree, item);
	}
	return item;
}

RpcItem_t* rpcAsyncCoreCallback(RpcAsyncCore_t* rpc, int rpcid, void* ret_msg) {
	RpcItem_t* item = rpc_get_item(&rpc->reg_tree, rpcid);
	if (item) {
		rpc_remove_node(&rpc->reg_tree, item);
		item->ret_msg = ret_msg;
		item->async_callback(item);
	}
	return item;
}

/*****************************************************************************************/

static void RpcFiberProcEntry(Fiber_t* fiber) {
	RpcFiberCore_t* rpc = (RpcFiberCore_t*)fiber->arg;
	while (1) {
		if (rpc->new_msg) {
			void* msg = rpc->new_msg;
			rpc->new_msg = NULL;
			rpc->msg_handler(rpc, msg);
		}
		if (rpc->disconnect_cmd) {
			void* msg = rpc->disconnect_cmd;
			rpc->disconnect_cmd = NULL;
			rpc->msg_handler(rpc, msg);
		}
		fiberSwitch(fiber, rpc->sche_fiber);
	}
	fiberSwitch(fiber, rpc->sche_fiber);
}

RpcFiberCore_t* rpcFiberCoreInit(RpcFiberCore_t* rpc, Fiber_t* sche_fiber, size_t stack_size) {
	rpc->fiber = fiberCreate(sche_fiber, stack_size, RpcFiberProcEntry);
	if (!rpc->fiber) {
		free(rpc);
		return NULL;
	}
	rpc->fiber->arg = rpc;
	rpc->sche_fiber = sche_fiber;
	rpc->new_msg = NULL;
	rpc->disconnect_cmd = NULL;
	rpc->msg_handler = NULL;
	rbtreeInit(&rpc->reg_tree, __keycmp);
	return rpc;
}

void rpcFiberCoreDestroy(RpcFiberCore_t* rpc) {
	fiberFree(rpc->fiber);
}

RpcItem_t* rpcFiberCoreRegItem(RpcFiberCore_t* rpc, RpcItem_t* item, long long timeout_msec) {
	if (rpc_reg_item(&rpc->reg_tree, item)) {
		item->timeout_msec = timeout_msec;
		return item;
	}
	return NULL;
}

RpcItem_t* rpcFiberCoreRemoveItem(RpcFiberCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_node(&rpc->reg_tree, item);
	}
	return item;
}

RpcItem_t* rpcFiberCoreReturnWait(RpcFiberCore_t* rpc, RpcItem_t* item) {
	fiberSwitch(rpc->fiber, rpc->sche_fiber);
	while (rpc->new_msg) {
		void* msg = rpc->new_msg;
		rpc->new_msg = NULL;
		rpc->msg_handler(rpc, msg);
		if (rpc->disconnect_cmd) {
			break;
		}
		fiberSwitch(rpc->fiber, rpc->sche_fiber);
	}
	return item;
}

int rpcFiberCoreReturnSwitch(RpcFiberCore_t* rpc, int rpcid, void* ret_msg) {
	RpcItem_t* item = rpc_get_item(&rpc->reg_tree, rpcid);
	if (item) {
		rpc_remove_node(&rpc->reg_tree, item);
		item->ret_msg = ret_msg;
		fiberSwitch(rpc->sche_fiber, rpc->fiber);
		return 1;
	}
	return 0;
}

void rpcFiberCoreMessageHandleSwitch(RpcFiberCore_t* rpc, void* new_msg) {
	rpc->new_msg = new_msg;
	fiberSwitch(rpc->sche_fiber, rpc->fiber);
}

void rpcFiberCoreDisconnectHandleSwitch(RpcFiberCore_t* rpc, void* disconnect_cmd) {
	rpc->disconnect_cmd = disconnect_cmd;
	fiberSwitch(rpc->sche_fiber, rpc->fiber);
}

#ifdef __cplusplus
}
#endif