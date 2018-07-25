//
// Created by hujianzhe on 16-12-24.
//

#ifndef UTIL_CPP_JSON_H
#define UTIL_CPP_JSON_H

#include "../../c/component/cJSON.h"
#include <string.h>

namespace Util {
class JsonRoot : public cJSON {
public:
	JsonRoot(void) : text(NULL) {
		memset((cJSON*)this, 0, sizeof(cJSON));
		type = cJSON_Object;
	}
	~JsonRoot(void) { cJSON_Reset(this); cJSON_FreeString(text); }

	char* serialize(bool format = false) {
		char* s = format ? cJSON_PrintFormatted(this) : cJSON_Print(this);
		if (!s) {
			return (char*)"";
		}
		cJSON_FreeString(text);
		text = s;
		return text;
	}

public:
	char* text;
};
}

#endif
