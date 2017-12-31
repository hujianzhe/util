//
// Created by hujianzhe on 16-8-17.
//

#include "../../c/syslib/file.h"
#include "../../c/syslib/statistics.h"
#include "nio_object.h"
#include "nio_object_manager.h"

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

Reactor_t* NioObjectManager::getReactor(void) { return &m_reactor; }

size_t NioObjectManager::checkObjectValid(void) {
	size_t count = 0;
	std::list<std::shared_ptr<NioObject> > expire_objs;
	//
	rwlock_LockWrite(&m_validLock);

	for (auto it = m_validObjects.begin(); it != m_validObjects.end(); ) {
		if (it->second->checkValid()) {
			++it;
		}
		else {
			expire_objs.push_back(it->second);
			m_validObjects.erase(it++);
		}
	}

	rwlock_Unlock(&m_validLock);
	//
	for (std::list<std::shared_ptr<NioObject> >::iterator it = expire_objs.begin();
			it != expire_objs.end(); ++it) {
		(*it)->shutdownDirect();
		++count;
	}
	return count;
}

size_t NioObjectManager::count(void) {
	rwlock_LockRead(&m_validLock);
	size_t count = m_validObjects.size();
	rwlock_Unlock(&m_validLock);
	return count;
}

void NioObjectManager::get(std::list<std::shared_ptr<NioObject> >& l) {
	rwlock_LockRead(&m_validLock);
	for (auto it = m_validObjects.begin(); it != m_validObjects.end(); ++it) {
		l.push_back(it->second);
	}
	rwlock_Unlock(&m_validLock);
}
void NioObjectManager::get(std::vector<std::shared_ptr<NioObject> >& v) {
	rwlock_LockRead(&m_validLock);
	v.reserve(m_validObjects.size());
	for (auto it = m_validObjects.begin(); it != m_validObjects.end(); ++it) {
		v.push_back(it->second);
	}
	rwlock_Unlock(&m_validLock);
}

bool NioObjectManager::add(const std::shared_ptr<NioObject>& object) {
	rwlock_LockWrite(&m_validLock);
	bool res = m_validObjects.insert(std::pair<FD_t, const std::shared_ptr<NioObject>& >(object->fd(), object)).second;
	rwlock_Unlock(&m_validLock);
	return res;
}

bool NioObjectManager::del(const std::shared_ptr<NioObject>& object) {
	bool exist = false;
	if (object) {
		object->invalid();
		rwlock_LockWrite(&m_validLock);
		exist = m_validObjects.erase(object->fd());
		rwlock_Unlock(&m_validLock);
	}
	else {
		exist = true;
	}
	return exist;
}

int NioObjectManager::wait(NioEv_t* e, int n, int msec) {
	return reactor_Wait(&m_reactor, e, n, msec);
}

list_node_t* NioObjectManager::result(NioEv_t* e, int n) {
	list_node_t* head = NULL, *tail = NULL;
	if (n <= 0) {
		return head;
	}

	rwlock_LockRead(&m_validLock);

	for (int i = 0; i < n; ++i) {
		FD_t fd;
		int event;
		void* ol;
		reactor_Result(&e[i], &fd, &event, &ol);
		auto iter = m_validObjects.find(fd);
		if (iter == m_validObjects.end() || NULL == iter->second) {
			continue;
		}

		struct NioEvent* ptr = new struct NioEvent(iter->second, event, ol);
		if (tail) {
			list_node_insert_back(tail, &ptr->m_listnode);
			tail = &ptr->m_listnode;
		}
		else {
			head = tail = &ptr->m_listnode;
		}
	}

	rwlock_Unlock(&m_validLock);

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
