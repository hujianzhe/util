//
// Created by hujianzhe
//

#include "../../inc/crt/json.h"

#if defined(_WIN32) || defined(_WIN64)
#define	_CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4267)
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum cJSON_Type {
	cJSON_Unknown,
	cJSON_Value,
	cJSON_Array,
	cJSON_Object
} cJSON_Type;

typedef enum cJSON_ValueType {
	cJSON_ValueType_Unknown,
	cJSON_ValueType_Integer,
	cJSON_ValueType_Double,
	cJSON_ValueType_String,
	cJSON_ValueType_WeakString,
} cJSON_ValueType;

#ifdef __cplusplus
extern "C" {
#endif

/* inner oper */

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

static const char* skip(const char* s) {
	while (*s && (unsigned char)*s <= 32) {
		s++;
	}
	return s;
}

static const char* skip_forward(const char* s) {
	while ((unsigned char)*s <= 32) {
		s--;
	}
	return s;
}

static const char* cJSON_strpbrk(const char* s) {
	while (1) {
		const char* p = strpbrk(s, "{}[],\":");
		if (p && p > s && p[-1] == '\\') {
			s = p + 1;
			continue;
		}
		return p;
	}
}

static char* cJSON_strdup(const char* str) {
	size_t len = strlen(str) + 1;
	char* copy = (char*)cJSON_malloc(len);
	return copy ? strcpy(copy, str) : NULL;
}

static char* cJSON_strndup(const char* str, size_t len) {
	char* copy = (char*)cJSON_malloc(len + 1);
	if (copy) {
		memmove(copy, str, len);
		copy[len] = 0;
	}
	return copy;
}

static cJSON* cJSON_NewNode(short type) {
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	if (node) {
		memset(node, 0, sizeof(cJSON));
		node->type = type;
	}
	return node;
}

static cJSON* cJSON_NewNodeWithName(short type, const char* name) {
	cJSON* node = cJSON_NewNode(type);
	if (node && name && name[0]) {
		size_t len = strlen(name);
		node->name = cJSON_strndup(name, len);
		if (!node->name) {
			cJSON_free(node);
			return NULL;
		}
		node->name_length = len;
		node->name_deep_copy = 1;
	}
	return node;
}

static cJSON* cJSON_NewAddNode(short parent_type, short type, const char* name) {
	if (cJSON_Value == parent_type) {
		return NULL;
	}
	else if (cJSON_Array == parent_type) {
		if (name && name[0]) {
			return NULL;
		}
	}
	else if (cJSON_Object == parent_type) {
		if (!name || !name[0]) {
			return NULL;
		}
	}
	return cJSON_NewNodeWithName(type, name);
}

static cJSON* cJSON_AssignName(cJSON* node, const char* name, size_t len) {
	node->name_length = len;
	if (node->name_deep_copy) {
		node->name = cJSON_strndup(name, len);
		if (!node->name) {
			return NULL;
		}
	}
	else {
		node->name = (char*)name;
	}
	return node;
}

static cJSON* cJSON_AssignValue(cJSON* node, const char* value, size_t len) {
	node->value_strlen = len;
	if (node->value_deep_copy) {
		node->value_string = cJSON_strndup(value, len);
		if (!node->value_string) {
			return NULL;
		}
	}
	else {
		node->value_string = (char*)value;
	}
	return node;
}

static void cJSON_FreeNode(cJSON* node) {
	if (!node) {
		return;
	}
	if (node->name && node->name_deep_copy) {
		cJSON_free(node->name);
	}
	if (node->value_string && node->value_deep_copy) {
		cJSON_free(node->value_string);
	}
	cJSON_free(node);
}

static cJSON* cJSON_DupNode(cJSON* node) {
	cJSON* dup_node = cJSON_NewNode(node->type);
	if (node->name) {
		dup_node->name = cJSON_strdup(node->name);
		if (!dup_node->name) {
			goto err;
		}
	}
	if (node->value_string) {
		dup_node->value_string = cJSON_strdup(node->value_string);
		if (!dup_node->value_string) {
			goto err;
		}
	}
	dup_node->name_length = node->name_length;
	dup_node->value_strlen = node->value_strlen;
	dup_node->name_deep_copy = 1;
	dup_node->value_deep_copy = 1;
	return dup_node;
err:
	cJSON_FreeNode(dup_node);
	return NULL;
}

static cJSON* cJSON_TreeBegin(cJSON* node) {
	if (node) {
		while (node->child) {
			node = node->child;
		}
	}
	return node;
}

static cJSON* cJSON_TreeNext(cJSON* node) {
	if (node->next) {
		node = node->next;
		while (node->child) {
			node = node->child;
		}
		return node;
	}
	return node->parent;
}

static void cJSON_FreeValueString(cJSON* node) {
	if (!node->value_string) {
		return;
	}
	if (!node->value_deep_copy) {
		return;
	}
	cJSON_free(node->value_string);
	node->value_string = NULL;
	node->value_strlen = 0;
}

static size_t cJSON_IntegerStrlen(long long v) {
	size_t len = (v < 0) ? 1 : 0;
	do {
		++len;
	} while (v /= 10);
	return len;
}

static size_t cJSON_DoubleStrlen(double v) {
	char test_buf;
	int len = snprintf(&test_buf, 0, "%f", v);
	return len < 0 ? 0 : len;
}

/* interface */

cJSON_Setting* cJSON_GetSetting(cJSON_Setting* s) {
	s->malloc_fn = cJSON_malloc;
	s->free_fn = cJSON_free;
	return s;
}

void cJSON_SetSetting(const cJSON_Setting* s) {
	if (!s) {
		return;
	}
	cJSON_malloc = s->malloc_fn;
	cJSON_free = s->free_fn;
}

cJSON* cJSON_GetField(cJSON* root, const char* name) {
	cJSON* child = root->child;
	if (child) {
		size_t len = strlen(name);
		while (child) {
			if (len == child->name_length &&
				0 == memcmp(child->name, name, child->name_length))
			{
				break;
			}
			child = child->next;
		}
	}
	return child;
}

cJSON* cJSON_GetIndex(cJSON* root, size_t idx) {
	cJSON* child;
	size_t i;
	if (idx >= root->child_num) {
		return NULL;
	}
	child = root->child;
	for (i = 0; i < idx && child; ++i, child = child->next);
	return child;
}

size_t cJSON_ChildNum(cJSON* root) {
	return root ? root->child_num : 0;
}

long long cJSON_GetInteger(cJSON* node) {
	long long v;
	size_t i;
	if (!node || node->type != cJSON_Value) {
		return 0;
	}
	if (cJSON_ValueType_Integer == node->value_type) {
		return node->value_integer;
	}
	if (cJSON_ValueType_Double == node->value_type) {
		return node->value_double;
	}
	if (!node->value_string || 0 == node->value_strlen) {
		return 0;
	}

	if ('-' == node->value_string[0] || '+' == node->value_string[0]) {
		i = 1;
	}
	else {
		i = 0;
	}
	v = 0;
	for (; i < node->value_strlen; ++i) {
		char c = node->value_string[i];
		if (c < '0' || c > '9') {
			break;
		}
		v *= 10;
		v += c - '0';
	}
	if ('-' == node->value_string[0]) {
		v *= -1;
	}
	return v;
}

double cJSON_GetDouble(cJSON* node) {
	double v;
	int dot_num, e_sign, e;
	size_t i, dot_i;
	if (!node || node->type != cJSON_Value) {
		return 0.0;
	}
	if (cJSON_ValueType_Double == node->value_type) {
		return node->value_double;
	}
	if (!node->value_string || 0 == node->value_strlen) {
		return 0.0;
	}

	if ('-' == node->value_string[0] || '+' == node->value_string[0]) {
		i = 1;
	}
	else {
		i = 0;
	}
	v = 0.0;
	dot_i = -1;
	dot_num = 0;
	e = 0;
	e_sign = 0;
	for (; i < node->value_strlen; ++i) {
		char c = node->value_string[i];
		if ('.' == c) {
			if (dot_i != -1) {
				break;
			}
			dot_i = i;
			continue;
		}
		if (c < '0' || c > '9') {
			if (!e_sign && ('e' == c || 'E' == c)) {
				if (i + 1 >= node->value_strlen) {
					break;
				}
				c = node->value_string[i + 1];
				if ('-' == c) {
					i++;
					e_sign = -1;
					continue;
				}
				else if ('+' == c) {
					i++;
					e_sign = 1;
					continue;
				}
				else if (c >= '0' && c <= '9') {
					e_sign = 1;
					continue;
				}
			}
			break;
		}
		if (e_sign) {
			e *= 10;
			e += c - '0';
			continue;
		}
		if (dot_i != -1) {
			dot_num++;
		}
		v *= 10.0;
		v += c - '0';
	}
	if ('-' == node->value_string[0]) {
		v *= -1.0;
	}
	for (i = 0; i < dot_num; ++i) {
		v /= 10;
	}
	if (e_sign > 0) {
		for (i = 0; i < e; ++i) {
			v *= 10;
		}
	}
	else if (e_sign < 0) {
		for (i = 0; i < e; ++i) {
			v /= 10;
		}
	}
	return v;
}

const char* cJSON_GetStringPtr(cJSON* node) {
	if (!node) {
		return NULL;
	}
	if (cJSON_ValueType_String == node->value_type ||
		cJSON_ValueType_WeakString == node->value_type)
	{
		return node->value_string;
	}
	return NULL;
}

size_t cJSON_GetStringLength(cJSON* node) {
	if (!node) {
		return 0;
	}
	if (cJSON_ValueType_String == node->value_type ||
		cJSON_ValueType_WeakString == node->value_type)
	{
		return node->value_strlen;
	}
	return 0;
}

cJSON* cJSON_SetInteger(cJSON* node, long long v) {
	if (!node || node->type != cJSON_Value) {
		return NULL;
	}
	cJSON_FreeValueString(node);
	node->value_integer = v;
	node->value_strlen = cJSON_IntegerStrlen(v);
	node->value_type = cJSON_ValueType_Integer;
	return node;
}

cJSON* cJSON_SetDouble(cJSON* node, double v) {
	size_t value_strlen;
	if (!node || node->type != cJSON_Value) {
		return NULL;
	}
	value_strlen = cJSON_DoubleStrlen(v);
	if (0 == value_strlen) {
		return NULL;
	}
	cJSON_FreeValueString(node);
	node->value_double = v;
	node->value_strlen = value_strlen;
	node->value_type = cJSON_ValueType_Double;
	return node;
}

cJSON* cJSON_SetString(cJSON* node, const char* s, size_t slen) {
	if (!node || node->type != cJSON_Value) {
		return NULL;
	}
	cJSON_FreeValueString(node);
	node->value_deep_copy = 1;
	cJSON_AssignValue(node, s, slen);
	node->value_type = cJSON_ValueType_String;
	return node;
}

cJSON* cJSON_Append(cJSON* parent, cJSON* node) {
	cJSON* c = parent->child;
	if (!c) {
		parent->child = node;
	}
	else {
		while (c->next) {
			c = c->next;
		}
		c->next = node;
		node->prev = c;
	}
	node->parent = parent;
	parent->child_num++;
	return node;
}

cJSON* cJSON_NewRoot(void) {
	return cJSON_NewNode(cJSON_Object);
}

cJSON* cJSON_NewRootArray(void) {
	return cJSON_NewNode(cJSON_Array);
}

cJSON* cJSON_AppendObject(cJSON* parent, const char* name) {
	cJSON* node = cJSON_NewAddNode(parent->type, cJSON_Object, name);
	if (!node) {
		return NULL;
	}
	cJSON_Append(parent, node);
	return node;
}

cJSON* cJSON_AppendArray(cJSON* parent, const char* name) {
	cJSON* node = cJSON_NewAddNode(parent->type, cJSON_Array, name);
	if (!node) {
		return NULL;
	}
	cJSON_Append(parent, node);
	return node;
}

cJSON* cJSON_AppendInteger(cJSON* parent, const char* name, long long v) {
	cJSON* node = cJSON_NewAddNode(parent->type, cJSON_Value, name);
	if (!node) {
		return NULL;
	}
	if (!cJSON_SetInteger(node, v)) {
		cJSON_FreeNode(node);
		return NULL;
	}
	cJSON_Append(parent, node);
	return node;
}

cJSON* cJSON_AppendDouble(cJSON* parent, const char* name, double v) {
	cJSON* node = cJSON_NewAddNode(parent->type, cJSON_Value, name);
	if (!node) {
		return NULL;
	}
	if (!cJSON_SetDouble(node, v)) {
		cJSON_FreeNode(node);
		return NULL;
	}
	cJSON_Append(parent, node);
	return node;
}

cJSON* cJSON_AppendString(cJSON* parent, const char* name, const char* v) {
	cJSON* node = cJSON_NewAddNode(parent->type, cJSON_Value, name);
	if (!node) {
		return NULL;
	}
	if (!cJSON_SetString(node, v, strlen(v))) {
		cJSON_FreeNode(node);
		return NULL;
	}
	cJSON_Append(parent, node);
	return node;
}

cJSON* cJSON_Detach(cJSON* node) {
	if (node) {
		cJSON *parent = node->parent;
		if (parent) {
			if (parent->child == node) {
				parent->child = node->next;
			}
			parent->child_num--;
		}
		if (node->prev) {
			node->prev->next = node->next;
		}
		if (node->next) {
			node->next->prev = node->prev;
		}
		node->parent = node->prev = node->next = NULL;
	}
	return node;
}

void cJSON_Delete(cJSON* node) {
	if (!cJSON_Detach(node)) {
		return;
	}
	node = cJSON_TreeBegin(node);
	while (node) {
		cJSON* next = cJSON_TreeNext(node);
		cJSON_FreeNode(node);
		node = next;
	}
}

/* parse string */

cJSON* cJSON_FromString(const char* s, int deep_copy) {
	int parse_ok;
	cJSON* node, *root;
	s = skip(s);
	if (*s != '{' && *s != '[') {
		return NULL;
	}
	else if ('{' == *s) {
		root = cJSON_NewNode(cJSON_Object);
	}
	else {
		root = cJSON_NewNode(cJSON_Array);
	}
	if (!root) {
		return NULL;
	}
	++s;
	root->name_deep_copy = deep_copy;
	root->value_deep_copy = deep_copy;
	parse_ok = 1;
	node = root;
	while (node) {
		const char* name_beg = NULL, *name_end = NULL;
		const char* value_beg = NULL, *value_end = NULL;
		cJSON* child;

		s = skip(s);
		if (*s == 0) {
			if (node->parent) {
				parse_ok = 0;
			}
			break;
		}
		if (*s == ',') {
			++s;
			continue;
		}
		if (*s == '}') {
			if (node->type != cJSON_Object) {
				parse_ok = 0;
				break;
			}
			node = node->parent;
			++s;
			continue;
		}
		if (*s == ']') {
			if (node->type != cJSON_Array) {
				parse_ok = 0;
				break;
			}
			node = node->parent;
			++s;
			continue;
		}
		if (node->type == cJSON_Object) {
			if (*s != '\"') {
				parse_ok = 0;
				break;
			}
			name_beg = s + 1;
			s = cJSON_strpbrk(name_beg);
			if (!s || *s != '\"') {
				parse_ok = 0;
				break;
			}
			name_end = s;
			s = skip(s + 1);
			if (*s != ':') {
				parse_ok = 0;
				break;
			}
			s = skip(s + 1);
		}
		if (*s == '{') {
			child = cJSON_NewNode(cJSON_Object);
			if (!child) {
				parse_ok = 0;
				break;
			}
			child->name_deep_copy = deep_copy;
			if (!cJSON_AssignName(child, name_beg, name_end - name_beg)) {
				cJSON_FreeNode(child);
				parse_ok = 0;
				break;
			}
			if (node) {
				cJSON_Append(node, child);
			}
			node = child;
			++s;
		}
		else if (*s == '[') {
			child = cJSON_NewNode(cJSON_Array);
			if (!child) {
				parse_ok = 0;
				break;
			}
			child->name_deep_copy = deep_copy;
			if (!cJSON_AssignName(child, name_beg, name_end - name_beg)) {
				cJSON_FreeNode(child);
				parse_ok = 0;
				break;
			}
			if (node) {
				cJSON_Append(node, child);
			}
			node = child;
			++s;
		}
		else if (*s == '\"') {
			++s;
			value_beg = s;
			s = cJSON_strpbrk(s);
			if (!s || *s != '\"') {
				parse_ok = 0;
				break;
			}
			value_end = skip_forward(s - 1) + 1;
			if (value_end < value_beg) {
				value_end = value_beg;
			}
			child = cJSON_NewNode(cJSON_Value);
			if (!child) {
				parse_ok = 0;
				break;
			}
			child->name_deep_copy = deep_copy;
			child->value_deep_copy = deep_copy;
			if (!cJSON_AssignName(child, name_beg, name_end - name_beg) ||
				!cJSON_AssignValue(child, value_beg, value_end - value_beg))
			{
				cJSON_FreeNode(child);
				parse_ok = 0;
				break;
			}
			child->value_type = cJSON_ValueType_String;
			if (node) {
				cJSON_Append(node, child);
			}
			++s;
		}
		else {
			value_beg = s;
			s = cJSON_strpbrk(s);
			if (!s) {
				parse_ok = 0;
				break;
			}
			value_end = skip_forward(s - 1) + 1;
			if (value_end < value_beg) {
				value_end = value_beg;
			}
			child = cJSON_NewNode(cJSON_Value);
			if (!child) {
				parse_ok = 0;
				break;
			}
			child->name_deep_copy = deep_copy;
			child->value_deep_copy = deep_copy;
			if (!cJSON_AssignName(child, name_beg, name_end - name_beg) ||
				!cJSON_AssignValue(child, value_beg, value_end - value_beg))
			{
				cJSON_FreeNode(child);
				parse_ok = 0;
				break;
			}
			child->value_type = cJSON_ValueType_WeakString;
			if (node) {
				cJSON_Append(node, child);
			}
		}
	}
	if (parse_ok) {
		return root;
	}
	cJSON_Delete(root);
	return NULL;
}

cJSON *cJSON_FromFile(const char* path) {
	cJSON* root = NULL;
	char* fc = NULL;
	FILE* fp = fopen(path, "r");
	do {
		size_t rz;
		long fz;
		if (!fp) {
			break;
		}
		if (fseek(fp, 0, SEEK_END)) {
			break;
		}
		fz = ftell(fp);
		if (fz <= 0) {
			break;
		}
		fc = (char*)malloc(fz + 1);
		if (!fc) {
			break;
		}
		if (fseek(fp, 0, SEEK_SET)) {
			break;
		}
		rz = fread(fc, 1, fz, fp);
		fc[rz] = 0;
		fclose(fp);
		fp = NULL;
		root = cJSON_FromString(fc, 1);
	} while (0);
	if (fp) {
		fclose(fp);
	}
	free(fc);
	return root;
}

size_t cJSON_BytesNum(cJSON* root) {
	size_t len = 0;
	cJSON* node;
	for (node = cJSON_TreeBegin(root); node; node = cJSON_TreeNext(node)) {
		if (cJSON_Object == node->type || cJSON_Array == node->type) {
			len += 2; /* {} or [] */
			if (node->child_num > 1) {
				len += node->child_num - 1; /* , */
			}
			if (node->name && node->name_length > 0) {
				len += node->name_length + 3; /* \"name_length\": */
			}
			continue;
		}
		if (node->type != cJSON_Value) {
			continue;
		}
		if (node->name && node->name_length > 0) {
			len += node->name_length + 3; /* \"name_length\": */
		}
		if (cJSON_ValueType_String == node->value_type) {
			len += node->value_strlen + 2; /* \"value_strlen\" */
		}
		else {
			len += node->value_strlen;
		}
	}
	return len;
}

static char* cJSON_NoLeafNodeToStringBegin(cJSON* node, char* buf) {
	size_t off = 0;
	if (node->name && node->name_length > 0) {
		buf[off++] = '\"';
		memmove(buf + off, node->name, node->name_length);
		off += node->name_length;
		buf[off++] = '\"';
		buf[off++] = ':';
	}
	if (node->type == cJSON_Object) {
		buf[off++] = '{';
	}
	else if (node->type == cJSON_Array) {
		buf[off++] = '[';
	}
	return buf + off;
}

static char* cJSON_NoLeafNodeToStringEnd(cJSON* node, char* buf) {
	if (node->type == cJSON_Object) {
		*(buf++) = '}';
	}
	else if (node->type == cJSON_Array) {
		*(buf++) = ']';
	}
	return buf;
}

static char* cJSON_LeafNodeToString(cJSON* node, char* buf) {
	size_t off = 0;
	if (node->name && node->name_length > 0) {
		buf[off++] = '\"';
		memmove(buf + off, node->name, node->name_length);
		off += node->name_length;
		buf[off++] = '\"';
		buf[off++] = ':';
	}
	if (cJSON_Object == node->type) {
		buf[off++] = '{';
		buf[off++] = '}';
	}
	else if (cJSON_Array == node->type) {
		buf[off++] = '[';
		buf[off++] = ']';
	}
	else if (cJSON_ValueType_Integer == node->value_type) {
		sprintf(buf + off, "%lld", node->value_integer);
		off += node->value_strlen;
	}
	else if (cJSON_ValueType_Double == node->value_type) {
		sprintf(buf + off, "%f", node->value_double);
		off += node->value_strlen;
	}
	else if (cJSON_ValueType_String == node->value_type) {
		buf[off++] = '\"';
		memmove(buf + off, node->value_string, node->value_strlen);
		off += node->value_strlen;
		buf[off++] = '\"';
	}
	else if (cJSON_ValueType_WeakString == node->value_type) {
		memmove(buf + off, node->value_string, node->value_strlen);
		off += node->value_strlen;
	}
	return buf + off;
}

char* cJSON_ToString(cJSON* root, char* buf) {
	char* p = buf;
	cJSON* node = root;
	while (node->child) {
		p = cJSON_NoLeafNodeToStringBegin(node, p);
		node = node->child;
	}
	p = cJSON_LeafNodeToString(node, p);
	while (node) {
		if (node->next) {
			*(p++) = ',';
			node = node->next;
			while (node->child) {
				p = cJSON_NoLeafNodeToStringBegin(node, p);
				node = node->child;
			}
			p = cJSON_LeafNodeToString(node, p);
			continue;
		}
		node = node->parent;
		if (node) {
			p = cJSON_NoLeafNodeToStringEnd(node, p);
		}
	}
	return buf;
}

#ifdef __cplusplus
}
#endif
