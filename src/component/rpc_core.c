//
// Created by hujianzhe
//

#include "../../inc/component/rpc_core.h"
#include "../../inc/sysapi/atomic.h"
#include "../../inc/sysapi/time.h"
#include <stdlib.h>

typedef struct RpcBatchNode_t {
	RBTreeNode_t node;
	List_t rpcitemlist;
} RpcBatchNode_t;

#ifdef __cplusplus
extern "C" {
#endif

static RpcItem_t* rpc_reg_item(RpcBaseCore_t* rpc_base, RpcItem_t* item, const void* batch_key) {
	RBTreeNode_t* exist_node = rbtreeInsertNode(&rpc_base->rpc_item_tree, &item->m_treenode);
	if (exist_node != &item->m_treenode) {
		return NULL;
	}
	if (batch_key) {
		RBTreeNodeKey_t rkey;
		RpcBatchNode_t* batch_node;
		rkey.ptr = batch_key;
		exist_node = rbtreeSearchKey(&rpc_base->rpc_item_batch_tree, rkey);
		if (exist_node) {
			batch_node = pod_container_of(exist_node, RpcBatchNode_t, node);
		}
		else {
			batch_node = (RpcBatchNode_t*)malloc(sizeof(RpcBatchNode_t));
			if (!batch_node) {
				rbtreeRemoveNode(&rpc_base->rpc_item_tree, &item->m_treenode);
				return NULL;
			}
			listInit(&batch_node->rpcitemlist);
			batch_node->node.key.ptr = batch_key;
			rbtreeInsertNode(&rpc_base->rpc_item_batch_tree, &batch_node->node);
		}
		item->batch_node = batch_node;
		listPushNodeBack(&batch_node->rpcitemlist, &item->m_listnode);
	}
	item->m_has_reg = 1;
	return item;
}

static void rpc_remove_item(RpcBaseCore_t* rpc_base, RpcItem_t* item) {
	rbtreeRemoveNode(&rpc_base->rpc_item_tree, &item->m_treenode);
	if (item->batch_node) {
		RpcBatchNode_t* batch_node = item->batch_node;
		listRemoveNode(&batch_node->rpcitemlist, &item->m_listnode);
		item->batch_node = NULL;
		if (!batch_node->rpcitemlist.head) {
			rbtreeRemoveNode(&rpc_base->rpc_item_batch_tree, &batch_node->node);
			free(batch_node);
		}
	}
	item->m_has_reg = 0;
}

static RpcItem_t* rpc_get_item(RpcBaseCore_t* rpc_base, int rpcid) {
	RBTreeNodeKey_t key;
	RBTreeNode_t* node;
	key.i32 = rpcid;
	node = rbtreeSearchKey(&rpc_base->rpc_item_tree, key);
	return node ? pod_container_of(node, RpcItem_t, m_treenode) : NULL;
}

static void rpc_remove_all_item(RpcBaseCore_t* rpc_base, List_t* rpcitemlist) {
	RBTreeNode_t* cur, *next;

	for (cur = rbtreeFirstNode(&rpc_base->rpc_item_batch_tree); cur; cur = next) {
		RpcBatchNode_t* batch = pod_container_of(cur, RpcBatchNode_t, node);
		next = rbtreeNextNode(cur);
		rbtreeRemoveNode(&rpc_base->rpc_item_batch_tree, cur);
		free(batch);
	}
	rbtreeInit(&rpc_base->rpc_item_batch_tree, rbtreeDefaultKeyCmpSZ);

	for (cur = rbtreeFirstNode(&rpc_base->rpc_item_tree); cur; cur = next) {
		RpcItem_t* item = pod_container_of(cur, RpcItem_t, m_treenode);
		next = rbtreeNextNode(cur);
		rbtreeRemoveNode(&rpc_base->rpc_item_tree, cur);
		item->m_has_reg = 0;
		item->batch_node = NULL;
		listPushNodeBack(rpcitemlist, &item->m_listnode);
	}
	rbtreeInit(&rpc_base->rpc_item_tree, rbtreeDefaultKeyCmpI32);
}

static void rpc_base_core_init(RpcBaseCore_t* rpc_base) {
	rbtreeInit(&rpc_base->rpc_item_tree, rbtreeDefaultKeyCmpI32);
	rbtreeInit(&rpc_base->rpc_item_batch_tree, rbtreeDefaultKeyCmpSZ);
	rpc_base->runthread = NULL;
}

/*****************************************************************************************/

static Atom32_t RPCID = 0;
int rpcGenId(void) {
	int id;
	while (0 == (id = _xadd32(&RPCID, 1) + 1));
	return id;
}

RpcItem_t* rpcItemSet(RpcItem_t* item, int rpcid) {
	item->m_treenode.key.i32 = rpcid;
	item->batch_node = NULL;
	item->m_has_reg = 0;
	item->id = rpcid;
	item->ret_msg = NULL;
	item->fiber = NULL;
	return item;
}

void rpcRemoveBatchNode(RpcBaseCore_t* rpc_base, const void* key, List_t* rpcitemlist) {
	RBTreeNodeKey_t rkey;
	RBTreeNode_t* node;
	rkey.ptr = key;
	node = rbtreeRemoveKey(&rpc_base->rpc_item_batch_tree, rkey);
	if (node) {
		RpcBatchNode_t* batch_node = pod_container_of(node, RpcBatchNode_t, node);
		ListNode_t* cur;
		for (cur = batch_node->rpcitemlist.head; cur; cur = cur->next) {
			RpcItem_t* item = pod_container_of(cur, RpcItem_t, m_listnode);
			item->batch_node = NULL;
		}
		if (rpcitemlist) {
			listAppend(rpcitemlist, &batch_node->rpcitemlist);
		}
		free(batch_node);
	}
}

/*****************************************************************************************/

RpcAsyncCore_t* rpcAsyncCoreInit(RpcAsyncCore_t* rpc) {
	rpc_base_core_init(&rpc->base);
	return rpc;
}

void rpcAsyncCoreDestroy(RpcAsyncCore_t* rpc) {
	// TODO
}

RpcItem_t* rpcAsyncCoreRegItem(RpcAsyncCore_t* rpc, RpcItem_t* item, const void* batch_key, void* req_arg, void(*ret_callback)(RpcAsyncCore_t*, RpcItem_t*)) {
	if (!rpc_reg_item(&rpc->base, item, batch_key)) {
		return NULL;
	}
	item->async_req_arg = req_arg;
	item->async_callback = ret_callback;
	return item;
}

RpcItem_t* rpcAsyncCoreUnregItem(RpcAsyncCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_item(&rpc->base, item);
	}
	return item;
}

static void rpc_async_callback(RpcAsyncCore_t* rpc, RpcItem_t* item, void* ret_msg) {
	rpc_remove_item(&rpc->base, item);
	item->ret_msg = ret_msg;
	item->async_callback(rpc, item);
}

RpcItem_t* rpcAsyncCoreCallback(RpcAsyncCore_t* rpc, int rpcid, void* ret_msg) {
	RpcItem_t* item = rpc_get_item(&rpc->base, rpcid);
	if (item) {
		rpc_async_callback(rpc, item, ret_msg);
	}
	return item;
}

void rpcAsyncCoreCancel(RpcAsyncCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_async_callback(rpc, item, NULL);
	}
}

void rpcAsyncCoreCancelAll(RpcAsyncCore_t* rpc, List_t* rpcitemlist) {
	List_t list;
	ListNode_t* cur;
	listInit(&list);
	rpc_remove_all_item(&rpc->base, &list);
	for (cur = list.head; cur; cur = cur->next) {
		RpcItem_t* item = pod_container_of(cur, RpcItem_t, m_listnode);
		item->ret_msg = NULL;
		item->async_callback(rpc, item);
	}
	if (rpcitemlist) {
		listAppend(rpcitemlist, &list);
	}
}

/*****************************************************************************************/

static void do_fiber_switch(RpcFiberCore_t* rpc, Fiber_t* dst_fiber) {
	Fiber_t* cur_fiber = rpc->cur_fiber;
	rpc->cur_fiber = dst_fiber;
	fiberSwitch(cur_fiber, dst_fiber);
}

static void RpcFiberProcEntry(Fiber_t* fiber) {
	RpcFiberCore_t* rpc = (RpcFiberCore_t*)fiber->arg;
	while (1) {
		if (rpc->new_msg) {
			void* msg = rpc->new_msg;
			rpc->new_msg = NULL;
			rpc->msg_handler(rpc, msg);
		}
		if (fiber != rpc->msg_fiber)
			rpc->ret_flag = 1;
		do_fiber_switch(rpc, rpc->sche_fiber);
	}
}

RpcFiberCore_t* rpcFiberCoreInit(RpcFiberCore_t* rpc, Fiber_t* sche_fiber, size_t stack_size, void(*msg_handler)(RpcFiberCore_t*, void*)) {
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
	rpc->msg_handler = msg_handler;
	rpc_base_core_init(&rpc->base);
	return rpc;
}

void rpcFiberCoreDestroy(RpcFiberCore_t* rpc) {
	fiberFree(rpc->msg_fiber);
}

RpcItem_t* rpcFiberCoreRegItem(RpcFiberCore_t* rpc, RpcItem_t* item, const void* batch_key) {
	if (rpc->cur_fiber == rpc->sche_fiber) {
		return NULL;
	}
	if (!rpc_reg_item(&rpc->base, item, batch_key)) {
		return NULL;
	}
	if (rpc->cur_fiber == rpc->msg_fiber) {
		Fiber_t* new_fiber = fiberCreate(rpc->sche_fiber, rpc->stack_size, RpcFiberProcEntry);
		if (!new_fiber) {
			rpc_remove_item(&rpc->base, item);
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
		rpc_remove_item(&rpc->base, item);
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
	if (rpc->cur_fiber != rpc->sche_fiber)
		return NULL;
	else {
		RpcItem_t* item = rpc_get_item(&rpc->base, rpcid);
		if (item) {
			rpc_remove_item(&rpc->base, item);
			rpc_fiber_resume(rpc, item, ret_msg);
		}
		return item;
	}
}

void rpcFiberCoreResumeMsg(RpcFiberCore_t* rpc, void* new_msg) {
	if (rpc->cur_fiber != rpc->sche_fiber)
		return;
	rpc->new_msg = new_msg;
	do_fiber_switch(rpc, rpc->msg_fiber);
}

void rpcFiberCoreCancel(RpcFiberCore_t* rpc, RpcItem_t* item) {
	if (item->m_has_reg) {
		rpc_remove_item(&rpc->base, item);
		if (rpc->cur_fiber != item->fiber)
			rpc_fiber_resume(rpc, item, NULL);
	}
}

void rpcFiberCoreCancelAll(RpcFiberCore_t* rpc, List_t* rpcitemlist) {
	List_t list;
	ListNode_t* cur;
	listInit(&list);
	rpc_remove_all_item(&rpc->base, &list);
	for (cur = list.head; cur; cur = cur->next) {
		RpcItem_t* item = pod_container_of(cur, RpcItem_t, m_listnode);
		if (rpc->cur_fiber == rpc->sche_fiber)
			rpc_fiber_resume(rpc, item, NULL);
	}
	if (rpcitemlist) {
		listAppend(rpcitemlist, &list);
	}
}

#ifdef __cplusplus
}
#endif
