//
// Created by hujianzhe on 16-12-24.
//

#ifndef UTIL_CPP_JSON_H
#define UTIL_CPP_JSON_H

#include "../../c/component/cJSON.h"

namespace Util {
class Json {
public:
	Json(cJSON* root = NULL) : m_root(root), m_str(NULL) {}
	~Json(void) { cJSON_Delete(m_root); cJSON_FreeString(m_str); }

	char* serialize(bool format = false);
	bool deserialize(const char* text);

	bool deserializeFromFile(const char* path);

	cJSON* root(void) const { return m_root; }
	cJSON* mutableRoot(void) {
		if (!m_root) {
			m_root = cJSON_NewObject(NULL);
		}
		return m_root;
	}

private:
	cJSON* m_root;
	char* m_str;
};
}

#endif
