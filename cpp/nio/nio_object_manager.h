//
// Created by hujianzhe on 16-8-17.
//

#ifndef	UTIL_CPP_NIO_NIO_OBJECT_MANAGER_H
#define	UTIL_CPP_NIO_NIO_OBJECT_MANAGER_H

#include "../../c/syslib/io.h"
#include "../../c/syslib/ipc.h"
#include "../../c/datastruct/list.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace Util {
class NioObject;
class NioObjectManager {
public:
	NioObjectManager(void);
	~NioObjectManager(void);

	Reactor_t* getReactor(void);

	size_t count(void);
	void get(std::vector<std::shared_ptr<NioObject> >& v);
	bool add(const std::shared_ptr<NioObject>& object);
	bool del(const std::shared_ptr<NioObject>& object);

	list_node_t* expireObjectList(void);

public:
	struct NioEvent : public list_node_t {
		std::shared_ptr<NioObject> obj;
		int event;

		NioEvent(const std::shared_ptr<NioObject>& obj, int event) :
			obj(obj),
			event(event)
		{}
	};

	int wait(NioEv_t* e, int n, int msec);
	list_node_t* result(NioEv_t* e, int n);
	void exec(struct NioEvent* objev);

private:
	NioObjectManager(const NioObjectManager& o);
	NioObjectManager& operator=(const NioObjectManager& o);

private:
	Reactor_t m_reactor;
	//
	RWLock_t m_lock;
	std::unordered_map<FD_t, std::shared_ptr<NioObject> > m_objects;
};
}

#endif //UTIL_NIO_OBJECT_MANAGER_H
