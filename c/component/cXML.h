//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPONENT_XML_H
#define	UTIL_C_COMPONENT_XML_H

#include "../compiler_define.h"
#include <stddef.h>

struct cXML_t;

typedef struct cXMLAttr_t {
	char*	name;
	char*	value;
	size_t	szname;
	size_t	szvalue;

	struct cXMLAttr_t *prev, *next;
	struct cXML_t* node;
	int		deep_copy;
	int		need_free;
} cXMLAttr_t;

typedef struct cXML_t {
	char*			name;
	char*			content;
	size_t			szname;
	size_t			szcontent;
	unsigned int	numattr;
	unsigned int	numchild;
	int				deep_copy;
	int				need_free;

	struct cXML_t*	parent;
	struct cXML_t*	child;
	struct cXML_t*	left;
	struct cXML_t*	right;
	cXMLAttr_t*		attr;
} cXML_t;

typedef struct cXMLHooks_t {
	void*(*malloc_fn)(size_t);
	void(*free_fn)(void*);
} cXMLHooks_t;

#ifdef  __cplusplus
extern "C" {
#endif

UTIL_LIBAPI void cXML_SetHooks(cXMLHooks_t* hooks);
UTIL_LIBAPI cXMLHooks_t* cXML_GetHooks(cXMLHooks_t* hooks);

UTIL_LIBAPI cXML_t* cXML_Create(cXML_t* node, int deep_cpoy);
UTIL_LIBAPI cXMLAttr_t* cXML_CreateAttr(cXMLAttr_t* attr, int deep_copy);
UTIL_LIBAPI cXML_t* cXML_AddAttr(cXML_t* node, cXMLAttr_t* attr);
UTIL_LIBAPI cXMLAttr_t* cXML_DetachAttr(cXMLAttr_t* attr);
UTIL_LIBAPI void cXML_DeleteAttr(cXMLAttr_t* attr);
UTIL_LIBAPI cXML_t* cXML_Add(cXML_t* node, cXML_t* item);
UTIL_LIBAPI cXML_t* cXML_Detach(cXML_t* node);
UTIL_LIBAPI void cXML_Delete(cXML_t* node);

UTIL_LIBAPI cXML_t* cXML_Parse(const char* data, int deep_copy);
UTIL_LIBAPI cXML_t* cXML_ParseFromFile(const char* path);
UTIL_LIBAPI cXML_t* cXML_FirstChild(cXML_t* node, const char* name);
UTIL_LIBAPI cXML_t* cXML_NextChild(cXML_t* node);
UTIL_LIBAPI cXMLAttr_t* cXML_GetAttr(cXML_t* node, const char* name);
UTIL_LIBAPI size_t cXML_ByteSize(cXML_t* root);
UTIL_LIBAPI char* cXML_Print(cXML_t* root, char* buffer);

#ifdef  __cplusplus
}
#endif

#endif
