//
// Created by hujianzhe
//

#include "../../inc/component/reactor.h"
#include "../../inc/sysapi/error.h"
#include "../../inc/sysapi/time.h"

#ifdef	__cplusplus
extern "C" {
#endif

static void update_timestamp(long long* src, long long ts) {
	if (*src <= 0 || *src > ts)
		*src = ts;
}

static void free_inbuf(ReactorObject_t* o) {
	if (o->m_inbuf) {
		free(o->m_inbuf);
		o->m_inbuf = NULL;
		o->m_inbuflen = 0;
		o->m_inbufoff = 0;
	}
}

static void free_io_resource(ReactorObject_t* o) {
	free_inbuf(o);
	if (INVALID_FD_HANDLE != o->fd) {
		socketClose(o->fd);
		o->fd = INVALID_FD_HANDLE;
	}
	if (o->m_readol) {
		nioFreeOverlapped(o->m_readol);
		o->m_readol = NULL;
	}
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
}

static void reactorobject_invalid_inner_handler(ReactorObject_t* o, long long now_msec) {
	if (SOCK_STREAM == o->socktype) {
		free_io_resource(o);
	}
	o->m_invalid_msec = now_msec;
	if (o->invalid_timeout_msec <= 0 ||
		o->invalid_timeout_msec + o->m_invalid_msec <= now_msec)
	{
		free_io_resource(o);
		if (o->inactive) {
			o->inactive(o);
			o->inactive = NULL;
		}
	}
	else {
		listInsertNodeBack(&o->reactor->m_invalidlist, o->reactor->m_invalidlist.tail, &o->m_invalidnode);
		update_timestamp(&o->reactor->event_msec, o->m_invalid_msec + o->invalid_timeout_msec);
	}
}

static void reactor_exec_cmdlist(Reactor_t* reactor) {
	ListNode_t* cur, *next;
	criticalsectionEnter(&reactor->m_cmdlistlock);
	cur = reactor->m_cmdlist.head;
	listInit(&reactor->m_cmdlist);
	criticalsectionLeave(&reactor->m_cmdlistlock);
	for (; cur; cur = next) {
		next = cur->next;
		reactor->exec_cmd(cur);
	}
}

static void reactor_exec_object(Reactor_t* reactor, long long now_msec) {
	HashtableNode_t *cur, *next;
	for (cur = hashtableFirstNode(&reactor->m_objht); cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_hashnode);
		next = hashtableNextNode(cur);
		if (o->valid) {
			o->exec(o);
			if (o->valid)
				continue;
		}
		hashtableRemoveNode(&reactor->m_objht, cur);
		reactorobject_invalid_inner_handler(o, now_msec);
	}
}

static void reactor_exec_invalidlist(Reactor_t* reactor, long long now_msec) {
	ListNode_t* cur, *next;
	List_t invalidfreelist;
	listInit(&invalidfreelist);
	for (cur = reactor->m_invalidlist.head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
		next = cur->next;
		if (o->m_invalid_msec + o->invalid_timeout_msec > now_msec) {
			update_timestamp(&reactor->event_msec, now_msec);
			break;
		}
		listRemoveNode(&reactor->m_invalidlist, cur);
		free_io_resource(o);
		if (o->inactive)
			listInsertNodeBack(&invalidfreelist, invalidfreelist.tail, cur);
	}
	for (cur = invalidfreelist.head; cur; cur = next) {
		ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
		next = cur->next;
		o->inactive(o);
		o->inactive = NULL;
	}
}

static void reactor_stream_accept(ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t saddr;
	FD_t connfd;
	for (connfd = nioAcceptFirst(o->fd, o->m_readol, &saddr.st);
		connfd != INVALID_FD_HANDLE;
		connfd = nioAcceptNext(o->fd, &saddr.st))
	{
		if (o->stream.accept)
			o->stream.accept(o, connfd, &saddr, timestamp_msec);
		else
			socketClose(connfd);
	}
}

static void reactor_readev(ReactorObject_t* o, long long timestamp_msec) {
	Sockaddr_t from_addr;
	if (SOCK_STREAM == o->socktype) {
		int res = socketTcpReadableBytes(o->fd);
		if (res < 0) {
			reactorobjectInvalid(o, timestamp_msec);
			return;
		}
		else if (0 == res) {
			reactorobjectInvalid(o, timestamp_msec);
			o->preread(o, NULL, 0, 0, timestamp_msec);
			return;
		}
		else {
			unsigned char* ptr = (unsigned char*)realloc(o->m_inbuf, o->m_inbuflen + res);
			if (!ptr) {
				reactorobjectInvalid(o, timestamp_msec);
				return;
			}
			o->m_inbuf = ptr;
			res = socketRead(o->fd, o->m_inbuf + o->m_inbufoff, res, 0, &from_addr.st);
			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK) {
					reactorobjectInvalid(o, timestamp_msec);
				}
				return;
			}
			else if (0 == res) {
				reactorobjectInvalid(o, timestamp_msec);
				o->preread(o, NULL, 0, 0, timestamp_msec);
				return;
			}
			o->m_inbuflen += res;
			res = o->preread(o, o->m_inbuf, o->m_inbuflen, o->m_inbufoff, timestamp_msec);
			if (res < 0) {
				reactorobjectInvalid(o, timestamp_msec);
				return;
			}
			o->m_inbufoff += res;
			if (o->m_inbufoff >= o->m_inbuflen)
				free_inbuf(o);
		}
	}
	else {
		unsigned int readtimes, readmaxtimes = 8;
		for (readtimes = 0; readtimes < readmaxtimes; ++readtimes) {
			int res;
			unsigned char* ptr;
			if (readtimes) {
				if (!o->m_inbuf) {
					o->m_inbuf = (unsigned char*)malloc(o->dgram.read_mtu);
					if (!o->m_inbuf) {
						reactorobjectInvalid(o, timestamp_msec);
						return;
					}
					o->m_inbuflen = o->dgram.read_mtu;
				}
				ptr = o->m_inbuf;
				res = socketRead(o->fd, o->m_inbuf, o->m_inbuflen, 0, &from_addr.st);
			}
			else {
				Iobuf_t iov;
				if (0 == nioOverlappedData(o->m_readol, &iov, &from_addr.st)) {
					++readmaxtimes;
					continue;
				}
				ptr = (unsigned char*)iobufPtr(&iov);
				res = iobufLen(&iov);
			}

			if (res < 0) {
				if (errnoGet() != EWOULDBLOCK)
					reactorobjectInvalid(o, timestamp_msec);
				return;
			}
			if (o->preread(o, ptr, res, 0, timestamp_msec) < 0) {
				reactorobjectInvalid(o, timestamp_msec);
				return;
			}
		}
	}
}

static int reactor_stream_connect(ReactorObject_t* o, long long timestamp_msec) {
	int err, ok;
	o->m_stream_connected = 1;
	if (o->m_writeol) {
		nioFreeOverlapped(o->m_writeol);
		o->m_writeol = NULL;
	}
	if (!nioConnectCheckSuccess(o->fd)) {
		err = errnoGet();
		ok = 0;
	}
	else if (!reactorobjectRequestRead(o)) {
		err = errnoGet();
		ok = 0;
	}
	else {
		err = 0;
		ok = 1;
	}
	o->stream.connect(o, err, timestamp_msec);
	return ok;
}

static int objht_keycmp(const void* node_key, const void* key) {
	return *(FD_t*)node_key != *(FD_t*)key;
}

static unsigned int objht_keyhash(const void* key) { return *(FD_t*)key; }

Reactor_t* reactorInit(Reactor_t* reactor) {
	Sockaddr_t saddr = { 0 };

	if (!socketPair(SOCK_STREAM, reactor->m_socketpair))
		return NULL;
	saddr.sa.sa_family = AF_INET;

	reactor->m_readol = nioAllocOverlapped(NIO_OP_READ, NULL, 0, 0);
	if (!reactor->m_readol) {
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		return NULL;
	}

	if (!nioCreate(&reactor->m_nio)) {
		nioFreeOverlapped(reactor->m_readol);
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		return NULL;
	}

	if (!socketNonBlock(reactor->m_socketpair[0], TRUE) ||
		!socketNonBlock(reactor->m_socketpair[1], TRUE) ||
		!nioReg(&reactor->m_nio, reactor->m_socketpair[0]) ||
		!nioCommit(&reactor->m_nio, reactor->m_socketpair[0], NIO_OP_READ, reactor->m_readol,
			(struct sockaddr*)&saddr, sockaddrLength(&saddr)))
	{
		nioFreeOverlapped(reactor->m_readol);
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		nioClose(&reactor->m_nio);
		return NULL;
	}

	if (!criticalsectionCreate(&reactor->m_cmdlistlock)) {
		nioFreeOverlapped(reactor->m_readol);
		socketClose(reactor->m_socketpair[0]);
		socketClose(reactor->m_socketpair[1]);
		nioClose(&reactor->m_nio);
		return NULL;
	}

	listInit(&reactor->m_cmdlist);
	listInit(&reactor->m_invalidlist);
	hashtableInit(&reactor->m_objht,
		reactor->m_objht_bulks, sizeof(reactor->m_objht_bulks) / sizeof(reactor->m_objht_bulks[0]),
		objht_keycmp, objht_keyhash);
	reactor->m_runthreadhasbind = 0;
	reactor->m_wake = 0;

	reactor->event_msec = 0;
	reactor->exec_cmd = NULL;
	reactor->free_cmd = NULL;
	return reactor;
}

void reactorWake(Reactor_t* reactor) {
	if (0 == _cmpxchg16(&reactor->m_wake, 1, 0)) {
		char c;
		socketWrite(reactor->m_socketpair[1], &c, sizeof(c), 0, NULL, 0);
	}
}

void reactorCommitCmd(Reactor_t* reactor, ListNode_t* cmdnode) {
	criticalsectionEnter(&reactor->m_cmdlistlock);
	listInsertNodeBack(&reactor->m_cmdlist, reactor->m_cmdlist.tail, cmdnode);
	criticalsectionLeave(&reactor->m_cmdlistlock);
	reactorWake(reactor);
}

void reactorCommitCmdList(Reactor_t* reactor, List_t* cmdlist) {
	criticalsectionEnter(&reactor->m_cmdlistlock);
	listMerge(&reactor->m_cmdlist, cmdlist);
	criticalsectionLeave(&reactor->m_cmdlistlock);
	reactorWake(reactor);
}

int reactorRegObject(Reactor_t* reactor, ReactorObject_t* o) {
	o->reactor = reactor;
	if (!nioReg(&reactor->m_nio, o->fd))
		return 0;
	if (SOCK_STREAM == o->socktype) {
		BOOL ret;
		if (!socketIsListened(o->fd, &ret))
			return 0;
		if (ret) {
			o->m_stream_listened = 1;
			if (!reactorobjectRequestRead(o))
				return 0;
		}
		else if (!socketIsConnected(o->fd, &ret))
			return 0;
		else if (ret) {
			o->m_stream_connected = 1;
			if (!reactorobjectRequestRead(o))
				return 0;
		}
		else {
			o->m_stream_connected = 0;
			if (!o->m_writeol) {
				o->m_writeol = nioAllocOverlapped(NIO_OP_CONNECT, NULL, 0, 0);
				if (!o->m_writeol)
					return 0;
			}
			if (!nioCommit(&reactor->m_nio, o->fd, NIO_OP_CONNECT, o->m_writeol,
				&o->stream.connect_addr.sa, sockaddrLength(&o->stream.connect_addr)))
			{
				return 0;
			}
			o->m_writeol_has_commit = 1;
		}
	}
	else if (!reactorobjectRequestRead(o))
		return 0;
	hashtableReplaceNode(hashtableInsertNode(&reactor->m_objht, &o->m_hashnode), &o->m_hashnode);
	return 1;
}

int reactorHandle(Reactor_t* reactor, NioEv_t e[], int n, long long timestamp_msec, int wait_msec) {
	if (!reactor->m_runthreadhasbind) {
		reactor->m_runthread = threadSelf();
		reactor->m_runthreadhasbind = 1;
	}

	if (reactor->event_msec > timestamp_msec) {
		int checkexpire_wait_msec = reactor->event_msec - timestamp_msec;
		if (checkexpire_wait_msec < wait_msec || wait_msec < 0)
			wait_msec = checkexpire_wait_msec;
	}
	else if (reactor->event_msec) {
		wait_msec = 0;
	}

	n = nioWait(&reactor->m_nio, e, n, wait_msec);
	if (n < 0) {
		return n;
	}
	else if (0 == n) {
		timestamp_msec += wait_msec;
	}
	else {
		int i;
		timestamp_msec = gmtimeMillisecond();
		for (i = 0; i < n; ++i) {
			HashtableNode_t* find_node;
			ReactorObject_t* o;
			FD_t fd;
			if (!nioEventOverlappedCheck(e + i))
				continue;
			fd = nioEventFD(e + i);
			if (fd == reactor->m_socketpair[0]) {
				Sockaddr_t saddr;
				char c[512];
				socketRead(fd, c, sizeof(c), 0, NULL);
				saddr.sa.sa_family = AF_INET;
				nioCommit(&reactor->m_nio, fd, NIO_OP_READ, reactor->m_readol, (struct sockaddr*)&saddr, sockaddrLength(&saddr));
				_xchg16(&reactor->m_wake, 0);
				continue;
			}
			find_node = hashtableSearchKey(&reactor->m_objht, &fd);
			if (!find_node)
				continue;
			o = pod_container_of(find_node, ReactorObject_t, m_hashnode);
			if (!o->valid)
				continue;
			switch (nioEventOpcode(e + i)) {
				case NIO_OP_READ:
					o->m_readol_has_commit = 0;
					if (SOCK_STREAM == o->socktype && o->m_stream_listened)
						reactor_stream_accept(o, timestamp_msec);
					else
						reactor_readev(o, timestamp_msec);
					if (!reactorobjectRequestRead(o))
						o->valid = 0;
					break;
				case NIO_OP_WRITE:
					o->m_writeol_has_commit = 0;
					if (SOCK_STREAM == o->socktype && !o->m_stream_connected)
						if (reactor_stream_connect(o, timestamp_msec))
							o->valid = 0;
					else
						o->writeev(o, timestamp_msec);
					break;
				default:
					o->valid = 0;
			}
			if (o->valid)
				continue;
			hashtableRemoveNode(&reactor->m_objht, &o->m_hashnode);
			reactorobject_invalid_inner_handler(o, timestamp_msec);
		}
	}
	reactor_exec_cmdlist(reactor);
	if (reactor->event_msec && timestamp_msec >= reactor->event_msec) {
		reactor->event_msec = 0;
		reactor_exec_object(reactor, timestamp_msec);
		reactor_exec_invalidlist(reactor, timestamp_msec);
	}
	return n;
}

void reactorDestroy(Reactor_t* reactor) {
	nioFreeOverlapped(reactor->m_readol);
	socketClose(reactor->m_socketpair[0]);
	socketClose(reactor->m_socketpair[1]);
	nioClose(&reactor->m_nio);
	criticalsectionClose(&reactor->m_cmdlistlock);
	do {
		ListNode_t* cur, *next;
		for (cur = reactor->m_cmdlist.head; cur; cur = next) {
			next = cur->next;
			if (reactor->free_cmd)
				reactor->free_cmd(cur);
		}
	} while (0);
	do {
		HashtableNode_t *cur, *next;
		for (cur = hashtableFirstNode(&reactor->m_objht); cur; cur = next) {
			ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_hashnode);
			next = hashtableNextNode(cur);
			free_io_resource(o);
			if (o->free) {
				o->free(o);
				o->free = NULL;
			}
		}
	} while (0);
	do {
		ListNode_t* cur, *next;
		for (cur = reactor->m_invalidlist.head; cur; cur = next) {
			ReactorObject_t* o = pod_container_of(cur, ReactorObject_t, m_invalidnode);
			next = cur->next;
			free_io_resource(o);
			if (o->free) {
				o->free(o);
				o->free = NULL;
			}
		}
	} while (0);
}

ReactorObject_t* reactorobjectInit(ReactorObject_t* o, FD_t fd, int domain, int socktype, int protocol) {
	int fd_create;
	if (INVALID_FD_HANDLE == fd) {
		fd = socket(domain, socktype, protocol);
		if (INVALID_FD_HANDLE == fd)
			return NULL;
		fd_create = 1;
	}
	else
		fd_create = 0;
	if (!socketNonBlock(fd, TRUE)) {
		if (fd_create)
			socketClose(fd);
		return NULL;
	}
	o->domain = domain;
	o->socktype = socktype;
	o->protocol = protocol;
	o->reactor = NULL;
	o->userdata = NULL;
	o->invalid_timeout_msec = 0;
	o->valid = 1;
	o->exec = NULL;
	o->preread = NULL;
	o->writeev = NULL;
	o->inactive = NULL;
	o->free = NULL;
	if (SOCK_STREAM == socktype)
		memset(&o->stream, 0, sizeof(o->stream));
	else {
		o->dgram.read_mtu = 1464;
		o->dgram.write_mtu = 548;
	}
	
	o->m_readol = NULL;
	o->m_writeol = NULL;
	o->m_invalid_msec = 0;
	o->m_readol_has_commit = 0;
	o->m_writeol_has_commit = 0;
	o->m_stream_connected = 0;
	o->m_stream_listened = 0;
	o->m_inbuf = NULL;
	o->m_inbuflen = 0;
	o->m_inbufoff = 0;
	return o;
}

int reactorobjectRequestRead(ReactorObject_t* o) {
	Sockaddr_t saddr;
	int opcode;
	if (!o->valid)
		return 0;
	else if (o->m_readol_has_commit)
		return 1;
	else if (SOCK_STREAM == o->socktype && o->m_stream_listened)
		opcode = NIO_OP_ACCEPT;
	else
		opcode = NIO_OP_READ;
	if (!o->m_readol) {
		o->m_readol = nioAllocOverlapped(opcode, NULL, 0, SOCK_STREAM != o->socktype ? 65000 : 0);
		if (!o->m_readol) {
			return 0;
		}
	}
	saddr.sa.sa_family = o->domain;
	if (nioCommit(&o->reactor->m_nio, o->fd, opcode, o->m_readol,
		&saddr.sa, sockaddrLength(&saddr)))
	{
		o->m_readol_has_commit = 1;
		return 1;
	}
	return 0;
}

int reactorobjectRequestWrite(ReactorObject_t* o) {
	Sockaddr_t saddr;
	if (!o->valid)
		return 0;
	else if (o->m_writeol_has_commit)
		return 1;
	else if (!o->m_writeol) {
		o->m_writeol = nioAllocOverlapped(NIO_OP_WRITE, NULL, 0, 0);
		if (!o->m_writeol) {
			return 0;
		}
	}
	saddr.sa.sa_family = o->domain;
	if (nioCommit(&o->reactor->m_nio, o->fd, NIO_OP_WRITE, o->m_writeol,
		&saddr.sa, sockaddrLength(&saddr)))
	{
		o->m_writeol_has_commit = 1;
		return 1;
	}
	return 0;
}

ReactorObject_t* reactorobjectInvalid(ReactorObject_t* o, long long timestamp_msec) {
	o->valid = 0;
	if (o->m_invalid_msec <= 0)
		o->m_invalid_msec = timestamp_msec;
	return o;
}

#ifdef	__cplusplus
}
#endif