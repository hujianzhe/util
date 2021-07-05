//
// Created by hujianzhe
//

#ifndef UTIL_C_CRT_EXPORT_FREE_H
#define	UTIL_C_CRT_EXPORT_FREE_H

#include "../compiler_define.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

__declspec_dll void utilExportFree(void* p);

#ifdef __cplusplus
}
#endif

#endif