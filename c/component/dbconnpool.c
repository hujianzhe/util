//
// Created by hujianzhe
//

#include "dbconnpool.h"
#include "db.h"
#include <stdlib.h>

typedef struct DBConnItem {
	HashtableNode_t m_node;
	DBHandle_t handle;
} DBConnItem;

#ifdef __cplusplus
extern "C" {
#endif

static int keycmp(struct HashtableNode_t* node, const void* key) {
	DBConnItem* item = pod_container_of(node, DBConnItem, m_node);
	return &item->handle != (DBHandle_t*)key;
}
static unsigned int keyhash(const void* key) {
	return (size_t)(DBHandle_t*)key;
}

DBConnPool_t* dbconnpoolInit(DBConnPool_t* pool, unsigned short connect_maxcnt) {
	pool->m_initok = 0;
	if (!criticalsectionCreate(&pool->m_lock))
		return NULL;
	pool->schema = DB_TYPE_RESERVED;
	pool->ip = NULL;
	pool->port = 0;
	pool->user = NULL;
	pool->pwd = NULL;
	pool->dbname = NULL;
	pool->connect_maxcnt = connect_maxcnt;
	pool->connect_timeout_second = -1;
	pool->m_connectcnt = 0;
	hashtableInit(&pool->m_table, pool->m_bulks, sizeof(pool->m_bulks) / sizeof(pool->m_bulks[0]), keycmp, keyhash);
	pool->m_initok = 1;
	return pool;
}

void dbconnpoolPushHandle(DBConnPool_t* pool, DBHandle_t* handle) {
	if (handle) {
		DBConnItem* item = pod_container_of(handle, DBConnItem, handle);

		criticalsectionEnter(&pool->m_lock);

		hashtableInsertNode(&pool->m_table, &item->m_node);

		criticalsectionLeave(&pool->m_lock);
	}
}

DBHandle_t* dbconnpoolPopHandle(DBConnPool_t* pool) {
	DBHandle_t* dbhandle = NULL;
	int new_connect = 0;

	criticalsectionEnter(&pool->m_lock);
	do {
		HashtableNode_t* node = hashtableFirstNode(&pool->m_table);
		if (node) {
			DBConnItem* item = pod_container_of(node, DBConnItem, m_node);
			dbhandle = &item->handle;

			hashtableRemoveNode(&pool->m_table, node);
		}
		else if (pool->m_connectcnt >= pool->connect_maxcnt) {
			break;
		}
		else {
			new_connect = 1;
			++pool->m_connectcnt;
		}
	} while (0);
	criticalsectionLeave(&pool->m_lock);

	if (dbhandle || !new_connect)
		return dbhandle;

	do {
		DBConnItem* item = (DBConnItem*)malloc(sizeof(DBConnItem));
		if (!item)
			return NULL;

		if (!dbCreateHandle(&item->handle, pool->schema)) {
			free(item);
			break;
		}
		if (pool->connect_timeout_second > 0) {
			if (!dbSetConnectTimeout(&item->handle, pool->connect_timeout_second)) {
				dbCloseHandle(&item->handle);
				free(item);
				break;
			}
		}
		if (!dbConnect(&item->handle, pool->ip, pool->port, pool->user, pool->pwd, pool->dbname)) {
			dbCloseHandle(&item->handle);
			free(item);
			break;
		}
		return &item->handle;
	} while (0);

	return NULL;
}

void dbconnpoolClean(DBConnPool_t* pool) {
	HashtableNode_t* cur, *next;

	criticalsectionEnter(&pool->m_lock);

	for (cur = hashtableFirstNode(&pool->m_table); cur; cur = next) {
		DBConnItem* item = pod_container_of(cur, DBConnItem, m_node);
		next = hashtableNextNode(cur);
		dbCloseHandle(&item->handle);
		free(item);
	}
	hashtableInit(&pool->m_table, pool->m_bulks, sizeof(pool->m_bulks) / sizeof(pool->m_bulks[0]), keycmp, keyhash);
	pool->m_connectcnt = 0;

	criticalsectionLeave(&pool->m_lock);
}

void dbconnpoolDestroy(DBConnPool_t* pool) {
	if (pool && pool->m_initok) {
		dbconnpoolClean(pool);
		criticalsectionClose(&pool->m_lock);
	}
}

#ifdef __cplusplus
}
#endif
