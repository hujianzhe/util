//
// Created by hujianzhe
//

#ifndef UTIL_C_DB_DB_H
#define UTIL_C_DB_DB_H

#ifdef DB_ENABLE_MYSQL
	#if defined(_WIN32) || defined(_WIN64)
		#include "../../c/syslib/platform_define.h"
		#include <mysql.h>
		#pragma comment(lib, "libmysql.lib")/* you need copy libmysql.dll to your exe path */
	#else
		#include <mysql/mysql.h>
	#endif
#endif
#include <stddef.h>

typedef enum DB_TYPE {
	DB_TYPE_RESERVED,
#ifdef DB_ENABLE_MYSQL
	DB_TYPE_MYSQL,
#endif
} DB_TYPE;

typedef enum DB_RETURN {
	DB_ERROR,
	DB_SUCCESS,
	DB_MORE_RESULT,
	DB_NO_RESULT,
	DB_NO_DATA,
} DB_RETURN;

typedef enum DB_FIELD_TYPE {
	DB_FIELD_TYPE_TINY,
	DB_FIELD_TYPE_SMALLINT,
	DB_FIELD_TYPE_INT,
	DB_FIELD_TYPE_BIGINT,
	DB_FIELD_TYPE_FLOAT,
	DB_FIELD_TYPE_DOUBLE,
	DB_FIELD_TYPE_VARCHAR,
	DB_FIELD_TYPE_BLOB,
} DB_FIELD_TYPE;

/* HANDLE */
typedef struct DB_HANDLE {
	int type;
	union {
		char reserved[1];
#ifdef DB_ENABLE_MYSQL
		struct {
			MYSQL mysql;
			const char* error_msg;
		} mysql;
#endif
	};
} DB_HANDLE;

/* STMT */
typedef struct DB_STMT {
	struct DB_HANDLE* handle;
	union {
		char reserved[1];
#ifdef DB_ENABLE_MYSQL
		struct {
			MYSQL_STMT* stmt;
			MYSQL_BIND* exec_param;
			unsigned short exec_param_count;
			unsigned short result_field_count;
			MYSQL_RES* result_field_meta;
			MYSQL_BIND* result_field_param;
			const char* error_msg;
		} mysql;
#endif
	};
} DB_STMT;

#ifdef __cplusplus
extern "C" {
#endif

/* HANDLE */
int db_HandleType(DB_HANDLE* handle);
DB_RETURN db_InitEnv(int type);
void db_CleanEnv(int type);
DB_RETURN db_AllocThreadLocalData(void);
void db_FreeThreadLocalData(void);
DB_HANDLE* db_CreateHandle(DB_HANDLE* handle, int type);
void db_CloseHandle(DB_HANDLE* handle);
DB_RETURN db_SetConnectTimeout(DB_HANDLE* handle, int sec);
DB_HANDLE* db_SetupConnect(DB_HANDLE* handle, const char *ip, unsigned short port, const char *user, const char *pwd, const char *database);
DB_RETURN db_PingConnectAlive(DB_HANDLE* handle);
const char* db_HandleErrorMessage(DB_HANDLE* handle);
/* transaction */
DB_RETURN db_EnableAutoCommit(DB_HANDLE* handle, int bool_val);
DB_RETURN db_Commit(DB_HANDLE* handle, int bool_val);
/* SQL execute */
DB_STMT* db_AllocStmt(DB_HANDLE* handle, DB_STMT* stmt);
DB_RETURN db_CloseStmt(DB_STMT* stmt);
const char* db_StmtErrorMessage(DB_STMT* stmt);
DB_RETURN db_SQLPrepare(DB_STMT* stmt, const char *sql, size_t length);
DB_RETURN db_SQLParamCount(DB_STMT* stmt, unsigned short *count);
DB_RETURN db_SQLBindExecuteParam(DB_STMT* stmt, unsigned short index, int field_type, const void* buffer, size_t buffer_length);
DB_RETURN db_SQLExecute(DB_STMT* stmt);
/* result set */
long long db_AutoIncrementValue(DB_STMT* stmt);
long long db_AffectedRows(DB_STMT* stmt);
DB_RETURN db_ResultFieldCount(DB_STMT* stmt, unsigned short *count);
DB_RETURN db_FirstResult(DB_STMT* stmt);
DB_RETURN db_NextResult(DB_STMT* stmt);
DB_RETURN db_FreeResult(DB_STMT* stmt);
DB_RETURN db_BindResultField(DB_STMT* stmt, unsigned short index, void* buffer, size_t buffer_length, size_t *value_length);
DB_RETURN db_FetchResult(DB_STMT* stmt);

#ifdef __cplusplus
}
#endif

#endif
