//
// Created by hujianzhe on 17-6-30.
//

#include "../../c/syslib/assert.h"
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
	assert_true(cslock_Create(&m_lock));
}

DatabaseConnectPool::~DatabaseConnectPool(void) {
	clean();
	cslock_Close(&m_lock);
}

DBHandle_t* DatabaseConnectPool::getConnection(void) {
	bool need_new = false;
	DBHandle_t* dbhandle = NULL;
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

void DatabaseConnectPool::pushConnection(DBHandle_t* dbhandle) {
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
	for (handle_iter it = m_dbhandles.begin(); it != m_dbhandles.end(); m_dbhandles.erase(it++)) {
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

DBHandle_t* DatabaseConnectPool::connect(void) {
	DBHandle_t* dbhandle = new DBHandle_t();
	do {
		if (!db_CreateHandle(dbhandle, m_dbType)) {
			break;
		}
		if (m_connectTimeout != INFTIM) {
			if (db_SetConnectTimeout(dbhandle, m_connectTimeout) != DB_SUCCESS) {
				break;
			}
		}
		if (!db_Connect(dbhandle, m_ip.c_str(), m_port, m_user.c_str(), m_pwd.c_str(), m_database.c_str())) {
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
