//
// Created by hujianzhe on 16-8-17.
//

#ifndef	UTIL_CPP_NIO_NIO_OBJECT_MANAGER_H
#define	UTIL_CPP_NIO_NIO_OBJECT_MANAGER_H

#include "../../c/syslib/ipc.h"
#include "../cpp_compiler_define.h"
#if __CPP_VERSION >= 2011
#include <unordered_map>
#else
#include <map>
#endif
#include <vector>

namespace Util {
class NioObject;
class NioObjectManager {
public:
	NioObjectManager(void);
	~NioObjectManager(void);

	size_t count(void);
	void get(std::vector<NioObject*>& v);
	NioObject* get(FD_t fd);
	void add(NioObject* object);
	void del(FD_t fd);
	int expire(NioObject* buf[], int n);

private:
	NioObjectManager(const NioObjectManager& o);
	NioObjectManager& operator=(const NioObjectManager& o);

private:
	//
	RWLock_t m_lock;
#if __CPP_VERSION >= 2011
	std::unordered_map<FD_t, NioObject*> m_objects;
	typedef std::unordered_map<FD_t, NioObject*>::iterator object_iter;
#else
	std::map<FD_t, NioObject*> m_objects;
	typedef std::map<FD_t, NioObject*>::iterator object_iter;
#endif
};
}

#endif //UTIL_NIO_OBJECT_MANAGER_H
