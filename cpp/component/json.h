//
// Created by hujianzhe on 16-12-24.
//

#ifndef UTIL_CPP_JSON_H
#define UTIL_CPP_JSON_H

#include "../../c/component/cJSON.h"
#include <string>

namespace Util {
class Json {
public:
	Json(cJSON* root = NULL) : m_root(root) {}

	std::string serialize(bool format = false) const;
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
};
}

#endif
