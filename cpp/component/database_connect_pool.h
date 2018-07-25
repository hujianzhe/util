//
// Created by hujianzhe on 17-6-30.
//

#ifndef	UTIL_CPP_DB_DATABASE_CONNECT_POOL_H
#define	UTIL_CPP_DB_DATABASE_CONNECT_POOL_H

#include "../../c/syslib/ipc.h"
#include "../../c/component/db.h"
#include "../cpp_compiler_define.h"
#include "../unordered_set.h"
#include <string>

namespace Util {
class DatabaseConnectPool {
public:
	DatabaseConnectPool(int db_type, const char* ip, unsigned short port, const char* user, const char* pwd, const char* database);
	~DatabaseConnectPool(void);

	void setConnectionAttribute(int timeout_sec, short max_connect_num) {
		m_connectTimeout = timeout_sec;
		m_connectMaxNum = max_connect_num;
	}

	DBHandle_t* getConnection(void);
	void pushConnection(DBHandle_t* dbhandle);
	void cleanConnection(void);

private:
	DBHandle_t* connect(void);
	void clean(void);

private:
	int m_connectTimeout;
	int m_connectNum;
	int m_connectMaxNum;

	const int m_dbType;
	const unsigned short m_port;
	const std::string m_ip;
	const std::string m_user;
	const std::string m_pwd;
	const std::string m_database;

	CSLock_t m_lock;
	std::unordered_set<DBHandle_t*> m_dbhandles;
	typedef std::unordered_set<DBHandle_t*>::iterator handle_iter;
};
}

#endif
