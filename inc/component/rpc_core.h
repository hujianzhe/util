//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPONENT_RPC_CORE_H
#define	UTIL_C_COMPONENT_RPC_CORE_H

#include "../datastruct/rbtree.h"
#include "../sysapi/process.h"

typedef struct RpcItem_t {
	RBTreeNode_t m_treenode;
	char m_has_reg;
	int id;
	long long timestamp_msec;
	long long timeout_msec;
	void* async_req_arg;
	void(*async_callback)(struct RpcItem_t*);
	Fiber_t* fiber;
	void* ret_msg;
} RpcItem_t;

typedef struct RpcFiberCore_t {
	Fiber_t* msg_fiber;
	Fiber_t* cur_fiber;
	Fiber_t* sche_fiber;
	int ret_flag;
	RBTree_t reg_tree;
	size_t stack_size;
	void* new_msg;
	void(*msg_handler)(struct RpcFiberCore_t*, void*);
} RpcFiberCore_t;

typedef struct RpcAsyncCore_t {
	RBTree_t reg_tree;
} RpcAsyncCore_t;

#ifdef __cplusplus
extern "C" {
#endif

RpcItem_t* rpcItemInit(RpcItem_t* item, int rpcid);

RpcAsyncCore_t* rpcAsyncCoreInit(RpcAsyncCore_t* rpc);
void rpcAsyncCoreDestroy(RpcAsyncCore_t* rpc);
RpcItem_t* rpcAsyncCoreRegItem(RpcAsyncCore_t* rpc, RpcItem_t* item, long long timeout_msec, void* req_arg, void(*ret_callback)(RpcItem_t*));
RpcItem_t* rpcAsyncCoreRemoveItem(RpcAsyncCore_t* rpc, RpcItem_t* item);
RpcItem_t* rpcAsyncCoreCallback(RpcAsyncCore_t* rpc, int rpcid, void* ret_msg);

RpcFiberCore_t* rpcFiberCoreInit(RpcFiberCore_t* rpc, Fiber_t* sche_fiber, size_t stack_size);
void rpcFiberCoreDestroy(RpcFiberCore_t* rpc);
RpcItem_t* rpcFiberCoreRegItem(RpcFiberCore_t* rpc, RpcItem_t* item, long long timeout_msec);
RpcItem_t* rpcFiberCoreRemoveItem(RpcFiberCore_t* rpc, RpcItem_t* item);
RpcItem_t* rpcFiberCoreReturnWait(RpcFiberCore_t* rpc, RpcItem_t* item);
RpcItem_t* rpcFiberCoreReturnSwitch(RpcFiberCore_t* rpc, int rpcid, void* ret_msg);
void rpcFiberCoreMessageHandleSwitch(RpcFiberCore_t* rpc, void* new_msg);

#ifdef __cplusplus
}
#endif

#endif // !UTIL_C_COMPONENT_RPC_CORE_H
