//
// Created by hujianzhe on 18-8-17.
//

#include "../../inc/datastruct/lengthfieldframe.h"
#include "../../inc/datastruct/memfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

int lengthfieldframeDecode(unsigned short lengthfieldsize,
		const unsigned char* buf, unsigned int len, unsigned char** data, unsigned int* datalen)
{
	if (lengthfieldsize > len)
		return 0;

	switch (lengthfieldsize) {
		case 2:
			*datalen = memFromBE16(*(unsigned short*)buf);
			break;

		case 4:
			*datalen = memFromBE32(*(unsigned int*)buf);
			break;

		default:
			return -1;
	}
	if (*datalen > len - lengthfieldsize)
		return 0;

	if (*datalen)
		*data = (unsigned char*)buf + lengthfieldsize;
	else
		*data = (unsigned char*)0;
	return lengthfieldsize + *datalen;
}

int lengthfieldframeEncode(void* lengthfieldbuf, unsigned short lengthfieldsize, unsigned int datalen) {
	switch (lengthfieldsize) {
		case 2:
			if (datalen > 0xffff)
				return 0;
			*(unsigned short*)lengthfieldbuf = memToBE16(datalen);
			return 1;

		case 4:
			*(unsigned int*)lengthfieldbuf = memToBE32(datalen);
			return 1;

		default:
			return 0;
	}
}

int lengthfieldframeDecode2(unsigned short lengthfieldsize, unsigned char* buf, unsigned int len) {
	int decodelen;
	if (len < lengthfieldsize)
		return 0;

	switch (lengthfieldsize) {
		case 2:
			decodelen = memFromBE16(*(unsigned short*)buf);
			break;

		case 4:
			decodelen = memFromBE32(*(unsigned int*)buf);
			break;

		default:
			return -1;
	}
	if (decodelen < lengthfieldsize) {
		return -1;
	}
	return decodelen <= len ? decodelen : 0;
}

#ifdef __cplusplus
}
#endif
