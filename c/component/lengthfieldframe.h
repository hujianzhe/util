//
// Created by hujianzhe on 18-8-17.
//

#ifndef	UTIL_C_COMPONENT_LENGTHFIELDFRAME_H
#define	UTIL_C_COMPONENT_LENGTHFIELDFRAME_H

#ifdef __cplusplus
extern "C" {
#endif

int lengthfieldframeDecode(unsigned short lengthfieldsize,
		unsigned char* buf, unsigned int len, unsigned char** data, unsigned int* datalen);
int lengthfieldframeEncode(unsigned char* lengthfieldbuf, unsigned short lengthfieldsize, unsigned int datalen);

#ifdef __cplusplus
}
#endif

#endif
