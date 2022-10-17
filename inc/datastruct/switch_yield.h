//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SWITCH_PROCESS_H
#define	UTIL_C_DATASTRUCT_SWITCH_PROCESS_H

#include "../compiler_define.h"

/* note: __st status enum define */

enum {
	SWITCH_STATUS_CANCEL = -3,
	SWITCH_STATUS_ERROR = -2,
	SWITCH_STATUS_FINISH = -1,
	SWITCH_STATUS_START = 0
};

/* note: SWITCH_PROCESS maybe hold execute context memory */
typedef struct SwitchCo_t {
	int st;
	void* ctx;
} SwitchCo_t;

/* note: __st is switch status, an integer value,
 * 
 * code example:
 *
	switch (co->st) {
		default:
			return; // avoid execute one more time
		case SWITCH_STATUS_START:
			// code ...
	}
	co->st = SWITCH_STATUS_FINISH;
	return;
 *
 *
 *
 */

#define SWITCH_YIELD(__st)					do { *(__st) = __LINE__; return; case __LINE__: ; } while(0)
#define SWITCH_YIELD_VALUE(__st, value)		do { *(__st) = __LINE__; return (value); case __LINE__: ; } while(0)

#endif
