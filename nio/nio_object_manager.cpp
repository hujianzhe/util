//
// Created by hujianzhe on 16-8-17.
//

#include "../syslib/file.h"
#include "../syslib/statistics.h"
#include "nio_object.h"
#include "nio_object_manager.h"
#include <assert.h>

namespace Util {
NioObjectManager::NioObjectManager(const NioObjectManager& o) {}
NioObjectManager& NioObjectManager::operator=(const NioObjectManager& o) { return *this; }
NioObjectManager::NioObjectManager(void) {
	rwlock_Create(&m_validLock);
	reactor_Create(&m_reactor);
}

NioObjectManager::~NioObjectManager(void) {
	rwlock_Close(&m_validLock);
	reactor_Close(&m_reactor);
}

REACTOR* NioObjectManager::getReactor(void) { return &m_reactor; }

size_t NioObjectManager::checkObjectValid(void) {
	size_t count = 0;
	std::list<std::shared_ptr<NioObject> > expire_objs;
	//
	assert(rwlock_LockWrite(&m_validLock, TRUE) == EXEC_SUCCESS);
	for (auto it = m_validObjects.begin(); it != m_validObjects.end(); ) {
		if (it->second->checkValid()) {
			++it;
		}
		else {
			expire_objs.push_back(it->second);
			m_validObjects.erase(it++);
		}
	}
	assert(rwlock_UnlockWrite(&m_validLock) == EXEC_SUCCESS);
	//
	for (std::list<std::shared_ptr<NioObject> >::iterator it = expire_objs.begin();
			it != expire_objs.end(); ++it) {
		(*it)->shutdownDirect();
		++count;
	}
	return count;
}

size_t NioObjectManager::count(void) {
	assert(rwlock_LockRead(&m_validLock, TRUE) == EXEC_SUCCESS);
	size_t count = m_validObjects.size();
	assert(rwlock_UnlockRead(&m_validLock) == EXEC_SUCCESS);
	return count;
}

void NioObjectManager::get(std::list<std::shared_ptr<NioObject> >& l) {
	assert(rwlock_LockRead(&m_validLock, TRUE) == EXEC_SUCCESS);
	for (auto it = m_validObjects.begin(); it != m_validObjects.end(); ++it) {
		l.push_back(it->second);
	}
	assert(rwlock_UnlockRead(&m_validLock) == EXEC_SUCCESS);
}
void NioObjectManager::get(std::vector<std::shared_ptr<NioObject> >& v) {
	assert(rwlock_LockRead(&m_validLock, TRUE) == EXEC_SUCCESS);
	v.reserve(m_validObjects.size());
	for (auto it = m_validObjects.begin(); it != m_validObjects.end(); ++it) {
		v.push_back(it->second);
	}
	assert(rwlock_UnlockRead(&m_validLock) == EXEC_SUCCESS);
}

bool NioObjectManager::add(const std::shared_ptr<NioObject>& object) {
	assert(rwlock_LockWrite(&m_validLock, TRUE) == EXEC_SUCCESS);
	bool res = m_validObjects.insert(std::pair<FD_HANDLE, const std::shared_ptr<NioObject>& >(object->fd(), object)).second;
	assert(rwlock_UnlockWrite(&m_validLock) == EXEC_SUCCESS);
	return res;
}

bool NioObjectManager::del(const std::shared_ptr<NioObject>& object) {
	bool exist = false;
	if (object) {
		object->invalid();
		assert(rwlock_LockWrite(&m_validLock, TRUE) == EXEC_SUCCESS);
		exist = m_validObjects.erase(object->fd());
		assert(rwlock_UnlockWrite(&m_validLock) == EXEC_SUCCESS);
	}
	else {
		exist = true;
	}
	return exist;
}

int NioObjectManager::wait(NIO_EVENT* e, int n, int msec) {
	return reactor_Wait(&m_reactor, e, n, msec);
}

list_node_t* NioObjectManager::result(NIO_EVENT* e, int n) {
	list_node_t* head = NULL, *tail = NULL;
	if (n <= 0) {
		return head;
	}
	assert(rwlock_LockRead(&m_validLock, TRUE) == EXEC_SUCCESS);
	for (int i = 0; i < n; ++i) {
		FD_HANDLE fd;
		int event;
		void* ol;
		reactor_Result(&e[i], &fd, &event, &ol);
		auto iter = m_validObjects.find(fd);
		if (iter == m_validObjects.end() || NULL == iter->second) {
			continue;
		}

		struct NioEvent* ptr = new struct NioEvent(iter->second, event, ol);
		if (tail) {
			list_node_insert_back(tail, &ptr->list_node);
			tail = &ptr->list_node;
		}
		else {
			head = tail = &ptr->list_node;
		}
	}
	assert(rwlock_UnlockRead(&m_validLock) == EXEC_SUCCESS);
	return head;
}

void NioObjectManager::exec(struct NioEvent* objev) {
	objev->obj->updateLastActiveTime();
	switch (objev->event) {
		case REACTOR_READ:
			objev->obj->onRead();
			break;

		case REACTOR_WRITE:
			objev->obj->onWrite();
			break;

		default:
			objev->obj->invalid();
	}
	if (!objev->obj->checkValid()) {
		objev->obj->shutdownDirect();
		del(objev->obj);
	}
}
}
