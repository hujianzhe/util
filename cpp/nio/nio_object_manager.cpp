//
// Created by hujianzhe on 16-8-17.
//

#include "../../c/syslib/assert.h"
#include "nio_object.h"
#include "nio_object_manager.h"

namespace Util {
NioObjectManager::NioObjectManager(const NioObjectManager& o) {}
NioObjectManager& NioObjectManager::operator=(const NioObjectManager& o) { return *this; }
NioObjectManager::NioObjectManager(void) {
	assertTRUE(rwlockCreate(&m_lock));
}

NioObjectManager::~NioObjectManager(void) {
	rwlockClose(&m_lock);
}

void NioObjectManager::get(std::vector<NioObject*>& v) {
	rwlockLockRead(&m_lock);
	v.reserve(m_objects.size());
	for (object_iter it = m_objects.begin(); it != m_objects.end(); ++it) {
		v.push_back(it->second);
	}
	rwlockUnlock(&m_lock);
}

NioObject* NioObjectManager::get(FD_t fd) {
	NioObject* object = NULL;
	rwlockLockRead(&m_lock);
	object_iter it = m_objects.find(fd);
	if (it != m_objects.end()) {
		object = it->second;
	}
	rwlockUnlock(&m_lock);
	return object;
}

void NioObjectManager::add(NioObject* object) {
	std::pair<FD_t, NioObject*> item(object->fd, object);
	rwlockLockWrite(&m_lock);
	m_objects.insert(item);
	rwlockUnlock(&m_lock);
}

NioObject* NioObjectManager::del(FD_t fd) {
	NioObject* object = NULL;
	rwlockLockWrite(&m_lock);
	object_iter it = m_objects.find(fd);
	if (it != m_objects.end()) {
		object = it->second;
		m_objects.erase(it);
	}
	rwlockUnlock(&m_lock);
	return object;
}

size_t NioObjectManager::expire(NioObject* buf[], size_t n) {
	size_t i = 0;
	rwlockLockWrite(&m_lock);
	for (object_iter iter = m_objects.begin(); iter != m_objects.end(); ) {
		NioObject* object = iter->second;
		if (object->checkValid()) {
			++iter;
			continue;
		}
		object->valid = false;
		if (i < n) {
			m_objects.erase(iter++);
			buf[i++] = object;
		}
		else {
			++iter;
		}
	}
	rwlockUnlock(&m_lock);
	return i;
}
}
