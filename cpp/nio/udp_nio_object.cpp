//
// Created by hujianzhe on 16-11-11.
//

#include "../../c/syslib/error.h"
#include "udp_nio_object.h"

namespace Util {
UdpNioObject::UdpNioObject(FD_t sockfd, int domain, unsigned short frame_length_limit) :
	NioObject(sockfd, domain, SOCK_DGRAM, 0),
	m_frameLengthLimit(frame_length_limit)
{
}

int UdpNioObject::recv(void) {
	int res;
	struct sockaddr_storage saddr;
	while (1) {
		unsigned char* buffer = (unsigned char*)alloca(m_frameLengthLimit);
		res = sock_Recv(m_fd, buffer, m_frameLengthLimit, 0, &saddr);
		if (res < 0) {
			if (error_code() != EWOULDBLOCK) {
				m_valid = false;
			}
			break;
		}
		else if (0 == res) {
			buffer = NULL;
		}

		if (onRead(buffer, res, &saddr) < 0) {
			m_valid = false;
			break;
		}
	}
	return res;
}
}