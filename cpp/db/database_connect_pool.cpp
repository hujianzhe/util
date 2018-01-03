//
// Created by hujianzhe on 17-6-30.
//

#include "database_connect_pool.h"

namespace Util {
DatabaseConnectPool::DatabaseConnectPool(int db_type, const char* ip, unsigned short port, const char* user, const char* pwd, const char* database) :
	m_connectTimeout(INFTIM),
	m_connectNum(0),
	m_connectMaxNum(1),
	m_dbType(db_type),
	m_ip(ip),
	m_port(port),
	m_user(user),
	m_pwd(pwd),
	m_database(database)
{
	assert_true(cslock_Create(&m_lock) == EXEC_SUCCESS);
}

DatabaseConnectPool::~DatabaseConnectPool(void) {
	clean();
	cslock_Close(&m_lock);
}

DB_HANDLE* DatabaseConnectPool::getConnection(void) {
	bool need_new = false;
	DB_HANDLE* dbhandle = NULL;
	cslock_Enter(&m_lock);
	do {
		if (m_dbhandles.empty()) {
			if (m_connectNum >= m_connectMaxNum) {
				break;
			}
			++m_connectNum;
			need_new = true;
		}
		else {
			dbhandle = *(m_dbhandles.begin());
			m_dbhandles.erase(m_dbhandles.begin());
		}
	} while (0);
	cslock_Leave(&m_lock);
	if (need_new) {
		dbhandle = connect();
	}
	return dbhandle;
}

void DatabaseConnectPool::pushConnection(DB_HANDLE* dbhandle) {
	/*
	 * dbhandle will be free when database connect pool call clean()
	 * if not call this funcion, you should free handle by yourself
	 */
	if (dbhandle) {
		cslock_Enter(&m_lock);
		m_dbhandles.insert(dbhandle);
		cslock_Leave(&m_lock);
	}
}

void DatabaseConnectPool::clean(void) {
	for (auto it = m_dbhandles.begin(); it != m_dbhandles.end(); m_dbhandles.erase(it++)) {
		db_CloseHandle(*it);
		delete (*it);
	}
	m_connectNum = 0;
}
void DatabaseConnectPool::cleanConnection(void) {
	cslock_Enter(&m_lock);
	clean();
	cslock_Leave(&m_lock);
}

DB_HANDLE* DatabaseConnectPool::connect(void) {
	DB_HANDLE* dbhandle = new DB_HANDLE();
	do {
		if (!db_CreateHandle(dbhandle, m_dbType)) {
			break;
		}
		if (m_connectTimeout != INFTIM) {
			if (db_SetConnectTimeout(dbhandle, m_connectTimeout) != DB_SUCCESS) {
				break;
			}
		}
		if (!db_SetupConnect(dbhandle, m_ip.c_str(), m_port, m_user.c_str(), m_pwd.c_str(), m_database.c_str())) {
			break;
		}
		return dbhandle;
	} while (0);
	delete dbhandle;
	cslock_Enter(&m_lock);
	--m_connectNum;
	cslock_Leave(&m_lock);
	return NULL;
}
}
