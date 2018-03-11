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
	unsigned char* buffer = (unsigned char*)alloca(m_frameLengthLimit);
	int res = sock_Recv(m_fd, buffer, m_frameLengthLimit, 0, &saddr);
	if (res < 0) {
		m_valid = false;
		return res;
	}
	else if (0 == res) {
		buffer = NULL;
	}
	if (onRead(buffer, res, &saddr) < 0) {
		m_valid = false;
	}
	return res;
}
}