//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPONENT_RPC_CORE_H
#define	UTIL_C_COMPONENT_RPC_CORE_H

#include "../datastruct/list.h"
#include "../datastruct/rbtree.h"
#include "../sysapi/process.h"

struct RpcItem_t;
struct RpcBatchNode_t;

typedef struct RpcBaseCore_t {
	RBTree_t rpc_item_tree;
	RBTree_t rpc_item_batch_tree;
	List_t timeout_list;
	void* runthread; /* userdata, library isn't use this field */
} RpcBaseCore_t;

typedef struct RpcAsyncCore_t {
	RpcBaseCore_t base;
} RpcAsyncCore_t;

typedef struct RpcFiberCore_t {
	RpcBaseCore_t base;
	Fiber_t* msg_fiber;
	Fiber_t* cur_fiber;
	Fiber_t* sche_fiber;
	int ret_flag;
	size_t stack_size;
	void* new_msg;
	struct RpcItem_t* reply_item;
	void(*msg_handler)(struct RpcFiberCore_t*, void*);
} RpcFiberCore_t;

typedef struct RpcItem_t {
	struct {
		size_t udata;
	}; /* user use, library not use this field */
	RBTreeNode_t m_treenode;
	ListNode_t m_listnode;
	ListNode_t m_listnode_timeout;
	struct RpcBatchNode_t* m_batch_node;
	char m_has_reg;
	int id;
	long long timestamp_msec;
	int timeout_msec;
	void* async_req_arg;
	void(*async_callback)(struct RpcItem_t*);
	Fiber_t* fiber;
	void* ret_msg;
} RpcItem_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll int rpcGenId(void);
__declspec_dll RpcItem_t* rpcItemSet(RpcItem_t* item, int rpcid, long long timestamp_msec, int timeout_msec);
__declspec_dll void rpcRemoveBatchNode(RpcBaseCore_t* rpc_base, const void* key, List_t* rpcitemlist);
__declspec_dll long long rpcGetMiniumTimeoutTimestamp(RpcBaseCore_t* rpc_base);
__declspec_dll RpcItem_t* rpcGetTimeoutItem(RpcBaseCore_t* rpc_base, long long timestamp_msec);

__declspec_dll RpcAsyncCore_t* rpcAsyncCoreInit(RpcAsyncCore_t* rpc);
__declspec_dll void rpcAsyncCoreDestroy(RpcAsyncCore_t* rpc);
__declspec_dll RpcItem_t* rpcAsyncCoreRegItem(RpcAsyncCore_t* rpc, RpcItem_t* item, const void* batch_key, void* req_arg, void(*ret_callback)(RpcItem_t*));
__declspec_dll RpcItem_t* rpcAsyncCoreUnregItem(RpcAsyncCore_t* rpc, RpcItem_t* item);
__declspec_dll RpcItem_t* rpcAsyncCoreCallback(RpcAsyncCore_t* rpc, int rpcid, void* ret_msg);
__declspec_dll void rpcAsyncCoreCancel(RpcAsyncCore_t* rpc, RpcItem_t* item);
__declspec_dll void rpcAsyncCoreCancelAll(RpcAsyncCore_t* rpc, List_t* rpcitemlist);

__declspec_dll RpcFiberCore_t* rpcFiberCoreInit(RpcFiberCore_t* rpc, Fiber_t* sche_fiber, size_t stack_size, void(*msg_handler)(RpcFiberCore_t*, void*));
__declspec_dll void rpcFiberCoreDestroy(RpcFiberCore_t* rpc);
__declspec_dll RpcItem_t* rpcFiberCoreRegItem(RpcFiberCore_t* rpc, RpcItem_t* item, const void* batch_key, void* req_arg, void(*ret_callback)(RpcItem_t*));
__declspec_dll RpcItem_t* rpcFiberCoreUnregItem(RpcFiberCore_t* rpc, RpcItem_t* item);
__declspec_dll RpcItem_t* rpcFiberCoreYield(RpcFiberCore_t* rpc);
__declspec_dll RpcItem_t* rpcFiberCoreResume(RpcFiberCore_t* rpc, int rpcid, void* ret_msg);
__declspec_dll void rpcFiberCoreResumeMsg(RpcFiberCore_t* rpc, void* new_msg);
__declspec_dll void rpcFiberCoreCancel(RpcFiberCore_t* rpc, RpcItem_t* item);
__declspec_dll void rpcFiberCoreCancelAll(RpcFiberCore_t* rpc, List_t* rpcitemlist);

#ifdef __cplusplus
}
#endif

#endif // !UTIL_C_COMPONENT_RPC_CORE_H
