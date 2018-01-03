//
// Created by hujianzhe on 16-11-11.
//

#include "udp_nio_object.h"

namespace Util {
UdpNioObject::UdpNioObject(FD_t sockfd, unsigned short frame_length_limit) :
	NioObject(sockfd, SOCK_DGRAM),
	m_frameLengthLimit(frame_length_limit)
{
}

int UdpNioObject::recv(void) {
	struct sockaddr_storage saddr;
	void* buffer = alloca(m_frameLengthLimit);
	int res = sock_Recv(m_fd, buffer, m_frameLengthLimit, 0, &saddr);
	if (res >= 0) {
		IoBuf_t inbuf;
		iobuffer_buf(&inbuf) = res ? (char*)buffer : NULL;
		iobuffer_len(&inbuf) = res;
		onRead(inbuf, &saddr, res);
	}
	else {
		m_valid = false;
	}
	return res;
}
}