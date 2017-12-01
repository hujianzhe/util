//
// Created by hujianzhe
//

#include "db.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* HANDLE */
int db_HandleType(DB_HANDLE* handle) { return handle->type; }

DB_RETURN db_InitEnv(int type) {
	DB_RETURN res = DB_ERROR;
	switch (type) {
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

void db_CleanEnv(int type) {
	switch (type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			mysql_library_end();
			break;
		}
		#endif
	}
}

DB_RETURN db_AllocThreadLocalData(void) {
	#ifdef DB_ENABLE_MYSQL
	return mysql_thread_init() ? DB_ERROR : DB_SUCCESS;
	#endif
	return DB_SUCCESS;
}

void db_FreeThreadLocalData(void) {
	#ifdef DB_ENABLE_MYSQL
	mysql_thread_end();
	#endif
}

DB_HANDLE* db_CreateHandle(DB_HANDLE* handle, int type) {
	handle->type = type;
	switch (type) {
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
	return handle;
}

void db_CloseHandle(DB_HANDLE* handle) {
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

DB_RETURN db_SetConnectTimeout(DB_HANDLE* handle, int sec) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_options(&handle->mysql.mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&sec)) {
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

DB_HANDLE* db_SetupConnect(DB_HANDLE* handle, const char* ip, unsigned short port, const char* user, const char* pwd, const char* database) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (!mysql_real_connect(&handle->mysql.mysql, ip, user, pwd, database, port, NULL, CLIENT_MULTI_STATEMENTS)) {
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
    return DB_SUCCESS == res ? handle : NULL;
}

DB_RETURN db_PingConnectAlive(DB_HANDLE* handle) {
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

const char* db_HandleErrorMessage(DB_HANDLE* handle) {
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
DB_RETURN db_EnableAutoCommit(DB_HANDLE* handle, int bool_val) {
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

DB_RETURN db_Commit(DB_HANDLE* handle, int bool_val) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (bool_val) {
				if (mysql_commit(&handle->mysql.mysql)) {
					handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
					break;
				}
			}
			else {
				if (mysql_rollback(&handle->mysql.mysql)) {
					handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
					break;
				}
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

/* SQL execute */
DB_STMT* db_AllocStmt(DB_HANDLE* handle, DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			stmt->mysql.stmt = mysql_stmt_init(&handle->mysql.mysql);
			if (NULL == stmt->mysql.stmt) {
				handle->mysql.error_msg = mysql_error(&handle->mysql.mysql);
				break;
			}
			stmt->handle = handle;
			stmt->mysql.exec_param = NULL;
			stmt->mysql.exec_param_count = 0;
			stmt->mysql.result_field_count = 0;
			stmt->mysql.result_field_meta = NULL;
			stmt->mysql.result_field_param = NULL;
			stmt->mysql.error_msg = "";
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return DB_SUCCESS == res ? stmt : NULL;
}

DB_RETURN db_CloseStmt(DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			free(stmt->mysql.exec_param);
			stmt->mysql.exec_param = NULL;
			if (mysql_stmt_close(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
    return res;
}

const char* db_StmtErrorMessage(DB_STMT* stmt) {
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

DB_RETURN db_SQLPrepare(DB_STMT* stmt, const char* sql, size_t length) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_stmt_prepare(stmt->mysql.stmt, sql, length)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			stmt->mysql.exec_param_count = mysql_stmt_param_count(stmt->mysql.stmt);
			if (stmt->mysql.exec_param_count) {
				stmt->mysql.exec_param = (MYSQL_BIND*)calloc(1, sizeof(MYSQL_BIND) * stmt->mysql.exec_param_count);
				if (!stmt->mysql.exec_param) {
					stmt->mysql.error_msg = "not enough memory to alloc MYSQL_BIND";
					break;
				}
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN db_SQLParamCount(DB_STMT* stmt, unsigned short* count) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			*count = stmt->mysql.exec_param_count;
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN db_SQLBindExecuteParam(DB_STMT* stmt, unsigned short index, int field_type, const void* buffer, size_t buffer_length) {
	/*
	 *	index start at 1
	 *
	 */
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (index > stmt->mysql.exec_param_count || 0 == index) {
				stmt->mysql.error_msg = "set bind param index invalid";
				break;
			}
			MYSQL_BIND* bind = stmt->mysql.exec_param + index - 1;
			bind->buffer_type = type_map_to_mysql[field_type];
			bind->buffer = (void*)buffer;
			bind->buffer_length = buffer_length;
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN db_SQLExecute(DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (stmt->mysql.exec_param_count) {
				if (mysql_stmt_bind_param(stmt->mysql.stmt, stmt->mysql.exec_param)) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					break;
				}
			}
			if (mysql_stmt_execute(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			free(stmt->mysql.exec_param);
			stmt->mysql.exec_param = NULL;
			stmt->mysql.exec_param_count = 0;
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

/* result set */
long long db_AutoIncrementValue(DB_STMT* stmt) {
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

long long db_AffectedRows(DB_STMT* stmt) {
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

DB_RETURN db_ResultFieldCount(DB_STMT* stmt, unsigned short* count) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			*count = stmt->mysql.result_field_count;
			res = DB_SUCCESS;
            break;
        }
		#endif
    }
    return res;
}

DB_RETURN db_FirstResult(DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (mysql_stmt_store_result(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			stmt->mysql.result_field_count = mysql_stmt_field_count(stmt->mysql.stmt);
			if (stmt->mysql.result_field_count) {
				stmt->mysql.result_field_param = (MYSQL_BIND*)calloc(1, sizeof(MYSQL_BIND) * stmt->mysql.result_field_count);
				if (NULL == stmt->mysql.result_field_param) {
					stmt->mysql.error_msg = "not enough memory to alloc MYSQL_BIND";
					mysql_stmt_free_result(stmt->mysql.stmt);
					break;
				}
				stmt->mysql.result_field_meta = mysql_stmt_result_metadata(stmt->mysql.stmt);
				if (NULL == stmt->mysql.result_field_meta) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					free(stmt->mysql.result_field_param);
					mysql_stmt_free_result(stmt->mysql.stmt);
					break;
				}
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
    }
    return res;
}

DB_RETURN db_NextResult(DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			#if MYSQL_VERSION_ID >= 50503
			int ret = mysql_stmt_next_result(stmt->mysql.stmt);
			if (-1 == ret)
				res = DB_NO_RESULT;
			else if (ret) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			else {
				stmt->mysql.result_field_count = mysql_stmt_field_count(stmt->mysql.stmt);
				if (stmt->mysql.result_field_count) {
					stmt->mysql.result_field_param = (MYSQL_BIND*)calloc(1, sizeof(MYSQL_BIND) * stmt->mysql.result_field_count);
					if (NULL == stmt->mysql.result_field_param) {
						stmt->mysql.error_msg = "not enough memory to alloc MYSQL_BIND";
						mysql_stmt_free_result(stmt->mysql.stmt);
						break;
					}
					stmt->mysql.result_field_meta = mysql_stmt_result_metadata(stmt->mysql.stmt);
					if (NULL == stmt->mysql.result_field_meta) {
						stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
						free(stmt->mysql.result_field_param);
						mysql_stmt_free_result(stmt->mysql.stmt);
						break;
					}
				}
				res = DB_MORE_RESULT;
			}
			#else
			res = DB_NO_RESULT;
			#endif
            break;
		}
		#endif
	}
	return res;
}

DB_RETURN db_FreeResult(DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			free(stmt->mysql.result_field_param);
			stmt->mysql.result_field_param = NULL;
			stmt->mysql.result_field_count = 0;
			if (stmt->mysql.result_field_meta) {
				mysql_free_result(stmt->mysql.result_field_meta);
				stmt->mysql.result_field_meta = NULL;
			}
			if (mysql_stmt_free_result(stmt->mysql.stmt)) {
				stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
				break;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN db_BindResultField(DB_STMT* stmt, unsigned short index, void* buffer, size_t buffer_length, size_t *value_length) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (index > stmt->mysql.result_field_count || 0 == index) {
				stmt->mysql.error_msg = "set bind param index invalid";
				break;
			}
			MYSQL_BIND* bind = stmt->mysql.result_field_param + index - 1;
			//bind->buffer_type = type_map_to_mysql[field_type];
			bind->buffer = buffer;
			bind->buffer_length = buffer_length;
			if (value_length) {
				union ending {
					unsigned short num;
					unsigned char is_lettle;
				} ending;
				ending.num = 1;
				if (sizeof(*bind->length) < sizeof(*value_length) && !ending.is_lettle)
					bind->length = ((unsigned long*)value_length) + 1;
				else
					bind->length = (unsigned long*)value_length;
				*value_length = 0;
			}
			res = DB_SUCCESS;
			break;
		}
		#endif
	}
	return res;
}

DB_RETURN db_FetchResult(DB_STMT* stmt) {
	DB_RETURN res = DB_ERROR;
	switch (stmt->handle->type) {
		#ifdef DB_ENABLE_MYSQL
		case DB_TYPE_MYSQL:
		{
			if (stmt->mysql.result_field_count) {
				unsigned short index;
				MYSQL_FIELD* field;
				for (index = 0; field = mysql_fetch_field(stmt->mysql.result_field_meta); ++index) {
					stmt->mysql.result_field_param[index].buffer_type = field->type;
				}
				if (mysql_stmt_bind_result(stmt->mysql.stmt, stmt->mysql.result_field_param)) {
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
					break;
				}
			}
			switch (mysql_stmt_fetch(stmt->mysql.stmt)) {
				case 0:
				case MYSQL_DATA_TRUNCATED:
					res = DB_SUCCESS;
					break;
				case MYSQL_NO_DATA:
					res = DB_NO_DATA;
					break;
				default:
					stmt->mysql.error_msg = mysql_stmt_error(stmt->mysql.stmt);
			}
			break;
		}
		#endif
	}
	return res;
}

#ifdef __cplusplus
}
#endif
