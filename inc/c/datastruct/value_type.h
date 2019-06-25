//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_VALUE_TYPE_H
#define	UTIL_C_DATASTRUCT_VALUE_TYPE_H

#include "../compiler_define.h"

typedef struct Value_t {
	void(*deleter)(void*);
	union {
		long long valueint;
		float valuefloat;
		double valuedouble;
		void* valueobject;
		const char* valuestring_const;
		char* valuestring;
		struct {
			unsigned char* ptr;
			ptrlen_t len;
		} valueblob;
	};
#ifdef __cplusplus
	Value_t(void) : deleter((void(*)(void*))0) {}
	~Value_t(void) {
		if (deleter) {
			deleter(valueobject);
			deleter = (void(*)(void*))0;
		}
	}
#endif
} Value_t;

static void value_deleter(struct Value_t* v) {
	if (v->deleter) {
		v->deleter(v->valueobject);
		v->deleter = (void(*)(void*))0;
	}
}

#endif // !UTIL_C_DATASTRUCT_VALUE_TYPE_H
