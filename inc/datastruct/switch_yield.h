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
			if (co->ctx) {
				// clean co->ctx
			}
			return; // avoid execute one more time
		case SWITCH_STATUS_START:
			// code ...
			// SWITCH_YIELD(&co->status);
			// code ...
	}
	// clean co->ctx
	co->status = SWITCH_STATUS_FINISH;
	return;
 *
 *
 *
 */

#define SWITCH_YIELD(status)				do { *(status) = __LINE__; return; case __LINE__: ; } while(0)
#define SWITCH_YIELD_VALUE(status, value)	do { *(status) = __LINE__; return (value); case __LINE__: ; } while(0)

#endif
