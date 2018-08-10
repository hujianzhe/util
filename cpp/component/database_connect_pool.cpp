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
	assertTRUE(criticalsectionCreate(&m_lock));
}

DatabaseConnectPool::~DatabaseConnectPool(void) {
	clean();
	criticalsectionClose(&m_lock);
}

DBHandle_t* DatabaseConnectPool::getConnection(void) {
	bool need_new = false;
	DBHandle_t* dbhandle = NULL;
	criticalsectionEnter(&m_lock);
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
	criticalsectionLeave(&m_lock);
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
		criticalsectionEnter(&m_lock);
		m_dbhandles.insert(dbhandle);
		criticalsectionLeave(&m_lock);
	}
}

void DatabaseConnectPool::clean(void) {
	for (handle_iter it = m_dbhandles.begin(); it != m_dbhandles.end(); m_dbhandles.erase(it++)) {
		dbCloseHandle(*it);
		delete (*it);
	}
	m_connectNum = 0;
}
void DatabaseConnectPool::cleanConnection(void) {
	criticalsectionEnter(&m_lock);
	clean();
	criticalsectionLeave(&m_lock);
}

DBHandle_t* DatabaseConnectPool::connect(void) {
	DBHandle_t* dbhandle = new DBHandle_t();
	do {
		if (!dbCreateHandle(dbhandle, m_dbType)) {
			break;
		}
		if (m_connectTimeout != INFTIM) {
			if (dbSetConnectTimeout(dbhandle, m_connectTimeout) != DB_SUCCESS) {
				break;
			}
		}
		if (!dbConnect(dbhandle, m_ip.c_str(), m_port, m_user.c_str(), m_pwd.c_str(), m_database.c_str())) {
			break;
		}
		return dbhandle;
	} while (0);
	delete dbhandle;
	criticalsectionEnter(&m_lock);
	--m_connectNum;
	criticalsectionLeave(&m_lock);
	return NULL;
}
}
