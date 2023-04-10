//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SERIAL_EXEC_H
#define	UTIL_C_DATASTRUCT_SERIAL_EXEC_H

#include "list.h"

struct SerialExecObj_t;

typedef struct SerialExecQueue_t {
	List_t list;
	struct SerialExecObj_t* exec_obj;
} SerialExecQueue_t;

typedef struct SerialExecObj_t {
	ListNode_t listnode;
	SerialExecQueue_t* dq;
	short hang_up;
} SerialExecObj_t;

#ifdef	__cplusplus
extern "C" {
#endif

__declspec_dll SerialExecQueue_t* SerialExecQueue_init(SerialExecQueue_t* dq);
__declspec_dll int SerialExecQueue_check_exec(SerialExecQueue_t* dq, SerialExecObj_t* obj);
__declspec_dll void SerialExecQueue_clear(SerialExecQueue_t* dq, void(*fn_free)(SerialExecObj_t*));
__declspec_dll SerialExecObj_t* SerialExecQueue_next(SerialExecQueue_t* dq);

#ifdef	__cplusplus
}
#endif

#endif
