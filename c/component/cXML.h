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
	struct cXML_t*	parent;
	struct cXML_t*	child;
	struct cXML_t*	left;
	struct cXML_t*	right;
	cXMLAttr_t*		attr;

	char*			name;
	char*			content;
	size_t			szname;
	size_t			szcontent;
	unsigned int	numattr;
	unsigned int	numchild;
	int				deep_copy;
} cXML_t;

typedef struct cXMLHooks_t {
	void*(*malloc_fn)(size_t);
	void(*free_fn)(void*);
} cXMLHooks_t;

UTIL_LIBAPI void cXML_SetHooks(cXMLHooks_t* hooks);
UTIL_LIBAPI cXMLHooks_t* cXML_GetHooks(cXMLHooks_t* hooks);

UTIL_LIBAPI cXML_t* cXML_Detach(cXML_t* node);
UTIL_LIBAPI void cXML_Delete(cXML_t* node);

UTIL_LIBAPI cXML_t* cXML_Parse(const char* data);
UTIL_LIBAPI cXML_t* cXML_ParseDirect(char* data);
UTIL_LIBAPI cXML_t* cXML_FirstChild(cXML_t* node, const char* name);
UTIL_LIBAPI cXML_t* cXML_NextChild(cXML_t* node);
UTIL_LIBAPI cXMLAttr_t* cXML_Attr(cXML_t* node, const char* name);
UTIL_LIBAPI size_t cXML_ByteSize(cXML_t* root);
UTIL_LIBAPI char* cXML_Print(cXML_t* root, char* buffer);

#ifdef  __cplusplus
}
#endif

#endif
