//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SWITCH_PROCESS_H
#define	UTIL_C_DATASTRUCT_SWITCH_PROCESS_H

#include "../compiler_define.h"

/* note: switch status enum define */

enum {
	SWITCH_STATUS_CANCEL = -3,
	SWITCH_STATUS_ERROR = -2,
	SWITCH_STATUS_FINISH = -1,
	SWITCH_STATUS_START = 0
};

/* note: a switch execute context */

typedef struct SwitchCo_t {
    int status; /* switch status */
    void* ctx; /* hold routine execute context */
} SwitchCo_t;

/* note: co->status is switch status, an integer value,
 * 
 * code example:
 *
	switch (co->status) {
		default:
			// (free or delete) co->ctx
			return; // avoid execute one more time
		case SWITCH_STATUS_START:
			// co->ctx = (malloc or new ctx)
			// code ...
			// SWITCH_YIELD(&co->status);
			// code ...
	}
	// (free or delete) co->ctx
	co->status = SWITCH_STATUS_FINISH;
	return;
 *
 *
 *
 */

#define SWITCH_YIELD(status)				do { *(status) = __LINE__; return; case __LINE__: ; } while(0)
#define SWITCH_YIELD_VALUE(status, value)	do { *(status) = __LINE__; return (value); case __LINE__: ; } while(0)

#define	SWITCH_WAIT_HEAD(child_status)	\
do {	\
	if (*(child_status) < 0) break

#define	SWITCH_WAIT_TAIL(child_status, cur_status)	\
	if (*(child_status) < 0)	\
		break;	\
	SWITCH_YIELD(cur_status);	\
} while (1)

#define	SWITCH_WAIT_FUNCTION(cur_status, child_status, sub_function_expr)	\
	SWITCH_WAIT_HEAD(child_status);	\
	sub_function_expr;	\
	SWITCH_WAIT_TAIL(child_status, cur_status)

/* note: direct use SwitchCo_t */

#define	SWITCH_CO_YIELD(ptr_co)					SWITCH_YIELD(&(ptr_co)->status)
#define	SWITCH_CO_YIELD_VALUE(ptr_co, value)	SWITCH_YIELD_VALUE(&(ptr_co)->status, value)

#define	SWITCH_CO_WAIT_HEAD(ptr_child_co)		SWITCH_WAIT_HEAD(&(ptr_child_co)->status)
#define	SWITCH_CO_WAIT_TAIL(ptr_child_co, ptr_co)	SWITCH_WAIT_TAIL(&(ptr_child_co)->status, &(ptr_co)->status)
#define	SWITCH_CO_WAIT_FUNCTION(ptr_co, ptr_child_co, sub_function_expr)	SWITCH_WAIT_FUNCTION(&(ptr_co)->status, &(ptr_child_co)->status, sub_function_expr)

#define	SWITCH_CO_MULTI_WAIT_HEAD()			do {
#define SWITCH_CO_MULTI_WAIT_TAIL(ptr_co, ptr_child_co_arr, child_co_cnt)	\
	if (1) {	\
		SwitchCo_t **MACRO_APPEND(__co,__LINE__), **MACRO_APPEND(__co_end,__LINE__) = ptr_child_co_arr + child_co_cnt;	\
		for (MACRO_APPEND(__co,__LINE__) = ptr_child_co_arr; MACRO_APPEND(__co,__LINE__) < MACRO_APPEND(__co_end,__LINE__); ++MACRO_APPEND(__co,__LINE__)) {	\
			if ((*MACRO_APPEND(__co,__LINE__))->status > 0) break;	\
		}	\
		if (MACRO_APPEND(__co,__LINE__) == MACRO_APPEND(__co_end,__LINE__)) break;	\
	}	\
	SWITCH_CO_YIELD(ptr_co);	\
} while (1)

#endif
