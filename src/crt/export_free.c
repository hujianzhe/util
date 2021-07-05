//
// Created by hujianzhe
//

#include "../../inc/crt/export_free.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void utilExportFree(void* p) {
	free(p);
}

#ifdef __cplusplus
}
#endif