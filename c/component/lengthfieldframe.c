//
// Created by hujianzhe on 18-8-17.
//

#include "../syslib/socket.h"
#include "lengthfieldframe.h"

#ifdef __cplusplus
extern "C" {
#endif

int lengthfieldframeDecode(unsigned short lengthfieldsize,
		unsigned char* buf, unsigned int len, unsigned char** data, unsigned int* datalen)
{
	if (lengthfieldsize > len)
		return 0;

	switch (lengthfieldsize) {
		case 2:
			*datalen = ntohs(*(unsigned short*)buf);
			break;

		case 4:
			*datalen = ntohl(*(unsigned int*)buf);
			break;

		default:
			return -1;
	}
	if (lengthfieldsize + *datalen > len)
		return 0;

	if (*datalen)
		*data = buf + lengthfieldsize;
	else
		*data = NULL;
	return lengthfieldsize + *datalen;
}

int lengthfieldframeEncode(void* lengthfieldbuf, unsigned short lengthfieldsize, unsigned int datalen) {
	switch (lengthfieldsize) {
		case 2:
			if (datalen > 0xffff)
				return 0;
			*(unsigned short*)lengthfieldbuf = htons(datalen);
			return 1;

		case 4:
			*(unsigned int*)lengthfieldbuf = htonl(datalen);
			return 1;

		default:
			return 0;
	}
}

#ifdef __cplusplus
}
#endif
