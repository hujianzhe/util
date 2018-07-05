//
// Created by hujianzhe on 16-5-2.
//

#include "json.h"
#include <stdlib.h>

namespace Util {
std::string Json::serialize(bool format) const {
	std::string res;
	do {
		if (!m_root) {
			break;
		}
		char* text = (format ? cJSON_Print(m_root) : cJSON_PrintUnformatted(m_root));
		if (!text) {
			break;
		}
		res = text;

		cJSON_Hooks hk;
		cJSON_GetHooks(&hk);
		hk.free_fn(text);
	} while (0);
	return res;
}
bool Json::deserialize(const char* text) {
	cJSON* root = cJSON_Parse(NULL, text);
	if (root) {
		cJSON_Delete(m_root);
		m_root = root;
		return true;
	}
	return false;
}
bool Json::deserializeFromFile(const char* path) {
	cJSON* root = cJSON_ParseFromFile(NULL, path);
	if (root) {
		cJSON_Delete(m_root);
		m_root = root;
		return true;
	}
	return false;
}
}
