//
// Created by hujianzhe
//

#include "uuid.h"

#ifdef	__cplusplus
extern "C" {
#endif

uuid_t* uuid_Create(uuid_t* uuid) {
#if defined(_WIN32) || defined(_WIN64)
	return CoCreateGuid(uuid) == S_OK ? uuid : NULL;
#else
	uuid_generate(*uuid);
	return uuid;
#endif
}

BOOL uuid_ToString(const uuid_t* uuid, uuid_string_t uuid_string) {
#if defined(_WIN32) || defined(_WIN64)
	int i;
	unsigned char* buffer;
	if (UuidToStringA(uuid, &buffer) != RPC_S_OK)
		return FALSE;
	for (i = 0; i < sizeof(uuid_string_t) && buffer[i]; ++i)
		uuid_string[i] = buffer[i];
	assert_true(RpcStringFreeA(&buffer) == RPC_S_OK);
	return TRUE;
#else
	uuid_unparse_lower(*uuid, uuid_string);
	return TRUE;
#endif
}

BOOL uuid_FromString(uuid_t* uuid, const uuid_string_t uuid_string) {
#if defined(_WIN32) || defined(_WIN64)
	return UuidFromStringA((RPC_CSTR)uuid_string, uuid) == RPC_S_OK;
#else
	return uuid_parse((char*)uuid_string, *uuid) == 0;
#endif
}

#ifdef	__cplusplus
}
#endif