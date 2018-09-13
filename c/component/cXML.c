//
// Created by hujianzhe
//

#include <string.h>
#include <malloc.h>
#include "cXML.h"

#define WHITESPACE " \t\n\r"

#ifdef  __cplusplus
extern "C" {
#endif

static void*(*cXML_malloc)(size_t sz) = malloc;
static void(*cXML_free)(void* ptr) = free;

void cXML_SetHooks(cXMLHooks_t* hooks) {
	if (!hooks) { /* Reset hooks */
		cXML_malloc = malloc;
		cXML_free = free;
		return;
	}

	cXML_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
	cXML_free = (hooks->free_fn) ? hooks->free_fn : free;
}

cXMLHooks_t* cXML_GetHooks(cXMLHooks_t* p) {
	static cXMLHooks_t hooks;
	if (!p)
		p = &hooks;
	p->malloc_fn = cXML_malloc;
	p->free_fn = cXML_free;
	return p;
}

static char* xt_strndup(char* s, size_t len) {
	if (len) {
		char* p = (char*)cXML_malloc(len + 1);
		if (p) {
			memcpy(p, s, len);
			p[len] = '\0';
			return p;
		}
	}
	return NULL;
}

static void xt_skip_ws(const char** data) {
	while (**data == ' ' || **data == '\t' || **data == '\n' || **data == '\r')
		(*data)++;
}

static void xt_skip_wsc(const char** data) {
	for (;;) {
		const char* p = *data;
		if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && *p != '<')
			break;
		if (*p == '<') {
			if (p[1] != '!' || p[2] != '-' || p[3] != '-')
				return;
			p += 4;
			for (;;) {
				if (*p == '\0')
					break;
				if (p[0] == '-' && p[1] == '-' && p[2] == '>') {
					p += 3;
					break;
				}
				p++;
			}
			*data = p;
		}
		else
			(*data)++;
	}
}

static int xt_skip_until(const char** data, const char* what) {
	for (;;) {
		const char* wsp = what;
		while (*wsp && *wsp != **data)
			wsp++;

		if (*wsp)
			return 1;
		else {
			(*data)++;
			if ('\0' == **data)
				return 0;
		}
	}
}

static int xt_skip_string(const char** data, char qch) {
	const char* S = *data;
	while (*S) {
		if (*S == qch) {
			/* backtrack for backslashes */
			int bs = 0;
			const char* T = S;
			while (T > *data && *(T - 1) == '\\') {
				bs++;
				T--;
			}

			if ((bs & 1) == 0) {
				*data = S;
				return 1;
			}
		}
		S++;
	}
	return 0;
}

static void xt_skip_hint(const char** data) {
	if (**data == '<' && (*data)[1] == '?') {
		*data += 2;
		while (*(*data - 1) != '?' || **data != '>') {
			if ('\0' == **data)
				return;
			(*data)++;
		}
		(*data)++;
	}
}

static cXML_t* xt_create_node(int deep_cpoy) {
	cXML_t* node = (cXML_t*)cXML_malloc(sizeof(cXML_t));
	node->parent = NULL;
	node->child = NULL;
	node->left = NULL;
	node->right = NULL;
	node->numchild = 0;
	node->content = NULL;
	node->name = NULL;
	node->attr = NULL;
	node->szcontent = 0;
	node->szname = 0;
	node->numattr = 0;
	node->deep_copy = deep_cpoy;
	return node;
}

cXML_t* cXML_Detach(cXML_t* node) {
	if (node) {
		if (node->parent && node->parent->child == node)
			node->parent->child = node->right;
		if (node->left)
			node->left->right = node->right;
		if (node->right)
			node->right->left = node->left;
		node->parent = node->left = node->right = NULL;
	}
	return node;
}

void cXML_Delete(cXML_t* node) {
	if (!node)
		return;
	if (node->child)
		cXML_Delete(node->child);
	if (node->right)
		cXML_Delete(node->right);

	if (node->deep_copy) {
		cXML_free(node->name);
		cXML_free(node->content);
	}

	do {
		cXMLAttr_t *cur, *next;
		for (cur = node->attr; cur; cur = next) {
			next = cur->next;
			if (node->deep_copy) {
				cXML_free(cur->name);
				cXML_free(cur->value);
			}
			cXML_free(cur);
		}
	} while (0);

	cXML_free(node);
}

static void xt_node_add_attrib(cXML_t* node, cXMLAttr_t* attrib) {
	attrib->prev = NULL;
	if (node->attr) {
		attrib->next = node->attr;
		node->attr->prev = attrib;
	}
	else
		attrib->next = NULL;
	node->attr = attrib;
	node->numattr++;
}

static cXML_t* xt_parse_node(int deep_copy, const char** data) {
	cXML_t* node;
	cXML_t* C;
	const char* S = *data;

	xt_skip_wsc(&S);
	if (*S != '<')
		return NULL;

	node = xt_create_node(deep_copy);
	/* node->header = S; */
	S++;

	/* name */
	xt_skip_ws(&S);
	node->name = (char*)S;
	if (!xt_skip_until(&S, WHITESPACE "/>"))
		goto fnq;
	node->szname = S - node->name;

	/* attributes */
	xt_skip_ws(&S);
	while (*S != '>' && *S != '/') {
		cXMLAttr_t* attrib = (cXMLAttr_t*)cXML_malloc(sizeof(cXMLAttr_t));
		if (!attrib)
			goto fnq;

		attrib->name = (char*)S;

		if (!xt_skip_until(&S, WHITESPACE "=/>"))
			goto fnq;

		attrib->szname = S - attrib->name;
		xt_skip_ws(&S);

		/* value */
		if (*S == '=') {
			S++;
			xt_skip_ws(&S);
			if (*S == '\"' || *S == '\'') {
				char qch = *S;
				S++;
				attrib->value = (char*)S;
				if (!xt_skip_string(&S, qch))
					goto fnq;
			}
			else
				goto fnq;

			attrib->szvalue = S++ - attrib->value;
			xt_skip_ws(&S);
		}

		xt_node_add_attrib(node, attrib);
	}

	if (*S == '/') {
		if (S[1] != '>')
			goto fnq;
		S += 2;
		/* node->szheader = S - node->header; */
		goto fbe; /* finished before end */
	}

	S++;
	/* node->szheader = S - node->header; */
	node->content = (char*)S;

	C = node;
	xt_skip_wsc(&S);

	if ('\0' == *S)
		goto fnq;

	while (*S) {
		if (*S == '<') {
			const char* RB = S;
			cXML_t* ch;

			if (S[1] == '/') {
				const char* EP;
				int len;

				S += 2;
				xt_skip_ws(&S);
				EP = S;
				if (!xt_skip_until(&S, WHITESPACE ">"))
					goto fnq;

				len = S - EP;
				if (len != node->szname || strncmp(node->name, EP, len) != 0) {
					*data = RB;
					goto fnq;
				}

				node->szcontent = RB - node->content;
				S++;
				break;
			}

			ch = xt_parse_node(deep_copy, &S);
			if (ch) {
				ch->parent = node;
				if (C == node)
					C->child = ch;
				else {
					C->right = ch;
					ch->left = C;
				}
				C = ch;
				node->numchild++;
				continue;
			}
			else
				goto fnq;
		}
		S++;
	}

fbe:
	*data = S;
	if (0 == node->szcontent)
		node->content = NULL;
	return node;

	/* free and quit */
fnq:
	cXML_Delete(node);
	return NULL;
}

static void xt_parse_to_string(cXML_t* node) {
	if (node->child)
		xt_parse_to_string(node->child);
	if (node->right)
		xt_parse_to_string(node->right);

	if (node->deep_copy) {
		cXMLAttr_t* attr;
		node->name = xt_strndup(node->name, node->szname);
		node->content = xt_strndup(node->content, node->szcontent);
		for (attr = node->attr; attr; attr = attr->next) {
			attr->name = xt_strndup(attr->name, attr->szname);
			attr->value = xt_strndup(attr->value, attr->szvalue);
		}
	}
	else {
		cXMLAttr_t* attr;
		node->name[node->szname] = '\0';
		/* node->header[node->szheader] = '\0'; */
		if (node->szcontent)
			node->content[node->szcontent] = '\0';

		for (attr = node->attr; attr; attr = attr->next) {
			if (attr->szname)
				attr->name[attr->szname] = '\0';
			if (attr->szvalue)
				attr->value[attr->szvalue] = '\0';
		}
	}
}

cXML_t* cXML_Parse(const char* data) {
	cXML_t* root;
	const char* S = data;
	xt_skip_wsc(&S);
	xt_skip_hint(&S);
	xt_skip_wsc(&S);
	root = xt_parse_node(1, &S);
	if (root)
		xt_parse_to_string(root);
	return root;
}

cXML_t* cXML_ParseDirect(char* data) {
	cXML_t* root;
	const char* S = data;
	xt_skip_wsc(&S);
	xt_skip_hint(&S);
	xt_skip_wsc(&S);
	root = xt_parse_node(0, &S);
	if (root)
		xt_parse_to_string(root);
	return root;
}

cXML_t* cXML_FirstChild(cXML_t* node, const char* name) {
	cXML_t* ch;
	for (ch = node->child; ch; ch = ch->right) {
		if (0 == strncmp(ch->name, name, ch->szname))
			break;
	}
	return ch;
}

cXML_t* cXML_NextChild(cXML_t* node) {
	cXML_t* ch;
	for (ch = node->right; ch; ch = ch->right) {
		if (0 == strncmp(ch->name, node->name, ch->szname))
			break;
	}
	return ch;
}

cXMLAttr_t* cXML_Attr(cXML_t* node, const char* name) {
	cXMLAttr_t *cur;
	for (cur = node->attr; cur; cur = cur->next) {
		if (strncmp(cur->name, name, cur->szname) == 0)
			break;
	}
	return cur;
}

#ifdef  __cplusplus
}
#endif
