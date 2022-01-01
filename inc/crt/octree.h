//
// Created by hujianzhe on 21-12-29
//

#ifndef	UTIL_C_CRT_OCTREE_H
#define	UTIL_C_CRT_OCTREE_H

#include "../compiler_define.h"
#include "../datastruct/list.h"

typedef struct OctreeNode_t {
	float pos[3];
	float half[3];
	List_t obj_list;
	int obj_cnt;
	unsigned int deep_num; // from 1,2,3...
	struct OctreeNode_t* parent;
	struct OctreeNode_t* childs;
} OctreeNode_t;

typedef struct OctreeObject_t {
	ListNode_t _node;
	float* pos;
	float* half;
	OctreeNode_t* oct;
	void* udata;
} OctreeObject_t;

typedef struct Octree_t {
	OctreeNode_t* nodes;
	unsigned int nodes_cnt;
	unsigned int max_deep_num; // must >= 1
	int max_cnt_per_node;
} Octree_t;

typedef struct OctreeFinder_t {
	OctreeNode_t** nodes;
	unsigned int cnt;
} OctreeFinder_t;

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll Octree_t* octreeInit(Octree_t* tree, const float pos[3], const float half[3], unsigned int max_deep_num);
__declspec_dll void octreeUpdateObject(Octree_t* tree, OctreeObject_t* obj);
__declspec_dll void octreeRemoveObject(OctreeObject_t* obj);
__declspec_dll void octreeFindNodes(OctreeNode_t* root, const float pos[3], const float half[3], OctreeFinder_t* finder);
__declspec_dll void octreeClear(Octree_t* tree);
__declspec_dll void octreeDestroy(Octree_t* tree);

#ifdef __cplusplus
}
#endif

#endif
