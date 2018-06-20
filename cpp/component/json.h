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
	~Json(void) { reset(); }

	std::string serialize(bool format = false) const;
	bool deserialize(const char* text);
	bool deserialize(const std::string& text) { return deserialize(text.c_str()); }

	bool deserializeFromFile(const char* path);
	bool deserializeFromFile(const std::string& path) { return deserializeFromFile(path.c_str()); }

	cJSON* root(void) const { return m_root; }
	cJSON* mutableRoot(void) {
		if (!m_root) {
			m_root = cJSON_CreateObject();
		}
		return m_root;
	}

	void reset(void) {
		cJSON_Delete(m_root);
		m_root = NULL;
	}

private:
	Json(const Json& o) {}
	Json& operator=(const Json& o) { return *this; }

private:
	cJSON* m_root;
};
}

#endif
