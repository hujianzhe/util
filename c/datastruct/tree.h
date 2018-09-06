//
// Created by hujianzhe
//

#ifndef UTIL_C_DATASTRUCT_TREE_H
#define	UTIL_C_DATASTRUCT_TREE_H

#include "../compiler_define.h"

typedef struct Tree_t {
	struct Tree_t *parent, *child, *left, *right;
} Tree_t;

#ifdef	__cplusplus
extern "C" {
#endif

UTIL_LIBAPI void treeInit(struct Tree_t* node);
UTIL_LIBAPI struct Tree_t* treeRoot(const struct Tree_t* node);
UTIL_LIBAPI void treeInsertChild(struct Tree_t* parent_node, struct Tree_t* new_node);
UTIL_LIBAPI void treeInsertBrother(struct Tree_t* node, struct Tree_t* new_node);
UTIL_LIBAPI void treeRemove(struct Tree_t* node);

UTIL_LIBAPI struct Tree_t* treeBegin(struct Tree_t* node);
UTIL_LIBAPI struct Tree_t* treeNext(struct Tree_t* node);

#ifdef	__cplusplus
}
#endif

#endif // !UTIL_DATASTRUCT_TREE_H_
