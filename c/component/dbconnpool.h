//
// Created by hujianzhe
//

#ifndef	UTIL_C_COMPONENT_DBCONNPOOL_H
#define	UTIL_C_COMPONENT_DBCONNPOOL_H

#include "../datastruct/hashtable.h"
#include "../syslib/ipc.h"

typedef struct DBConnPool_t {
	int schema;
	const char* ip;
	unsigned short port;
	const char* user;
	const char* pwd;
	const char* dbname;
	unsigned short connect_maxcnt;
	unsigned short connect_timeout_second;
	int m_initok;
	unsigned short m_connectcnt;
	CriticalSection_t m_lock;
	Hashtable_t m_table;
	HashtableNode_t* m_bulks[32];
} DBConnPool_t;

struct DBHandle_t;

#ifdef __cplusplus
extern "C" {
#endif

DBConnPool_t* dbconnpoolInit(DBConnPool_t* pool, unsigned short connect_maxcnt);
void dbconnpoolPushHandle(DBConnPool_t* pool, struct DBHandle_t* handle);
struct DBHandle_t* dbconnpoolPopHandle(DBConnPool_t* pool);
void dbconnpoolClean(DBConnPool_t* pool);
void dbconnpoolDestroy(DBConnPool_t* pool);

#ifdef __cplusplus
}
#endif

#endif
