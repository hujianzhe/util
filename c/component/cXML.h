//
// Created by hujianzhe
//

#ifndef UTIL_C_COMPONENT_XML_H
#define	UTIL_C_COMPONENT_XML_H

#include "../compiler_define.h"
#include <stddef.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct cXMLAttr_t {
	struct cXMLAttr_t *prev, *next;
	char*	name;
	char*	value;
	size_t	szname;
	size_t	szvalue;
} cXMLAttr_t;

typedef struct cXML_t {
	struct cXML_t*	tree_parent;
	struct cXML_t*	tree_child;
	struct cXML_t*	tree_next;
	cXMLAttr_t*		attr;
	char*			name;
	char*			content;
	size_t			szname;
	size_t			szcontent;
	int				numattr;
	int				numchild;
	int				deep_copy;
} cXML_t;

typedef struct cXML_Hooks {
	void*(*malloc_fn)(size_t);
	void(*free_fn)(void*);
} cXML_Hooks;

UTIL_LIBAPI void cXML_SetHooks(cXML_Hooks* hooks);
UTIL_LIBAPI cXML_Hooks* cXML_GetHooks(cXML_Hooks* hooks);
UTIL_LIBAPI void cXML_Delete(cXML_t* root);

UTIL_LIBAPI cXML_t* cXML_Parse(const char* data);
UTIL_LIBAPI cXML_t* cXML_ParseDirect(char* data);
UTIL_LIBAPI cXML_t* cXML_FirstChild(cXML_t* node, const char* name);
UTIL_LIBAPI cXML_t* cXML_NextChild(cXML_t* node);
UTIL_LIBAPI cXMLAttr_t* cXML_Attr(cXML_t* node, const char* name);

#ifdef  __cplusplus
}
#endif

#endif
