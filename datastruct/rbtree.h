//
// Created by hujianzhe
//

#ifndef	UTIL_DATASTRUCT_RBTREE_H_
#define	UTIL_DATASTRUCT_RBTREE_H_

typedef union rbtree_node_key_t {
	double f64;
	float f32;
	char c;
	unsigned char uc;
	char i8;
	unsigned char u8;
	short i16;
	unsigned short u16;
	int i32;
	unsigned int u32;
	long long i64;
	unsigned long long u64;
	char* s;
	void* p;
} rbtree_node_key_t;
typedef int(*rbtree_node_key_cmp)(union rbtree_node_key_t, union rbtree_node_key_t);

typedef struct rbtree_node_t {
	unsigned long rb_color;
	struct rbtree_node_t *rb_parent;
	struct rbtree_node_t *rb_right;
	struct rbtree_node_t *rb_left;
	union rbtree_node_key_t rb_key;
} rbtree_node_t;

typedef struct rbtree_t {
	struct rbtree_node_t *rb_tree_node;
} rbtree_t;

#ifdef	__cplusplus
extern "C" {
#endif

void rbtree_node_init(struct rbtree_node_t* node, union rbtree_node_key_t key);
int rbtree_insert_node(struct rbtree_t* root, struct rbtree_node_t* node, rbtree_node_key_cmp cmp);
struct rbtree_node_t* rbtree_search_node(struct rbtree_t* root, union rbtree_node_key_t key, rbtree_node_key_cmp cmp);
void rbtree_remove_node(struct rbtree_t* root, struct rbtree_node_t* node);
int rbtree_remove_key(struct rbtree_t* root, union rbtree_node_key_t key, rbtree_node_key_cmp cmp);

struct rbtree_node_t* rbtree_next_node(struct rbtree_node_t* node);
struct rbtree_node_t* rbtree_prev_node(struct rbtree_node_t* node);
struct rbtree_node_t* rbtree_first_node(struct rbtree_t* root);
struct rbtree_node_t* rbtree_last_node(struct rbtree_t* root);

#ifdef	__cplusplus
}
#endif

#endif
