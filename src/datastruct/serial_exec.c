//
// Created by hujianzhe on 2023-4-10.
//

#include "../../inc/datastruct/serial_exec.h"

#ifdef	__cplusplus
extern "C" {
#endif

SerialExecQueue_t* SerialExecQueue_init(SerialExecQueue_t* dq) {
	listInit(&dq->list);
	dq->exec_obj = (SerialExecObj_t*)0;
	dq->cur_count = 0;
	return dq;
}

int SerialExecQueue_check_exec(SerialExecQueue_t* dq, SerialExecObj_t* obj) {
	if (dq->exec_obj && dq->exec_obj != obj) {
		if (obj->hang_up) {
			return 0;
		}
		obj->hang_up = 1;
		listPushNodeBack(&dq->list, &obj->listnode);
		dq->cur_count += 1;
		return 0;
	}
	dq->exec_obj = obj;
	obj->dq = dq;
	obj->hang_up = 0;
	return 1;
}

void SerialExecQueue_clear(SerialExecQueue_t* dq, void(*fn_free)(SerialExecObj_t*)) {
	if (dq->exec_obj) {
		dq->exec_obj->dq = (SerialExecQueue_t*)0;
		dq->exec_obj = (SerialExecObj_t*)0;
	}
	if (fn_free) {
		ListNode_t *cur, *next;
		for (cur = dq->list.head; cur; cur = next) {
			SerialExecObj_t* obj = pod_container_of(cur, SerialExecObj_t, listnode);
			next = cur->next;

			fn_free(obj);
		}
	}
	listInit(&dq->list);
	dq->cur_count = 0;
}

SerialExecObj_t* SerialExecQueue_next(SerialExecQueue_t* dq) {
	ListNode_t* node = listPopNodeFront(&dq->list);
	dq->exec_obj = (SerialExecObj_t*)0;
	if (!node) {
		return (SerialExecObj_t*)0;
	}
	dq->cur_count -= 1;
	return pod_container_of(node, SerialExecObj_t, listnode);
}

#ifdef	__cplusplus
}
#endif
