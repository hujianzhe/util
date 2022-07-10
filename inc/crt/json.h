//
// Created by hujianzhe
//

#ifndef	UTIL_C_CRT_JSON_H
#define	UTIL_C_CRT_JSON_H

#include "../compiler_define.h"
#include <stddef.h>

typedef struct cJSON_Setting {
	void *(*malloc_fn)(size_t sz);
	void (*free_fn)(void *ptr);
	unsigned int precision;
} cJSON_Setting;

typedef struct cJSON {
	/* private */
	short type;
	short value_type;
	short name_deep_copy;
	short value_deep_copy;

	/* public read only */
	char *name;
	size_t name_length;

	/* private */
	char *value_string;
	size_t value_strlen;
	long long value_integer;
	double value_double;

	/* public read only */
	struct cJSON *parent;
	struct cJSON *next, *prev;
	struct cJSON *child;
	size_t child_num;
} cJSON;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll cJSON_Setting* cJSON_GetSetting(cJSON_Setting* s);
__declspec_dll void cJSON_SetSetting(const cJSON_Setting* s);

__declspec_dll cJSON* cJSON_GetField(cJSON* root, const char* name);
__declspec_dll cJSON* cJSON_GetIndex(cJSON* root, size_t idx);
__declspec_dll size_t cJSON_ChildNum(cJSON* root);

__declspec_dll long long cJSON_GetInteger(cJSON* node);
__declspec_dll double cJSON_GetDouble(cJSON* node);
__declspec_dll const char* cJSON_GetStringPtr(cJSON* node);
__declspec_dll size_t cJSON_GetStringLength(cJSON* node);

__declspec_dll cJSON* cJSON_SetInteger(cJSON* node, long long v);
__declspec_dll cJSON* cJSON_SetDouble(cJSON* node, double v);
__declspec_dll cJSON* cJSON_SetString(cJSON* node, const char* s, size_t slen);

__declspec_dll cJSON* cJSON_NewRoot(void);
__declspec_dll cJSON* cJSON_NewRootArray(void);
__declspec_dll cJSON* cJSON_AppendObject(cJSON* parent, const char* name);
__declspec_dll cJSON* cJSON_AppendArray(cJSON* parent, const char* name);
__declspec_dll cJSON* cJSON_AppendInteger(cJSON* parent, const char* name, long long v);
__declspec_dll cJSON* cJSON_AppendDouble(cJSON* parent, const char* name, double v);
__declspec_dll cJSON* cJSON_AppendString(cJSON* parent, const char* name, const char* v);

__declspec_dll cJSON* cJSON_Append(cJSON* parent, cJSON* node);
__declspec_dll cJSON* cJSON_Detach(cJSON* node);
__declspec_dll void cJSON_Delete(cJSON* node);

__declspec_dll cJSON* cJSON_FromString(const char* s, int deep_copy);
__declspec_dll cJSON* cJSON_FromFile(const char* path);
__declspec_dll size_t cJSON_BytesNum(cJSON* root);
__declspec_dll char* cJSON_ToString(cJSON* root, char* buf);

#ifdef __cplusplus
}
#endif

#endif
