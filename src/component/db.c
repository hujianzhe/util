//
// Created by hujianzhe
//

#include "../../inc/component/db.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
	DB_TYPE_RESERVED,
#ifdef DB_ENABLE_MYSQL
	DB_TYPE_MYSQL,
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

static int dbname_to_dbtype(const char* name) {
#ifdef DB_ENABLE_MYSQL
	if (!strcmp(name, "mysql"))
		return DB_TYPE_MYSQL;
#endif
	return DB_TYPE_RESERVED;
}

#ifdef DB_ENABLE_MYSQL
static enum enum_field_types type_map_to_mysql[] = {
	MYSQL_TYPE_TINY,
	MYSQL_TYPE_SHORT,
	MYSQL_TYPE_LONG,
	MYSQL_TYPE_LONGLONG,
	MYSQL_TYPE_FLOAT,
	MYSQL_TYPE_DOUBLE,
	MYSQL_TYPE_VAR_STRING,
	MYSQL_TYPE_BLOB,
};

static MYSQL_TIME* tm2mysqltime(const struct tm* tm, MYSQL_TIME* mt) {
	mt->year = tm->tm_year;
	mt->month = tm->tm_mon;
	mt->day = tm->tm_mday;
	mt->hour = tm->tm_hour;
	mt->minute = tm->tm_min;
	mt->second = tm->tm_sec;
	mt->second_part = 0;
	mt->time_type = MYSQL_TIMESTAMP_DATETIME;
	return mt;
}
#endif

/* env */
DB_RETURN dbInitEnv(const char* dbtype) {
	DB_RETURN res = DB_ERROR;
	switch (dbname_to_dbtype(dbtype)) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_library_init(0, NULL, NULL))
				break;
			if (mysql_thread_safe() != 1) {
				mysql_library_end();
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

void dbCleanEnv(const char* dbtype) {
	switch (dbname_to_dbtype(dbtype)) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			mysql_library_end();
			break;
		}
		#endif
	}
}

DB_RETURN dbAllocTls(void) {
	#ifdef DB_ENABLE_MYSQL
	return mysql_thread_init() ? DB_ERROR : DB_SUCCESS;
	#endif
	return DB_SUCCESS;
}

void dbFreeTls(void) {
	#ifdef DB_ENABLE_MYSQL
	mysql_thread_end();
	#endif
}

/* handle */
DBHandle_t* dbCreateHandle(DBHandle_t* handle, const char* dbtype) {
	handle->type = dbname_to_dbtype(dbtype);
	handle->initok = 0;
	handle->connectok = 0;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			handle->mysql.error_msg = "";
			if (!mysql_init(&handle->mysql.mysql)) {
				handle = NULL;
				break;
			}
			/* 
			 * mysql_thread_init() is automatically called by mysql_init() 
			 */
			break;
		}
		#endif

		default:
			handle = NULL;
	}
	if (handle)
		handle->initok = 1;
	return handle;
}

void dbCloseHandle(DBHandle_t* handle) {
	if (0 == handle->initok)
		return;
	handle->initok = 0;
	handle->connectok = 0;

	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			mysql_close(&handle->mysql.mysql);
			break;
		}
		#endif
	}
}

DBHandle_t* dbConnect(DBHandle_t* handle, const char* ip, unsigned short port, const char* user, const char* pwd, const char* dbname, int timeout_sec) {
	DB_RETURN res = DB_ERROR;

	if (handle->connectok)
		return handle;

	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (timeout_sec > 0) {
				if (mysql_options(&handle->mysql.mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec)) {
					handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
					break;
				}
			}
			if (!mysql_real_connect(&handle->mysql.mysql, ip, user, pwd, dbname, port, NULL, CLIENT_MULTI_STATEMENTS)) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			/* mysql_query(env->hEnv,"set names utf8"); */
			if (mysql_set_character_set(&handle->mysql.mysql, "utf8")) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	if (DB_SUCCESS == res) {
		handle->connectok = 1;
		return handle;
	}
	return NULL;
}

DBHandle_t* dbConnectURL(DBHandle_t* handle, URL_t* url, int timeout_sec) {
	return dbConnect(handle, url->host, url->port, url->user, url->pwd, url->path + 1, timeout_sec);
}

DBHandle_t* dbConnectStringURL(DBHandle_t* handle, const char* str, int timeout_sec) {
	const char* errmsg;
	if (handle->connectok)
		return handle;
	do {
		char* buf;
		URL_t url;
		unsigned int bufsize = urlParsePrepare(&url, str);
		if (0 == bufsize) {
			errmsg = "URL format invalid";
			break;
		}
		buf = (char*)malloc(bufsize);
		if (!buf) {
			errmsg = "memory not enough";
			break;
		}
		handle = dbConnectURL(handle, urlParseFinish(&url, buf), timeout_sec);
		free(buf);
		return handle;
	} while (0);
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
			handle->mysql.error_msg = errmsg;
			break;
		#endif
	}
	return NULL;
}

DB_RETURN dbCheckAlive(DBHandle_t* handle) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_ping(&handle->mysql.mysql)) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

const char* dbHandleErrorMessage(DBHandle_t* handle) {
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			return handle->mysql.error_msg;
		}
		#endif

		default:
			return "undefined database handle type";
	}
}

/* transaction */
DB_RETURN dbEnableAutoCommit(DBHandle_t* handle, int bool_val) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_autocommit(&handle->mysql.mysql, bool_val != 0)) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN dbStartTransaction(DBHandle_t* handle) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_query(&handle->mysql.mysql, "start transaction")) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN dbCommit(DBHandle_t* handle) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_commit(&handle->mysql.mysql)) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN dbRollback(DBHandle_t* handle) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_rollback(&handle->mysql.mysql)) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

/* SQL execute */
DBStmt_t* dbAllocStmt(DBHandle_t* handle, DBStmt_t* stmt) {
	DB_RETURN res = DB_ERROR;
	stmt->handle = NULL;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			stmt->mysql.stmt = mysql_stmt_init(&handle->mysql.mysql);
			if (NULL == stmt->mysql.stmt) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			stmt->mysql.exec_param = NULL;
			stmt->mysql.exec_param_count = 0;
			stmt->mysql.result_field_count = 0;
			stmt->mysql.result_field_meta = NULL;
			stmt->mysql.result_field_param = NULL;
			stmt->mysql.result_index = 0;
			stmt->mysql.error_msg = "";
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	if (DB_SUCCESS == res) {
		stmt->handle = handle;
		return stmt;
	}
	return NULL;
}

DB_RETURN dbCloseStmt(DBStmt_t* stmt) {
	DB_RETURN res = DB_ERROR;
	if (!stmt->handle)
		return DB_SUCCESS;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			free(stmt->mysql.exec_param);
			stmt->mysql.exec_param = NULL;
			stmt->mysql.exec_param_count = 0;

			if (dbFreeResult(stmt) != DB_SUCCESS)
				break;

			if (stmt->mysql.stmt && mysql_stmt_close(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			stmt->mysql.stmt = NULL;
			stmt->mysql.result_index = 0;

			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	stmt->handle = NULL;
    return res;
}

const char* dbStmtErrorMessage(DBStmt_t* stmt) {
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			return stmt->mysql.error_msg;
		}
		#endif

		default:
			return "undefined database handle type";
	}
}

DB_RETURN dbSQLPrepareExecute(DBStmt_t* stmt, const char* sql, size_t sqllen, DBExecuteParam_t* param, unsigned short paramcnt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			/* prepare */
			if (mysql_stmt_prepare(stmt->mysql.stmt, sql, (unsigned long)sqllen)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			stmt->mysql.exec_param_count = (unsigned short)mysql_stmt_param_count(stmt->mysql.stmt);
			if (stmt->mysql.exec_param_count) {
				unsigned short i;
				stmt->mysql.exec_param = (MYSQL_BIND*)calloc(1, sizeof(MYSQL_BIND) * stmt->mysql.exec_param_count);
				if (!stmt->mysql.exec_param) {
					stmt->mysql.error_msg = "not enough memory to alloc MYSQL_BIND";
					break;
				}
				/* bind execute param */
				for (i = 0; i < paramcnt && i < stmt->mysql.exec_param_count; ++i) {
					MYSQL_BIND* bind = stmt->mysql.exec_param + i;
					if (param[i].field_type < 0 ||
						param[i].field_type >= sizeof(type_map_to_mysql) / sizeof(type_map_to_mysql[0]))
					{
						stmt->mysql.error_msg = "set bind param field type invalid";
						break;
					}
					bind->buffer_type = type_map_to_mysql[param[i].field_type];
					bind->buffer = (void*)(param[i].buffer);
					bind->buffer_length = (unsigned long)(param[i].buffer_length);
				}
				if (mysql_stmt_bind_param(stmt->mysql.stmt, stmt->mysql.exec_param)) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					break;
				}
			}
			/* execute */
			if (mysql_stmt_execute(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			free(stmt->mysql.exec_param);
			stmt->mysql.exec_param = NULL;
			stmt->mysql.exec_param_count = 0;
			stmt->mysql.result_index = 0;

			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

/* result set */
long long dbAutoIncrementValue(DBStmt_t* stmt) {
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			return mysql_stmt_insert_id(stmt->mysql.stmt);
		}
		#endif
		default:
			return -1;
	}
}

long long dbAffectedRows(DBStmt_t* stmt) {
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			my_ulonglong affectRows = mysql_stmt_affected_rows(stmt->mysql.stmt);
			if (affectRows != MYSQL_COUNT_ERROR)
				return affectRows;
			else
			{
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				return -1;
			}
			break;
		}
		#endif
		default:
			return -1;
	}
}

short dbGetResult(DBStmt_t* stmt) {
	short result_field_count = -1;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			/* get result */
			if (0 == stmt->mysql.result_index) {
				if (mysql_stmt_store_result(stmt->mysql.stmt)) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					break;
				}
			}
			else {
				#if MYSQL_VERSION_ID >= 50503
				int ret = mysql_stmt_next_result(stmt->mysql.stmt);
				if (-1 == ret) {
					/* no more result */
					break;
				}
				else if (ret) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					break;
				}
				#else
				break;
				#endif
			}
			/* result info */
			result_field_count = mysql_stmt_field_count(stmt->mysql.stmt);
			if (result_field_count > 0) {
				/* result meta */
				stmt->mysql.result_field_meta = mysql_stmt_result_metadata(stmt->mysql.stmt);
				if (NULL == stmt->mysql.result_field_meta) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					mysql_stmt_free_result(stmt->mysql.stmt);
					result_field_count = -1;
					break;
				}
				/* result field */
				stmt->mysql.result_field_param = (MYSQL_BIND*)calloc(1, sizeof(MYSQL_BIND) * result_field_count);
				if (NULL == stmt->mysql.result_field_param) {
					stmt->mysql.error_msg = "not enough memory to alloc MYSQL_BIND";
					mysql_stmt_free_result(stmt->mysql.stmt);
					result_field_count = -1;
					break;
				}
				stmt->mysql.result_field_count = result_field_count;
				stmt->mysql.result_index = 1;
			}
			else if (0 == result_field_count) {
				mysql_stmt_free_result(stmt->mysql.stmt);
				stmt->mysql.result_index = 1;
			}
			
			break;
		}
		#endif
    }
    return result_field_count;
}

DB_RETURN dbFreeResult(DBStmt_t* stmt) {
	DB_RETURN res = DB_ERROR;
	if (!stmt->handle)
		return DB_SUCCESS;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			free(stmt->mysql.result_field_param);
			stmt->mysql.result_field_param = NULL;

			if (stmt->mysql.result_field_meta) {
				mysql_free_result(stmt->mysql.result_field_meta);
				stmt->mysql.result_field_meta = NULL;
			}

			if (stmt->mysql.result_field_count && mysql_stmt_free_result(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			stmt->mysql.result_field_count = 0;

			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

short dbFetchResult(DBStmt_t* stmt, DBResultParam_t* param, unsigned short paramcnt) {
	short result_field_count = -1;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			/* bind result param */
			if (stmt->mysql.result_field_count) {
				unsigned short i;
				for (i = 0; i < paramcnt && i < stmt->mysql.result_field_count; ++i) {
					MYSQL_FIELD* field = mysql_fetch_field(stmt->mysql.result_field_meta);
					if (field) {
						MYSQL_BIND* bind = stmt->mysql.result_field_param + i;
						bind->buffer_type = field->type;
						bind->buffer = param[i].buffer;
						bind->buffer_length = (unsigned long)(param[i].buffer_length);
						if (param[i].value_length) {
							*param[i].value_length = 0;
							if (sizeof(*(bind->length)) == sizeof(*(param[i].value_length)))
								bind->length = (unsigned long*)(param[i].value_length);
							else
								bind->length = &param[i].mysql_value_length;
						}
					}
					else break;
				}
				if (mysql_stmt_bind_result(stmt->mysql.stmt, stmt->mysql.result_field_param)) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					break;
				}
			}
			/* fetch result */
			switch (mysql_stmt_fetch(stmt->mysql.stmt)) {
				case 0:
				case MYSQL_DATA_TRUNCATED:
				{
					if (sizeof(*(param[0].value_length)) != sizeof(*(((MYSQL_BIND*)0)->length))) {
						unsigned short i;
						for (i = 0; i < paramcnt && i < stmt->mysql.result_field_count; ++i) {
							if (param[i].value_length) {
								*param[i].value_length = param[i].mysql_value_length;
							}
						}
					}
					result_field_count = stmt->mysql.result_field_count;
					break;
				}
				case MYSQL_NO_DATA:
					result_field_count = 0;
					break;
				default:
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
			}

			break;
		}
		#endif
	}
	return result_field_count;
}

#ifdef __cplusplus
}
#endif
