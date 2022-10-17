//
// Created by hujianzhe
//

#ifndef	UTIL_C_DATASTRUCT_SWITCH_PROCESS_H
#define	UTIL_C_DATASTRUCT_SWITCH_PROCESS_H

#include "../compiler_define.h"

enum {
	SWITCH_STATUS_ERROR = -2,
	SWITCH_STATUS_FINISHED = -1,
	SWITCH_STATUS_INIT = 0
};

/* note: SWITCH_PROCESS maybe hold execute context memory */
typedef struct SwitchCo_t {
	int st;
	void* ctx;
} SwitchCo_t;

/* note: __st is switch status, an integer value */

#define	SWITCH_PROCESS_BEGIN(__st)			switch (*(__st)) { case 0: ;
#define SWITCH_YIELD(__st)					do { *(__st) = __LINE__; return; case __LINE__: ; } while(0)
#define SWITCH_YIELD_VALUE(__st, value)		do { *(__st) = __LINE__; return (value); case __LINE__: ; } while(0)
#define	SWITCH_PROCESS_END()				}

#endif
